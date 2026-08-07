// Host MIDI bulk OUT — Linux snd_usbmidi_emagic_output (≤ maxpacket + trailing 0xFF).

#include "Usb/WinUsbTransport.h"

#include <algorithm>
#include <cstring>

#ifdef _WIN32

bool WinUsbTransport::WriteEmagicHostMidi(
    const uint8_t* data,
    std::size_t size,
    std::string& errorOut,
    BetweenChunksFn betweenChunks,
    void* betweenChunksCtx)
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

    const std::size_t maxPacket = BulkOutMaxPacketSize();
    if (maxPacket < 2 || maxPacket > 512)
    {
        errorOut = "WinUSB Emagic bulk OUT max packet size is invalid";
        return false;
    }

    const std::size_t payload = size - ((data[size - 1] == 0xFF) ? 1u : 0u);
    if (payload == 0)
    {
        errorOut = "WinUSB WriteEmagicHostMidi payload is empty after stripping pad";
        return false;
    }

    uint8_t packet[512] = {};
    for (std::size_t offset = 0; offset < payload;)
    {
        const std::size_t chunk = (std::min)(maxPacket - 1, payload - offset);
        std::memcpy(packet, data + offset, chunk);
        packet[chunk] = 0xFF;
        if (!WriteBulk(packet, chunk + 1, errorOut))
        {
            return false;
        }
        offset += chunk;
        if (betweenChunks != nullptr && offset < payload)
        {
            betweenChunks(betweenChunksCtx);
        }
    }
    errorOut.clear();
    return true;
}

#else // !_WIN32

bool WinUsbTransport::WriteEmagicHostMidi(
    const uint8_t* /*data*/,
    std::size_t /*size*/,
    std::string& errorOut,
    BetweenChunksFn /*betweenChunks*/,
    void* /*betweenChunksCtx*/)
{
    errorOut = "WinUSB WriteEmagicHostMidi requires Windows";
    return false;
}

#endif // _WIN32
