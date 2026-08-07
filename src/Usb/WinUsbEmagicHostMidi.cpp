// Host MIDI bulk OUT — Linux snd_usbmidi_emagic_output (≤ maxpacket + trailing 0xFF).

#include "Usb/WinUsbTransport.h"

#include <algorithm>
#include <cstring>

#ifdef _WIN32

#include "Usb/WinUsbOpenSupport.h"

#include <windows.h>
#include <winusb.h>

namespace
{
constexpr std::size_t kEmagicOutChunkCap = 32;
constexpr DWORD kOutWaitSliceMs = 1;

struct EmagicOutChunkWrite
{
    WINUSB_INTERFACE_HANDLE winUsb = nullptr;
    UCHAR pipeId = 0;
    uint8_t* packet = nullptr;
    std::size_t packetSize = 0;
    const WinUsbTransport::EmagicBetweenChunks* betweenChunks = nullptr;
};

std::size_t resolveEmagicOutMaxPacket(std::size_t endpointMax, std::string& errorOut)
{
    if (endpointMax < 2 || endpointMax > 512)
    {
        errorOut = "WinUSB Emagic bulk OUT max packet size is invalid";
        return 0;
    }
    // Clamp to full-speed Emagic packet size. Some descriptors advertise 512 B;
    // huge OUT URBs starve between-chunk IN demux during DIN-rate loopback.
    return (std::min)(endpointMax, kEmagicOutChunkCap);
}

void invokeBetweenChunks(const WinUsbTransport::EmagicBetweenChunks* betweenChunks)
{
    if (betweenChunks != nullptr && betweenChunks->fn != nullptr)
    {
        betweenChunks->fn(betweenChunks->ctx);
    }
}

bool waitEmagicOutOverlapped(
    WINUSB_INTERFACE_HANDLE winUsb,
    OVERLAPPED& overlapped,
    const WinUsbTransport::EmagicBetweenChunks* betweenChunks,
    std::string& errorOut)
{
    for (;;)
    {
        // Demux IN while OUT is in flight — sync WritePipe starved completions
        // under DIN-rate full duplex (lab: 128 B gaps on 4096).
        invokeBetweenChunks(betweenChunks);
        const DWORD wait = WaitForSingleObject(overlapped.hEvent, kOutWaitSliceMs);
        if (wait == WAIT_OBJECT_0)
        {
            return true;
        }
        if (wait != WAIT_TIMEOUT)
        {
            errorOut = formatWin32Error(
                "WaitForSingleObject failed for Emagic bulk OUT", GetLastError());
            return false;
        }
    }
}

bool finishEmagicOutOverlapped(
    WINUSB_INTERFACE_HANDLE winUsb,
    OVERLAPPED& overlapped,
    std::size_t packetSize,
    std::string& errorOut)
{
    ULONG transferred = 0;
    if (!WinUsb_GetOverlappedResult(winUsb, &overlapped, &transferred, FALSE))
    {
        errorOut = formatWin32Error(
            "WinUsb_GetOverlappedResult failed for Emagic bulk OUT", GetLastError());
        return false;
    }
    if (transferred != packetSize)
    {
        errorOut = "WinUsb_WritePipe short write for Emagic bulk OUT";
        return false;
    }
    return true;
}

bool writeEmagicOutChunkOverlapped(const EmagicOutChunkWrite& write, std::string& errorOut)
{
    HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (event == nullptr)
    {
        errorOut = formatWin32Error(
            "CreateEventW failed for Emagic bulk OUT", GetLastError());
        return false;
    }

    OVERLAPPED overlapped = {};
    overlapped.hEvent = event;
    ULONG transferred = 0;
    const BOOL ok = WinUsb_WritePipe(
        write.winUsb,
        write.pipeId,
        write.packet,
        static_cast<ULONG>(write.packetSize),
        &transferred,
        &overlapped);
    const DWORD startError = ok ? ERROR_SUCCESS : GetLastError();
    if (!ok && startError != ERROR_IO_PENDING)
    {
        CloseHandle(event);
        errorOut = formatWin32Error(
            "WinUsb_WritePipe failed for Emagic bulk OUT", startError);
        return false;
    }

    const bool waited =
        waitEmagicOutOverlapped(write.winUsb, overlapped, write.betweenChunks, errorOut);
    const bool finished =
        waited && finishEmagicOutOverlapped(
                      write.winUsb, overlapped, write.packetSize, errorOut);
    CloseHandle(event);
    return finished;
}

struct EmagicHostMidiChunkJob
{
    WINUSB_INTERFACE_HANDLE winUsb = nullptr;
    UCHAR pipeId = 0;
    const uint8_t* data = nullptr;
    std::size_t payload = 0;
    std::size_t maxPacket = 0;
    const WinUsbTransport::EmagicBetweenChunks* betweenChunks = nullptr;
};

bool writeEmagicHostMidiChunks(const EmagicHostMidiChunkJob& job, std::string& errorOut)
{
    uint8_t packet[512] = {};
    for (std::size_t offset = 0; offset < job.payload;)
    {
        const std::size_t chunk = (std::min)(job.maxPacket - 1, job.payload - offset);
        std::memcpy(packet, job.data + offset, chunk);
        packet[chunk] = 0xFF;
        EmagicOutChunkWrite write;
        write.winUsb = job.winUsb;
        write.pipeId = job.pipeId;
        write.packet = packet;
        write.packetSize = chunk + 1;
        write.betweenChunks = job.betweenChunks;
        if (!writeEmagicOutChunkOverlapped(write, errorOut))
        {
            return false;
        }
        offset += chunk;
    }
    invokeBetweenChunks(job.betweenChunks);
    return true;
}
} // namespace

bool WinUsbTransport::WriteEmagicHostMidi(
    const uint8_t* data,
    std::size_t size,
    std::string& errorOut,
    const EmagicBetweenChunks* betweenChunks)
{
    if (!IsOpen())
    {
        errorOut = "WinUSB WriteEmagicHostMidi requires an open transport with bulk pipes";
        return false;
    }
    if (data == nullptr && size != 0)
    {
        errorOut = "WinUSB WriteEmagicHostMidi data pointer is null";
        return false;
    }
    if (size == 0)
    {
        errorOut.clear();
        return true;
    }

    const std::size_t maxPacket = resolveEmagicOutMaxPacket(BulkOutMaxPacketSize(), errorOut);
    if (maxPacket == 0)
    {
        return false;
    }

    const std::size_t payload = size - ((data[size - 1] == 0xFF) ? 1u : 0u);
    if (payload == 0)
    {
        errorOut = "WinUSB WriteEmagicHostMidi payload is empty after stripping pad";
        return false;
    }

    EmagicHostMidiChunkJob job;
    job.winUsb = static_cast<WINUSB_INTERFACE_HANDLE>(winUsbHandle_);
    job.pipeId = bulkOutPipeId_;
    job.data = data;
    job.payload = payload;
    job.maxPacket = maxPacket;
    job.betweenChunks = betweenChunks;
    if (!writeEmagicHostMidiChunks(job, errorOut))
    {
        return false;
    }
    errorOut.clear();
    return true;
}

#else // !_WIN32

bool WinUsbTransport::WriteEmagicHostMidi(
    const uint8_t* /*data*/,
    std::size_t /*size*/,
    std::string& errorOut,
    const EmagicBetweenChunks* /*betweenChunks*/)
{
    errorOut = "WinUSB WriteEmagicHostMidi requires Windows";
    return false;
}

#endif // _WIN32
