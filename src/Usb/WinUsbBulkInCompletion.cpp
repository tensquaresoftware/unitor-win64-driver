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

void WinUsbTransport::failBulkInCompletion(std::string error) noexcept
{
    bulkInCompletionFailed_.store(true);
    bulkInCompletionError_ = std::move(error);
    signalBulkInDataReady();
}

bool WinUsbTransport::armBulkInCompletionWait(void* waitHandlesOut) noexcept
{
    HANDLE stop = static_cast<HANDLE>(bulkInStopEvent_);
    auto* waitHandles = static_cast<HANDLE*>(waitHandlesOut);
    std::lock_guard<std::mutex> lock(bulkInRingMutex_);
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

bool WinUsbTransport::deliverOrderedBulkInPackets() noexcept
{
    if (bulkInPacketHandler_ == nullptr)
    {
        std::lock_guard<std::mutex> lock(bulkInRingMutex_);
        auto* ring = bulkInAsRing(bulkInAsyncRing_);
        if (ring != nullptr && bulkInFindReorderBySeq(*ring, ring->nextCompleteSeq) != nullptr)
        {
            signalBulkInDataReady();
        }
        return true;
    }

    for (;;)
    {
        uint8_t copy[512] = {};
        std::size_t size = 0;
        {
            std::lock_guard<std::mutex> lock(bulkInRingMutex_);
            std::string error;
            BulkInAsyncPacket packet;
            const int popped = tryPopBulkInPacket(packet, error);
            if (popped < 0)
            {
                failBulkInCompletion(std::move(error));
                return false;
            }
            if (popped == 0)
            {
                return true;
            }
            if (packet.size > sizeof(copy))
            {
                failBulkInCompletion("WinUSB bulk IN delivered packet exceeds copy buffer");
                return false;
            }
            size = packet.size;
            if (size > 0 && packet.data != nullptr)
            {
                std::memcpy(copy, packet.data, size);
            }
        }

        if (!bulkInPacketHandler_(bulkInPacketHandlerCtx_, copy, size))
        {
            failBulkInCompletion("WinUSB bulk IN packet handler failed");
            return false;
        }
        signalBulkInDataReady();
    }
}

bool WinUsbTransport::harvestBulkInCompletionOnce() noexcept
{
    {
        std::lock_guard<std::mutex> lock(bulkInRingMutex_);
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
    return deliverOrderedBulkInPackets();
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
    bulkInCompletionError_.clear();

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
