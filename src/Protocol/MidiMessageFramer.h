// Frames raw MIDI byte spans into complete commands for host SendToHost.

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

// Hard SysEx hold cap (device→host). V1 Matrix frames (275 / 351 B) fit; do not
// grow unbounded. Oversize hits are counted — not silent drops.
inline constexpr std::size_t kMaxSysexHoldBytes = 1024;

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

    // Oversize SysEx that hit kMaxSysexHoldBytes (English-diagnosable failure path).
    std::uint64_t OversizeSysexRejectCount() const noexcept;
    std::uint64_t ConsumeOversizeSysexRejectCount() noexcept;
    bool IsHoldingSysEx() const noexcept { return inSysex_; }
    std::size_t HeldSysexSize() const noexcept { return buffer_.size(); }
    // Close an open SysEx with a trailing F7 (WinUSB first-burst may drop the
    // device's closing packet while all prior bytes are already held).
    void FinalizeHeldSysex(const MidiFramedMessageSink& sink);

private:
    void emitBuffer(const MidiFramedMessageSink& sink);
    void handleStatusByte(uint8_t status, const MidiFramedMessageSink& sink);
    void handleDataByte(uint8_t data, const MidiFramedMessageSink& sink);
    void beginChannelOrSystem(uint8_t status);
    void beginSysEx();
    void endSysEx(const MidiFramedMessageSink& sink);
    void appendSysexData(uint8_t data);
    void appendChannelData(uint8_t data, const MidiFramedMessageSink& sink);
    bool hasIncompleteOuterMessage() const noexcept;
    bool tryBeginQuarterFrameInterrupt(uint8_t status) noexcept;
    void clearQuarterFrameInterrupt() noexcept;
    void emitQuarterFrameInterrupt(uint8_t data, const MidiFramedMessageSink& sink);
    static std::size_t channelDataLength(uint8_t status) noexcept;

    std::vector<uint8_t> buffer_;
    uint8_t runningStatus_ = 0;
    std::size_t expectedLength_ = 0;
    bool inSysex_ = false;
    std::uint64_t oversizeSysexRejectCount_ = 0;

    // MTC quarter-frame (0xF1) may interrupt an incomplete channel message or an
    // open SysEx hold without aborting them. Unlike one-byte realtime (>= 0xF8),
    // the interrupt is a two-byte nested state until the data nibble arrives.
    bool interruptActive_ = false;
    uint8_t interruptBuffer_[2] = {};
};
