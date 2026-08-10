// Emagic session wake: init drain + Set Computer Mode (WinUSB bulk).

#include "Usb/WinUsbTransport.h"

#ifdef _WIN32

#include "Protocol/EmagicCableMapper.h"
#include "Usb/WinUsbOpenDetail.h"
#include "Usb/WinUsbOpenSupport.h"

#include <cstring>

namespace
{
constexpr std::uint32_t kSessionBulkTimeoutMs = 3000;
constexpr std::uint32_t kDrainInTimeoutMs = 200;

bool parseProjectDeviceInterfaceGuid(GUID& guidOut, std::string& errorOut)
{
    const std::size_t length = std::strlen(kMt4WinUsbDeviceInterfaceGuid);
    std::wstring wide(kMt4WinUsbDeviceInterfaceGuid, kMt4WinUsbDeviceInterfaceGuid + length);
    if (FAILED(CLSIDFromString(wide.c_str(), &guidOut)))
    {
        errorOut = "Internal error: invalid project DeviceInterfaceGUID constant";
        return false;
    }
    return true;
}

struct OpenedHandleSink
{
    void** deviceHandle = nullptr;
    void** winUsbHandle = nullptr;
    void** winUsbRootHandle = nullptr;
    void** winUsbAssociated = nullptr;
    std::size_t* winUsbAssociatedCount = nullptr;
};

void adoptOpenedHandles(WinUsbHandles& handles, OpenedHandleSink& sink)
{
    *sink.deviceHandle = handles.device;
    *sink.winUsbHandle = handles.winUsb;
    *sink.winUsbRootHandle = handles.winUsbRoot;
    *sink.winUsbAssociatedCount = handles.associatedCount;
    for (std::size_t index = 0; index < handles.associatedCount; ++index)
    {
        sink.winUsbAssociated[index] = handles.associated[index];
    }
    handles.associatedCount = 0;
    handles.winUsb = nullptr;
    handles.winUsbRoot = nullptr;
    handles.device = INVALID_HANDLE_VALUE;
}
} // namespace

bool WinUsbTransport::Open(
    const DeviceProfile& profile,
    std::string& errorOut,
    WinUsbOpenOptions options)
{
    Close();

    GUID projectGuid = {};
    if (!parseProjectDeviceInterfaceGuid(projectGuid, errorOut))
    {
        return false;
    }

    WinUsbHandles handles;
    WinUsbOpenRequest openRequest;
    openRequest.profile = &profile;
    openRequest.projectGuid = &projectGuid;
    openRequest.preferZadig = options.allowZadigFallback;
    openRequest.handles = &handles;
    std::wstring selectedWide;
    if (!bindUtf8SelectedDevicePath(
            options.selectedDevicePath,
            selectedWide,
            openRequest.selectedDevicePath,
            errorOut)
        || !openWinUsbHandles(openRequest, errorOut))
    {
        return false;
    }

    OpenedHandleSink sink{
        &deviceHandle_,
        &winUsbHandle_,
        &winUsbRootHandle_,
        winUsbAssociated_,
        &winUsbAssociatedCount_};
    adoptOpenedHandles(handles, sink);
    if (!discoverBulkPipes(errorOut)
        || !applyBulkTransferTimeouts(errorOut)
        || !prepareBulkPipes(errorOut))
    {
        Close();
        return false;
    }

    errorOut.clear();
    return true;
}

bool WinUsbTransport::setPipeTransferTimeoutMs(
    unsigned char pipeId,
    std::uint32_t timeoutMs,
    const char* failureContext,
    std::string& errorOut)
{
    ULONG value = timeoutMs;
    if (!WinUsb_SetPipePolicy(
            static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_),
            pipeId,
            PIPE_TRANSFER_TIMEOUT,
            sizeof(value),
            &value))
    {
        errorOut = formatWin32Error(failureContext, GetLastError());
        return false;
    }
    return true;
}

bool WinUsbTransport::setBulkTransferTimeoutMs(
    std::uint32_t timeoutMs,
    std::string& errorOut)
{
    if (!setPipeTransferTimeoutMs(
            bulkInPipeId_,
            timeoutMs,
            "WinUsb_SetPipePolicy(PIPE_TRANSFER_TIMEOUT) failed for Emagic bulk IN",
            errorOut))
    {
        return false;
    }
    return setPipeTransferTimeoutMs(
        bulkOutPipeId_,
        timeoutMs,
        "WinUsb_SetPipePolicy(PIPE_TRANSFER_TIMEOUT) failed for Emagic bulk OUT",
        errorOut);
}

bool WinUsbTransport::applyBulkTransferTimeouts(std::string& errorOut)
{
    // Bound blocking ReadBulk / WriteBulk so DeviceSession Stop can progress.
    // 100ms was too short for Emagic init magic on some lab hosts (Boot Camp) —
    // Win32 error 121 (ERROR_SEM_TIMEOUT) on the first bulk OUT.
    return setBulkTransferTimeoutMs(kSessionBulkTimeoutMs, errorOut);
}

