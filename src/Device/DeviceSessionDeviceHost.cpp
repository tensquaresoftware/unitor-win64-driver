#include "Device/DeviceSession.h"
#include "Device/DeviceSessionSupport.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr std::size_t kBulkInBurstMaxPackets = 8;
} // namespace

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
    if (isIdentityReply(midiBytes, byteCount))
    {
        deviceHostCounters_.AddIdentityReplyIn();
        std::cerr << "device→host: Identity Reply SendToHost ok (in_port="
                  << (inPortIndex + 1) << " bytes=" << byteCount << ")\n"
                  << std::flush;
    }
}

void DeviceSession::noteFramerOversizeRejects(
    std::size_t inPortIndex,
    uint8_t cableIndex,
    std::uint64_t rejectCount)
{
    if (rejectCount == 0)
    {
        return;
    }
    recordPumpFailure(formatPortCableFailure(
        "Device→host SysEx incomplete or corrupt",
        inPortIndex,
        cableIndex,
        "dump rejected (oversize hold cap and/or nested F0 restart; kMaxSysexHoldBytes="
            + std::to_string(kMaxSysexHoldBytes) + ")"));
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

    const std::uint64_t rejects = inFramers_[inPortIndex].ConsumeOversizeSysexRejectCount();
    if (rejects > 0)
    {
        noteFramerOversizeRejects(inPortIndex, cableIndex, rejects);
    }
}

void DeviceSession::failReaderOnReadBulkError(const std::string& error)
{
    const std::size_t discarded = clearHostOutboundQueue();
    std::string detail = "Device→host ReadBulk failed: " + error;
    if (discarded > 0)
    {
        detail += "; discarded " + std::to_string(discarded) + " queued host→device message(s)";
    }
    recordPumpFailure(detail);
}

bool DeviceSession::collectBulkReadPending(
    const uint8_t* readBuffer,
    std::size_t bytesRead,
    std::vector<std::pair<uint8_t, std::vector<uint8_t>>>& pending,
    std::string& errorOut)
{
    // Caller holds usbIoMutex_.
    noteBulkReadCounters(bytesRead, 0);

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

    if (pending.size() > 0)
    {
        noteBulkReadCounters(0, pending.size());
    }
    return true;
}

void DeviceSession::forwardPendingProductMidi(
    const std::vector<std::pair<uint8_t, std::vector<uint8_t>>>& pending)
{
    for (const auto& entry : pending)
    {
        if (stopPump_.load())
        {
            break;
        }
        forwardDeviceMidi(entry.first, entry.second.data(), entry.second.size());
    }
}

bool DeviceSession::processBulkReadLocked(
    const uint8_t* readBuffer,
    std::size_t bytesRead,
    std::string& errorOut)
{
    // Caller holds usbIoMutex_. Forward under the lock so Encode cannot race demux.
    std::vector<std::pair<uint8_t, std::vector<uint8_t>>> pending;
    if (!collectBulkReadPending(readBuffer, bytesRead, pending, errorOut))
    {
        return false;
    }
    forwardPendingProductMidi(pending);
    return true;
}

int DeviceSession::readAndProcessOneBurstPacket(uint8_t* readBuffer, std::size_t capacity)
{
    if (stopPump_.load())
    {
        return 0;
    }

    std::size_t bytesRead = 0;
    std::string error;
    if (!transport_.ReadBulk(readBuffer, capacity, bytesRead, error))
    {
        if (transport_.LastReadTimedOut())
        {
            return 0;
        }
        failReaderOnReadBulkError(error);
        return -1;
    }
    if (bytesRead == 0)
    {
        return 0;
    }
    if (!processBulkReadLocked(readBuffer, bytesRead, error))
    {
        recordPumpFailure("Device→host DecodeFromDevice failed: " + error);
        return -1;
    }
    return 1;
}

bool DeviceSession::drainPendingBulkInBurst(uint8_t* readBuffer, std::size_t capacity)
{
    // Caller must hold usbIoMutex_ and must not have another ReadBulk pending.
    if (readBuffer == nullptr || capacity == 0 || stopPump_.load())
    {
        return true;
    }

    std::string error;
    if (!transport_.BeginShortBulkInDrain(error))
    {
        recordPumpFailure("Device→host short bulk IN drain arm failed: " + error);
        return false;
    }

    bool ok = true;
    for (std::size_t packet = 0; packet < kBulkInBurstMaxPackets; ++packet)
    {
        const int step = readAndProcessOneBurstPacket(readBuffer, capacity);
        if (step == 0)
        {
            break;
        }
        if (step < 0)
        {
            ok = false;
            break;
        }
    }

    std::string restoreError;
    if (!transport_.RestoreSessionBulkTimeouts(restoreError))
    {
        recordPumpFailure(
            "Device→host failed to restore session bulk timeouts: " + restoreError);
        return false;
    }
    return ok;
}

bool DeviceSession::handleReaderAfterSuccessfulRead(
    uint8_t* readBuffer,
    std::size_t capacity,
    std::size_t bytesRead,
    std::string& errorOut)
{
    if (bytesRead != 0 && !processBulkRead(readBuffer, bytesRead, errorOut))
    {
        recordPumpFailure("Device→host DecodeFromDevice failed: " + errorOut);
        return false;
    }
    if (bytesRead > 0)
    {
        // Catch a second USB packet that arrived during demux/forward.
        std::lock_guard<std::mutex> lock(usbIoMutex_);
        if (!drainPendingBulkInBurst(readBuffer, capacity))
        {
            return false;
        }
    }
    drainHostOutboundFromReader();
    return true;
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
            // No pending Read: safe to Write then briefly poll IN (Emagic half-duplex).
            drainHostOutboundFromReader();
            continue;
        }
        if (!readOk)
        {
            failReaderOnReadBulkError(error);
            break;
        }
        if (!handleReaderAfterSuccessfulRead(readBuffer, capacity, bytesRead, error))
        {
            break;
        }
    }
}

bool DeviceSession::processBulkRead(
    const uint8_t* readBuffer,
    std::size_t bytesRead,
    std::string& errorOut)
{
    std::vector<std::pair<uint8_t, std::vector<uint8_t>>> pending;
    {
        std::lock_guard<std::mutex> lock(usbIoMutex_);
        if (!collectBulkReadPending(readBuffer, bytesRead, pending, errorOut))
        {
            return false;
        }
    }
    forwardPendingProductMidi(pending);
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
