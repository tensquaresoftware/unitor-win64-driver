// Bulk IN deliver-queue demux (reader drain + OUT between-chunk drain).

#include "Device/DeviceSession.h"
#include "Device/DeviceSessionSupport.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace
{
void appendDiscardedSuffix(std::string& detail, std::size_t discarded)
{
    if (discarded == 0)
    {
        return;
    }
    detail += "; discarded " + std::to_string(discarded) + " queued message(s)";
}
} // namespace

void DeviceSession::resetLongSysexGapProbe() noexcept
{
    gapEnqueuedPackets_.store(0, std::memory_order_relaxed);
    gapEnqueuedBytes_.store(0, std::memory_order_relaxed);
    gapEnqueuedExact32_.store(0, std::memory_order_relaxed);
    gapEnqueuedSize0_.store(0, std::memory_order_relaxed);
    gapDrainedPackets_.store(0, std::memory_order_relaxed);
    gapDrainedUsbBytes_.store(0, std::memory_order_relaxed);
    gapHoldPushMidiBytes_.store(0, std::memory_order_relaxed);
    gapMinArmedUrbs_.store(kBulkInAsyncSlotCount, std::memory_order_relaxed);
    gapMaxDeliverDepth_.store(0, std::memory_order_relaxed);
    gapRejectEnqueue_.store(0, std::memory_order_relaxed);
    if (mapper_ != nullptr)
    {
        mapper_->ClearSysexF5DiagCounts();
    }
    longSysexGapProbeActive_.store(true, std::memory_order_release);
}

void DeviceSession::noteGapProbeMinArmed() noexcept
{
    if (!longSysexGapProbeActive_.load(std::memory_order_acquire))
    {
        return;
    }
    const std::size_t armed = transport_.CountArmedBulkInSlots();
    std::size_t minArmed = gapMinArmedUrbs_.load(std::memory_order_relaxed);
    while (armed < minArmed
        && !gapMinArmedUrbs_.compare_exchange_weak(
            minArmed, armed, std::memory_order_relaxed))
    {
    }
}

void DeviceSession::noteGapProbeEnqueue(std::size_t usbBytes) noexcept
{
    if (!longSysexGapProbeActive_.load(std::memory_order_acquire))
    {
        return;
    }
    gapEnqueuedPackets_.fetch_add(1, std::memory_order_relaxed);
    gapEnqueuedBytes_.fetch_add(usbBytes, std::memory_order_relaxed);
    if (usbBytes == 32)
    {
        gapEnqueuedExact32_.fetch_add(1, std::memory_order_relaxed);
    }
    else if (usbBytes == 0)
    {
        gapEnqueuedSize0_.fetch_add(1, std::memory_order_relaxed);
    }
    noteGapProbeMinArmed();
}

void DeviceSession::noteGapProbeDrain(std::size_t usbBytes) noexcept
{
    if (!longSysexGapProbeActive_.load(std::memory_order_acquire))
    {
        return;
    }
    gapDrainedPackets_.fetch_add(1, std::memory_order_relaxed);
    gapDrainedUsbBytes_.fetch_add(usbBytes, std::memory_order_relaxed);
    noteGapProbeMinArmed();
}

void DeviceSession::noteGapProbeHoldPush(std::size_t midiBytes) noexcept
{
    if (!longSysexGapProbeActive_.load(std::memory_order_acquire) || midiBytes == 0)
    {
        return;
    }
    gapHoldPushMidiBytes_.fetch_add(midiBytes, std::memory_order_relaxed);
}

void DeviceSession::maybeNoteGapProbeHoldPush(
    std::size_t inPortIndex,
    const MidiPushView& push) noexcept
{
    const bool openHold = inFramers_[inPortIndex].IsHoldingSysEx();
    const bool startsSysex = push.count > 0 && push.bytes != nullptr && push.bytes[0] == 0xF0;
    if (openHold || startsSysex)
    {
        noteGapProbeHoldPush(push.count);
    }
}

void DeviceSession::noteGapProbeDeliverDepth(std::size_t depth) noexcept
{
    if (!longSysexGapProbeActive_.load(std::memory_order_acquire))
    {
        return;
    }
    std::size_t maxDepth = gapMaxDeliverDepth_.load(std::memory_order_relaxed);
    while (depth > maxDepth
        && !gapMaxDeliverDepth_.compare_exchange_weak(
            maxDepth, depth, std::memory_order_relaxed))
    {
    }
}

