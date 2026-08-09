// Enumerate present WinUSB interfaces for the project GUID without opening the device.

#include "App/Mt4WinUsbPresence.h"

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
