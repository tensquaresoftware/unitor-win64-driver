// Shared WinUSB open helpers (Windows only). Used by WinUsbOpenDetail.cpp.

#pragma once

#ifdef _WIN32

#include "Usb/WinUsbOpenDetail.h"

#include <string>
#include <vector>

void closeWinUsbHandles(WinUsbHandles& handles) noexcept;

std::string formatWin32Error(const char* context, DWORD errorCode);

bool queryInterfaceNumber(
    WINUSB_INTERFACE_HANDLE winUsb,
    UCHAR& interfaceNumberOut,
    std::string& errorOut);

bool prepareEmagicBulkPipes(
    WINUSB_INTERFACE_HANDLE iface,
    UCHAR bulkOutPipeId,
    UCHAR bulkInPipeId,
    std::string& errorOut);

struct RetainAssociatedRequest
{
    WINUSB_INTERFACE_HANDLE winUsbRoot = nullptr;
    uint8_t ifnum = 0;
    WinUsbHandles* claimedOut = nullptr;
    WINUSB_INTERFACE_HANDLE* matchOut = nullptr;
};

bool retainAssociatedForIfnum(RetainAssociatedRequest& request, std::string& errorOut);

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
