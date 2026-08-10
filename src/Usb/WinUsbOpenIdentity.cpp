// UTF-8 / wide helpers and device identity property reads for WinUSB open/enumerate.

#include "Usb/WinUsbOpenSupport.h"

#ifdef _WIN32

#include <cstdio>
#include <vector>

std::string wideToUtf8Lossy(const std::wstring& wide)
{
    if (wide.empty())
    {
        return {};
    }
    const int needed = WideCharToMultiByte(
        CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (needed <= 0)
    {
        return {};
    }
    std::string out(static_cast<std::size_t>(needed), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        out.data(),
        needed,
        nullptr,
        nullptr);
    return out;
}

std::wstring utf8ToWideLossy(const std::string& text)
{
    if (text.empty())
    {
        return {};
    }
    const int needed = MultiByteToWideChar(
        CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    if (needed <= 0)
    {
        return {};
    }
    std::wstring out(static_cast<std::size_t>(needed), L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), out.data(), needed);
    return out;
}

bool bindUtf8SelectedDevicePath(
    const std::string& selectedDevicePathUtf8,
    std::wstring& selectedWideOut,
    const wchar_t*& selectedPathOut,
    std::string& errorOut)
{
    if (selectedDevicePathUtf8.empty())
    {
        selectedPathOut = nullptr;
        return true;
    }
    selectedWideOut = utf8ToWideLossy(selectedDevicePathUtf8);
    if (selectedWideOut.empty())
    {
        errorOut = "selectedDevicePath UTF-8 to wide conversion failed";
        return false;
    }
    selectedPathOut = selectedWideOut.c_str();
    return true;
}

bool readDeviceInstanceId(
    HDEVINFO deviceInfo,
    SP_DEVINFO_DATA& devInfo,
    std::wstring& instanceIdOut,
    std::string& errorOut)
{
    wchar_t buffer[512] = {};
    if (!SetupDiGetDeviceInstanceIdW(
            deviceInfo, &devInfo, buffer, static_cast<DWORD>(std::size(buffer)), nullptr))
    {
        errorOut = formatWin32Error("SetupDiGetDeviceInstanceIdW failed", GetLastError());
        return false;
    }
    instanceIdOut = buffer;
    return true;
}

bool readDeviceSerialNumber(
    HDEVINFO deviceInfo,
    SP_DEVINFO_DATA& devInfo,
    std::wstring& serialOut)
{
    serialOut.clear();
    HKEY deviceKey = SetupDiOpenDevRegKey(
        deviceInfo, &devInfo, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    if (deviceKey == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    wchar_t value[256] = {};
    DWORD valueType = 0;
    DWORD valueSize = sizeof(value);
    const LONG status = RegQueryValueExW(
        deviceKey,
        L"SerialNumber",
        nullptr,
        &valueType,
        reinterpret_cast<LPBYTE>(value),
        &valueSize);
    RegCloseKey(deviceKey);
    if (status != ERROR_SUCCESS || (valueType != REG_SZ && valueType != REG_EXPAND_SZ))
    {
        return false;
    }
    serialOut = value;
    return !serialOut.empty();
}

bool deviceHardwareIdSuggestsIfnum(
    HDEVINFO deviceInfo,
    SP_DEVINFO_DATA& devInfo,
    uint8_t ifnum)
{
    char mi[8] = {};
    std::snprintf(mi, sizeof(mi), "%02X", static_cast<unsigned>(ifnum));
    const std::wstring wideNeedle = utf8ToWideLossy(std::string("&MI_") + mi);

    DWORD propertyType = 0;
    DWORD requiredSize = 0;
    SetupDiGetDeviceRegistryPropertyW(
        deviceInfo, &devInfo, SPDRP_HARDWAREID, &propertyType, nullptr, 0, &requiredSize);
    if (requiredSize == 0)
    {
        return true; // Open still verifies ifnum.
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
        return true;
    }

    bool sawMi = false;
    for (const wchar_t* cursor = buffer.data(); cursor != nullptr && *cursor != L'\0';
         cursor += wcslen(cursor) + 1)
    {
        if (wcsstr(cursor, L"&MI_") == nullptr)
        {
            continue;
        }
        sawMi = true;
        if (wcsstr(cursor, wideNeedle.c_str()) != nullptr)
        {
            return true;
        }
    }
    return !sawMi;
}

#endif // _WIN32
