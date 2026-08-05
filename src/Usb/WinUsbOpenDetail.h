// Internal WinUSB open helpers (Windows only). Not part of the public Usb API.

#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <setupapi.h>
#include <winusb.h>
#include <objbase.h>

#include "Profile/DeviceProfile.h"

#include <cstddef>
#include <string>

struct WinUsbHandles
{
    HANDLE device = INVALID_HANDLE_VALUE;
    // Handle from WinUsb_Initialize (must stay open while associated handles are used).
    WINUSB_INTERFACE_HANDLE winUsbRoot = nullptr;
    // Interface matching DeviceProfile::ifnum (root or an associated handle).
    WINUSB_INTERFACE_HANDLE winUsb = nullptr;
    // All associated interfaces kept open (MT4 needs siblings claimed for bulk OUT).
    WINUSB_INTERFACE_HANDLE associated[8] = {};
    std::size_t associatedCount = 0;
};

bool openByDeviceInterfaceGuid(
    const GUID& interfaceGuid,
    const DeviceProfile& profile,
    WinUsbHandles& handles,
    std::string& errorOut);

bool openZadigFallback(
    const DeviceProfile& profile,
    WinUsbHandles& handles,
    std::string& errorOut);

struct WinUsbOpenRequest
{
    const DeviceProfile* profile = nullptr;
    const GUID* projectGuid = nullptr;
    bool preferZadig = false;
    WinUsbHandles* handles = nullptr;
};

// preferZadig: HWID/Zadig first (lab --dev-zadig); else GUID-only fail-closed.
bool openWinUsbHandles(WinUsbOpenRequest& request, std::string& errorOut);

#endif // _WIN32
