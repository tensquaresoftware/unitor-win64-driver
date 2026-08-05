#include "Usb/WinUsbTransport.h"

#ifdef _WIN32

#include "Usb/WinUsbOpenDetail.h"

#include <cstring>

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
} // namespace

WinUsbTransport::~WinUsbTransport()
{
    Close();
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
    if (openByDeviceInterfaceGuid(projectGuid, profile, handles, errorOut))
    {
        deviceHandle_ = handles.device;
        winUsbHandle_ = handles.winUsb;
        errorOut.clear();
        return true;
    }

    const std::string guidError = errorOut;

    if (!options.allowZadigFallback)
    {
        if (looksLikeIfnumMismatch(guidError)
            || guidError.find("refusing ambiguous open") != std::string::npos)
        {
            errorOut = guidError;
        }
        else
        {
            errorOut =
                "WinUSB device interface GUID not available or open failed: " + guidError
                + ". Bind MT4 with installer/mt4-winusb.inf (see docs/dev/winusb-bind.md).";
        }
        return false;
    }

    if (!openZadigFallback(profile, handles, errorOut))
    {
        errorOut =
            "GUID-first open failed (" + guidError + "); Zadig fallback also failed: "
            + errorOut;
        return false;
    }

    deviceHandle_ = handles.device;
    winUsbHandle_ = handles.winUsb;
    errorOut.clear();
    return true;
}

void WinUsbTransport::Close() noexcept
{
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
    return winUsbHandle_ != nullptr;
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

#endif // _WIN32
