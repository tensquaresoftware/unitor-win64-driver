#include "Midi/Midi1StreamAssembler.h"

void Midi1StreamAssembler::Clear() noexcept
{
    hold_.clear();
    inSysex_ = false;
}

bool Midi1StreamAssembler::HoldingSysex() const noexcept
{
    return inSysex_;
}

std::size_t Midi1StreamAssembler::HeldBytes() const noexcept
{
    return hold_.size();
}

void Midi1StreamAssembler::appendSysexByte(uint8_t value, const EmitFn& emit)
{
    hold_.push_back(value);
    if (hold_.size() > kMaxSysexHoldBytes)
    {
        Clear();
        return;
    }
    if (value == 0xF7)
    {
        emit(hold_.data(), hold_.size());
        Clear();
    }
}

bool Midi1StreamAssembler::consumeSysexHoldByte(uint8_t value, const EmitFn& emit)
{
    // Interrupted SysEx: channel/system-common aborts hold (realtime already handled).
    if (value >= 0x80 && value < 0xF8)
    {
        Clear();
        return false;
    }
    appendSysexByte(value, emit);
    return true;
}

void Midi1StreamAssembler::Append(
    const uint8_t* midi,
    std::size_t byteCount,
    const EmitFn& emit)
{
    if (midi == nullptr || byteCount == 0 || !emit)
    {
        return;
    }

    std::size_t index = 0;
    while (index < byteCount)
    {
        const uint8_t value = midi[index];
        if (value >= 0xF8)
        {
            emit(&midi[index], 1);
            ++index;
            continue;
        }

        if (inSysex_)
        {
            if (consumeSysexHoldByte(value, emit))
            {
                ++index;
            }
            continue;
        }

        if (value == 0xF0)
        {
            inSysex_ = true;
            hold_.clear();
            hold_.push_back(value);
            ++index;
            continue;
        }

        // Non-SysEx: backends that fragment only SysEx typically deliver the rest
        // as one complete message per Append — forward the remainder once.
        emit(midi + index, byteCount - index);
        return;
    }
}
