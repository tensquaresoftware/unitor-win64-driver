// Bulk IN deliver-queue demux (reader drain + OUT between-chunk drain).

#include "Device/DeviceSession.h"
#include "Device/DeviceSessionSupport.h"

#include <chrono>
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
    std::cerr << "host→device: long SysEx WriteBulk ok (out_port=" << (item.outPortIndex + 1)
              << " midi_bytes=" << item.midi.size() << " encoded_bytes=" << encodedBytes
              << " out_ms=" << outMs.count()
              << " deliver_q=" << deliverDepthAtStart << "->" << bulkInDeliverQueueDepth()
              << " deliver_hw=" << bulkInDeliverHighWater_.load()
              << " pending_urbs=" << transport_.CountPendingBulkInSlots() << "/"
              << kBulkInAsyncSlotCount << ")\n"
              << std::flush;
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
    // Overlapped OUT already pumps this callback during Wait — no Sleep here
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
            continue;
        }
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
    // Matrix dump one byte short of trailing F7 � narrow idle finalize only.
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
}

void DeviceSession::abandonIdlePartialSysexHold(std::size_t inPortIndex)
{
    std::lock_guard<std::mutex> lock(usbIoMutex_);
    abandonIdlePartialSysexHoldUnlocked(inPortIndex);
}
