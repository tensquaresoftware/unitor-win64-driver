#include "Protocol/MidiMessageFramer.h"

namespace
{
constexpr std::size_t kMaxSysexHoldBytes = 1024;
constexpr uint8_t kMtcQuarterFrame = 0xF1;

bool isRealtimeStatus(uint8_t status) noexcept
{
    return status >= 0xF8;
}

bool isSystemExclusiveStart(uint8_t status) noexcept
{
    return status == 0xF0;
}

bool isSystemExclusiveEnd(uint8_t status) noexcept
{
    return status == 0xF7;
}
} // namespace

void MidiMessageFramer::Reset() noexcept
{
    buffer_.clear();
    runningStatus_ = 0;
    expectedLength_ = 0;
    inSysex_ = false;
    interruptActive_ = false;
}

void MidiMessageFramer::emitBuffer(const MidiFramedMessageSink& sink)
{
    if (buffer_.empty() || !sink)
    {
        return;
    }
    sink(buffer_.data(), buffer_.size());
    buffer_.clear();
    expectedLength_ = 0;
}

std::size_t MidiMessageFramer::channelDataLength(uint8_t status) noexcept
{
    if (status >= 0xF0)
    {
        if (status == 0xF1 || status == 0xF3)
        {
            return 2; // status + 1 data
        }
        if (status == 0xF2)
        {
            return 3; // status + 2 data
        }
        if (status == 0xF6)
        {
            return 1; // status only
        }
        return 0;
    }

    const uint8_t high = static_cast<uint8_t>(status & 0xF0);
    if (high == 0xC0 || high == 0xD0)
    {
        return 2; // status + 1 data
    }
    return 3; // status + 2 data
}

void MidiMessageFramer::beginChannelOrSystem(uint8_t status)
{
    const std::size_t length = channelDataLength(status);
    if (length == 0)
    {
        // Undefined / unsupported system-common status: drop and resync.
        buffer_.clear();
        expectedLength_ = 0;
        inSysex_ = false;
        return;
    }

    buffer_.clear();
    buffer_.push_back(status);
    inSysex_ = false;
    expectedLength_ = length;
    if (status < 0xF0)
    {
        runningStatus_ = status;
    }
    else
    {
        runningStatus_ = 0;
    }
}

bool MidiMessageFramer::hasIncompleteOuterMessage() const noexcept
{
    return inSysex_ || (expectedLength_ > 0 && buffer_.size() < expectedLength_);
}

bool MidiMessageFramer::tryBeginQuarterFrameInterrupt(uint8_t status) noexcept
{
    if (status != kMtcQuarterFrame || !hasIncompleteOuterMessage())
    {
        return false;
    }
    // Incomplete outer quarter-frame: replace via beginChannelOrSystem — do not nest.
    if (!inSysex_ && !buffer_.empty() && buffer_[0] == kMtcQuarterFrame)
    {
        return false;
    }
    interruptActive_ = true;
    interruptBuffer_[0] = status;
    return true;
}

void MidiMessageFramer::clearQuarterFrameInterrupt() noexcept
{
    interruptActive_ = false;
}

void MidiMessageFramer::emitQuarterFrameInterrupt(
    uint8_t data,
    const MidiFramedMessageSink& sink)
{
    interruptBuffer_[1] = data;
    if (sink)
    {
        sink(interruptBuffer_, 2);
    }
    clearQuarterFrameInterrupt();
}

void MidiMessageFramer::beginSysEx()
{
    buffer_.clear();
    buffer_.push_back(0xF0);
    inSysex_ = true;
    expectedLength_ = 0;
    runningStatus_ = 0;
}

void MidiMessageFramer::endSysEx(const MidiFramedMessageSink& sink)
{
    if (!inSysex_)
    {
        return;
    }
    buffer_.push_back(0xF7);
    emitBuffer(sink);
    inSysex_ = false;
}

void MidiMessageFramer::handleStatusByte(uint8_t status, const MidiFramedMessageSink& sink)
{
    if (isRealtimeStatus(status))
    {
        const uint8_t realtime = status;
        if (sink)
        {
            sink(&realtime, 1);
        }
        return;
    }

    // Quarter-frame during open SysEx or an incomplete channel/system message:
    // emit as a nested 2-byte command without aborting the outer assembly.
    if (tryBeginQuarterFrameInterrupt(status))
    {
        return;
    }

    if (interruptActive_)
    {
        clearQuarterFrameInterrupt();
    }

    if (isSystemExclusiveStart(status))
    {
        beginSysEx();
        return;
    }

    if (isSystemExclusiveEnd(status))
    {
        endSysEx(sink);
        return;
    }

    beginChannelOrSystem(status);
    if (expectedLength_ == 1)
    {
        emitBuffer(sink);
    }
}

void MidiMessageFramer::appendSysexData(uint8_t data)
{
    if (buffer_.size() >= kMaxSysexHoldBytes)
    {
        Reset();
        return;
    }
    buffer_.push_back(data);
}

void MidiMessageFramer::appendChannelData(uint8_t data, const MidiFramedMessageSink& sink)
{
    if (buffer_.empty())
    {
        if (runningStatus_ == 0)
        {
            return;
        }
        beginChannelOrSystem(runningStatus_);
        buffer_.push_back(data);
    }
    else
    {
        buffer_.push_back(data);
    }

    if (expectedLength_ > 0 && buffer_.size() >= expectedLength_)
    {
        emitBuffer(sink);
    }
}

void MidiMessageFramer::handleDataByte(uint8_t data, const MidiFramedMessageSink& sink)
{
    if (interruptActive_)
    {
        emitQuarterFrameInterrupt(data, sink);
        return;
    }

    if (inSysex_)
    {
        appendSysexData(data);
        return;
    }

    appendChannelData(data, sink);
}

void MidiMessageFramer::Push(
    const uint8_t* bytes,
    std::size_t byteCount,
    const MidiFramedMessageSink& sink)
{
    if (bytes == nullptr || byteCount == 0)
    {
        return;
    }

    for (std::size_t index = 0; index < byteCount; ++index)
    {
        const uint8_t value = bytes[index];
        if (value & 0x80)
        {
            handleStatusByte(value, sink);
        }
        else
        {
            handleDataByte(value, sink);
        }
    }
}
