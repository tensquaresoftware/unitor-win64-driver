// Host→device outbound queue drain (librarian-scale SysEx bursts).

#include "Device/DeviceSession.h"
#include "Device/DeviceSessionSupport.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <utility>

namespace
{
constexpr std::size_t kEncodeBufferCapacity = 16384;

void appendDiscardedSuffix(std::string& detail, std::size_t discarded)
{
    if (discarded == 0)
    {
        return;
    }
    detail += "; discarded " + std::to_string(discarded) + " queued message(s)";
}
} // namespace

bool DeviceSession::hostOutboundWriteBlocked() const noexcept
{
    if (std::chrono::steady_clock::now() < hostOutEarliestSteady_)
    {
        return true;
    }
    if (anyInFramerHoldingSysex())
    {
        return true;
    }
    if (expectInBurstUntil_.time_since_epoch().count() != 0
        && std::chrono::steady_clock::now() < expectInBurstUntil_)
    {
        return true;
    }
    return false;
}

bool DeviceSession::expectInBurstActive() const noexcept
{
    return expectInBurstUntil_.time_since_epoch().count() != 0
        && std::chrono::steady_clock::now() < expectInBurstUntil_;
}

void DeviceSession::armExpectInBurstAfterHostSysex(
    std::size_t outPortIndex,
    const uint8_t* midiBytes,
    std::size_t midiBytesCount) noexcept
{
    // Only Matrix dump requests arm the post-request IN quiet window / F0 repair.
    constexpr auto kExpectWindow = std::chrono::milliseconds(3500);
    if (!isMatrixDumpRequest(midiBytes, midiBytesCount))
    {
        return;
    }
    expectInBurstUntil_ = std::chrono::steady_clock::now() + kExpectWindow;
    lastDumpOutPort_ = outPortIndex;
    lastDumpRequest_.assign(midiBytes, midiBytes + midiBytesCount);
    dumpRequestRetryRemaining_ = kMatrixDumpSizeRejectRetries;
}

void DeviceSession::clearExpectInBurst() noexcept
{
    expectInBurstUntil_ = {};
    dumpRequestRetryRemaining_ = 0;
    lastDumpRequest_.clear();
}

void DeviceSession::hostToDeviceThunk(
    void* context,
    std::size_t outPortIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount)
{
    auto* session = static_cast<DeviceSession*>(context);
    if (session == nullptr)
    {
        return;
    }
    session->handleHostMidi(outPortIndex, midiBytes, byteCount);
}

std::size_t DeviceSession::clearHostOutboundQueue() noexcept
{
    std::lock_guard<std::mutex> queueLock(hostOutboundMutex_);
    const std::size_t dropped = hostOutbound_.MessageCount();
    hostOutbound_.Clear();
    return dropped;
}

void DeviceSession::handleHostMidi(
    std::size_t outPortIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount)
{
    if (midiBytes == nullptr || byteCount == 0)
    {
        return;
    }
    if (stopPump_.load() || !running_.load())
    {
        return;
    }

    // Queue under a dedicated lock so VirtualMIDI PARSE_RX callbacks are not the
    // sole unbounded work path under usbIoMutex_ (AD-18 / NFR-R3). WriteBulk and
    // SendToHost run only on the reader thread — never nest teVirtualMIDI here.
    {
        std::lock_guard<std::mutex> queueLock(hostOutboundMutex_);
        if (!hostOutbound_.TryPush(outPortIndex, midiBytes, byteCount))
        {
            const uint8_t cable =
                (outPortIndex < outPortCount_) ? outCableByPort_[outPortIndex] : 0xFF;
            recordPumpFailure(formatPortCableFailure(
                "Host→device outbound queue overflow",
                outPortIndex,
                cable,
                "librarian-scale burst exceeded bounded queue"));
            return;
        }
    }

    transport_.signalBulkInDataReady();
    // Do not drain here: WriteBulk / SendToHost must stay on the reader thread
    // (teVirtualMIDI PARSE_RX must not nest host→device OUT or deferred SendToHost).
}

void DeviceSession::drainHostOutbound()
{
    // Opportunistic drain when the reader is waiting (lock free). WriteBulk runs
    // while other bulk IN async slots remain submitted (Linux INPUT_URBS model).
    std::unique_lock<std::mutex> lock(usbIoMutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return;
    }
    drainHostOutboundLocked(lock);
}

