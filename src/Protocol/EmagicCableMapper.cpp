#include "Protocol/EmagicCableMapper.h"

namespace
{
bool maskHasProductCable(uint16_t cableMask, uint8_t cableIndex) noexcept
{
    // Emagic wire uses 4-bit cable ids (0..15); reject before shift to avoid UB.
    if (cableIndex >= 16 || cableIndex == kEmagicBroadcastCableIndex)
    {
        return false;
    }

    const uint16_t bit = static_cast<uint16_t>(1u << cableIndex);
    return (cableMask & bit) != 0;
}

std::size_t truncateAtEndMarker(const uint8_t* bulkBytes, std::size_t bulkSize) noexcept
{
    for (std::size_t index = 0; index < bulkSize; ++index)
    {
        if (bulkBytes[index] == kEmagicEndOfValidData)
        {
            return index;
        }
    }
    return bulkSize;
}

std::size_t findPortSwitchOffset(const uint8_t* bytes, std::size_t size) noexcept
{
    for (std::size_t index = 0; index < size; ++index)
    {
        if (bytes[index] == kEmagicPortSwitch)
        {
            return index;
        }
    }
    return size;
}
} // namespace

EmagicCableMapper::EmagicCableMapper(const DeviceProfile& profile)
    : profile_(profile)
{
}

bool EmagicCableMapper::IsProductOutCable(uint8_t cableIndex) const noexcept
{
    return maskHasProductCable(profile_.outCables, cableIndex);
}

bool EmagicCableMapper::IsProductInCable(uint8_t cableIndex) const noexcept
{
    return maskHasProductCable(profile_.inCables, cableIndex);
}

void EmagicCableMapper::ResetInputState() noexcept
{
    currentInCable_ = 0;
    seenF5_ = false;
}

bool EmagicCableMapper::appendPortSwitch(
    uint8_t cableIndex,
    EncodeBuffer& buffer,
    std::string& errorOut)
{
    if (buffer.size + 2 > buffer.capacity)
    {
        errorOut = "Encode buffer too small for Emagic port switch";
        return false;
    }

    buffer.bytes[buffer.size++] = kEmagicPortSwitch;
    buffer.bytes[buffer.size++] = static_cast<uint8_t>((cableIndex + 1) & 15);
    currentOutCable_ = cableIndex;
    return true;
}

bool EmagicCableMapper::appendMidiBytes(
    const EncodeRequest& request,
    EncodeBuffer& buffer,
    std::string& errorOut)
{
    if (buffer.size + request.midiSize > buffer.capacity)
    {
        errorOut = "Encode buffer too small for MIDI payload";
        return false;
    }

    for (std::size_t index = 0; index < request.midiSize; ++index)
    {
        buffer.bytes[buffer.size++] = request.midiBytes[index];
    }
    return true;
}

void EmagicCableMapper::appendTrailingPad(EncodeBuffer& buffer) noexcept
{
    if (buffer.size == 0 || buffer.size >= buffer.capacity)
    {
        return;
    }

    // Linux snd_usbmidi_emagic_output: one trailing 0xFF, then a short URB
    // (transfer length = data + 1). Do not fill to wMaxPacketSize — lab showed
    // full-packet pad did not fix Device Inquiry Identity loss (~45 % still).
    buffer.bytes[buffer.size++] = kEmagicEndOfValidData;
}

bool EmagicCableMapper::EncodeToDevice(
    const EncodeRequest& request,
    EncodeBuffer& buffer,
    std::string& errorOut)
{
    buffer.size = 0;

    if (buffer.bytes == nullptr || buffer.capacity == 0)
    {
        errorOut = "Encode buffer is missing";
        return false;
    }
    if (request.midiBytes == nullptr && request.midiSize != 0)
    {
        errorOut = "MIDI payload pointer is null";
        return false;
    }
    if (!IsProductOutCable(request.cableIndex))
    {
        errorOut = "Cable index is not a product OUT port for this DeviceProfile";
        return false;
    }

    const uint8_t previousOutCable = currentOutCable_;
    if (request.cableIndex != currentOutCable_)
    {
        if (!appendPortSwitch(request.cableIndex, buffer, errorOut))
        {
            return false;
        }
    }

    if (!appendMidiBytes(request, buffer, errorOut))
    {
        // Port switch may have updated mapper state before MIDI copy failed.
        currentOutCable_ = previousOutCable;
        return false;
    }

    appendTrailingPad(buffer);
    errorOut.clear();
    return true;
}

bool EmagicCableMapper::consumePendingPortSwitch(
    const uint8_t*& cursor,
    std::size_t& remaining) noexcept
{
    if (remaining == 0)
    {
        return true;
    }

    if (cursor[0] < 0x80)
    {
        currentInCable_ = static_cast<uint8_t>((cursor[0] - 1) & 15);
        ++cursor;
        --remaining;
    }

    seenF5_ = false;
    return true;
}

bool EmagicCableMapper::demuxUntilPortSwitch(
    const uint8_t*& cursor,
    std::size_t& remaining,
    const MidiCableSink& sink)
{
    const std::size_t midiLength = findPortSwitchOffset(cursor, remaining);
    if (midiLength > 0 && sink && IsProductInCable(currentInCable_))
    {
        sink(currentInCable_, cursor, midiLength);
    }

    cursor += midiLength;
    remaining -= midiLength;
    if (remaining == 0)
    {
        return true;
    }

    // cursor[0] == F5
    seenF5_ = true;
    ++cursor;
    --remaining;
    return consumePendingPortSwitch(cursor, remaining);
}

bool EmagicCableMapper::DecodeFromDevice(
    const uint8_t* bulkBytes,
    std::size_t bulkSize,
    const MidiCableSink& sink,
    std::string& errorOut)
{
    if (bulkBytes == nullptr && bulkSize != 0)
    {
        errorOut = "Bulk IN pointer is null";
        return false;
    }

    std::size_t remaining = truncateAtEndMarker(bulkBytes, bulkSize);
    const uint8_t* cursor = bulkBytes;

    if (seenF5_)
    {
        if (!consumePendingPortSwitch(cursor, remaining))
        {
            errorOut = "Failed to complete split F5 port switch";
            return false;
        }
    }

    while (remaining > 0)
    {
        if (!demuxUntilPortSwitch(cursor, remaining, sink))
        {
            errorOut = "Failed to demultiplex Emagic bulk IN";
            return false;
        }
    }

    errorOut.clear();
    return true;
}
