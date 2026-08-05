#include "Device/DeviceSession.h"
#include "Device/DeviceSessionSupport.h"

#include <utility>
#include <vector>

void DeviceSession::resetInFramers() noexcept
{
    for (std::size_t index = 0; index < kMaxMidiBackendInPorts; ++index)
    {
        inFramers_[index].Reset();
    }
}

void DeviceSession::noteBulkReadCounters(std::size_t bulkBytes, std::size_t demuxSpans)
{
    deviceHostCounters_.AddBulkAndDemux(bulkBytes, demuxSpans);
}

void DeviceSession::sendFramedToHost(
    std::size_t inPortIndex,
    uint8_t cableIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount)
{
    std::string error;
    if (!midiBackend_->SendToHost(inPortIndex, midiBytes, byteCount, error))
    {
        deviceHostCounters_.AddSendFail();
        recordPumpFailure(
            formatPortCableFailure("Device→host SendToHost", inPortIndex, cableIndex, error));
        return;
    }

    deviceHostCounters_.AddSendOk();
}

void DeviceSession::forwardDeviceMidi(
    uint8_t cableIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount)
{
    if (stopPump_.load() || midiBackend_ == nullptr || midiBytes == nullptr || byteCount == 0)
    {
        return;
    }

    const std::size_t inPortIndex = findInPortIndex(cableIndex);
    if (inPortIndex >= inPortCount_)
    {
        // Non-product / Broadcast cables are ignored (no Virtual Port).
        return;
    }

    inFramers_[inPortIndex].Push(
        midiBytes,
        byteCount,
        [this, inPortIndex, cableIndex](const uint8_t* framed, std::size_t framedSize) {
            sendFramedToHost(inPortIndex, cableIndex, framed, framedSize);
        });
}

void DeviceSession::readerLoop()
{
    const std::size_t capacity = transport_.BulkInReadCapacity();
    uint8_t readBuffer[512] = {};
    if (capacity == 0 || capacity > sizeof(readBuffer))
    {
        recordPumpFailure("Device→host bulk IN read capacity is invalid");
        return;
    }

    while (!stopPump_.load())
    {
        std::size_t bytesRead = 0;
        std::string error;
        const bool readOk = transport_.ReadBulk(readBuffer, capacity, bytesRead, error);

        if (stopPump_.load())
        {
            break;
        }
        if (!readOk && transport_.LastReadTimedOut())
        {
            continue;
        }
        if (!readOk)
        {
            recordPumpFailure("Device→host ReadBulk failed: " + error);
            break;
        }
        if (bytesRead == 0)
        {
            continue;
        }
        if (!processBulkRead(readBuffer, bytesRead, error))
        {
            recordPumpFailure("Device→host DecodeFromDevice failed: " + error);
            break;
        }
    }
}

bool DeviceSession::processBulkRead(
    const uint8_t* readBuffer,
    std::size_t bytesRead,
    std::string& errorOut)
{
    // Count USB bytes before demux so empty-IN vs decode-fail is visible in lab counters.
    noteBulkReadCounters(bytesRead, 0);

    std::vector<std::pair<uint8_t, std::vector<uint8_t>>> pending;
    {
        std::lock_guard<std::mutex> lock(usbIoMutex_);
        if (stopPump_.load() || mapper_ == nullptr || midiBackend_ == nullptr)
        {
            return true;
        }

        if (!mapper_->DecodeFromDevice(
                readBuffer,
                bytesRead,
                [this, &pending](uint8_t cableIndex, const uint8_t* midi, std::size_t n) {
                    appendPendingProductMidi(pending, cableIndex, midi, n);
                },
                errorOut))
        {
            return false;
        }
    }

    if (pending.size() > 0)
    {
        noteBulkReadCounters(0, pending.size());
    }

    for (const auto& entry : pending)
    {
        if (stopPump_.load())
        {
            break;
        }
        forwardDeviceMidi(entry.first, entry.second.data(), entry.second.size());
    }

    return true;
}

void DeviceSession::appendPendingProductMidi(
    std::vector<std::pair<uint8_t, std::vector<uint8_t>>>& pending,
    uint8_t cableIndex,
    const uint8_t* midi,
    std::size_t n)
{
    if (stopPump_.load() || midi == nullptr || n == 0
        || findInPortIndex(cableIndex) >= inPortCount_)
    {
        return;
    }
    pending.emplace_back(cableIndex, std::vector<uint8_t>(midi, midi + n));
}
