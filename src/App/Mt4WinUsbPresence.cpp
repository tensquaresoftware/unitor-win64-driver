// Enumerate present WinUSB interfaces for the project GUID without opening the device.

#include "App/Mt4WinUsbPresence.h"

#include "Profile/DeviceProfile.h"
#include "Usb/WinUsbTransport.h"

#include <cstdio>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <objbase.h>
#include <setupapi.h>

#include "Usb/WinUsbOpenDetail.h"
#include "Usb/WinUsbOpenSupport.h"
#endif

namespace
{
#ifdef _WIN32
bool parseProjectGuid(GUID& guidOut, std::string& detailOut)
{
    wchar_t wide[64] = {};
    const int converted = MultiByteToWideChar(
        CP_UTF8,
        0,
        kMt4WinUsbDeviceInterfaceGuid,
        -1,
        wide,
        static_cast<int>(sizeof(wide) / sizeof(wide[0])));
    if (converted <= 0
        || CLSIDFromString(wide, &guidOut) != NOERROR)
    {
        detailOut = "Failed to parse project WinUSB DeviceInterfaceGUID";
        return false;
    }
    return true;
}

UnitIdentityKind toRegistryKind(Mt4UnitIdentityKind kind)
{
    return kind == Mt4UnitIdentityKind::Serial ? UnitIdentityKind::Serial
                                               : UnitIdentityKind::Topology;
}

Mt4WinUsbPresence enumHasPresentInterface(const GUID& guid, std::string& detailOut)
{
    HDEVINFO deviceInfo = SetupDiGetClassDevsW(
        &guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfo == INVALID_HANDLE_VALUE)
    {
        detailOut = "SetupDiGetClassDevsW failed while checking MT4 presence";
        return Mt4WinUsbPresence::Error;
    }

    SP_DEVICE_INTERFACE_DATA interfaceData = {};
    interfaceData.cbSize = sizeof(interfaceData);
    SetLastError(0);
    const BOOL found = SetupDiEnumDeviceInterfaces(
        deviceInfo, nullptr, &guid, 0, &interfaceData);
    const DWORD enumError = GetLastError();
    SetupDiDestroyDeviceInfoList(deviceInfo);

    if (found)
    {
        detailOut.clear();
        return Mt4WinUsbPresence::Present;
    }

    if (enumError == ERROR_NO_MORE_ITEMS)
    {
        detailOut = "No present device interface for project WinUSB GUID";
        return Mt4WinUsbPresence::Absent;
    }

    char buffer[96] = {};
    std::snprintf(
        buffer,
        sizeof(buffer),
        "SetupDiEnumDeviceInterfaces failed (GetLastError=%lu)",
        static_cast<unsigned long>(enumError));
    detailOut = buffer;
    return Mt4WinUsbPresence::Error;
}

bool appendPresentInterface(
    const Mt4PresentWinUsbInterface& entry,
    std::vector<Mt4WinUsbInterfaceInfo>& interfacesOut,
    std::string& errorOut)
{
    Mt4WinUsbInterfaceInfo info;
    info.devicePathUtf8 = wideToUtf8Lossy(entry.devicePath);
    if (info.devicePathUtf8.empty())
    {
        errorOut = "MT4 device path UTF-8 conversion failed";
        return false;
    }
    info.identityKey = entry.identityKey;
    info.identityKind = toRegistryKind(entry.identityKind);
    info.topologyKey = entry.topologyKey;
    interfacesOut.push_back(std::move(info));
    return true;
}
#endif
} // namespace

Mt4WinUsbPresence queryMt4WinUsbPresence(std::string& detailOut)
{
#ifdef _WIN32
    GUID guid = {};
    if (!parseProjectGuid(guid, detailOut))
    {
        return Mt4WinUsbPresence::Error;
    }
    return enumHasPresentInterface(guid, detailOut);
#else
    detailOut = "MT4 WinUSB presence check requires Windows";
    return Mt4WinUsbPresence::Error;
#endif
}

bool listMt4WinUsbInterfaces(
    std::vector<Mt4WinUsbInterfaceInfo>& interfacesOut,
    std::string& errorOut)
{
    interfacesOut.clear();
#ifdef _WIN32
    const DeviceProfile* mt4 = findDeviceProfile(kEmagicVendorId, kMt4ProductId);
    if (mt4 == nullptr)
    {
        errorOut = "MT4 DeviceProfile not found";
        return false;
    }
    GUID guid = {};
    if (!parseProjectGuid(guid, errorOut))
    {
        return false;
    }
    std::vector<Mt4PresentWinUsbInterface> raw;
    if (!enumeratePresentMt4WinUsbInterfaces(guid, *mt4, raw, errorOut))
    {
        return false;
    }
    interfacesOut.reserve(raw.size());
    for (const Mt4PresentWinUsbInterface& entry : raw)
    {
        if (!appendPresentInterface(entry, interfacesOut, errorOut))
        {
            interfacesOut.clear();
            return false;
        }
    }
    errorOut.clear();
    return true;
#else
    errorOut = "MT4 WinUSB interface listing requires Windows";
    return false;
#endif
}