LongSysexGapProbeSnapshot DeviceSession::snapshotLongSysexGapProbe() const noexcept
{
    LongSysexGapProbeSnapshot snap;
    snap.enqueuedPackets = gapEnqueuedPackets_.load(std::memory_order_relaxed);
    snap.enqueuedBytes = gapEnqueuedBytes_.load(std::memory_order_relaxed);
    snap.enqueuedExact32 = gapEnqueuedExact32_.load(std::memory_order_relaxed);
    snap.enqueuedSize0 = gapEnqueuedSize0_.load(std::memory_order_relaxed);
    snap.drainedPackets = gapDrainedPackets_.load(std::memory_order_relaxed);
    snap.drainedUsbBytes = gapDrainedUsbBytes_.load(std::memory_order_relaxed);
    snap.holdPushMidiBytes = gapHoldPushMidiBytes_.load(std::memory_order_relaxed);
    snap.minArmedUrbs = gapMinArmedUrbs_.load(std::memory_order_relaxed);
    snap.maxDeliverDepth = gapMaxDeliverDepth_.load(std::memory_order_relaxed);
    snap.rejectEnqueue = gapRejectEnqueue_.load(std::memory_order_relaxed);
    if (mapper_ != nullptr)
    {
        snap.sysexF5Strips = mapper_->SysexF5StripCount();
        snap.stickyF5SysexPreserves = mapper_->StickyF5SysexPreserveCount();
    }
    return snap;
}

bool DeviceSession::tryPushBulkInPacket(
    const uint8_t* data,
    std::size_t size,
    std::size_t& depthOut)
{
    std::lock_guard<std::mutex> lock(bulkInDeliverMutex_);
    if (bulkInDeliverQueue_.size() >= kMaxQueuedBulkInPackets)
    {
        return false;
    }
    QueuedBulkInPacket packet;
    packet.size = size;
    if (size > 0 && data != nullptr)
    {
        std::memcpy(packet.data.data(), data, size);
    }
    bulkInDeliverQueue_.push_back(packet);
    depthOut = bulkInDeliverQueue_.size();
    std::size_t high = bulkInDeliverHighWater_.load(std::memory_order_relaxed);
    while (depthOut > high
        && !bulkInDeliverHighWater_.compare_exchange_weak(
            high, depthOut, std::memory_order_relaxed))
    {
    }
    return true;
}

bool DeviceSession::enqueueBulkInPacket(const uint8_t* data, std::size_t size)
{
    if (size > 512)
    {
        if (longSysexGapProbeActive_.load(std::memory_order_acquire))
        {
            gapRejectEnqueue_.fetch_add(1, std::memory_order_relaxed);
        }
        recordPumpFailure(
            "Device→host bulk IN enqueue rejected: packet size "
            + std::to_string(size) + " exceeds 512");
        return false;
    }
    std::size_t depth = 0;
    if (!tryPushBulkInPacket(data, size, depth))
    {
        if (longSysexGapProbeActive_.load(std::memory_order_acquire))
        {
            gapRejectEnqueue_.fetch_add(1, std::memory_order_relaxed);
        }
        recordPumpFailure(
            "Device→host bulk IN enqueue rejected: deliver queue full ("
            + std::to_string(kMaxQueuedBulkInPackets)
            + ") during host→device WriteBurst");
        return false;
    }
    noteGapProbeEnqueue(size);
    noteGapProbeDeliverDepth(depth);
    return true;
}

bool DeviceSession::failHostOutboundBetweenChunkDemux(
    std::size_t outPortIndex,
    uint8_t cableIndex)
{
    const std::size_t discarded = clearHostOutboundQueue();
    std::string detail =
        "IN demux failed during host→device WriteEmagicHostMidi chunking";
    appendDiscardedSuffix(detail, discarded);
    recordPumpFailure(
        formatPortCableFailure("Host→device WriteBulk", outPortIndex, cableIndex, detail));
    return false;
}

std::size_t DeviceSession::bulkInDeliverQueueDepth()
{
    std::lock_guard<std::mutex> deliverLock(bulkInDeliverMutex_);
    return bulkInDeliverQueue_.size();
}

void DeviceSession::logLongSysexHostOutWrite(
    const HostOutboundItem& item,
    std::size_t encodedBytes,
    std::chrono::milliseconds outMs,
    std::size_t deliverDepthAtStart)
{
    if (item.midi.empty() || item.midi[0] != 0xF0 || item.midi.size() < 512)
    {
        return;
    }
    const LongSysexGapProbeSnapshot gap = snapshotLongSysexGapProbe();
    std::cerr << "host→device: long SysEx WriteBulk ok (out_port=" << (item.outPortIndex + 1)
              << " midi_bytes=" << item.midi.size() << " encoded_bytes=" << encodedBytes
              << " out_ms=" << outMs.count()
              << " deliver_q=" << deliverDepthAtStart << "->" << bulkInDeliverQueueDepth()
              << " deliver_hw=" << bulkInDeliverHighWater_.load()
              << " pending_urbs=" << transport_.CountPendingBulkInSlots() << "/"
              << kBulkInAsyncSlotCount
              << " armed_urbs=" << transport_.CountArmedBulkInSlots() << "/"
              << kBulkInAsyncSlotCount
              << " gap_min_armed=" << gap.minArmedUrbs;
    if (gap.minArmedUrbs == 0)
    {
        std::cerr << " gap_armed_starved=yes";
    }
    if (gap.rejectEnqueue != 0)
    {
        std::cerr << " gap_reject_enq=" << gap.rejectEnqueue;
    }
    std::cerr << ")\n" << std::flush;
}

