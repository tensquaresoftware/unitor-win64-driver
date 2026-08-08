#include "Device/DeviceSession.h"
#include "Device/DeviceSessionSupport.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace
{
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

bool DeviceSession::anyInFramerHoldingSysex() const noexcept
{
    for (std::size_t index = 0; index < inPortCount_; ++index)
    {
        if (inFramers_[index].IsHoldingSysEx())
        {
            return true;
        }
    }
    return false;
}

void DeviceSession::finalizeOneHeldSysex(std::size_t inPortIndex)
{
    MidiMessageFramer& framer = inFramers_[inPortIndex];
    const uint8_t cableIndex = inCableByPort_[inPortIndex];
    const std::size_t held = framer.HeldSysexSize();
    framer.FinalizeHeldSysex(
        [this, inPortIndex, cableIndex](const uint8_t* framed, std::size_t framedSize) {
            sendFramedToHost(inPortIndex, cableIndex, framed, framedSize);
        });
    std::cerr << "SysEx idle-finalize: in_port=" << inPortIndex << " cable="
              << static_cast<unsigned>(cableIndex) << " held_before_f7=" << held
              << "\n"
              << std::flush;
    const std::uint64_t rejects = framer.ConsumeOversizeSysexRejectCount();
    if (rejects > 0)
    {
        noteFramerOversizeRejects(inPortIndex, cableIndex, rejects);
    }
}

void DeviceSession::noteBulkReadCounters(std::size_t bulkBytes, std::size_t demuxSpans)
{
    deviceHostCounters_.AddBulkAndDemux(bulkBytes, demuxSpans);
    // Pad-only Emagic URBs produce no MIDI. Refresh the idle clock for long /
    // partial SysEx holds so they are not abandoned mid-transfer — but never for
    // Matrix near-complete holds (274/350): pad must not postpone trailing-F7
    // idle-finalize (overnight TIMEOUT with no frames).
    if (bulkBytes == 0 || !anyInFramerHoldingSysex())
    {
        return;
    }
    for (std::size_t index = 0; index < inPortCount_; ++index)
    {
        if (!inFramers_[index].IsHoldingSysEx())
        {
            continue;
        }
        const std::size_t held = inFramers_[index].HeldSysexSize();
        if (held != 274 && held != 350)
        {
            lastBulkInPacketSteady_ = std::chrono::steady_clock::now();
            return;
        }
    }
}

void DeviceSession::noteSendToHostSuccess(
    std::size_t inPortIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount)
{
    deviceHostCounters_.AddSendOk();
    logLongSysexSendToHost(
        inPortIndex, midiBytes, byteCount, bulkInDeliverHighWater_.load());
    if (isMatrixDumpReply(midiBytes, byteCount) && isExactMatrixDumpLength(byteCount))
    {
        clearExpectInBurst();
    }
    if (isIdentityReply(midiBytes, byteCount))
    {
        deviceHostCounters_.AddIdentityReplyIn();
        std::cerr << "device→host: Identity Reply SendToHost ok (in_port="
                  << (inPortIndex + 1) << " bytes=" << byteCount << ")\n"
                  << std::flush;
    }
}

void DeviceSession::sendFramedToHost(
    std::size_t inPortIndex,
    uint8_t cableIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount)
{
    if (expectInBurstActive() && isMatrixDumpReply(midiBytes, byteCount)
        && !isExactMatrixDumpLength(byteCount))
    {
        (void)rejectShortMatrixDumpAndRetry(byteCount);
        return;
    }

    if (deferHostSendDuringOut_)
    {
        DeferredHostSend deferred;
        deferred.inPortIndex = inPortIndex;
        deferred.cableIndex = cableIndex;
        deferred.midi.assign(midiBytes, midiBytes + byteCount);
        deferredHostSends_.push_back(std::move(deferred));
        return;
    }

    std::string error;
    if (!midiBackend_->SendToHost(inPortIndex, midiBytes, byteCount, error))
    {
        deviceHostCounters_.AddSendFail();
        recordPumpFailure(
            formatPortCableFailure("Device→host SendToHost", inPortIndex, cableIndex, error));
        return;
    }
    noteSendToHostSuccess(inPortIndex, midiBytes, byteCount);
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
        return;
    }

    static thread_local std::vector<uint8_t> repairStorage;
    const bool armRepair =
        expectInBurstActive() && !inFramers_[inPortIndex].IsHoldingSysEx();
    const MidiPushView push =
        maybePrependLostLeadingF0(armRepair, midiBytes, byteCount, repairStorage);
    inFramers_[inPortIndex].Push(
        push.bytes,
        push.count,
        [this, inPortIndex, cableIndex](const uint8_t* framed, std::size_t framedSize) {
            sendFramedToHost(inPortIndex, cableIndex, framed, framedSize);
        });
    maybeLogFirstBurst(cableIndex, inPortIndex, midiBytes, byteCount);

    const std::uint64_t rejects = inFramers_[inPortIndex].ConsumeOversizeSysexRejectCount();
    if (rejects > 0)
    {
        noteFramerOversizeRejects(inPortIndex, cableIndex, rejects);
    }
}

