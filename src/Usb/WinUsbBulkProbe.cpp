#include "Usb/WinUsbBulkProbe.h"

#ifdef _WIN32

#include "Profile/DeviceProfile.h"
#include "Protocol/EmagicCableMapper.h"
#include "Usb/WinUsbOpenDetail.h"
#include "Usb/WinUsbOpenSupport.h"

#include <iostream>
#include <string>

namespace
{
struct BulkOutWrite
{
    WINUSB_INTERFACE_HANDLE iface = nullptr;
    UCHAR outPipe = 0;
    const uint8_t* bytes = nullptr;
    std::size_t size = 0;
};

bool writeStaticOut(const BulkOutWrite& write, std::string& errorOut)
{
    ULONG timeoutMs = 3000;
    if (!WinUsb_SetPipePolicy(
            write.iface,
            write.outPipe,
            PIPE_TRANSFER_TIMEOUT,
            sizeof(timeoutMs),
            &timeoutMs))
    {
        errorOut = formatWin32Error("SetPipePolicy timeout", GetLastError());
        return false;
    }
    if (!prepareEmagicBulkPipes(write.iface, write.outPipe, 0, errorOut))
    {
        return false;
    }

    ULONG transferred = 0;
    if (!WinUsb_WritePipe(
            write.iface,
            write.outPipe,
            const_cast<PUCHAR>(write.bytes),
            static_cast<ULONG>(write.size),
            &transferred,
            nullptr)
        || transferred != write.size)
    {
        errorOut = formatWin32Error("WritePipe", GetLastError());
        return false;
    }
    errorOut.clear();
    return true;
}

bool findBulkOut(WINUSB_INTERFACE_HANDLE iface, UCHAR& outPipe)
{
    USB_INTERFACE_DESCRIPTOR desc = {};
    if (!WinUsb_QueryInterfaceSettings(iface, 0, &desc))
    {
        return false;
    }
    for (UCHAR index = 0; index < desc.bNumEndpoints; ++index)
    {
        WINUSB_PIPE_INFORMATION pipe = {};
        if (!WinUsb_QueryPipe(iface, 0, index, &pipe))
        {
            return false;
        }
        if (pipe.PipeType == UsbdPipeTypeBulk && (pipe.PipeId & 0x80) == 0)
        {
            outPipe = pipe.PipeId;
            return true;
        }
    }
    return false;
}

bool openProbeHandles(
    const DeviceProfile& profile,
    WinUsbHandles& handles,
    UCHAR& outPipe,
    std::string& errorOut)
{
    if (!openZadigFallback(profile, handles, errorOut))
    {
        return false;
    }
    UCHAR ifnum = 0;
    if (!queryInterfaceNumber(handles.winUsb, ifnum, errorOut))
    {
        return false;
    }
    if (!findBulkOut(handles.winUsb, outPipe))
    {
        errorOut = "No bulk OUT on profile interface";
        return false;
    }
    std::cout << "Open OK (ifnum=" << static_cast<unsigned>(ifnum)
              << " associated=" << handles.associatedCount << " out=0x" << std::hex
              << static_cast<unsigned>(outPipe) << std::dec << ")\n";
    return true;
}

void writeFinishBestEffort(WINUSB_INTERFACE_HANDLE iface, UCHAR outPipe)
{
    BulkOutWrite write{iface, outPipe, kEmagicFinishMagic, kEmagicFinishMagicSize};
    std::string finishError;
    if (writeStaticOut(write, finishError))
    {
        std::cout << "finish magic write OK (device back to Patch)\n";
        return;
    }
    std::cout << "finish magic skipped: " << finishError << '\n';
}
} // namespace

int runMt4UsbBulkProbe(std::string& errorOut)
{
    const DeviceProfile* mt4 = findDeviceProfile(kEmagicVendorId, kMt4ProductId);
    if (mt4 == nullptr)
    {
        errorOut = "MT4 DeviceProfile not found";
        return 1;
    }

    WinUsbHandles handles;
    UCHAR outPipe = 0;
    if (!openProbeHandles(*mt4, handles, outPipe, errorOut))
    {
        closeWinUsbHandles(handles);
        return 1;
    }

    std::cout << "writing Emagic init magic...\n";
    BulkOutWrite initWrite{
        handles.winUsb, outPipe, kEmagicInitMagic, kEmagicInitMagicSize};
    if (!writeStaticOut(initWrite, errorOut))
    {
        closeWinUsbHandles(handles);
        return 1;
    }

    std::cout << "init magic write OK\n";
    writeFinishBestEffort(handles.winUsb, outPipe);
    closeWinUsbHandles(handles);
    errorOut.clear();
    return 0;
}

#else

int runMt4UsbBulkProbe(std::string& errorOut)
{
    errorOut = "--probe-usb requires Windows";
    return 1;
}

#endif
