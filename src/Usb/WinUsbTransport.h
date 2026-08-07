// WinUSB transport open/close + bulk I/O — GUID-first device interface enumeration (AD-12).
// Usb may include Profile; Profile must never include WinUSB headers.

#pragma once

#include "Profile/DeviceProfile.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

// Single source of truth for the project WinUSB DeviceInterfaceGUID.
// Must match installer/mt4-winusb.inf DeviceInterfaceGUIDs value exactly.
// Open parses this string into a GUID — do not duplicate a binary GUID elsewhere.
inline constexpr const char* kMt4WinUsbDeviceInterfaceGuid =
    "{aa209017-cf8a-49ad-a0e7-701187ff7e05}";

// Linux snd-usb-midi INPUT_URBS — keep this many bulk IN transfers always pending.
inline constexpr std::size_t kBulkInAsyncSlotCount = 7;

struct WinUsbOpenOptions
{
    // Contributor escape hatch for Zadig-bound / Boot Camp lab machines.
    // When true, open HWID/Zadig-first (same path as --probe-usb); GUID is backup.
    // Default builds remain GUID-only and fail closed when the GUID is missing.
    bool allowZadigFallback = false;
};

// One completed bulk IN packet from the async ring (buffer owned by the transport).
struct BulkInAsyncPacket
{
    const uint8_t* data = nullptr;
    std::size_t size = 0;
    std::size_t slot = 0;
};

// Linux urb complete → protocol input: invoked from the bulk IN completion thread
// with a stable packet copy (ring mutex not held). Return false on fatal demux error.
using BulkInPacketHandler =
    bool (*)(void* context, const uint8_t* data, std::size_t size);

class WinUsbTransport
{
public:
    WinUsbTransport() = default;
    ~WinUsbTransport();

    WinUsbTransport(const WinUsbTransport&) = delete;
    WinUsbTransport& operator=(const WinUsbTransport&) = delete;

    // GUID-first open. On failure, writes English diagnostic to errorOut and returns false.
    bool Open(
        const DeviceProfile& profile,
        std::string& errorOut,
        WinUsbOpenOptions options = {});

    void Close() noexcept;
    bool IsOpen() const noexcept;

    // Synchronous bulk OUT/IN on discovered Emagic MIDI pipes (after successful Open).
    bool WriteBulk(const uint8_t* data, std::size_t size, std::string& errorOut);
    // Host MIDI path: Linux snd_usbmidi_emagic_output — each USB packet ≤ wMaxPacket
    // and ends with 0xFF (strips a trailing encode pad, then re-chunks).
    bool WriteEmagicHostMidi(const uint8_t* data, std::size_t size, std::string& errorOut);
    bool ReadBulk(
        uint8_t* buffer,
        std::size_t capacity,
        std::size_t& bytesRead,
        std::string& errorOut);

    // Emagic wake: init (+ drain IN reply) then Set Computer Mode so DIN→USB IN works.
    // drainedBytesOut receives total bulk IN bytes read during init drains (may be 0).
    bool WriteEmagicInitSequence(
        std::string& errorOut,
        std::size_t* drainedBytesOut = nullptr);

    // Host read size for bulk IN (endpoint wMaxPacketSize). Prefer this over a large
    // buffer: Emagic pads full packets with 0xFF, so a 512-byte ReadBulk never sees a
    // short packet and times out with 0 bytes under default WinUSB coalescing.
    std::size_t BulkInReadCapacity() const noexcept;

    // Emagic bulk OUT endpoint wMaxPacketSize (informational; encode uses short pad).
    std::size_t BulkOutMaxPacketSize() const noexcept;

    // True when the last ReadBulk failed because PIPE_TRANSFER_TIMEOUT elapsed.
    bool LastReadTimedOut() const noexcept;

    // Lab half-duplex helper for init drain / legacy sync paths.
    bool BeginShortBulkInDrain(std::string& errorOut);
    bool RestoreSessionBulkTimeouts(std::string& errorOut);

    // Best-effort short IN drain (post Computer Mode kick / residual).
    // Returns false when the short-drain policy could not be armed.
    bool DrainBulkInBestEffort(std::size_t maxPackets, std::size_t& drainedOut, std::string& errorOut);