void DeviceSession::maybeLogFirstBurst(
    uint8_t cableIndex,
    std::size_t inPortIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount)
{
    if (firstBurstDiagRemaining_.load(std::memory_order_relaxed) == 0
        || firstBurstDiagRemaining_.fetch_sub(1, std::memory_order_relaxed) == 0)
    {
        return;
    }
    const MidiMessageFramer& framer = inFramers_[inPortIndex];
    logFirstBurstSpan(FirstBurstDiag{
        cableIndex,
        midiBytes,
        byteCount,
        framer.IsHoldingSysEx(),
        framer.HeldSysexSize(),
        transport_.CountPendingBulkInSlots()});
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

bool DeviceSession::bulkInPacketThunk(
    void* context,
    const uint8_t* data,
    std::size_t size)
{
    auto* session = static_cast<DeviceSession*>(context);
    if (session == nullptr || session->stopPump_.load())
    {
        return true;
    }
    return session->enqueueBulkInPacket(data, size);
}

bool DeviceSession::enqueueBulkInPacket(const uint8_t* data, std::size_t size)
{
    if (size > 512)
    {
        recordPumpFailure(
            "Device→host bulk IN enqueue rejected: packet size "
            + std::to_string(size) + " exceeds 512");
        return false;
    }
    std::lock_guard<std::mutex> lock(bulkInDeliverMutex_);
    if (bulkInDeliverQueue_.size() >= kMaxQueuedBulkInPackets)
    {
        recordPumpFailure(
            "Device→host bulk IN enqueue rejected: deliver queue full ("
            + std::to_string(kMaxQueuedBulkInPackets)
            + ") during host→device WriteBurst");
        return false;
    }
    QueuedBulkInPacket packet;
    packet.size = size;
    if (size > 0 && data != nullptr)
    {
        std::memcpy(packet.data.data(), data, size);
    }
    bulkInDeliverQueue_.push_back(packet);
    const std::size_t depth = bulkInDeliverQueue_.size();
    std::size_t high = bulkInDeliverHighWater_.load(std::memory_order_relaxed);
    while (depth > high
        && !bulkInDeliverHighWater_.compare_exchange_weak(
            high, depth, std::memory_order_relaxed))
    {
    }
    return true;
}

int DeviceSession::drainQueuedBulkInPackets()
{
    std::lock_guard<std::mutex> usbLock(usbIoMutex_);
    if (stopPump_.load())
    {
        return 0;
    }
    return drainQueuedBulkInPacketsHoldingUsbIo() ? 1 : -1;
}

int DeviceSession::readerWaitOnceAsync()
{
    // Completion thread: harvest → reorder → enqueue. Reader: demux + VirtualMIDI + OUT.
    const std::uint32_t timeoutMs =
        hostOutboundPending() ? kReaderOutboundWaitMs : kReaderIdleWaitMs;

    std::string error;
    const int waitRc = transport_.WaitBulkInReaderTick(timeoutMs, error);
    if (stopPump_.load())
    {
        return 0;
    }
    if (waitRc < 0)
    {
        failReaderOnReadBulkError(error);
        return -1;
    }

    const int drainRc = drainQueuedBulkInPackets();
    if (drainRc <= 0)
    {
        return drainRc;
    }

    if (anyInFramerHoldingSysex())
    {
        finalizeIdleHeldSysex();
    }
    if (!hostOutboundWriteBlocked())
    {
        drainHostOutbound();
    }
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
    transport_.ClearBulkInPacketHandler();
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
    // Only MIDI bytes reset the SysEx idle-finalize clock (pad-only USB packets must not).
    lastBulkInPacketSteady_ = std::chrono::steady_clock::now();
    pending.emplace_back(cableIndex, std::vector<uint8_t>(midi, midi + n));
}
