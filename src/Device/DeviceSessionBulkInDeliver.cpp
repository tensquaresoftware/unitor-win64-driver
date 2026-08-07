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
