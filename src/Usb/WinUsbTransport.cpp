#include "Usb/WinUsbTransport.h"

#ifdef _WIN32

#include "Protocol/EmagicCableMapper.h"
#include "Usb/WinUsbOpenDetail.h"
#include "Usb/WinUsbOpenSupport.h"

#include <cstring>
#include <sstream>

namespace
{
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

bool isBulkOutPipe(const WINUSB_PIPE_INFORMATION& pipe) noexcept
{
    return pipe.PipeType == UsbdPipeTypeBulk
        && (pipe.PipeId & 0x80) == 0;
}

bool isBulkInPipe(const WINUSB_PIPE_INFORMATION& pipe) noexcept
{
    return pipe.PipeType == UsbdPipeTypeBulk
        && (pipe.PipeId & 0x80) != 0;
}

struct BulkPipeClaim
{
    unsigned char pipeId = 0;
    bool found = false;
};

bool claimUniqueBulkPipe(
    unsigned char pipeId,
    BulkPipeClaim& claim,
    const char* ambiguousMessage,
    std::string& errorOut)
{
    if (claim.found)
    {
        errorOut = ambiguousMessage;
        return false;
    }
    claim.pipeId = pipeId;
    claim.found = true;
    return true;
}

bool considerBulkEndpoint(
    const WINUSB_PIPE_INFORMATION& pipe,
    BulkPipeClaim& outClaim,
    BulkPipeClaim& inClaim,
    std::string& errorOut)
{
    if (isBulkOutPipe(pipe))
    {
        return claimUniqueBulkPipe(
            pipe.PipeId,
            outClaim,
            "Opened WinUSB interface has ambiguous bulk OUT pipes",
            errorOut);
    }
    if (isBulkInPipe(pipe))
    {
        return claimUniqueBulkPipe(
            pipe.PipeId,
            inClaim,
            "Opened WinUSB interface has ambiguous bulk IN pipes",
            errorOut);
    }
    return true;
}

} // namespace

WinUsbTransport::~WinUsbTransport()
{
    Close();
}

void WinUsbTransport::clearPipeState() noexcept
{
    bulkOutPipeId_ = 0;
    bulkInPipeId_ = 0;
    pipesReady_ = false;
    lastReadTimedOut_ = false;
}

bool WinUsbTransport::applyBulkTransferTimeouts(std::string& errorOut)
{
    // Bound blocking ReadBulk / WriteBulk so DeviceSession Stop can progress.
    // 100ms was too short for Emagic init magic on some lab hosts (Boot Camp) —
    // Win32 error 121 (ERROR_SEM_TIMEOUT) on the first bulk OUT.
    constexpr ULONG kBulkTransferTimeoutMs = 3000;
    ULONG timeoutMs = kBulkTransferTimeoutMs;
    if (!WinUsb_SetPipePolicy(
            static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_),
            bulkInPipeId_,
            PIPE_TRANSFER_TIMEOUT,
            sizeof(timeoutMs),
            &timeoutMs))
    {
        errorOut = formatWin32Error(
            "WinUsb_SetPipePolicy(PIPE_TRANSFER_TIMEOUT) failed for Emagic bulk IN",
            GetLastError());
        return false;
    }
    timeoutMs = kBulkTransferTimeoutMs;
    if (!WinUsb_SetPipePolicy(
            static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_),
            bulkOutPipeId_,
            PIPE_TRANSFER_TIMEOUT,
            sizeof(timeoutMs),
            &timeoutMs))
    {
        errorOut = formatWin32Error(
            "WinUsb_SetPipePolicy(PIPE_TRANSFER_TIMEOUT) failed for Emagic bulk OUT",
            GetLastError());
        return false;
    }
    return true;
}

bool WinUsbTransport::prepareBulkPipes(std::string& errorOut)
{
    return prepareEmagicBulkPipes(
        static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_),
        bulkOutPipeId_,
        bulkInPipeId_,
        errorOut);
}

