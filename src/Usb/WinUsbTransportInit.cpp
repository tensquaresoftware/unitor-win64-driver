// Emagic session wake: init drain + Set Computer Mode (WinUSB bulk).

#include "Usb/WinUsbTransport.h"

#ifdef _WIN32

#include "Protocol/EmagicCableMapper.h"
#include "Usb/WinUsbOpenSupport.h"

namespace
{
constexpr std::uint32_t kSessionBulkTimeoutMs = 3000;
constexpr std::uint32_t kDrainInTimeoutMs = 200;
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
