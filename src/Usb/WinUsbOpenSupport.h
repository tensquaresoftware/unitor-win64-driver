// Shared WinUSB open helpers (Windows only). Used by WinUsbOpenDetail.cpp.

#pragma once

#ifdef _WIN32

#include "Usb/WinUsbOpenDetail.h"

#include <string>
#include <vector>

void closeWinUsbHandles(WinUsbHandles& handles) noexcept;

std::string formatWin32Error(const char* context, DWORD errorCode);

std::string buildCompositeHardwareId(const DeviceProfile& profile);

bool deviceMatchesHardwareId(
    HDEVINFO deviceInfo,
    SP_DEVINFO_DATA& devInfo,
    const std::string& hardwareId);

bool initializeFromDevicePath(
    const wchar_t* devicePath,
    const DeviceProfile& profile,
    WinUsbHandles& handles,
    std::string& errorOut);

struct InterfaceOpenArgs
{
    HDEVINFO deviceInfo;
    SP_DEVICE_INTERFACE_DATA* interfaceData;
    const DeviceProfile* profile;
    const std::string* requiredHardwareId;
    WinUsbHandles* handles;
};

bool tryOpenInterfaceDetail(InterfaceOpenArgs& args, std::string& errorOut);

bool readDeviceInterfaceGuidsFromRegistry(
    HDEVINFO deviceInfo,
    SP_DEVINFO_DATA& devInfo,
    std::vector<GUID>& guidsOut,
    std::string& errorOut);

struct GuidOpenRequest
{
    const GUID* interfaceGuid = nullptr;
    const DeviceProfile* profile = nullptr;
    const std::string* requiredHardwareId = nullptr;
    WinUsbHandles* handles = nullptr;
};

bool openByDeviceInterfaceGuidFiltered(GuidOpenRequest& request, std::string& errorOut);

#endif // _WIN32
