#include "Usb/WinUsbOpenDetail.h"

#ifdef _WIN32

#include "Usb/WinUsbOpenSupport.h"

#include <vector>

namespace
{
struct CandidateContext
{
    HDEVINFO deviceInfo;
    SP_DEVICE_INTERFACE_DATA* interfaceData;
    GuidOpenRequest* request;
    WinUsbHandles* chosen;
    int* matchCount;
};

bool considerInterfaceCandidate(CandidateContext& ctx, std::string& lastError)
{
    WinUsbHandles trial;
    InterfaceOpenArgs args{
        ctx.deviceInfo,
        ctx.interfaceData,
        ctx.request->profile,
        ctx.request->requiredHardwareId,
        &trial};
    if (!tryOpenInterfaceDetail(args, lastError))
    {
        return true;
    }

    ++(*ctx.matchCount);
    if (*ctx.matchCount == 1)
    {
        *ctx.chosen = trial;
        return true;
    }

    closeWinUsbHandles(*ctx.chosen);
    closeWinUsbHandles(trial);
    lastError =
        "Multiple WinUSB interfaces match project GUID and ifnum; "
        "refusing ambiguous open";
    return false;
}

struct MatchedDeviceOpenArgs
{
    HDEVINFO deviceInfo;
    SP_DEVINFO_DATA* devInfo;
    const DeviceProfile* profile;
    const std::string* hardwareId;
    WinUsbHandles* handles;
};

bool tryOpenMatchedDeviceGuids(MatchedDeviceOpenArgs& args, std::string& errorOut)
{
    std::vector<GUID> guids;
    if (!readDeviceInterfaceGuidsFromRegistry(
            args.deviceInfo, *args.devInfo, guids, errorOut))
    {
        return false;
    }

    for (const GUID& guid : guids)
    {
        GuidOpenRequest request;
        request.interfaceGuid = &guid;
        request.profile = args.profile;
        request.requiredHardwareId = args.hardwareId;
        request.handles = args.handles;
        if (openByDeviceInterfaceGuidFiltered(request, errorOut))
        {
            return true;
        }
    }

    return false;
}

// Returns 1 and fills chosenOut on a unique HWID match; 0 if none; -1 if ambiguous.
int findUniqueHardwareIdDevice(
    HDEVINFO deviceInfo,
    const std::string& hardwareId,
    SP_DEVINFO_DATA& chosenOut)
{
    SP_DEVINFO_DATA devInfo = {};
    devInfo.cbSize = sizeof(devInfo);
    int matchCount = 0;
    for (DWORD index = 0; SetupDiEnumDeviceInfo(deviceInfo, index, &devInfo); ++index)
    {
        if (!deviceMatchesHardwareId(deviceInfo, devInfo, hardwareId))
        {
            continue;
        }
        ++matchCount;
        if (matchCount == 1)
        {
            chosenOut = devInfo;
        }
        else
        {
            return -1;
        }
    }
    return matchCount == 1 ? 1 : 0;
}
} // namespace

bool openByDeviceInterfaceGuidFiltered(GuidOpenRequest& request, std::string& errorOut)
{
    HDEVINFO deviceInfo = SetupDiGetClassDevsW(
        request.interfaceGuid,
        nullptr,
        nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfo == INVALID_HANDLE_VALUE)
    {
        errorOut = formatWin32Error("SetupDiGetClassDevsW failed", GetLastError());
        return false;
    }

    SP_DEVICE_INTERFACE_DATA interfaceData = {};
    interfaceData.cbSize = sizeof(interfaceData);

    int matchCount = 0;
    WinUsbHandles chosen;
    std::string lastError = "No present device interface for project WinUSB GUID";

    for (DWORD index = 0;
         SetupDiEnumDeviceInterfaces(
             deviceInfo, nullptr, request.interfaceGuid, index, &interfaceData);
         ++index)
    {
        CandidateContext ctx{
            deviceInfo, &interfaceData, &request, &chosen, &matchCount};
        if (!considerInterfaceCandidate(ctx, lastError))
        {
            SetupDiDestroyDeviceInfoList(deviceInfo);
            errorOut = lastError;
            return false;
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfo);
    if (matchCount != 1)
    {
        errorOut = lastError;
        return false;
    }

    *request.handles = chosen;
    return true;
}

bool openByDeviceInterfaceGuid(
    const GUID& interfaceGuid,
    const DeviceProfile& profile,
    WinUsbHandles& handles,
    std::string& errorOut)
{
    GuidOpenRequest request;
    request.interfaceGuid = &interfaceGuid;
    request.profile = &profile;
    request.requiredHardwareId = nullptr;
    request.handles = &handles;
    return openByDeviceInterfaceGuidFiltered(request, errorOut);
}

bool openZadigFallback(
    const DeviceProfile& profile,
    WinUsbHandles& handles,
    std::string& errorOut)
{
    const std::string hardwareId = buildCompositeHardwareId(profile);

    HDEVINFO deviceInfo = SetupDiGetClassDevsW(
        nullptr, L"USB", nullptr, DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if (deviceInfo == INVALID_HANDLE_VALUE)
    {
        errorOut = formatWin32Error(
            "Zadig fallback SetupDiGetClassDevsW failed", GetLastError());
        return false;
    }

    SP_DEVINFO_DATA chosenDevInfo = {};
    chosenDevInfo.cbSize = sizeof(chosenDevInfo);
    const int match = findUniqueHardwareIdDevice(deviceInfo, hardwareId, chosenDevInfo);
    if (match != 1)
    {
        SetupDiDestroyDeviceInfoList(deviceInfo);
        errorOut = match < 0
            ? "Multiple USB devices match profile hardware ID for Zadig fallback; "
              "refusing ambiguous open"
            : "No USB device matching profile hardware ID for Zadig fallback";
        return false;
    }

    std::string lastError;
    MatchedDeviceOpenArgs args{
        deviceInfo, &chosenDevInfo, &profile, &hardwareId, &handles};
    const bool opened = tryOpenMatchedDeviceGuids(args, lastError);
    SetupDiDestroyDeviceInfoList(deviceInfo);
    if (!opened)
    {
        errorOut = lastError;
        return false;
    }
    return true;
}

#endif // _WIN32