bool WinUsbTransport::discoverBulkPipes(std::string& errorOut)
{
    clearPipeState();

    USB_INTERFACE_DESCRIPTOR iface = {};
    if (!WinUsb_QueryInterfaceSettings(
            static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_),
            0,
            &iface))
    {
        errorOut = "WinUsb_QueryInterfaceSettings failed while discovering bulk pipes";
        return false;
    }

    BulkPipeClaim outClaim;
    BulkPipeClaim inClaim;
    for (UCHAR index = 0; index < iface.bNumEndpoints; ++index)
    {
        WINUSB_PIPE_INFORMATION pipe = {};
        if (!WinUsb_QueryPipe(
                static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_),
                0,
                index,
                &pipe))
        {
            errorOut = "WinUsb_QueryPipe failed while discovering bulk pipes";
            return false;
        }
        if (!considerBulkEndpoint(pipe, outClaim, inClaim, errorOut))
        {
            return false;
        }
    }

    if (!outClaim.found || !inClaim.found)
    {
        errorOut = "Opened WinUSB interface has no bulk IN/OUT pair for Emagic MIDI";
        return false;
    }

    bulkOutPipeId_ = outClaim.pipeId;
    bulkInPipeId_ = inClaim.pipeId;
    pipesReady_ = true;
    return true;
}

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
    if (!openWinUsbHandles(openRequest, errorOut))
    {
        return false;
    }

    deviceHandle_ = handles.device;
    winUsbHandle_ = handles.winUsb;
    winUsbRootHandle_ = handles.winUsbRoot;
    winUsbAssociatedCount_ = handles.associatedCount;
    for (std::size_t index = 0; index < handles.associatedCount; ++index)
    {
        winUsbAssociated_[index] = handles.associated[index];
    }
    handles.associatedCount = 0;
    handles.winUsb = nullptr;
    handles.winUsbRoot = nullptr;
    handles.device = INVALID_HANDLE_VALUE;

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

bool WinUsbTransport::LastReadTimedOut() const noexcept
{
    return lastReadTimedOut_;
}

void WinUsbTransport::Close() noexcept
{
    clearPipeState();

    for (std::size_t index = 0; index < winUsbAssociatedCount_; ++index)
    {
        if (winUsbAssociated_[index] != nullptr)
        {
            WinUsb_Free(static_cast<WINUSB_INTERFACE_HANDLE>(winUsbAssociated_[index]));
            winUsbAssociated_[index] = nullptr;
        }
    }
    winUsbAssociatedCount_ = 0;
    winUsbHandle_ = nullptr;

    if (winUsbRootHandle_ != nullptr)
    {
        WinUsb_Free(static_cast<WINUSB_INTERFACE_HANDLE>(winUsbRootHandle_));
        winUsbRootHandle_ = nullptr;
    }

    if (deviceHandle_ != nullptr && deviceHandle_ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(static_cast<HANDLE>(deviceHandle_));
        deviceHandle_ = nullptr;
    }
}

bool WinUsbTransport::IsOpen() const noexcept
{
    return winUsbHandle_ != nullptr && pipesReady_;
}

bool WinUsbTransport::WriteBulk(
    const uint8_t* data,
    std::size_t size,
    std::string& errorOut)
{
    if (!IsOpen())
    {
        errorOut = "WinUSB WriteBulk requires an open transport with bulk pipes";
        return false;
    }
    if (data == nullptr && size != 0)
    {
        errorOut = "WinUSB WriteBulk data pointer is null";
        return false;
    }

    ULONG transferred = 0;
    const BOOL ok = WinUsb_WritePipe(
        static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_),
        bulkOutPipeId_,
        const_cast<PUCHAR>(reinterpret_cast<const UCHAR*>(data)),
        static_cast<ULONG>(size),
        &transferred,
        nullptr);

    if (!ok)
    {
        errorOut = formatWin32Error(
            "WinUsb_WritePipe failed for Emagic bulk OUT", GetLastError());
        return false;
    }
    if (transferred != size)
    {
        std::ostringstream stream;
        stream << "WinUsb_WritePipe short write for Emagic bulk OUT (sent "
               << transferred << " of " << size << " bytes)";
        errorOut = stream.str();
        return false;
    }

    errorOut.clear();
    return true;
}

