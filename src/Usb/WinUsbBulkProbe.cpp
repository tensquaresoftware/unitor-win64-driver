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

bool setPipeTimeoutMs(
    WINUSB_INTERFACE_HANDLE iface,
    UCHAR pipeId,
    ULONG timeoutMs,
    std::string& errorOut)
{
    if (!WinUsb_SetPipePolicy(
            iface, pipeId, PIPE_TRANSFER_TIMEOUT, sizeof(timeoutMs), &timeoutMs))
    {
        errorOut = formatWin32Error("SetPipePolicy timeout", GetLastError());
        return false;
    }
    return true;
}

bool writeStaticOut(const BulkOutWrite& write, std::string& errorOut)
{
    if (!setPipeTimeoutMs(write.iface, write.outPipe, 3000, errorOut))
    {
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

bool findBulkPipe(
    WINUSB_INTERFACE_HANDLE iface,
    bool wantIn,
    UCHAR& pipeOut)
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
        const bool isBulk = pipe.PipeType == UsbdPipeTypeBulk;
        const bool isIn = (pipe.PipeId & 0x80) != 0;
        if (isBulk && isIn == wantIn)
        {
            pipeOut = pipe.PipeId;
            return true;
        }
    }
    return false;
}

std::size_t drainBulkIn(
    WINUSB_INTERFACE_HANDLE iface,
    UCHAR inPipe,
    std::size_t maxPackets)
{
    std::string ignored;
    (void)setPipeTimeoutMs(iface, inPipe, 200, ignored);

    std::size_t total = 0;
    UCHAR buffer[512] = {};
    for (std::size_t packet = 0; packet < maxPackets; ++packet)
    {
        ULONG transferred = 0;
        if (!WinUsb_ReadPipe(
                iface,
                inPipe,
                buffer,
                sizeof(buffer),
                &transferred,
                nullptr)
            || transferred == 0)
        {
            break;
        }
        total += transferred;
    }
    return total;
}

struct ProbeOpenResult
{
    WinUsbHandles* handles = nullptr;
    UCHAR outPipe = 0;
    UCHAR inPipe = 0;
};

bool openProbeHandles(
    const DeviceProfile& profile,
    ProbeOpenResult& result,
    std::string& errorOut)
{
    if (result.handles == nullptr)
    {
        errorOut = "Probe open result handles pointer is null";
        return false;
    }
    if (!openZadigFallback(profile, *result.handles, errorOut))
    {
        return false;
    }
    UCHAR ifnum = 0;
    if (!queryInterfaceNumber(result.handles->winUsb, ifnum, errorOut))
    {
        return false;
    }
    if (!findBulkPipe(result.handles->winUsb, false, result.outPipe)
        || !findBulkPipe(result.handles->winUsb, true, result.inPipe))
    {
        errorOut = "No bulk IN/OUT pair on profile interface";
        return false;
    }
    std::cout << "Open OK (ifnum=" << static_cast<unsigned>(ifnum)
              << " associated=" << result.handles->associatedCount << " out=0x"
              << std::hex << static_cast<unsigned>(result.outPipe) << " in=0x"
              << static_cast<unsigned>(result.inPipe) << std::dec << ")\n";
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
    ProbeOpenResult opened;
    opened.handles = &handles;
    if (!openProbeHandles(*mt4, opened, errorOut))
    {
        closeWinUsbHandles(handles);
        return 1;
    }

    if (!prepareEmagicBulkPipes(
            handles.winUsb, opened.outPipe, opened.inPipe, errorOut))
    {
        closeWinUsbHandles(handles);
        return 1;
    }

    std::cout << "writing Emagic init magic...\n";
    BulkOutWrite initWrite{
        handles.winUsb, opened.outPipe, kEmagicInitMagic, kEmagicInitMagicSize};
    if (!writeStaticOut(initWrite, errorOut))
    {
        closeWinUsbHandles(handles);
        return 1;
    }

    std::cout << "init magic write OK\n";
    const std::size_t drained = drainBulkIn(handles.winUsb, opened.inPipe, 8);
    std::cout << "drained " << drained << " bulk IN byte(s) after init\n";

    writeFinishBestEffort(handles.winUsb, opened.outPipe);
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
