// Always-pending WinUSB bulk IN ring (Linux snd-usb-midi INPUT_URBS model).

#include "Usb/WinUsbTransport.h"

#ifdef _WIN32

#include "Usb/WinUsbOpenDetail.h"
#include "Usb/WinUsbOpenSupport.h"

#include <cstring>

namespace
{
struct BulkInAsyncSlotState
{
    OVERLAPPED overlapped = {};
    HANDLE event = nullptr;
    uint8_t buffer[512] = {};
    bool pending = false;
};

struct BulkInAsyncRingState
{
    BulkInAsyncSlotState slots[kBulkInAsyncSlotCount] = {};
    std::size_t capacity = 0;
    bool active = false;
};

BulkInAsyncRingState* asRing(void* pointer) noexcept
{
    return static_cast<BulkInAsyncRingState*>(pointer);
}

bool collectBulkInAsyncEvents(
    BulkInAsyncRingState& ring,
    HANDLE* eventsOut,
    std::string& errorOut)
{
    for (std::size_t index = 0; index < kBulkInAsyncSlotCount; ++index)
    {
        eventsOut[index] = ring.slots[index].event;
        if (eventsOut[index] == nullptr)
        {
            errorOut = "WinUSB bulk IN async event is missing";
            return false;
        }
    }
    return true;
}

int completeBulkInAsyncSlot(
    WINUSB_INTERFACE_HANDLE winUsb,
    BulkInAsyncSlotState& state,
    BulkInAsyncPacket& packetOut,
    std::string& errorOut)
{
    DWORD transferred = 0;
    if (!WinUsb_GetOverlappedResult(winUsb, &state.overlapped, &transferred, FALSE))
    {
        const DWORD code = GetLastError();
        state.pending = false;
        if (code == ERROR_OPERATION_ABORTED)
        {
            errorOut = "WinUSB bulk IN async aborted";
            return -1;
        }
        errorOut = formatWin32Error("WinUsb_GetOverlappedResult failed for Emagic bulk IN", code);
        return -1;
    }

    state.pending = false;
    packetOut.data = state.buffer;
    packetOut.size = static_cast<std::size_t>(transferred);
    errorOut.clear();
    return 1;
}
} // namespace

bool WinUsbTransport::armInfiniteBulkInTimeout(std::string& errorOut)
{
    // 0 = wait until data or AbortPipe (matches always-pending Linux URBs).
    return setPipeTransferTimeoutMs(
        bulkInPipeId_,
        0,
        "WinUsb_SetPipePolicy(PIPE_TRANSFER_TIMEOUT=0) failed for Emagic bulk IN",
        errorOut);
}

void WinUsbTransport::releaseBulkInAsyncSlots() noexcept
{
    auto* ring = asRing(bulkInAsyncRing_);
    if (ring == nullptr)
    {
        return;
    }
    for (std::size_t index = 0; index < kBulkInAsyncSlotCount; ++index)
    {
        BulkInAsyncSlotState& slot = ring->slots[index];
        if (slot.event != nullptr)
        {
            CloseHandle(slot.event);
            slot.event = nullptr;
        }
        slot.pending = false;
        std::memset(&slot.overlapped, 0, sizeof(slot.overlapped));
    }
    delete ring;
    bulkInAsyncRing_ = nullptr;
}

bool WinUsbTransport::submitBulkInAsyncSlot(std::size_t slot, std::string& errorOut)
{
    auto* ring = asRing(bulkInAsyncRing_);
    if (ring == nullptr || !ring->active || slot >= kBulkInAsyncSlotCount)
    {
        errorOut = "WinUSB bulk IN async slot is not armed";
        return false;
    }
    if (ring->capacity == 0 || ring->capacity > sizeof(ring->slots[0].buffer))
    {
        errorOut = "WinUSB bulk IN async capacity is invalid";
        return false;
    }

    BulkInAsyncSlotState& state = ring->slots[slot];
    if (state.event == nullptr)
    {
        errorOut = "WinUSB bulk IN async event is missing";
        return false;
    }

    std::memset(&state.overlapped, 0, sizeof(state.overlapped));
    state.overlapped.hEvent = state.event;
    if (!ResetEvent(state.event))
    {
        errorOut = formatWin32Error("ResetEvent failed for Emagic bulk IN async", GetLastError());
        return false;
    }

    ULONG transferred = 0;
    const BOOL ok = WinUsb_ReadPipe(
        static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_),
        bulkInPipeId_,
        state.buffer,
        static_cast<ULONG>(ring->capacity),
        &transferred,
        &state.overlapped);
    if (!ok)
    {
        const DWORD code = GetLastError();
        if (code != ERROR_IO_PENDING)
        {
            state.pending = false;
            errorOut = formatWin32Error("WinUsb_ReadPipe async failed for Emagic bulk IN", code);
            return false;
        }
    }
    state.pending = true;
    errorOut.clear();
    return true;
}

