// Internal ring + reorder helpers for WinUsbBulkInAsync.cpp (Windows only).

#pragma once

#include "Usb/WinUsbTransport.h"

#include "Usb/WinUsbOpenDetail.h"
#include "Usb/WinUsbOpenSupport.h"

#include <cstdint>
#include <cstring>
#include <string>

#ifdef _WIN32

struct BulkInAsyncSlotState
{
    OVERLAPPED overlapped = {};
    HANDLE event = nullptr;
    uint8_t buffer[512] = {};
    bool pending = false;
    std::uint64_t submitSeq = 0;
};

struct BulkInReorderEntry
{
    bool occupied = false;
    std::uint64_t seq = 0;
    std::size_t size = 0;
    std::size_t sourceSlot = 0;
    uint8_t data[512] = {};
};

// Enough to hold a full Matrix master dump in flight (~351 packets) plus headroom
// so harvest can always resubmit (Linux completes→resubmits in the URB callback;
// a full reorder would stall in-flight URBs and drop mid-SysEx packets).
inline constexpr std::size_t kBulkInReorderCount = 512;

struct BulkInAsyncRingState
{
    BulkInAsyncSlotState slots[kBulkInAsyncSlotCount] = {};
    BulkInReorderEntry reorder[kBulkInReorderCount] = {};
    std::size_t capacity = 0;
    bool active = false;
    std::uint64_t nextSubmitSeq = 0;
    std::uint64_t nextCompleteSeq = 0;
    ULONGLONG gapWaitStartMs = 0;
    // Delivered packet lives here so reorder entries free immediately on pop.
    uint8_t deliverScratch[512] = {};
    std::size_t deliverSize = 0;
    std::size_t deliverSlot = 0;
};

inline BulkInAsyncRingState* bulkInAsRing(void* pointer) noexcept
{
    return static_cast<BulkInAsyncRingState*>(pointer);
}

inline BulkInReorderEntry* bulkInFindReorderBySeq(
    BulkInAsyncRingState& ring,
    std::uint64_t seq) noexcept
{
    for (std::size_t index = 0; index < kBulkInReorderCount; ++index)
    {
        BulkInReorderEntry& entry = ring.reorder[index];
        if (entry.occupied && entry.seq == seq)
        {
            return &entry;
        }
    }
    return nullptr;
}

inline BulkInReorderEntry* bulkInFindFreeReorder(BulkInAsyncRingState& ring) noexcept
{
    for (std::size_t index = 0; index < kBulkInReorderCount; ++index)
    {
        if (!ring.reorder[index].occupied)
        {
            return &ring.reorder[index];
        }
    }
    return nullptr;
}

inline bool bulkInHasBufferedSeqAbove(
    const BulkInAsyncRingState& ring,
    std::uint64_t seq) noexcept
{
    for (std::size_t index = 0; index < kBulkInReorderCount; ++index)
    {
        const BulkInReorderEntry& entry = ring.reorder[index];
        if (entry.occupied && entry.seq > seq)
        {
            return true;
        }
    }
    return false;
}

inline int bulkInCompleteSlot(
    WINUSB_INTERFACE_HANDLE winUsb,
    BulkInAsyncSlotState& state,
    std::size_t& sizeOut,
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
    sizeOut = static_cast<std::size_t>(transferred);
    errorOut.clear();
    return 1;
}

inline bool bulkInStoreReorder(
    BulkInAsyncRingState& ring,
    std::size_t slotIndex,
    std::size_t size,
    std::string& errorOut)
{
    BulkInReorderEntry* entry = bulkInFindFreeReorder(ring);
    if (entry == nullptr)
    {
        errorOut = "WinUSB bulk IN reorder buffer is full";
        return false;
    }
    BulkInAsyncSlotState& state = ring.slots[slotIndex];
    if (size > sizeof(entry->data))
    {
        errorOut = "WinUSB bulk IN completed size exceeds reorder capacity";
        return false;
    }
    entry->occupied = true;
    entry->seq = state.submitSeq;
    entry->size = size;
    entry->sourceSlot = slotIndex;
    if (size > 0)
    {
        std::memcpy(entry->data, state.buffer, size);
    }
    return true;
}

