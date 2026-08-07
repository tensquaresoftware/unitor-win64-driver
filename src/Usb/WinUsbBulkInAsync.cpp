// Always-pending WinUSB bulk IN ring (Linux snd-usb-midi INPUT_URBS model).

#include "Usb/WinUsbTransport.h"

#ifdef _WIN32

#include "Usb/WinUsbBulkInAsyncRing.h"

#include <cstring>

bool WinUsbTransport::armInfiniteBulkInTimeout(std::string& errorOut)
{
    return setPipeTransferTimeoutMs(
        bulkInPipeId_,
        0,
        "WinUsb_SetPipePolicy(PIPE_TRANSFER_TIMEOUT=0) failed for Emagic bulk IN",
        errorOut);
}

bool WinUsbTransport::enableBulkInRawIo(std::string& errorOut)
{
    UCHAR rawIo = TRUE;
    if (!WinUsb_SetPipePolicy(
            static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_),
            bulkInPipeId_,
            RAW_IO,
            sizeof(rawIo),
            &rawIo))
    {
        errorOut = formatWin32Error(
            "WinUsb_SetPipePolicy(RAW_IO) failed for Emagic bulk IN", GetLastError());
        return false;
    }
    errorOut.clear();
    return true;
}

void WinUsbTransport::releaseBulkInAsyncSlots() noexcept
{
    auto* ring = bulkInAsRing(bulkInAsyncRing_);
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
    auto* ring = bulkInAsRing(bulkInAsyncRing_);
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
    state.submitSeq = ring->nextSubmitSeq++;
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
    if (!armInfiniteBulkInTimeout(errorOut) || !enableBulkInRawIo(errorOut))
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
            errorOut =
                formatWin32Error("CreateEventW failed for Emagic bulk IN async", GetLastError());
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

    if (!startBulkInCompletionThread(errorOut))
    {
        StopBulkInAsyncRing();
        return false;
    }
    errorOut.clear();
    return true;
}

void WinUsbTransport::AbortBulkInAsyncRing() noexcept
{
    auto* ring = bulkInAsRing(bulkInAsyncRing_);
    if (ring == nullptr || !ring->active || !IsOpen())
    {
        return;
    }
    (void)WinUsb_AbortPipe(
        static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_), bulkInPipeId_);
}

namespace
{
void drainPendingBulkInSlot(
    WINUSB_INTERFACE_HANDLE winUsb,
    UCHAR pipeId,
    bool deviceOpen,
    BulkInAsyncSlotState& slot)
{
    if (!slot.pending || slot.event == nullptr)
    {
        return;
    }
    for (int attempt = 0; attempt < 5; ++attempt)
    {
        if (WaitForSingleObject(slot.event, 1000) == WAIT_OBJECT_0)
        {
            break;
        }
        if (deviceOpen)
        {
            (void)WinUsb_AbortPipe(winUsb, pipeId);
        }
    }
    DWORD transferred = 0;
    (void)WinUsb_GetOverlappedResult(winUsb, &slot.overlapped, &transferred, FALSE);
    slot.pending = false;
}
} // namespace

void WinUsbTransport::StopBulkInAsyncRing() noexcept
{
    auto* ring = bulkInAsRing(bulkInAsyncRing_);
    if (ring == nullptr && !bulkInCompletionThread_.joinable())
    {
        return;
    }
    if (ring != nullptr)
    {
        ring->active = false;
    }
    if (IsOpen())
    {
        (void)WinUsb_AbortPipe(
            static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_), bulkInPipeId_);
    }
    stopBulkInCompletionThread();

    ring = bulkInAsRing(bulkInAsyncRing_);
    if (ring != nullptr)
    {
        auto* winUsb = static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_);
        for (std::size_t index = 0; index < kBulkInAsyncSlotCount; ++index)
        {
            drainPendingBulkInSlot(winUsb, bulkInPipeId_, IsOpen(), ring->slots[index]);
        }
        releaseBulkInAsyncSlots();
    }
    if (IsOpen())
    {
        std::string ignored;
        (void)RestoreSessionBulkTimeouts(ignored);
    }
}

bool WinUsbTransport::IsBulkInAsyncRingActive() const noexcept
{
    const auto* ring = bulkInAsRing(bulkInAsyncRing_);
    return ring != nullptr && ring->active;
}

std::size_t WinUsbTransport::CountPendingBulkInSlots() noexcept
{
    std::lock_guard<std::mutex> lock(bulkInRingMutex_);
    const auto* ring = bulkInAsRing(bulkInAsyncRing_);
    if (ring == nullptr || !ring->active)
    {
        return 0;
    }
    std::size_t pending = 0;
    for (std::size_t index = 0; index < kBulkInAsyncSlotCount; ++index)
    {
        if (ring->slots[index].pending)
        {
            ++pending;
        }
    }
    return pending;
}

