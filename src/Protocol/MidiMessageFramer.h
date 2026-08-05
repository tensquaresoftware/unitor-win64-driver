// Frames raw MIDI byte spans into complete commands for host SendToHost.

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

using MidiFramedMessageSink =
    std::function<void(const uint8_t* midiBytes, std::size_t byteCount)>;

class MidiMessageFramer
{
public:
    void Reset() noexcept;
    void Push(
        const uint8_t* bytes,
        std::size_t byteCount,
        const MidiFramedMessageSink& sink);

private:
    void emitBuffer(const MidiFramedMessageSink& sink);
    void handleStatusByte(uint8_t status, const MidiFramedMessageSink& sink);
    void handleDataByte(uint8_t data, const MidiFramedMessageSink& sink);
    void beginChannelOrSystem(uint8_t status);
    static std::size_t channelDataLength(uint8_t status) noexcept;

    std::vector<uint8_t> buffer_;
    uint8_t runningStatus_ = 0;
    std::size_t expectedLength_ = 0;
    bool inSysex_ = false;
};