bool WinUsbTransport::DrainBulkInBestEffort(
    std::size_t maxPackets,
    std::size_t& drainedOut,
    std::string& errorOut)
{
    drainedOut = 0;
    if (!BeginShortBulkInDrain(errorOut))
    {
        return false;
    }
    drainedOut = drainBulkInUntilIdle(maxPackets);
    std::string restoreError;
    if (!RestoreSessionBulkTimeouts(restoreError))
    {
        errorOut = restoreError;
        return false;
    }
    errorOut.clear();
    return true;
}

std::size_t WinUsbTransport::drainBulkInUntilIdle(std::size_t maxPackets)
{
    std::size_t total = 0;
    const std::size_t capacity = BulkInReadCapacity();
    uint8_t buffer[512] = {};
    if (capacity == 0 || capacity > sizeof(buffer))
    {
        return 0;
    }
    for (std::size_t packet = 0; packet < maxPackets; ++packet)
    {
        std::size_t bytesRead = 0;
        std::string ignored;
        if (!ReadBulk(buffer, capacity, bytesRead, ignored))
        {
            break;
        }
        if (bytesRead == 0)
        {
            break;
        }
        total += bytesRead;
    }
    return total;
}

bool WinUsbTransport::writeInitMagicWithFinishRetry(std::string& errorOut)
{
    if (WriteBulk(kEmagicInitMagic, kEmagicInitMagicSize, errorOut))
    {
        return true;
    }

    const std::string firstError = errorOut;
    std::string ignored;
    (void)WriteBulk(kEmagicFinishMagic, kEmagicFinishMagicSize, ignored);
    if (WriteBulk(kEmagicInitMagic, kEmagicInitMagicSize, errorOut))
    {
        return true;
    }

    errorOut = firstError + "; retry after finish also failed: " + errorOut;
    return false;
}

bool WinUsbTransport::armShortInDrainTimeout(std::string& errorOut)
{
    return setPipeTransferTimeoutMs(
        bulkInPipeId_,
        kDrainInTimeoutMs,
        "WinUsb_SetPipePolicy(PIPE_TRANSFER_TIMEOUT) failed for Emagic bulk IN",
        errorOut);
}

bool WinUsbTransport::BeginShortBulkInDrain(std::string& errorOut)
{
    return armShortInDrainTimeout(errorOut);
}

bool WinUsbTransport::RestoreSessionBulkTimeouts(std::string& errorOut)
{
    return setBulkTransferTimeoutMs(kSessionBulkTimeoutMs, errorOut);
}

bool WinUsbTransport::WriteEmagicInitSequence(
    std::string& errorOut,
    std::size_t* drainedBytesOut)
{
    std::size_t drained = 0;
    std::string ignored;
    auto* const iface = static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_);

    (void)WinUsb_AbortPipe(iface, bulkOutPipeId_);
    (void)WinUsb_AbortPipe(iface, bulkInPipeId_);
    (void)WinUsb_ResetPipe(iface, bulkOutPipeId_);
    (void)WinUsb_ResetPipe(iface, bulkInPipeId_);
    if (!prepareBulkPipes(errorOut))
    {
        return false;
    }

    // Lab: after init OUT without reading IN, further OUT NAKs (probe finish 121).
    // Pre-drain any pending reply, then wake OUT again at full session timeout.
    if (!armShortInDrainTimeout(errorOut))
    {
        return false;
    }
    drained += drainBulkInUntilIdle(8);
    if (!setBulkTransferTimeoutMs(kSessionBulkTimeoutMs, errorOut)
        || !writeInitMagicWithFinishRetry(errorOut))
    {
        return false;
    }

    // Must drain the init reply before any further OUT (second init / computer mode).
    if (!armShortInDrainTimeout(errorOut))
    {
        return false;
    }
    drained += drainBulkInUntilIdle(8);

    (void)WriteBulk(kEmagicInitMagic, kEmagicInitMagicSize, ignored);
    drained += drainBulkInUntilIdle(8);

    if (!WriteBulk(
            kEmagicSetComputerModeMagic, kEmagicSetComputerModeMagicSize, errorOut))
    {
        (void)setBulkTransferTimeoutMs(kSessionBulkTimeoutMs, ignored);
        return false;
    }
    drained += drainBulkInUntilIdle(8);

    if (!setBulkTransferTimeoutMs(kSessionBulkTimeoutMs, errorOut))
    {
        return false;
    }

    if (drainedBytesOut != nullptr)
    {
        *drainedBytesOut = drained;
    }
    errorOut.clear();
    return true;
}

#else // !_WIN32

bool WinUsbTransport::DrainBulkInBestEffort(
    std::size_t /*maxPackets*/,
    std::size_t& drainedOut,
    std::string& errorOut)
{
    drainedOut = 0;
    errorOut = "WinUSB DrainBulkInBestEffort requires Windows";
    return false;
}

bool WinUsbTransport::WriteEmagicInitSequence(
    std::string& errorOut,
    std::size_t* drainedBytesOut)
{
    if (drainedBytesOut != nullptr)
    {
        *drainedBytesOut = 0;
    }
    errorOut = "WinUSB WriteEmagicInitSequence requires Windows";
    return false;
}

#endif // _WIN32