void WinUsbTransport::SetBulkInPacketHandler(
    BulkInPacketHandler handler,
    void* context) noexcept
{
    bulkInPacketHandler_ = handler;
    bulkInPacketHandlerCtx_ = context;
}

void WinUsbTransport::ClearBulkInPacketHandler() noexcept
{
    bulkInPacketHandler_ = nullptr;
    bulkInPacketHandlerCtx_ = nullptr;
}

int WinUsbTransport::tryPopBulkInPacket(BulkInAsyncPacket& packetOut, std::string& errorOut)
{
    if (bulkInCompletionFailed_.load())
    {
        errorOut = bulkInCompletionError_.empty()
            ? "WinUSB bulk IN completion thread failed"
            : bulkInCompletionError_;
        return -1;
    }
    auto* ring = bulkInAsRing(bulkInAsyncRing_);
    if (ring == nullptr || !ring->active)
    {
        errorOut = "WinUSB bulk IN async ring is not active";
        return -1;
    }
    if (!bulkInPopNextOrdered(*ring, packetOut))
    {
        return 0;
    }
    errorOut.clear();
    return 1;
}

int WinUsbTransport::WaitBulkInReaderTick(std::uint32_t timeoutMs, std::string& errorOut)
{
    auto* dataReady = static_cast<HANDLE>(bulkInDataReadyEvent_);
    if (dataReady == nullptr)
    {
        errorOut = "WinUSB bulk IN async ring is not active";
        return -1;
    }
    if (bulkInCompletionFailed_.load())
    {
        errorOut = bulkInCompletionError_.empty()
            ? "WinUSB bulk IN completion thread failed"
            : bulkInCompletionError_;
        return -1;
    }
    const DWORD wait = WaitForSingleObject(dataReady, timeoutMs);
    if (wait == WAIT_FAILED)
    {
        errorOut = formatWin32Error(
            "WaitForSingleObject failed for Emagic bulk IN reader tick", GetLastError());
        return -1;
    }
    if (bulkInCompletionFailed_.load())
    {
        errorOut = bulkInCompletionError_.empty()
            ? "WinUSB bulk IN completion thread failed"
            : bulkInCompletionError_;
        return -1;
    }
    errorOut.clear();
    return 1;
}

bool WinUsbTransport::ResubmitBulkInAsyncSlot(std::size_t slot, std::string& errorOut)
{
    auto* ring = bulkInAsRing(bulkInAsyncRing_);
    if (ring != nullptr && slot < kBulkInAsyncSlotCount && ring->slots[slot].pending)
    {
        errorOut.clear();
        return true;
    }
    return submitBulkInAsyncSlot(slot, errorOut);
}

int WinUsbTransport::PumpBulkInAsyncCompletions(std::string& errorOut)
{
    if (!IsBulkInAsyncRingActive())
    {
        errorOut = "WinUSB bulk IN async ring is not active";
        return -1;
    }
    if (bulkInCompletionFailed_.load())
    {
        errorOut = bulkInCompletionError_.empty()
            ? "WinUSB bulk IN completion thread failed"
            : bulkInCompletionError_;
        return -1;
    }
    errorOut.clear();
    return 1;
}

#else // !_WIN32

bool WinUsbTransport::StartBulkInAsyncRing(std::string& errorOut)
{
    errorOut = "WinUSB StartBulkInAsyncRing requires Windows";
    return false;
}

void WinUsbTransport::StopBulkInAsyncRing() noexcept
{
    ClearBulkInPacketHandler();
}

bool WinUsbTransport::IsBulkInAsyncRingActive() const noexcept
{
    return false;
}

std::size_t WinUsbTransport::CountPendingBulkInSlots() noexcept
{
    return 0;
}

void WinUsbTransport::AbortBulkInAsyncRing() noexcept {}

void WinUsbTransport::SetBulkInPacketHandler(
    BulkInPacketHandler handler,
    void* context) noexcept
{
    (void)handler;
    (void)context;
}

void WinUsbTransport::ClearBulkInPacketHandler() noexcept {}

int WinUsbTransport::WaitBulkInReaderTick(
    std::uint32_t /*timeoutMs*/,
    std::string& errorOut)
{
    errorOut = "WinUSB WaitBulkInReaderTick requires Windows";
    return -1;
}

bool WinUsbTransport::ResubmitBulkInAsyncSlot(std::size_t /*slot*/, std::string& errorOut)
{
    errorOut = "WinUSB ResubmitBulkInAsyncSlot requires Windows";
    return false;
}

int WinUsbTransport::PumpBulkInAsyncCompletions(std::string& errorOut)
{
    errorOut = "WinUSB PumpBulkInAsyncCompletions requires Windows";
    return -1;
}

#endif // _WIN32
