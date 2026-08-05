#include "Usb/WinUsbOpenSupport.h"

#ifdef _WIN32

#include <cstddef>
#include <sstream>
#include <vector>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "winusb.lib")
#pragma comment(lib, "ole32.lib")

void closeWinUsbHandles(WinUsbHandles& handles) noexcept
{
    for (std::size_t index = 0; index < handles.associatedCount; ++index)
    {
        if (handles.associated[index] != nullptr)
        {
            WinUsb_Free(handles.associated[index]);
            handles.associated[index] = nullptr;
        }
    }
    handles.associatedCount = 0;
    handles.winUsb = nullptr;

    if (handles.winUsbRoot != nullptr)
    {
        WinUsb_Free(handles.winUsbRoot);
        handles.winUsbRoot = nullptr;
    }

    if (handles.device != INVALID_HANDLE_VALUE && handles.device != nullptr)
    {
        CloseHandle(handles.device);
        handles.device = INVALID_HANDLE_VALUE;
    }
}

std::string formatWin32Error(const char* context, DWORD errorCode)
{
    std::ostringstream stream;
    stream << context << " (Win32 error " << errorCode << ")";
    return stream.str();
}

bool queryInterfaceNumber(
    WINUSB_INTERFACE_HANDLE winUsb,
    UCHAR& interfaceNumberOut,
    std::string& errorOut)
{
    USB_INTERFACE_DESCRIPTOR descriptor = {};
    if (!WinUsb_QueryInterfaceSettings(winUsb, 0, &descriptor))
    {
        errorOut = formatWin32Error(
            "WinUsb_QueryInterfaceSettings failed", GetLastError());
        return false;
    }

    interfaceNumberOut = descriptor.bInterfaceNumber;
    return true;
}

bool prepareEmagicBulkPipes(
    WINUSB_INTERFACE_HANDLE iface,
    UCHAR bulkOutPipeId,
    UCHAR bulkInPipeId,
    std::string& errorOut)
{
    UCHAR autoClear = TRUE;
    if (!WinUsb_SetPipePolicy(
            iface, bulkOutPipeId, AUTO_CLEAR_STALL, sizeof(autoClear), &autoClear))
    {
        errorOut = formatWin32Error(
            "WinUsb_SetPipePolicy(AUTO_CLEAR_STALL) failed for Emagic bulk OUT",
            GetLastError());
        return false;
    }
    if (bulkInPipeId != 0)
    {
        (void)WinUsb_SetPipePolicy(
            iface, bulkInPipeId, AUTO_CLEAR_STALL, sizeof(autoClear), &autoClear);
    }
    return true;
}

