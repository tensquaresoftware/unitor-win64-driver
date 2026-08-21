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
    const wchar_t* selectedDevicePath;
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
    // When non-null, only the matching device interface path may open.
    const wchar_t* selectedDevicePath = nullptr;
    WinUsbHandles* handles = nullptr;
};

bool openByDeviceInterfaceGuidFiltered(GuidOpenRequest& request, std::string& errorOut);

std::string wideToUtf8Lossy(const std::wstring& wide);
std::wstring utf8ToWideLossy(const std::string& text);

bool readDeviceInstanceId(
    HDEVINFO deviceInfo,
    SP_DEVINFO_DATA& devInfo,
    std::wstring& instanceIdOut,
    std::string& errorOut);

bool readDeviceSerialNumber(
    HDEVINFO deviceInfo,
    SP_DEVINFO_DATA& devInfo,
    std::wstring& serialOut);

bool deviceHardwareIdSuggestsIfnum(
    HDEVINFO deviceInfo,
    SP_DEVINFO_DATA& devInfo,
    uint8_t ifnum);

bool bindUtf8SelectedDevicePath(
    const std::string& selectedDevicePathUtf8,
    std::wstring& selectedWideOut,
    const wchar_t*& selectedPathOut,
    std::string& errorOut);

std::string buildParentHardwareId(const DeviceProfile& profile);
std::string buildCompositeHardwareId(const DeviceProfile& profile);

// Returns 1 and fills chosenOut on a unique HWID match; 0 if none; -1 if ambiguous.
int findUniqueHardwareIdDevice(
    HDEVINFO deviceInfo,
    const std::string& hardwareId,
    SP_DEVINFO_DATA& chosenOut);

#endif // _WIN32
