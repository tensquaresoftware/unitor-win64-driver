#include "Device/DeviceSession.h"
#include "Device/DeviceSessionSupport.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace
{
// Wake often enough to drain host→device while IN URBs stay pending (Linux model).
constexpr std::uint32_t kReaderIdleWaitMs = 50;
constexpr std::uint32_t kReaderOutboundWaitMs = 5;
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

bool DeviceSession::hostOutboundPending() const
{
    std::lock_guard<std::mutex> queueLock(hostOutboundMutex_);
    return hostOutbound_.MessageCount() > 0;
}

bool DeviceSession::processAsyncBulkInPacket(const BulkInAsyncPacket& packet)
{
    if (packet.size == 0)
    {
        return true;
    }

    std::string error;
    std::lock_guard<std::mutex> lock(usbIoMutex_);
    if (!processBulkReadLocked(packet.data, packet.size, error))
    {
        recordPumpFailure("Device→host DecodeFromDevice failed: " + error);
        return false;
    }
    return true;
}

bool DeviceSession::processAndResubmitAsyncPacket(const BulkInAsyncPacket& packet)
{
    std::string error;
    if (!processAsyncBulkInPacket(packet))
    {
        return false;
    }
    if (!transport_.ResubmitBulkInAsyncSlot(packet.slot, error))
    {
        if (!stopPump_.load())
        {
            failReaderOnReadBulkError(error);
        }
        return false;
    }
    return true;
}

int DeviceSession::harvestReadyAsyncPackets(BulkInAsyncPacket firstPacket)
{
    BulkInAsyncPacket packet = firstPacket;
    for (;;)
    {
        if (!processAndResubmitAsyncPacket(packet))
        {
            return stopPump_.load() ? 0 : -1;
        }
        if (stopPump_.load())
        {
            return 0;
        }

        BulkInAsyncPacket more;
        std::string error;
        const int moreRc = transport_.WaitBulkInAsyncPacket(0, more, error);
        if (moreRc == 0)
        {
            return 1;
        }
        if (moreRc < 0)
        {
            // AbortPipe during Stop is expected — do not record a false pump failure.
            if (stopPump_.load())
            {
                return 0;
            }
            failReaderOnReadBulkError(error);
            return -1;
        }
        packet = more;
    }
}

int DeviceSession::readerWaitOnceAsync()
{
    // No usbIoMutex_ during Wait — other IN slots stay pending while OUT writes.
    const std::uint32_t timeoutMs =
        hostOutboundPending() ? kReaderOutboundWaitMs : kReaderIdleWaitMs;

    BulkInAsyncPacket packet;
    std::string error;
    const int waitRc = transport_.WaitBulkInAsyncPacket(timeoutMs, packet, error);
    if (stopPump_.load())
    {
        return 0;
    }
    if (waitRc < 0)
    {
        if (stopPump_.load())
        {
            return 0;
        }
        failReaderOnReadBulkError(error);
        return -1;
    }
    if (waitRc == 1)
    {
        // Harvest completed slots before sleeping (keep INPUT_URBS depth).
        const int harvestRc = harvestReadyAsyncPackets(packet);
        if (harvestRc <= 0)
        {
            return harvestRc;
        }
    }

    drainHostOutbound();
    return 1;
}

void DeviceSession::readerLoop()
{
    // Ring is armed in startPump before the host MIDI sink goes live.
    while (!stopPump_.load())
    {
        if (readerWaitOnceAsync() <= 0)
        {
            break;
        }
    }

    transport_.StopBulkInAsyncRing();
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