    // Always-pending multi-buffer bulk IN (Linux INPUT_URBS model). Call after Open
    // and before the session reader waits; Stop aborts in-flight reads.
    bool StartBulkInAsyncRing(std::string& errorOut);
    void StopBulkInAsyncRing() noexcept;
    bool IsBulkInAsyncRingActive() const noexcept;
    // AbortPipe to wake WaitBulkInReaderTick / completion waits during session Stop.
    void AbortBulkInAsyncRing() noexcept;
    // Optional: demux on the completion thread (set before StartBulkInAsyncRing).
    void SetBulkInPacketHandler(BulkInPacketHandler handler, void* context) noexcept;
    void ClearBulkInPacketHandler() noexcept;
    // Reader tick: 1 = woke (delivery or timeout), 0 unused, -1 = fatal / aborted.
    int WaitBulkInReaderTick(std::uint32_t timeoutMs, std::string& errorOut);
    // Complete signaled slots into the reorder buffer and resubmit immediately
    // (Linux snd_usbmidi_in_urb_complete). Does not pop/deliver MIDI.
    int PumpBulkInAsyncCompletions(std::string& errorOut);
    bool ResubmitBulkInAsyncSlot(std::size_t slot, std::string& errorOut);
    void signalBulkInDataReady() noexcept;

private:
#ifdef _WIN32
    bool discoverBulkPipes(std::string& errorOut);
    bool applyBulkTransferTimeouts(std::string& errorOut);
    bool setBulkTransferTimeoutMs(std::uint32_t timeoutMs, std::string& errorOut);
    bool setPipeTransferTimeoutMs(
        unsigned char pipeId,
        std::uint32_t timeoutMs,
        const char* failureContext,
        std::string& errorOut);
    bool prepareBulkPipes(std::string& errorOut);
    bool writeInitMagicWithFinishRetry(std::string& errorOut);
    bool armShortInDrainTimeout(std::string& errorOut);
    std::size_t drainBulkInUntilIdle(std::size_t maxPackets);
    void clearPipeState() noexcept;
    bool submitBulkInAsyncSlot(std::size_t slot, std::string& errorOut);
    void releaseBulkInAsyncSlots() noexcept;
    bool armInfiniteBulkInTimeout(std::string& errorOut);
    bool enableBulkInRawIo(std::string& errorOut);
    void bulkInCompletionLoop();
    bool startBulkInCompletionThread(std::string& errorOut);
    void stopBulkInCompletionThread() noexcept;
    void failBulkInCompletion(std::string error) noexcept;
    bool armBulkInCompletionWait(void* waitHandlesOut) noexcept;
    bool harvestBulkInCompletionOnce() noexcept;
    bool deliverOrderedBulkInPackets() noexcept;
    int tryPopBulkInPacket(BulkInAsyncPacket& packetOut, std::string& errorOut);

    void* deviceHandle_ = nullptr;   // HANDLE
    void* winUsbHandle_ = nullptr;   // WINUSB_INTERFACE_HANDLE (ifnum match)
    void* winUsbRootHandle_ = nullptr; // WINUSB_INTERFACE_HANDLE from Initialize
    void* winUsbAssociated_[8] = {};
    std::size_t winUsbAssociatedCount_ = 0;
    unsigned char bulkOutPipeId_ = 0;
    unsigned char bulkInPipeId_ = 0;
    std::uint16_t bulkInMaxPacketSize_ = 0;
    std::uint16_t bulkOutMaxPacketSize_ = 0;
    bool pipesReady_ = false;
    bool lastReadTimedOut_ = false;
    void* bulkInAsyncRing_ = nullptr; // BulkInAsyncRingState*
    // Completion thread owns harvest+resubmit (+ optional demux handler).
    std::mutex bulkInRingMutex_;
    std::thread bulkInCompletionThread_;
    void* bulkInDataReadyEvent_ = nullptr; // HANDLE auto-reset
    void* bulkInStopEvent_ = nullptr;      // HANDLE manual-reset
    std::atomic<bool> bulkInCompletionFailed_{false};
    std::string bulkInCompletionError_;
    BulkInPacketHandler bulkInPacketHandler_ = nullptr;
    void* bulkInPacketHandlerCtx_ = nullptr;
#else
    bool open_ = false;
    bool lastReadTimedOut_ = false;
#endif
};