void DeviceSession::failHostOutboundDrain(const std::string& reason)
{
    const std::size_t discarded = clearHostOutboundQueue();
    if (discarded == 0)
    {
        recordPumpFailure(reason);
        return;
    }
    recordPumpFailure(reason + " (discarded " + std::to_string(discarded) + " queued message(s))");
}

void DeviceSession::noteHostOutboundCounters(
    const HostOutboundItem& item,
    std::size_t encodedBytes) noexcept
{
    deviceHostCounters_.AddHostOutOk();
    if (!isUniversalDeviceInquiry(item.midi.data(), item.midi.size()))
    {
        return;
    }

    deviceHostCounters_.AddInquiryOut();
    long long msSinceRing = -1;
    const std::int64_t armedMs =
        bulkInRingArmedSteadyMs_.load(std::memory_order_acquire);
    if (armedMs >= 0)
    {
        const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now().time_since_epoch())
                               .count();
        msSinceRing = nowMs - armedMs;
    }
    logDeviceInquiryHostOut(DeviceInquiryHostOutDiag{
        item.outPortIndex,
        item.midi.size(),
        encodedBytes,
        encodedBytes >= item.midi.size() + 3,
        transport_.IsBulkInAsyncRingActive(),
        msSinceRing,
        transport_.CountPendingBulkInSlots(),
        kBulkInAsyncSlotCount,
        !firstHostInquiryLogged_.exchange(true)});
}

bool DeviceSession::finishHostOutboundWrite(const HostOutWriteFinishArgs& finish)
{
    if (betweenOutChunkDemuxFailed_)
    {
        return failHostOutboundBetweenChunkDemux(
            finish.item->outPortIndex, finish.cableIndex);
    }
    if (!finish.wrote)
    {
        const std::size_t discarded = clearHostOutboundQueue();
        appendDiscardedSuffix(*finish.error, discarded);
        recordPumpFailure(formatPortCableFailure(
            "Host→device WriteBulk",
            finish.item->outPortIndex,
            finish.cableIndex,
            *finish.error));
        return false;
    }
    noteHostOutboundCounters(*finish.item, finish.encodedBytes);
    logLongSysexHostOutWrite(
        *finish.item,
        finish.encodedBytes,
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - finish.outStarted),
        finish.deliverDepthAtStart);
    if (!finish.item->midi.empty() && finish.item->midi[0] == 0xF0)
    {
        armExpectInBurstAfterHostSysex(
            finish.item->outPortIndex, finish.item->midi.data(), finish.item->midi.size());
    }
    return true;
}

bool DeviceSession::writeHostOutboundItem(
    const HostOutboundItem& item,
    HostEncodeScratch& scratch,
    std::unique_lock<std::mutex>& /*usbIoLock*/)
{
    std::string error;
    betweenOutChunkDemuxFailed_ = false;
    deferredHostSends_.clear();
    deferHostSendDuringOut_ = true;
    if (!item.midi.empty() && item.midi[0] == 0xF0 && item.midi.size() >= 512)
    {
        resetLongSysexGapProbe();
    }
    const auto outStarted = std::chrono::steady_clock::now();
    const std::size_t deliverDepthAtStart = bulkInDeliverQueueDepth();
    // Drain+frame IN between Emagic OUT packets while usbIoMutex_ stays held;
    // SendToHost is deferred until Write finishes (avoid nested teVirtualMIDI).
    const WinUsbTransport::EmagicBetweenChunks between{
        &DeviceSession::betweenOutChunksDrainIn, this, &betweenOutChunkDemuxFailed_};
    const bool wrote =
        transport_.WriteEmagicHostMidi(scratch.bytes, scratch.size, error, &between);
    // Arm expect before flush so an early dump reply can clearExpect (no phantom window).
    HostOutWriteFinishArgs finish;
    finish.item = &item;
    finish.cableIndex = scratch.cableIndex;
    finish.encodedBytes = scratch.size;
    finish.error = &error;
    finish.outStarted = outStarted;
    finish.deliverDepthAtStart = deliverDepthAtStart;
    finish.wrote = wrote;
    const bool finishedOk = finishHostOutboundWrite(finish);
    flushDeferredHostSends();
    // Stop gap sampling after deferred SendToHost so later traffic does not keep
    // calling CountArmedBulkInSlots on every bulk IN enqueue.
    if (!item.midi.empty() && item.midi[0] == 0xF0 && item.midi.size() >= 512)
    {
        longSysexGapProbeActive_.store(false, std::memory_order_release);
    }
    return finishedOk;
}

