// Bulk IN completion thread — Linux snd_usbmidi_in_urb_complete model on WinUSB.

#include "Usb/WinUsbTransport.h"

#ifdef _WIN32

#include "Usb/WinUsbBulkInAsyncRing.h"

#include <cstring>
#include <system_error>

namespace
{
constexpr DWORD kCompletionWaitSliceMs = 50;

bool fillCompletionWaitHandles(
    BulkInAsyncRingState& ring,
    HANDLE stop,
    HANDLE* waitHandlesOut,
    std::string& errorOut)
{
    waitHandlesOut[0] = stop;
    for (std::size_t i = 0; i < kBulkInAsyncSlotCount; ++i)
    {
        waitHandlesOut[i + 1] = ring.slots[i].event;
        if (waitHandlesOut[i + 1] == nullptr)
        {
            errorOut = "WinUSB bulk IN async event is missing";
            return false;
        }
    }
    return true;
}

int harvestSignaledOnly(
    WINUSB_INTERFACE_HANDLE winUsb,
    WinUsbTransport* transport,
    BulkInAsyncRingState& ring,
    std::string& errorOut)
{
    return bulkInHarvestSignaled(winUsb, transport, ring, errorOut);
}
} // namespace

void WinUsbTransport::signalBulkInDataReady() noexcept
{
    auto* event = static_cast<HANDLE>(bulkInDataReadyEvent_);
    if (event != nullptr)
    {
        (void)SetEvent(event);
    }
}

std::size_t WinUsbTransport::CountArmedBulkInSlots() noexcept
{
    std::lock_guard lock(bulkInRingMutex_);
    const auto* ring = bulkInAsRing(bulkInAsyncRing_);
    if (ring == nullptr || !ring->active)
    {
        return 0;
    }
    std::size_t armed = 0;
    for (std::size_t index = 0; index < kBulkInAsyncSlotCount; ++index)
    {
        const BulkInAsyncSlotState& slot = ring->slots[index];
        if (!slot.pending || slot.event == nullptr)
        {
            continue;
        }
        // Signaled ⇒ completed at HC; not an armed inbound buffer anymore.
        // WAIT_FAILED must not count as armed (would hide starvation).
        const DWORD wait = WaitForSingleObject(slot.event, 0);
        if (wait == WAIT_TIMEOUT)
        {
            ++armed;
        }
    }
    return armed;
}

void WinUsbTransport::failBulkInCompletion(std::string error) noexcept
{
    bulkInCompletionFailed_.store(true);
    {
        std::lock_guard lock(bulkInRingMutex_);
        bulkInCompletionError_ = std::move(error);
    }
    signalBulkInDataReady();
}

bool WinUsbTransport::armBulkInCompletionWait(void* waitHandlesOut) noexcept
{
    HANDLE stop = static_cast<HANDLE>(bulkInStopEvent_);
    auto* waitHandles = static_cast<HANDLE*>(waitHandlesOut);
    std::lock_guard lock(bulkInRingMutex_);
    auto* ring = bulkInAsRing(bulkInAsyncRing_);
    if (ring == nullptr || !ring->active || stop == nullptr)
    {
        return false;
    }
    std::string error;
    if (!fillCompletionWaitHandles(*ring, stop, waitHandles, error))
    {
        failBulkInCompletion(std::move(error));
        return false;
    }
    return true;
}

bool WinUsbTransport::popOrderedPacketCopy(
    uint8_t* copy,
    std::size_t copyCapacity,
    std::size_t& sizeOut,
    std::string& errorOut) noexcept
{
    BulkInAsyncPacket packet;
    const int popped = tryPopBulkInPacket(packet, errorOut);
    if (popped <= 0)
    {
        sizeOut = 0;
        return popped == 0;
    }
    if (packet.size > copyCapacity)
    {
        errorOut = "WinUSB bulk IN delivered packet exceeds copy buffer";
        return false;
    }
    sizeOut = packet.size;
    if (sizeOut > 0 && packet.data != nullptr)
    {
        std::memcpy(copy, packet.data, sizeOut);
    }
    return true;
}

bool WinUsbTransport::harvestSignaledUnderLock(std::string& errorOut) noexcept
{
    // Caller holds bulkInRingMutex_. Resubmit before pop so deliver storms cannot
    // leave the HC with 0 armed IN URBs (lab −32 B SysEx / deliver_hw spikes).
    auto* ring = bulkInAsRing(bulkInAsyncRing_);
    if (ring == nullptr || !ring->active)
    {
        return true;
    }
    return harvestSignaledOnly(
               static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_), this, *ring, errorOut)
        >= 0;
}

bool WinUsbTransport::popOneOrderedBulkInForHandler(
    uint8_t* copy,
    std::size_t copyCapacity,
    std::size_t& sizeOut,
    std::string& errorOut) noexcept
{
    std::lock_guard lock(bulkInRingMutex_);
    if (!harvestSignaledUnderLock(errorOut))
    {
        return false;
    }
    return popOrderedPacketCopy(copy, copyCapacity, sizeOut, errorOut);
}

