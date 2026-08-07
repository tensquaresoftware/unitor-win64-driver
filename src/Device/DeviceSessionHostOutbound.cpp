// Host→device outbound queue drain (librarian-scale SysEx bursts).

#include "Device/DeviceSession.h"
#include "Device/DeviceSessionSupport.h"

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

    // VirtualMIDI thread: do not poll bulk IN here — reader may own a pending ReadBulk.
    drainHostOutbound();
}

void DeviceSession::drainHostOutbound()
{
    // try_lock: if processBulkRead holds usbIoMutex_, leave work queued; the
    // reader loop drains after releasing the decode lock.
    std::unique_lock<std::mutex> lock(usbIoMutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return;
    }
    drainHostOutboundLocked(false);
}

void DeviceSession::drainHostOutboundFromReader()
{
    std::lock_guard<std::mutex> lock(usbIoMutex_);
    drainHostOutboundLocked(true);
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

void DeviceSession::noteHostOutboundCounters(const HostOutboundItem& item) noexcept
{
    deviceHostCounters_.AddHostOutOk();
    if (isUniversalDeviceInquiry(item.midi.data(), item.midi.size()))
    {
        deviceHostCounters_.AddInquiryOut();
        std::cerr << "host→device: Device Inquiry WriteBulk ok (out_port="
                  << (item.outPortIndex + 1) << " bytes=" << item.midi.size() << ")\n"
                  << std::flush;
    }
}

bool DeviceSession::writeHostOutboundItem(
    const HostOutboundItem& item,
    HostEncodeScratch& scratch)
{
    std::string error;
    if (transport_.WriteBulk(scratch.bytes, scratch.size, error))
    {
        noteHostOutboundCounters(item);
        return true;
    }
    const std::size_t discarded = clearHostOutboundQueue();
    appendDiscardedSuffix(error, discarded);
    recordPumpFailure(formatPortCableFailure(
        "Host→device WriteBulk", item.outPortIndex, scratch.cableIndex, error));
    return false;
}

bool DeviceSession::encodeWritePopOneHostOutbound(
    HostEncodeScratch& scratch,
    uint8_t* readBuffer,
    std::size_t readCapacity,
    bool drainInAfterWrite)
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
    if (drainInAfterWrite && readCapacity > 0
        && !drainPendingBulkInBurst(readBuffer, readCapacity))
    {
        return false;
    }

    std::lock_guard<std::mutex> queueLock(hostOutboundMutex_);
    HostOutboundItem committed;
    return hostOutbound_.TryPop(committed);
}

void DeviceSession::drainHostOutboundLocked(bool drainInAfterEachWrite)
{
    uint8_t encodeBytes[kEncodeBufferCapacity] = {};
    uint8_t readBuffer[512] = {};
    const std::size_t readCapacity = transport_.BulkInReadCapacity();
    const std::size_t burstCapacity =
        (readCapacity > 0 && readCapacity <= sizeof(readBuffer)) ? readCapacity : 0;
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
        if (!encodeWritePopOneHostOutbound(
                scratch, readBuffer, burstCapacity, drainInAfterEachWrite))
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