void DeviceSession::restoreMapperOutCable(uint8_t previousOutCable) noexcept
{
    if (mapper_ != nullptr)
    {
        mapper_->RestoreOutCable(previousOutCable);
    }
}

bool DeviceSession::encodeWritePopOneHostOutbound(
    HostEncodeScratch& scratch,
    std::unique_lock<std::mutex>& usbIoLock)
{
    HostOutboundItem item;
    {
        std::lock_guard<std::mutex> queueLock(hostOutboundMutex_);
        if (!hostOutbound_.TryCopyFront(item))
        {
            return false;
        }
    }

    scratch.size = 0;
    scratch.cableIndex = 0;
    const uint8_t previousOutCable =
        mapper_ != nullptr ? mapper_->CurrentOutCable() : static_cast<uint8_t>(0xFF);
    if (!encodeHostMidiLocked(item.outPortIndex, item.midi.data(), item.midi.size(), scratch))
    {
        failHostOutboundDrain("Host→device encode path aborted");
        return false;
    }
    if (stopPump_.load() || !running_.load())
    {
        restoreMapperOutCable(previousOutCable);
        failHostOutboundDrain("Host→device pump stopped during drain");
        return false;
    }
    if (!writeHostOutboundItem(item, scratch, usbIoLock))
    {
        restoreMapperOutCable(previousOutCable);
        return false;
    }

    std::lock_guard<std::mutex> queueLock(hostOutboundMutex_);
    HostOutboundItem committed;
    return hostOutbound_.TryPop(committed);
}

void DeviceSession::drainHostOutboundLocked(std::unique_lock<std::mutex>& usbIoLock)
{
    if (!transport_.IsBulkInAsyncRingActive())
    {
        if (running_.load() && hostOutboundPending())
        {
            failHostOutboundDrain(
                "Host→device WriteBulk skipped: bulk IN async ring inactive");
        }
        return;
    }
    clearExpectInBurstIfExpired();
    if (hostOutboundWriteBlocked())
    {
        return;
    }

    uint8_t encodeBytes[kEncodeBufferCapacity] = {};
    HostEncodeScratch scratch{encodeBytes, sizeof(encodeBytes), 0, 0};
    while (!stopPump_.load())
    {
        {
            std::lock_guard<std::mutex> queueLock(hostOutboundMutex_);
            if (hostOutbound_.MessageCount() == 0)
            {
                return;
            }
        }
        if (hostOutboundWriteBlocked() || !encodeWritePopOneHostOutbound(scratch, usbIoLock))
        {
            return;
        }
        if (!usbIoLock.owns_lock())
        {
            failHostOutboundDrain("Host→device usbIoMutex lost during OUT chunking");
            return;
        }
    }
    failHostOutboundDrain("Host→device pump stopped");
}

bool DeviceSession::encodeHostMidiLocked(
    std::size_t outPortIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount,
    HostEncodeScratch& scratch)
{
    if (stopPump_.load() || !running_.load() || mapper_ == nullptr || midiBackend_ == nullptr)
    {
        return false;
    }
    if (outPortIndex >= outPortCount_)
    {
        recordPumpFailure(
            "Host→device rejected invalid OUT port index " + std::to_string(outPortIndex));
        return false;
    }

    scratch.cableIndex = outCableByPort_[outPortIndex];
    EncodeRequest request{scratch.cableIndex, midiBytes, byteCount};
    EncodeBuffer buffer{scratch.bytes, scratch.capacity, 0};
    std::string error;
    if (!mapper_->EncodeToDevice(request, buffer, error))
    {
        recordPumpFailure(formatPortCableFailure(
            "Host→device encode", outPortIndex, scratch.cableIndex, error));
        return false;
    }
    scratch.size = buffer.size;
    return true;
}