bool WinUsbTransport::deliverOrderedBulkInPackets() noexcept
{
    BulkInPacketHandler handler = nullptr;
    void* handlerCtx = nullptr;
    {
        std::lock_guard lock(bulkInRingMutex_);
        handler = bulkInPacketHandler_;
        handlerCtx = bulkInPacketHandlerCtx_;
        if (handler == nullptr)
        {
            auto* ring = bulkInAsRing(bulkInAsyncRing_);
            if (ring != nullptr
                && bulkInFindReorderBySeq(*ring, ring->nextCompleteSeq) != nullptr)
            {
                signalBulkInDataReady();
            }
            return true;
        }
    }

    for (;;)
    {
        uint8_t copy[512] = {};
        std::size_t size = 0;
        std::string popError;
        if (!popOneOrderedBulkInForHandler(copy, sizeof(copy), size, popError))
        {
            failBulkInCompletion(std::move(popError));
            return false;
        }
        if (size == 0)
        {
            return true;
        }
        if (!handler(handlerCtx, copy, size))
        {
            failBulkInCompletion("WinUSB bulk IN packet handler failed");
            return false;
        }
        // Handler (enqueue) can outlast a full ring of completions; re-arm before
        // the next pop so gap_min_armed does not sit at 0 across the handler gap.
        {
            std::lock_guard lock(bulkInRingMutex_);
            std::string harvestError;
            if (!harvestSignaledUnderLock(harvestError))
            {
                failBulkInCompletion(std::move(harvestError));
                return false;
            }
        }
        signalBulkInDataReady();
    }
}

bool WinUsbTransport::harvestBulkInCompletionOnce() noexcept
{
    // Harvest → deliver (which re-harvests) → one trailing harvest so slots that
    // completed during the last handler call are armed again before Wait.
    for (int pass = 0; pass < 2; ++pass)
    {
        {
            std::lock_guard lock(bulkInRingMutex_);
            auto* ring = bulkInAsRing(bulkInAsyncRing_);
            if (ring == nullptr || !ring->active)
            {
                return false;
            }
            std::string error;
            if (harvestSignaledOnly(
                    static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_), this, *ring, error)
                < 0)
            {
                failBulkInCompletion(std::move(error));
                return false;
            }
        }
        if (pass == 0 && !deliverOrderedBulkInPackets())
        {
            return false;
        }
    }
    return true;
}

void WinUsbTransport::bulkInCompletionLoop()
{
    HANDLE waitHandles[kBulkInAsyncSlotCount + 1] = {};
    for (;;)
    {
        if (!armBulkInCompletionWait(waitHandles))
        {
            return;
        }
        const DWORD wait = WaitForMultipleObjects(
            static_cast<DWORD>(kBulkInAsyncSlotCount + 1),
            waitHandles,
            FALSE,
            kCompletionWaitSliceMs);
        if (wait == WAIT_OBJECT_0)
        {
            return;
        }
        if (wait == WAIT_FAILED)
        {
            failBulkInCompletion(formatWin32Error(
                "WaitForMultipleObjects failed for Emagic bulk IN completion", GetLastError()));
            return;
        }
        if (!harvestBulkInCompletionOnce())
        {
            return;
        }
    }
}

bool WinUsbTransport::startBulkInCompletionThread(std::string& errorOut)
{
    stopBulkInCompletionThread();
    bulkInCompletionFailed_.store(false);
    {
        std::lock_guard lock(bulkInRingMutex_);
        bulkInCompletionError_.clear();
    }

    HANDLE dataReady = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    HANDLE stop = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (dataReady == nullptr || stop == nullptr)
    {
        if (dataReady != nullptr)
        {
            CloseHandle(dataReady);
        }
        if (stop != nullptr)
        {
            CloseHandle(stop);
        }
        errorOut = formatWin32Error(
            "CreateEventW failed for Emagic bulk IN completion thread", GetLastError());
        return false;
    }
    bulkInDataReadyEvent_ = dataReady;
    bulkInStopEvent_ = stop;
    try
    {
        bulkInCompletionThread_ = std::thread([this]() { bulkInCompletionLoop(); });
    }
    catch (const std::system_error& ex)
    {
        CloseHandle(dataReady);
        CloseHandle(stop);
        bulkInDataReadyEvent_ = nullptr;
        bulkInStopEvent_ = nullptr;
        errorOut = std::string("Failed to start Emagic bulk IN completion thread: ") + ex.what();
        return false;
    }
    errorOut.clear();
    return true;
}

void WinUsbTransport::stopBulkInCompletionThread() noexcept
{
    if (bulkInStopEvent_ != nullptr)
    {
        (void)SetEvent(static_cast<HANDLE>(bulkInStopEvent_));
    }
    if (bulkInCompletionThread_.joinable())
    {
        bulkInCompletionThread_.join();
    }
    if (bulkInDataReadyEvent_ != nullptr)
    {
        CloseHandle(static_cast<HANDLE>(bulkInDataReadyEvent_));
        bulkInDataReadyEvent_ = nullptr;
    }
    if (bulkInStopEvent_ != nullptr)
    {
        CloseHandle(static_cast<HANDLE>(bulkInStopEvent_));
        bulkInStopEvent_ = nullptr;
    }
}

#endif // _WIN32