bool WinUsbTransport::StartBulkInAsyncRing(std::string& errorOut)
{
    StopBulkInAsyncRing();

    if (!IsOpen())
    {
        errorOut = "WinUSB StartBulkInAsyncRing requires an open transport with bulk pipes";
        return false;
    }

    const std::size_t capacity = BulkInReadCapacity();
    if (capacity == 0 || capacity > 512)
    {
        errorOut = "WinUSB bulk IN read capacity is invalid for async ring";
        return false;
    }
    if (!armInfiniteBulkInTimeout(errorOut))
    {
        return false;
    }

    auto* ring = new BulkInAsyncRingState();
    ring->capacity = capacity;
    ring->active = true;
    bulkInAsyncRing_ = ring;

    for (std::size_t index = 0; index < kBulkInAsyncSlotCount; ++index)
    {
        HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (event == nullptr)
        {
            errorOut = formatWin32Error("CreateEventW failed for Emagic bulk IN async", GetLastError());
            StopBulkInAsyncRing();
            return false;
        }
        ring->slots[index].event = event;
        if (!submitBulkInAsyncSlot(index, errorOut))
        {
            StopBulkInAsyncRing();
            return false;
        }
    }

    errorOut.clear();
    return true;
}

void WinUsbTransport::AbortBulkInAsyncRing() noexcept
{
    auto* ring = asRing(bulkInAsyncRing_);
    if (ring == nullptr || !ring->active || !IsOpen())
    {
        return;
    }
    (void)WinUsb_AbortPipe(
        static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_), bulkInPipeId_);
}

void WinUsbTransport::StopBulkInAsyncRing() noexcept
{
    auto* ring = asRing(bulkInAsyncRing_);
    if (ring == nullptr)
    {
        return;
    }

    ring->active = false;
    if (IsOpen())
    {
        (void)WinUsb_AbortPipe(
            static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_), bulkInPipeId_);
    }

    for (std::size_t index = 0; index < kBulkInAsyncSlotCount; ++index)
    {
        BulkInAsyncSlotState& slot = ring->slots[index];
        if (!slot.pending || slot.event == nullptr)
        {
            continue;
        }
        // Retry Abort+wait so we do not free OVERLAPPED while I/O is outstanding.
        for (int attempt = 0; attempt < 5; ++attempt)
        {
            const DWORD wait = WaitForSingleObject(slot.event, 1000);
            if (wait == WAIT_OBJECT_0)
            {
                break;
            }
            if (IsOpen())
            {
                (void)WinUsb_AbortPipe(
                    static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_), bulkInPipeId_);
            }
        }
        DWORD transferred = 0;
        (void)WinUsb_GetOverlappedResult(
            static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_),
            &slot.overlapped,
            &transferred,
            FALSE);
        slot.pending = false;
    }

    releaseBulkInAsyncSlots();

    if (IsOpen())
    {
        std::string ignored;
        (void)RestoreSessionBulkTimeouts(ignored);
    }
}

bool WinUsbTransport::IsBulkInAsyncRingActive() const noexcept
{
    const auto* ring = asRing(bulkInAsyncRing_);
    return ring != nullptr && ring->active;
}

int WinUsbTransport::WaitBulkInAsyncPacket(
    std::uint32_t timeoutMs,
    BulkInAsyncPacket& packetOut,
    std::string& errorOut)
{
    packetOut = {};
    auto* ring = asRing(bulkInAsyncRing_);
    if (ring == nullptr || !ring->active)
    {
        errorOut = "WinUSB bulk IN async ring is not active";
        return -1;
    }

    HANDLE events[kBulkInAsyncSlotCount] = {};
    if (!collectBulkInAsyncEvents(*ring, events, errorOut))
    {
        return -1;
    }

    const DWORD wait = WaitForMultipleObjects(
        static_cast<DWORD>(kBulkInAsyncSlotCount),
        events,
        FALSE,
        timeoutMs);
    if (wait == WAIT_TIMEOUT)
    {
        errorOut.clear();
        return 0;
    }
    if (wait < WAIT_OBJECT_0 || wait >= WAIT_OBJECT_0 + kBulkInAsyncSlotCount)
    {
        errorOut = formatWin32Error(
            "WaitForMultipleObjects failed for Emagic bulk IN async", GetLastError());
        return -1;
    }

    const std::size_t slot = static_cast<std::size_t>(wait - WAIT_OBJECT_0);
    packetOut.slot = slot;
    return completeBulkInAsyncSlot(
        static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_),
        ring->slots[slot],
        packetOut,
        errorOut);
}

bool WinUsbTransport::ResubmitBulkInAsyncSlot(std::size_t slot, std::string& errorOut)
{
    return submitBulkInAsyncSlot(slot, errorOut);
}

#else // !_WIN32

bool WinUsbTransport::StartBulkInAsyncRing(std::string& errorOut)
{
    errorOut = "WinUSB StartBulkInAsyncRing requires Windows";
    return false;
}

void WinUsbTransport::StopBulkInAsyncRing() noexcept {}

bool WinUsbTransport::IsBulkInAsyncRingActive() const noexcept
{
    return false;
}

void WinUsbTransport::AbortBulkInAsyncRing() noexcept {}

int WinUsbTransport::WaitBulkInAsyncPacket(
    std::uint32_t /*timeoutMs*/,
    BulkInAsyncPacket& packetOut,
    std::string& errorOut)
{
    packetOut = {};
    errorOut = "WinUSB WaitBulkInAsyncPacket requires Windows";
    return -1;
}

bool WinUsbTransport::ResubmitBulkInAsyncSlot(std::size_t /*slot*/, std::string& errorOut)
{
    errorOut = "WinUSB ResubmitBulkInAsyncSlot requires Windows";
    return false;
}

#endif // _WIN32