inline int bulkInHarvestSignaled(
    WINUSB_INTERFACE_HANDLE winUsb,
    WinUsbTransport* transport,
    BulkInAsyncRingState& ring,
    std::string& errorOut)
{
    for (std::size_t index = 0; index < kBulkInAsyncSlotCount; ++index)
    {
        BulkInAsyncSlotState& state = ring.slots[index];
        if (!state.pending || state.event == nullptr)
        {
            continue;
        }
        if (WaitForSingleObject(state.event, 0) != WAIT_OBJECT_0)
        {
            continue;
        }
        while (bulkInFindFreeReorder(ring) == nullptr)
        {
            if (bulkInFindReorderBySeq(ring, ring.nextCompleteSeq) != nullptr)
            {
                return 1;
            }
            errorOut = "WinUSB bulk IN reorder buffer is full";
            return -1;
        }
        std::size_t size = 0;
        if (bulkInCompleteSlot(winUsb, state, size, errorOut) < 0)
        {
            if (!ring.active && errorOut == "WinUSB bulk IN async aborted")
            {
                errorOut.clear();
                return 1;
            }
            return -1;
        }
        if (!bulkInStoreReorder(ring, index, size, errorOut))
        {
            return -1;
        }
        if (!transport->ResubmitBulkInAsyncSlot(index, errorOut))
        {
            return -1;
        }
    }
    return 1;
}

inline bool bulkInPopNextOrdered(
    BulkInAsyncRingState& ring,
    BulkInAsyncPacket& packetOut) noexcept
{
    BulkInReorderEntry* entry = bulkInFindReorderBySeq(ring, ring.nextCompleteSeq);
    if (entry == nullptr)
    {
        return false;
    }
    if (entry->size > sizeof(ring.deliverScratch))
    {
        return false;
    }
    if (entry->size > 0)
    {
        std::memcpy(ring.deliverScratch, entry->data, entry->size);
    }
    ring.deliverSize = entry->size;
    ring.deliverSlot = entry->sourceSlot;
    entry->occupied = false;
    packetOut.data = ring.deliverScratch;
    packetOut.size = ring.deliverSize;
    packetOut.slot = ring.deliverSlot;
    ++ring.nextCompleteSeq;
    ring.gapWaitStartMs = 0;
    return true;
}

// Skip a lost URB when later packets are already buffered (unblocks stranded F7).
inline bool bulkInTrySkipLostSeq(BulkInAsyncRingState& ring, DWORD gapTimeoutMs) noexcept
{
    if (!bulkInHasBufferedSeqAbove(ring, ring.nextCompleteSeq))
    {
        ring.gapWaitStartMs = 0;
        return false;
    }
    const ULONGLONG nowMs = GetTickCount64();
    if (gapTimeoutMs == 0)
    {
        ++ring.nextCompleteSeq;
        ring.gapWaitStartMs = 0;
        return true;
    }
    if (ring.gapWaitStartMs == 0)
    {
        ring.gapWaitStartMs = nowMs;
        return false;
    }
    if (nowMs - ring.gapWaitStartMs < static_cast<ULONGLONG>(gapTimeoutMs))
    {
        return false;
    }
    ++ring.nextCompleteSeq;
    ring.gapWaitStartMs = 0;
    return true;
}

inline void bulkInReleaseHeldDeliver(BulkInAsyncRingState& /*ring*/) noexcept {}

inline bool bulkInCollectEvents(
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

// 1 = signaled, 0 = timeout, -1 = fatal.
inline int bulkInWaitAnySlot(HANDLE* events, DWORD sliceMs, std::string& errorOut)
{
    const DWORD wait = WaitForMultipleObjects(
        static_cast<DWORD>(kBulkInAsyncSlotCount), events, FALSE, sliceMs);
    if (wait == WAIT_TIMEOUT)
    {
        return 0;
    }
    if (wait < WAIT_OBJECT_0 || wait >= WAIT_OBJECT_0 + kBulkInAsyncSlotCount)
    {
        errorOut = formatWin32Error(
            "WaitForMultipleObjects failed for Emagic bulk IN async", GetLastError());
        return -1;
    }
    return 1;
}

struct BulkInHarvestCtx
{
    WINUSB_INTERFACE_HANDLE winUsb = nullptr;
    WinUsbTransport* transport = nullptr;
    BulkInAsyncRingState* ring = nullptr;
};

// 1 = packet ready, 0 = nothing yet, -1 = fatal.
inline int bulkInHarvestOrSkip(
    BulkInHarvestCtx& ctx,
    BulkInAsyncPacket& packetOut,
    std::string& errorOut)
{
    if (bulkInHarvestSignaled(ctx.winUsb, ctx.transport, *ctx.ring, errorOut) < 0)
    {
        return -1;
    }
    if (bulkInPopNextOrdered(*ctx.ring, packetOut))
    {
        errorOut.clear();
        return 1;
    }
    // Linux never skips a pending IN URB to "unblock" later ones — doing so
    // mid-SysEx drops bytes (lab: discard frames with wrong length). Wait for
    // the head seq; SysEx idle-finalize covers a true missing trailing F7.
    return 0;
}

#endif // _WIN32