bool WinUsbTransport::WriteEmagicInitSequence(std::string& errorOut)
{
    std::string ignored;
    if (!WriteBulk(kEmagicInitMagic, kEmagicInitMagicSize, errorOut))
    {
        const std::string firstError = errorOut;
        (void)WriteBulk(kEmagicFinishMagic, kEmagicFinishMagicSize, ignored);
        if (!WriteBulk(kEmagicInitMagic, kEmagicInitMagicSize, errorOut))
        {
            errorOut = firstError + "; retry after finish also failed: " + errorOut;
            return false;
        }
    }
    (void)WriteBulk(kEmagicInitMagic, kEmagicInitMagicSize, ignored);
    errorOut.clear();
    return true;
}

bool WinUsbTransport::ReadBulk(
    uint8_t* buffer,
    std::size_t capacity,
    std::size_t& bytesRead,
    std::string& errorOut)
{
    bytesRead = 0;
    lastReadTimedOut_ = false;

    if (!IsOpen())
    {
        errorOut = "WinUSB ReadBulk requires an open transport with bulk pipes";
        return false;
    }
    if (buffer == nullptr || capacity == 0)
    {
        errorOut = "WinUSB ReadBulk buffer is missing";
        return false;
    }

    ULONG transferred = 0;
    const BOOL ok = WinUsb_ReadPipe(
        static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_),
        bulkInPipeId_,
        reinterpret_cast<PUCHAR>(buffer),
        static_cast<ULONG>(capacity),
        &transferred,
        nullptr);

    if (!ok)
    {
        const DWORD code = GetLastError();
        if (code == ERROR_SEM_TIMEOUT)
        {
            // PIPE_TRANSFER_TIMEOUT elapsed — caller may retry or stop cleanly.
            lastReadTimedOut_ = true;
            errorOut = "WinUSB ReadBulk timed out on Emagic bulk IN";
            return false;
        }
        errorOut = formatWin32Error("WinUsb_ReadPipe failed for Emagic bulk IN", code);
        return false;
    }

    bytesRead = static_cast<std::size_t>(transferred);
    errorOut.clear();
    return true;
}

#else // !_WIN32

WinUsbTransport::~WinUsbTransport()
{
    Close();
}

bool WinUsbTransport::Open(
    const DeviceProfile& /*profile*/,
    std::string& errorOut,
    WinUsbOpenOptions /*options*/)
{
    Close();
    open_ = false;
    errorOut = "WinUSB requires Windows";
    return false;
}

void WinUsbTransport::Close() noexcept
{
    open_ = false;
}

bool WinUsbTransport::IsOpen() const noexcept
{
    return open_;
}

bool WinUsbTransport::LastReadTimedOut() const noexcept
{
    return lastReadTimedOut_;
}

bool WinUsbTransport::WriteBulk(
    const uint8_t* /*data*/,
    std::size_t /*size*/,
    std::string& errorOut)
{
    errorOut = "WinUSB WriteBulk requires Windows";
    return false;
}

bool WinUsbTransport::WriteEmagicInitSequence(std::string& errorOut)
{
    errorOut = "WinUSB WriteEmagicInitSequence requires Windows";
    return false;
}

bool WinUsbTransport::ReadBulk(
    uint8_t* /*buffer*/,
    std::size_t /*capacity*/,
    std::size_t& bytesRead,
    std::string& errorOut)
{
    bytesRead = 0;
    lastReadTimedOut_ = false;
    errorOut = "WinUSB ReadBulk requires Windows";
    return false;
}

#endif // _WIN32