namespace
{
std::wstring utf8ToWide(const std::string& text)
{
    return std::wstring(text.begin(), text.end());
}

bool hardwareIdEntryMatches(const wchar_t* entry, const std::wstring& needle)
{
    if (entry == nullptr || *entry == L'\0')
    {
        return false;
    }

    if (_wcsicmp(entry, needle.c_str()) == 0)
    {
        return true;
    }

    const auto miPos = needle.rfind(L"&MI_");
    if (miPos == std::wstring::npos)
    {
        // Parent / whole-device needle USB\VID_xxxx&PID_yyyy: allow &REV_… but not &MI_.
        const std::wstring revPrefix = needle + L"&REV_";
        if (_wcsnicmp(entry, revPrefix.c_str(), revPrefix.size()) != 0)
        {
            return false;
        }
        return wcsstr(entry, L"&MI_") == nullptr;
    }

    // REV-tolerant: USB\VID_xxxx&PID_yyyy&REV_....&MI_zz still matches profile MI.
    const std::wstring vidPid = needle.substr(0, miPos);
    const std::wstring miToken = needle.substr(miPos);
    if (_wcsnicmp(entry, vidPid.c_str(), vidPid.size()) != 0)
    {
        return false;
    }

    return wcsstr(entry, miToken.c_str()) != nullptr;
}

bool hardwareIdListContains(
    const std::vector<wchar_t>& multiSz,
    const std::string& asciiNeedle)
{
    const std::wstring needle = utf8ToWide(asciiNeedle);
    const wchar_t* cursor = multiSz.data();
    while (cursor != nullptr && *cursor != L'\0')
    {
        if (hardwareIdEntryMatches(cursor, needle))
        {
            return true;
        }
        cursor += wcslen(cursor) + 1;
    }
    return false;
}

bool queryRegGuidValue(
    HKEY key,
    const wchar_t* valueName,
    std::vector<wchar_t>& bufferOut,
    DWORD& valueTypeOut)
{
    DWORD valueType = 0;
    DWORD valueSize = 0;
    LONG status = RegQueryValueExW(
        key, valueName, nullptr, &valueType, nullptr, &valueSize);
    if (status != ERROR_SUCCESS || valueSize == 0)
    {
        return false;
    }

    bufferOut.assign(valueSize / sizeof(wchar_t) + 2, L'\0');
    status = RegQueryValueExW(
        key,
        valueName,
        nullptr,
        &valueType,
        reinterpret_cast<LPBYTE>(bufferOut.data()),
        &valueSize);
    if (status != ERROR_SUCCESS)
    {
        return false;
    }

    valueTypeOut = valueType;
    return true;
}

void appendParsedGuids(
    const std::vector<wchar_t>& buffer,
    DWORD valueType,
    std::vector<GUID>& guidsOut)
{
    if (valueType != REG_SZ && valueType != REG_MULTI_SZ && valueType != REG_EXPAND_SZ)
    {
        return;
    }

    const wchar_t* cursor = buffer.data();
    while (cursor != nullptr && *cursor != L'\0')
    {
        GUID guid = {};
        if (SUCCEEDED(CLSIDFromString(cursor, &guid)))
        {
            guidsOut.push_back(guid);
        }

        if (valueType != REG_MULTI_SZ)
        {
            break;
        }
        cursor += wcslen(cursor) + 1;
    }
}

bool collectGuidsFromKey(HKEY key, std::vector<GUID>& guidsOut)
{
    std::vector<wchar_t> buffer;
    DWORD valueType = 0;
    const size_t before = guidsOut.size();

    if (queryRegGuidValue(key, L"DeviceInterfaceGUID", buffer, valueType))
    {
        appendParsedGuids(buffer, valueType, guidsOut);
    }

    if (queryRegGuidValue(key, L"DeviceInterfaceGUIDs", buffer, valueType))
    {
        appendParsedGuids(buffer, valueType, guidsOut);
    }

    return guidsOut.size() > before;
}
} // namespace

bool deviceMatchesHardwareId(
    HDEVINFO deviceInfo,
    SP_DEVINFO_DATA& devInfo,
    const std::string& hardwareId)
{
    DWORD propertyType = 0;
    DWORD requiredSize = 0;
    SetupDiGetDeviceRegistryPropertyW(
        deviceInfo,
        &devInfo,
        SPDRP_HARDWAREID,
        &propertyType,
        nullptr,
        0,
        &requiredSize);
    if (requiredSize == 0)
    {
        return false;
    }

    std::vector<wchar_t> buffer(requiredSize / sizeof(wchar_t) + 1);
    if (!SetupDiGetDeviceRegistryPropertyW(
            deviceInfo,
            &devInfo,
            SPDRP_HARDWAREID,
            &propertyType,
            reinterpret_cast<PBYTE>(buffer.data()),
            requiredSize,
            nullptr))
    {
        return false;
    }

    return hardwareIdListContains(buffer, hardwareId);
}

