// Host→device outbound queue drain (librarian-scale SysEx bursts).

#include "Device/DeviceSession.h"
#include "Device/DeviceSessionSupport.h"

#include <chrono>
#include <iostream>
#include <mutex>
#include <string>

namespace
{
constexpr std::size_t kEncodeBufferCapacity = 4096;

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
    dumpRequestRetryRemaining_ = 1;
}

void DeviceSession::clearExpectInBurst() noexcept
{
    expectInBurstUntil_ = {};
    dumpRequestRetryRemaining_ = 0;
    lastDumpRequest_.clear();
}

bool DeviceSession::rejectShortMatrixDumpAndRetry(std::size_t gotLength)
{
    std::cerr << "SysEx size reject: len=" << gotLength
              << " (Matrix dump; keeping expect window)\n"
              << std::flush;
    if (dumpRequestRetryRemaining_ == 0 || lastDumpRequest_.empty())
    {
        // Exhausted: drop the bad frame and end the quiet window so OUT unblocks.
        clearExpectInBurst();
        return false;
    }
    --dumpRequestRetryRemaining_;

    uint8_t encodeBytes[64] = {};
    HostEncodeScratch scratch{encodeBytes, sizeof(encodeBytes), 0, 0};
    if (!encodeHostMidiLocked(
            lastDumpOutPort_, lastDumpRequest_.data(), lastDumpRequest_.size(), scratch))
    {
        clearExpectInBurst();
        return false;
    }
    std::string error;
    if (!transport_.WriteEmagicHostMidi(scratch.bytes, scratch.size, error))
    {
        std::cerr << "SysEx size reject: dump-request retry WriteBulk failed: " << error
                  << "\n"
                  << std::flush;
        clearExpectInBurst();
        return false;
    }
    expectInBurstUntil_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(3500);
    deviceHostCounters_.AddHostOutOk();
    std::cerr << "SysEx size reject: re-sent dump request (" << lastDumpRequest_.size()
              << " B)\n"
              << std::flush;
    return true;
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
    // sole unbounded work path under usbIoMutex_ (AD-18 / NFR-R3).
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

    // try_lock drain when the reader is between Wait timeouts (IN URBs stay pending).
    drainHostOutbound();
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
    drainHostOutboundLocked();
}

void DeviceSession::failHostOutboundDrain(const std::string& reason)
{
    const std::size_t discarded = clearHostOutboundQueue();
    if (discarded == 0)
    {
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
    const bool ringActive = transport_.IsBulkInAsyncRingActive();
    const bool firstShot = !firstHostInquiryLogged_.exchange(true);
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
    const bool includedF5 = encodedBytes >= item.midi.size() + 3;
    std::cerr << "host→device: Device Inquiry WriteBulk ok (out_port="
              << (item.outPortIndex + 1) << " midi_bytes=" << item.midi.size()
              << " encoded_bytes=" << encodedBytes
              << " f5_switch=" << (includedF5 ? "yes" : "no")
              << " ring_active=" << (ringActive ? "yes" : "no")
              << " ms_since_ring_arm=" << msSinceRing
              << " pending_urbs=" << transport_.CountPendingBulkInSlots() << "/"
              << kBulkInAsyncSlotCount
              << " first_after_start=" << (firstShot ? "yes" : "no") << ")\n"
              << std::flush;
}

bool DeviceSession::writeHostOutboundItem(
    const HostOutboundItem& item,
    HostEncodeScratch& scratch)
{
    std::string error;
    if (transport_.WriteEmagicHostMidi(scratch.bytes, scratch.size, error))
    {
        noteHostOutboundCounters(item, scratch.size);
        if (!item.midi.empty() && item.midi[0] == 0xF0)
        {
            armExpectInBurstAfterHostSysex(
                item.outPortIndex, item.midi.data(), item.midi.size());
        }
        return true;
    }
    const std::size_t discarded = clearHostOutboundQueue();
    appendDiscardedSuffix(error, discarded);
    recordPumpFailure(formatPortCableFailure(
        "Host→device WriteBulk", item.outPortIndex, scratch.cableIndex, error));
    return false;
}

bool DeviceSession::encodeWritePopOneHostOutbound(HostEncodeScratch& scratch)
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
    if (!encodeHostMidiLocked(item.outPortIndex, item.midi.data(), item.midi.size(), scratch))
    {
        failHostOutboundDrain("Host→device encode path aborted");
        return false;
    }
    if (stopPump_.load() || !running_.load())
    {
        failHostOutboundDrain("Host→device pump stopped during drain");
        return false;
    }
    if (!writeHostOutboundItem(item, scratch))
    {
        return false;
    }

    std::lock_guard<std::mutex> queueLock(hostOutboundMutex_);
    HostOutboundItem committed;
    return hostOutbound_.TryPop(committed);
}

void DeviceSession::drainHostOutboundLocked()
{
    // Never WriteBulk while the async IN ring is down (Start race / Stop teardown).
    if (!transport_.IsBulkInAsyncRingActive())
    {
        if (running_.load() && hostOutboundPending())
        {
            failHostOutboundDrain(
                "Host→device WriteBulk skipped: bulk IN async ring inactive");
        }
        return;
    }
    // Post-start calm, mid–SysEx hold, or post-dump-request expect-IN window.
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
        if (hostOutboundWriteBlocked())
        {
            return;
        }
        if (!encodeWritePopOneHostOutbound(scratch))
        {
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
