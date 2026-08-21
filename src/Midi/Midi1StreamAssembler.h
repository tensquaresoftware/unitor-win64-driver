// MIDI 1.0 byte-stream assembler for host→device backends that fragment SysEx.

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

// Reassembles MIDI 1.0 bytes into complete messages. SysEx (F0…F7) may span Append
// calls; channel/system-common chunks that arrive complete are forwarded as-is.
// Real-time bytes (F8–FF) emit immediately without disturbing SysEx hold.
class Midi1StreamAssembler
{
public:
    // Match DeviceSession encode / framer long-SysEx ceiling.
    static constexpr std::size_t kMaxSysexHoldBytes = 16384;

    using EmitFn = std::function<void(const uint8_t* midi, std::size_t byteCount)>;

    void Append(const uint8_t* midi, std::size_t byteCount, const EmitFn& emit);
    void Clear() noexcept;
    bool HoldingSysex() const noexcept;
    std::size_t HeldBytes() const noexcept;

private:
    void appendSysexByte(uint8_t value, const EmitFn& emit);
    // true → advance index; false → Clear() already ran, reprocess status byte.
    bool consumeSysexHoldByte(uint8_t value, const EmitFn& emit);

    std::vector<uint8_t> hold_;
    bool inSysex_ = false;
};