void DeviceSession::betweenOutChunksDrainIn(void* context)
{
    auto* session = static_cast<DeviceSession*>(context);
    if (session == nullptr || session->stopPump_.load())
    {
        return;
    }
    if (!session->drainQueuedBulkInPacketsHoldingUsbIo())
    {
        session->betweenOutChunkDemuxFailed_ = true;
        return;
    }
    // Overlapped OUT already pumps this callback during Wait → no Sleep here
    // (Sleep under usbIoMutex_ only delayed the next chunk without helping IN).
}

void DeviceSession::flushDeferredHostSends()
{
    deferHostSendDuringOut_ = false;
    std::vector<DeferredHostSend> pending;
    pending.swap(deferredHostSends_);
    for (const DeferredHostSend& item : pending)
    {
        if (stopPump_.load() || item.midi.empty())
        {
            continue;
        }
        sendFramedToHost(item.inPortIndex, item.cableIndex, item.midi.data(), item.midi.size());
    }
}

bool DeviceSession::drainQueuedBulkInPacketsHoldingUsbIo()
{
    // Caller holds usbIoMutex_. Never lock it here (OUT between-chunks path).
    for (;;)
    {
        QueuedBulkInPacket queued;
        {
            std::lock_guard<std::mutex> lock(bulkInDeliverMutex_);
            if (bulkInDeliverQueue_.empty())
            {
                return true;
            }
            queued = bulkInDeliverQueue_.front();
            bulkInDeliverQueue_.pop_front();
        }
        if (stopPump_.load())
        {
            return true;
        }
        if (queued.size == 0)
        {
            noteGapProbeDrain(0);
            continue;
        }
        noteGapProbeDrain(queued.size);
        std::string error;
        if (!processBulkReadLocked(queued.data.data(), queued.size, error))
        {
            recordPumpFailure("Device→host DecodeFromDevice failed: " + error);
            return false;
        }
    }
}

void DeviceSession::finalizeIdleHeldSysex()
{
    // Matrix dump one byte short of trailing F7 ? narrow idle finalize only.
    constexpr auto kIdleFinalize = std::chrono::milliseconds(80);
    // Abandon only small stuck holds. Long SysEx can pause on pad-only URBs.
    constexpr auto kAbandonHold = std::chrono::milliseconds(500);
    constexpr std::size_t kAbandonMaxHeldBytes = 400;
    const auto now = std::chrono::steady_clock::now();
    if (lastBulkInPacketSteady_.time_since_epoch().count() == 0
        || now - lastBulkInPacketSteady_ < kIdleFinalize)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(usbIoMutex_);
    for (std::size_t index = 0; index < inPortCount_; ++index)
    {
        if (!inFramers_[index].IsHoldingSysEx())
        {
            continue;
        }
        const std::size_t held = inFramers_[index].HeldSysexSize();
        if (held == 274 || held == 350)
        {
            finalizeOneHeldSysex(index);
            continue;
        }
        if (held <= kAbandonMaxHeldBytes
            && now - lastBulkInPacketSteady_ >= kAbandonHold)
        {
            abandonIdlePartialSysexHoldUnlocked(index);
        }
    }
}

void DeviceSession::abandonIdlePartialSysexHoldUnlocked(std::size_t inPortIndex)
{
    if (!inFramers_[inPortIndex].IsHoldingSysEx())
    {
        return;
    }
    const std::size_t abandoned = inFramers_[inPortIndex].HeldSysexSize();
    inFramers_[inPortIndex].Reset();
    std::cerr << "SysEx hold abandoned after idle: in_port=" << inPortIndex
              << " held=" << abandoned << "\n"
              << std::flush;
    // While waiting for a Matrix dump, abandon must not leave a silent expect
    // window (lab TIMEOUT last=none). Re-issue the dump request once if allowed.
    if (expectInBurstActive() && !lastDumpRequest_.empty())
    {
        (void)rejectShortMatrixDumpAndRetry(abandoned);
    }
}

void DeviceSession::abandonIdlePartialSysexHold(std::size_t inPortIndex)
{
    std::lock_guard<std::mutex> lock(usbIoMutex_);
    abandonIdlePartialSysexHoldUnlocked(inPortIndex);
}
