// WinUSB transport open/close + bulk I/O — GUID-first device interface enumeration (AD-12).
// Usb may include Profile; Profile must never include WinUSB headers.

#pragma once

#include "Profile/DeviceProfile.h"

#include <cstddef>
#include <cstdint>
#include <string>

// Single source of truth for the project WinUSB DeviceInterfaceGUID.
// Must match installer/mt4-winusb.inf DeviceInterfaceGUIDs value exactly.
// Open parses this string into a GUID — do not duplicate a binary GUID elsewhere.
inline constexpr const char* kMt4WinUsbDeviceInterfaceGuid =
    "{aa209017-cf8a-49ad-a0e7-701187ff7e05}";

struct WinUsbOpenOptions
{
    // Contributor escape hatch for Zadig-bound machines without the project GUID.
    // Default builds remain GUID-first and fail closed when the GUID is missing.
    bool allowZadigFallback = false;
};

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
    bool ReadBulk(
        uint8_t* buffer,
        std::size_t capacity,
        std::size_t& bytesRead,
        std::string& errorOut);

    // True when the last ReadBulk failed because PIPE_TRANSFER_TIMEOUT elapsed.
    bool LastReadTimedOut() const noexcept;

private:
#ifdef _WIN32
    bool discoverBulkPipes(std::string& errorOut);
    bool applyBulkInTransferTimeout(std::string& errorOut);
    void clearPipeState() noexcept;

    void* deviceHandle_ = nullptr;   // HANDLE
    void* winUsbHandle_ = nullptr;   // WINUSB_INTERFACE_HANDLE
    unsigned char bulkOutPipeId_ = 0;
    unsigned char bulkInPipeId_ = 0;
    bool pipesReady_ = false;
    bool lastReadTimedOut_ = false;
#else
    bool open_ = false;
    bool lastReadTimedOut_ = false;
#endif
};