bool initializeFromDevicePath(
    const wchar_t* devicePath,
    const DeviceProfile& profile,
    WinUsbHandles& handles,
    std::string& errorOut)
{
    HANDLE device = CreateFileW(
        devicePath,
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        nullptr);
    if (device == INVALID_HANDLE_VALUE)
    {
        errorOut = formatWin32Error("CreateFileW on WinUSB path failed", GetLastError());
        return false;
    }

    WINUSB_INTERFACE_HANDLE winUsbRoot = nullptr;
    if (!WinUsb_Initialize(device, &winUsbRoot))
    {
        errorOut = formatWin32Error("WinUsb_Initialize failed", GetLastError());
        CloseHandle(device);
        return false;
    }

    WINUSB_INTERFACE_HANDLE winUsbMatch = nullptr;
    WinUsbHandles claimed = {};
    RetainAssociatedRequest retain{winUsbRoot, profile.ifnum, &claimed, &winUsbMatch};
    if (!retainAssociatedForIfnum(retain, errorOut))
    {
        for (std::size_t index = 0; index < claimed.associatedCount; ++index)
        {
            WinUsb_Free(claimed.associated[index]);
        }
        WinUsb_Free(winUsbRoot);
        CloseHandle(device);
        return false;
    }

    handles.device = device;
    handles.winUsbRoot = winUsbRoot;
    handles.winUsb = winUsbMatch;
    handles.associatedCount = claimed.associatedCount;
    for (std::size_t index = 0; index < claimed.associatedCount; ++index)
    {
        handles.associated[index] = claimed.associated[index];
    }
    return true;
}

bool tryOpenInterfaceDetail(InterfaceOpenArgs& args, std::string& errorOut)
{
    DWORD requiredSize = 0;
    SetupDiGetDeviceInterfaceDetailW(
        args.deviceInfo, args.interfaceData, nullptr, 0, &requiredSize, nullptr);
    if (requiredSize == 0)
    {
        errorOut = formatWin32Error(
            "SetupDiGetDeviceInterfaceDetailW size query failed", GetLastError());
        return false;
    }

    std::vector<uint8_t> buffer(requiredSize);
    auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buffer.data());
    detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

    SP_DEVINFO_DATA devInfo = {};
    devInfo.cbSize = sizeof(devInfo);

    if (!SetupDiGetDeviceInterfaceDetailW(
            args.deviceInfo,
            args.interfaceData,
            detail,
            requiredSize,
            nullptr,
            &devInfo))
    {
        errorOut = formatWin32Error(
            "SetupDiGetDeviceInterfaceDetailW failed", GetLastError());
        return false;
    }

    if (args.requiredHardwareId != nullptr
        && !deviceMatchesHardwareId(
            args.deviceInfo, devInfo, *args.requiredHardwareId))
    {
        errorOut = "Device interface hardware ID does not match DeviceProfile";
        return false;
    }

    return initializeFromDevicePath(
        detail->DevicePath, *args.profile, *args.handles, errorOut);
}

bool readDeviceInterfaceGuidsFromRegistry(
    HDEVINFO deviceInfo,
    SP_DEVINFO_DATA& devInfo,
    std::vector<GUID>& guidsOut,
    std::string& errorOut)
{
    guidsOut.clear();

    HKEY deviceKey = SetupDiOpenDevRegKey(
        deviceInfo, &devInfo, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    if (deviceKey == INVALID_HANDLE_VALUE)
    {
        errorOut = formatWin32Error("SetupDiOpenDevRegKey failed", GetLastError());
        return false;
    }

    collectGuidsFromKey(deviceKey, guidsOut);

    HKEY parametersKey = nullptr;
    if (RegOpenKeyExW(
            deviceKey,
            L"Device Parameters",
            0,
            KEY_READ,
            &parametersKey)
        == ERROR_SUCCESS)
    {
        collectGuidsFromKey(parametersKey, guidsOut);
        RegCloseKey(parametersKey);
    }

    RegCloseKey(deviceKey);

    if (guidsOut.empty())
    {
        errorOut =
            "Device registry has no DeviceInterfaceGUID(s) (Zadig/INF bind missing?)";
        return false;
    }

    return true;
}

#endif // _WIN32
