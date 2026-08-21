// Enumerate present MT4 WinUSB interfaces without opening/claiming devices.

#include "Usb/WinUsbOpenDetail.h"

#ifdef _WIN32

#include "Usb/WinUsbOpenSupport.h"

#include <vector>

namespace
{
bool looksLikeStableUsbSerial(const std::wstring& serialOrTail)
{
    if (serialOrTail.empty())
    {
        return false;
    }
    // SetupAPI auto instance tails look like "5&2E5C3A&0&4" (contain '&').
    return serialOrTail.find(L'&') == std::wstring::npos;
}

bool trySetSerialIdentity(const std::wstring& serialWide, Mt4PresentWinUsbInterface& infoOut)
{
    if (!looksLikeStableUsbSerial(serialWide))
    {
        return false;
    }
    const std::string serialUtf8 = wideToUtf8Lossy(serialWide);
    if (serialUtf8.empty())
    {
        return false;
    }
    infoOut.identityKind = Mt4UnitIdentityKind::Serial;
    infoOut.identityKey = serialUtf8;
    return true;
}

bool resolveInterfaceIdentity(
    HDEVINFO deviceInfo,
    SP_DEVINFO_DATA& devInfo,
    Mt4PresentWinUsbInterface& infoOut,
    std::string& errorOut)
{
    std::wstring instanceId;
    if (!readDeviceInstanceId(deviceInfo, devInfo, instanceId, errorOut))
    {
        return false;
    }
    infoOut.topologyKey = wideToUtf8Lossy(instanceId);
    if (infoOut.topologyKey.empty())
    {
        errorOut = "MT4 topology identity UTF-8 conversion failed";
        return false;
    }

    std::wstring serial;
    if (readDeviceSerialNumber(deviceInfo, devInfo, serial)
        && trySetSerialIdentity(serial, infoOut))
    {
        return true;
    }

    const auto slash = instanceId.find_last_of(L'\\');
    if (slash != std::wstring::npos && slash + 1 < instanceId.size())
    {
        if (trySetSerialIdentity(instanceId.substr(slash + 1), infoOut))
        {
            return true;
        }
    }

    infoOut.identityKind = Mt4UnitIdentityKind::Topology;
    infoOut.identityKey = infoOut.topologyKey;
    return true;
}

struct CollectPresentArgs
{
    HDEVINFO deviceInfo = nullptr;
    SP_DEVICE_INTERFACE_DATA* interfaceData = nullptr;
    const DeviceProfile* profile = nullptr;
    std::vector<Mt4PresentWinUsbInterface>* interfacesOut = nullptr;
};

bool collectPresentInterface(CollectPresentArgs& args, std::string& errorOut)
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

    if (!deviceHardwareIdSuggestsIfnum(args.deviceInfo, devInfo, args.profile->ifnum))
    {
        return true;
    }

    Mt4PresentWinUsbInterface info;
    info.devicePath = detail->DevicePath;
    if (!resolveInterfaceIdentity(args.deviceInfo, devInfo, info, errorOut))
    {
        return false;
    }
    args.interfacesOut->push_back(std::move(info));
    return true;
}
} // namespace

bool enumeratePresentMt4WinUsbInterfaces(
    const GUID& interfaceGuid,
    const DeviceProfile& profile,
    std::vector<Mt4PresentWinUsbInterface>& interfacesOut,
    std::string& errorOut)
{
    interfacesOut.clear();
    HDEVINFO deviceInfo = SetupDiGetClassDevsW(
        &interfaceGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfo == INVALID_HANDLE_VALUE)
    {
        errorOut = formatWin32Error("SetupDiGetClassDevsW failed", GetLastError());
        return false;
    }

    SP_DEVICE_INTERFACE_DATA interfaceData = {};
    interfaceData.cbSize = sizeof(interfaceData);
    for (DWORD index = 0;
         SetupDiEnumDeviceInterfaces(
             deviceInfo, nullptr, &interfaceGuid, index, &interfaceData);
         ++index)
    {
        CollectPresentArgs args{deviceInfo, &interfaceData, &profile, &interfacesOut};
        if (!collectPresentInterface(args, errorOut))
        {
            SetupDiDestroyDeviceInfoList(deviceInfo);
            return false;
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfo);
    errorOut.clear();
    return true;
}

bool tryEnumerateZadigLabPresentMt4Interface(
    const DeviceProfile& profile,
    Mt4PresentWinUsbInterface& interfaceOut,
    std::string& errorOut)
{
    interfaceOut = Mt4PresentWinUsbInterface{};
    const std::string compositeId = buildCompositeHardwareId(profile);
    const std::string parentId = buildParentHardwareId(profile);

    HDEVINFO deviceInfo = SetupDiGetClassDevsW(
        nullptr, L"USB", nullptr, DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if (deviceInfo == INVALID_HANDLE_VALUE)
    {
        errorOut = formatWin32Error(
            "Zadig lab presence SetupDiGetClassDevsW failed", GetLastError());
        return false;
    }

    SP_DEVINFO_DATA chosenDevInfo = {};
    chosenDevInfo.cbSize = sizeof(chosenDevInfo);
    int match = findUniqueHardwareIdDevice(deviceInfo, compositeId, chosenDevInfo);
    if (match == 0)
    {
        match = findUniqueHardwareIdDevice(deviceInfo, parentId, chosenDevInfo);
    }
    if (match < 0)
    {
        SetupDiDestroyDeviceInfoList(deviceInfo);
        errorOut =
            "Multiple USB devices match profile hardware ID for Zadig lab presence; "
            "refusing ambiguous list";
        return false;
    }
    if (match == 0)
    {
        SetupDiDestroyDeviceInfoList(deviceInfo);
        errorOut.clear();
        return true;
    }

    // Empty path: session Open with --dev-zadig uses HWID Zadig fallback (single-unit).
    if (!resolveInterfaceIdentity(deviceInfo, chosenDevInfo, interfaceOut, errorOut))
    {
        SetupDiDestroyDeviceInfoList(deviceInfo);
        interfaceOut = Mt4PresentWinUsbInterface{};
        return false;
    }
    SetupDiDestroyDeviceInfoList(deviceInfo);
    errorOut.clear();
    return true;
}

#endif // _WIN32
