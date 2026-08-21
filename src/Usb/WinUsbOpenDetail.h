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
#include <vector>

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
    // When non-null and non-empty, open this device interface path (UTF-16).
    const wchar_t* selectedDevicePath = nullptr;
    WinUsbHandles* handles = nullptr;
};

// preferZadig: HWID/Zadig first (lab --dev-zadig); else GUID-only fail-closed.
// Selected path skips unique-match enumeration and opens that instance only.
bool openWinUsbHandles(WinUsbOpenRequest& request, std::string& errorOut);

enum class Mt4UnitIdentityKind : unsigned char
{
    Serial = 1,
    Topology = 2
};

// One present WinUSB interface matching project GUID + profile ifnum (no open claim).
struct Mt4PresentWinUsbInterface
{
    std::wstring devicePath;
    std::string identityKey;
    Mt4UnitIdentityKind identityKind = Mt4UnitIdentityKind::Topology;
    // Always filled when known so serial→topology migration can keep the same K.
    std::string topologyKey;
};

// Enumerate all present MT4 WinUSB interfaces for the project GUID (paths + identity).
bool enumeratePresentMt4WinUsbInterfaces(
    const GUID& interfaceGuid,
    const DeviceProfile& profile,
    std::vector<Mt4PresentWinUsbInterface>& interfacesOut,
    std::string& errorOut);

// Lab --dev-zadig only: when the project GUID has no interfaces, detect one unique
// HWID-matched USB node (composite MI_xx preferred, else parent). Fills identity;
// leaves devicePath empty so Open uses Zadig fallback (single-unit). Returns false
// on SetupAPI failure or ambiguous multi-match; true with empty identityKey = Absent.
bool tryEnumerateZadigLabPresentMt4Interface(
    const DeviceProfile& profile,
    Mt4PresentWinUsbInterface& interfaceOut,
    std::string& errorOut);

#endif // _WIN32
