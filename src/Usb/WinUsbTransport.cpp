#include "Usb/WinUsbTransport.h"

#ifdef _WIN32

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

bool looksLikeIfnumMismatch(const std::string& message)
{
    return message.find("does not match DeviceProfile ifnum") != std::string::npos;
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

WinUsbTransport::~WinUsbTransport()
{
    Close();
}

void WinUsbTransport::clearPipeState() noexcept
{
    bulkOutPipeId_ = 0;
    bulkInPipeId_ = 0;
    pipesReady_ = false;
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
    if (!openByDeviceInterfaceGuid(projectGuid, profile, handles, errorOut))
    {
        const std::string guidError = errorOut;
        if (!options.allowZadigFallback)
        {
            rejectGuidOpenFailure(guidError, errorOut);
            return false;
        }

        if (!openZadigFallback(profile, handles, errorOut))
        {
            errorOut =
                "GUID-first open failed (" + guidError
                + "); Zadig fallback also failed: " + errorOut;
            return false;
        }
    }

    deviceHandle_ = handles.device;
    winUsbHandle_ = handles.winUsb;
    if (!discoverBulkPipes(errorOut))
    {
        Close();
        return false;
    }

    errorOut.clear();
    return true;
}

void WinUsbTransport::Close() noexcept
{
    clearPipeState();

    if (winUsbHandle_ != nullptr)
    {
        WinUsb_Free(static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_));
        winUsbHandle_ = nullptr;
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

bool WinUsbTransport::ReadBulk(
    uint8_t* buffer,
    std::size_t capacity,
    std::size_t& bytesRead,
    std::string& errorOut)
{
    bytesRead = 0;

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
        errorOut = formatWin32Error(
            "WinUsb_ReadPipe failed for Emagic bulk IN", GetLastError());
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

bool WinUsbTransport::WriteBulk(
    const uint8_t* /*data*/,
    std::size_t /*size*/,
    std::string& errorOut)
{
    errorOut = "WinUSB WriteBulk requires Windows";
    return false;
}

bool WinUsbTransport::ReadBulk(
    uint8_t* /*buffer*/,
    std::size_t /*capacity*/,
    std::size_t& bytesRead,
    std::string& errorOut)
{
    bytesRead = 0;
    errorOut = "WinUSB ReadBulk requires Windows";
    return false;
}

#endif // _WIN32
