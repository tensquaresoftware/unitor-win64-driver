#include "Usb/WinUsbOpenDetail.h"

#ifdef _WIN32

#include "Usb/WinUsbOpenSupport.h"

#include <cstdio>
#include <sstream>
#include <vector>

namespace
{
void appendHex4(std::ostringstream& stream, uint16_t value)
{
    char buffer[8] = {};
    std::snprintf(buffer, sizeof(buffer), "%04X", static_cast<unsigned>(value));
    stream << buffer;
}
} // namespace

std::string buildParentHardwareId(const DeviceProfile& profile)
{
    std::ostringstream stream;
    stream << "USB\\VID_";
    appendHex4(stream, profile.vid);
    stream << "&PID_";
    appendHex4(stream, profile.pid);
    return stream.str();
}

std::string buildCompositeHardwareId(const DeviceProfile& profile)
{
    std::ostringstream stream;
    stream << buildParentHardwareId(profile);
    stream << "&MI_";
    char mi[8] = {};
    std::snprintf(mi, sizeof(mi), "%02X", static_cast<unsigned>(profile.ifnum));
    stream << mi;
    return stream.str();
}

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
        ctx.request->selectedDevicePath,
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
} // namespace

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
    // Prefer composite MI_xx nodes; some lab binds (Zadig on Class_FF parent) only
    // expose USB\VID_xxxx&PID_yyyy without &MI_.
    const std::string compositeId = buildCompositeHardwareId(profile);
    const std::string parentId = buildParentHardwareId(profile);

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
    std::string hardwareId = compositeId;
    int match = findUniqueHardwareIdDevice(deviceInfo, compositeId, chosenDevInfo);
    if (match == 0)
    {
        hardwareId = parentId;
        match = findUniqueHardwareIdDevice(deviceInfo, parentId, chosenDevInfo);
    }
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

namespace
{
bool looksLikeIfnumMismatch(const std::string& message)
{
    return message.find("does not match DeviceProfile ifnum") != std::string::npos;
}

void rejectGuidOpenFailure(const std::string& guidError, std::string& errorOut)
{
    if (looksLikeIfnumMismatch(guidError)
        || guidError.find("refusing ambiguous open") != std::string::npos)
    {
        errorOut = guidError;
        return;
    }

    errorOut =
        "WinUSB device interface GUID not available or open failed: " + guidError
        + ". Bind MT4 with installer/mt4-winusb.inf (see docs/dev/winusb-bind.md).";
}
} // namespace

bool openWinUsbHandles(WinUsbOpenRequest& request, std::string& errorOut)
{
    if (request.profile == nullptr || request.projectGuid == nullptr
        || request.handles == nullptr)
    {
        errorOut = "openWinUsbHandles requires profile, GUID, and handles";
        return false;
    }

    if (request.selectedDevicePath != nullptr && request.selectedDevicePath[0] != L'\0')
    {
        if (initializeFromDevicePath(
                request.selectedDevicePath, *request.profile, *request.handles, errorOut))
        {
            return true;
        }
        return false;
    }

    if (request.preferZadig)
    {
        if (openZadigFallback(*request.profile, *request.handles, errorOut))
        {
            return true;
        }
        const std::string zadigError = errorOut;
        if (openByDeviceInterfaceGuid(
                *request.projectGuid, *request.profile, *request.handles, errorOut))
        {
            return true;
        }
        errorOut =
            "Zadig-first open failed (" + zadigError
            + "); GUID open also failed: " + errorOut;
        return false;
    }

    if (openByDeviceInterfaceGuid(
            *request.projectGuid, *request.profile, *request.handles, errorOut))
    {
        return true;
    }
    rejectGuidOpenFailure(errorOut, errorOut);
    return false;
}

bool claimOneAssociated(RetainAssociatedRequest& request, std::string& errorOut)
{
    for (UCHAR associatedIndex = 0;; ++associatedIndex)
    {
        WINUSB_INTERFACE_HANDLE associated = nullptr;
        if (!WinUsb_GetAssociatedInterface(
                request.winUsbRoot, associatedIndex, &associated))
        {
            const DWORD err = GetLastError();
            if (err == ERROR_NO_MORE_ITEMS || err == ERROR_INVALID_PARAMETER)
            {
                return true;
            }
            errorOut = formatWin32Error("WinUsb_GetAssociatedInterface failed", err);
            return false;
        }

        if (request.claimedOut->associatedCount
            >= sizeof(request.claimedOut->associated)
                / sizeof(request.claimedOut->associated[0]))
        {
            WinUsb_Free(associated);
            errorOut = "Too many associated WinUSB interfaces to retain";
            return false;
        }

        request.claimedOut->associated[request.claimedOut->associatedCount++] =
            associated;

        UCHAR associatedIfnum = 0;
        if (!queryInterfaceNumber(associated, associatedIfnum, errorOut))
        {
            return false;
        }
        if (associatedIfnum == request.ifnum)
        {
            *request.matchOut = associated;
        }
    }
}

bool retainAssociatedForIfnum(RetainAssociatedRequest& request, std::string& errorOut)
{
    *request.matchOut = nullptr;
    request.claimedOut->associatedCount = 0;

    UCHAR rootIfnum = 0;
    if (!queryInterfaceNumber(request.winUsbRoot, rootIfnum, errorOut))
    {
        return false;
    }
    if (rootIfnum == request.ifnum)
    {
        *request.matchOut = request.winUsbRoot;
    }

    if (!claimOneAssociated(request, errorOut))
    {
        return false;
    }
    if (*request.matchOut != nullptr)
    {
        return true;
    }

    std::ostringstream stream;
    stream << "Bound USB interface number " << static_cast<unsigned>(rootIfnum)
           << " does not match DeviceProfile ifnum "
           << static_cast<unsigned>(request.ifnum)
           << " (no matching associated interface)";
    errorOut = stream.str();
    return false;
}

#endif // _WIN32
