// MTC quarter-frame / full-frame vectors for MidiMessageFramer smoke.

#include "App/FramerSmoke.h"

#include "Protocol/MidiMessageFramer.h"

#include <cstdint>
#include <iostream>
#include <vector>

namespace
{
constexpr uint8_t kNoteOnStatus = 0x90;
constexpr uint8_t kMiddleC = 0x3C;
constexpr uint8_t kNoteVelocity = 0x40;
constexpr uint8_t kTimingClock = 0xF8;
constexpr uint8_t kMtcQuarterFrame = 0xF1;
constexpr uint8_t kSysexStart = 0xF0;
constexpr uint8_t kSysexEnd = 0xF7;
constexpr uint8_t kSysexManufacturer = 0x7D;
constexpr uint8_t kSysexPayload = 0x10;
constexpr uint8_t kMtcFullFrameDeviceId = 0x7F;
constexpr uint8_t kMtcFullFrameSubId1 = 0x01;
constexpr uint8_t kMtcFullFrameSubId2 = 0x01;

using MessageList = std::vector<std::vector<uint8_t>>;

struct FullFrameTime
{
    uint8_t hours = 0;
    uint8_t minutes = 0;
    uint8_t seconds = 0;
    uint8_t frames = 0;
};

MidiFramedMessageSink makeCollector(MessageList& messages)
{
    return [&](const uint8_t* midi, std::size_t n) {
        messages.emplace_back(midi, midi + n);
    };
}

bool expectSingleByte(const std::vector<uint8_t>& message, uint8_t status)
{
    return message.size() == 1 && message[0] == status;
}

bool expectQuarterFrame(const std::vector<uint8_t>& message, uint8_t dataNibble)
{
    return message.size() == 2 && message[0] == kMtcQuarterFrame && message[1] == dataNibble;
}

bool expectFullFrame(const std::vector<uint8_t>& message, const FullFrameTime& time)
{
    const uint8_t expected[] = {
        kSysexStart,
        kMtcFullFrameDeviceId,
        kMtcFullFrameDeviceId,
        kMtcFullFrameSubId1,
        kMtcFullFrameSubId2,
        time.hours,
        time.minutes,
        time.seconds,
        time.frames,
        kSysexEnd};
    if (message.size() != sizeof(expected))
    {
        return false;
    }
    for (std::size_t index = 0; index < sizeof(expected); ++index)
    {
        if (message[index] != expected[index])
        {
            return false;
        }
    }
    return true;
}

bool testFramerLoneQuarterFrame()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t span[] = {kMtcQuarterFrame, 0x01};
    framer.Push(span, sizeof(span), sink);
    if (messages.size() != 1 || !expectQuarterFrame(messages[0], 0x01))
    {
        std::cerr << "Framer failed to emit lone MTC quarter-frame\n";
        return false;
    }
    return true;
}

bool testFramerEightQuarterFrameSequence()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    uint8_t span[16] = {};
    for (uint8_t type = 0; type < 8; ++type)
    {
        span[static_cast<std::size_t>(type) * 2U] = kMtcQuarterFrame;
        span[static_cast<std::size_t>(type) * 2U + 1U] =
            static_cast<uint8_t>(static_cast<uint8_t>(type << 4) | 0x05);
    }
    framer.Push(span, sizeof(span), sink);
    if (messages.size() != 8)
    {
        std::cerr << "Framer failed to emit eight MTC quarter-frames\n";
        return false;
    }
    for (uint8_t type = 0; type < 8; ++type)
    {
        const uint8_t expectedData =
            static_cast<uint8_t>(static_cast<uint8_t>(type << 4) | 0x05);
        if (!expectQuarterFrame(messages[type], expectedData))
        {
            std::cerr << "Framer MTC quarter-frame sequence byte mismatch\n";
            return false;
        }
    }
    return true;
}

bool testFramerFullFrameSysex()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const FullFrameTime time{0x20, 0x15, 0x30, 0x10};
    const uint8_t span[] = {
        kSysexStart,
        kMtcFullFrameDeviceId,
        kMtcFullFrameDeviceId,
        kMtcFullFrameSubId1,
        kMtcFullFrameSubId2,
        time.hours,
        time.minutes,
        time.seconds,
        time.frames,
        kSysexEnd};
    framer.Push(span, sizeof(span), sink);
    if (messages.size() != 1 || !expectFullFrame(messages[0], time))
    {
        std::cerr << "Framer failed to emit MTC full-frame SysEx\n";
        return false;
    }
    return true;
}

bool testFramerQuarterFrameInterleavedMidNote()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t span[] = {
        kNoteOnStatus,
        kMtcQuarterFrame,
        0x23,
        kMiddleC,
        kNoteVelocity};
    framer.Push(span, sizeof(span), sink);
    if (messages.size() != 2)
    {
        std::cerr << "Framer mid-note quarter-frame interleave message count failed\n";
        return false;
    }
    if (!expectQuarterFrame(messages[0], 0x23))
    {
        std::cerr << "Framer mid-note quarter-frame was not emitted first\n";
        return false;
    }
    if (messages[1].size() != 3 || messages[1][0] != kNoteOnStatus
        || messages[1][1] != kMiddleC || messages[1][2] != kNoteVelocity)
    {
        std::cerr << "Framer mid-note quarter-frame corrupted the note\n";
        return false;
    }
    return true;
}

bool matchSysexWithoutQuarterFrame(const std::vector<uint8_t>& sysex)
{
    const uint8_t expected[] = {
        kSysexStart,
        kSysexManufacturer,
        kSysexPayload,
        kSysexEnd};
    if (sysex.size() != sizeof(expected))
    {
        return false;
    }
    for (std::size_t index = 0; index < sizeof(expected); ++index)
    {
        if (sysex[index] != expected[index])
        {
            return false;
        }
    }
    return true;
}

bool testFramerQuarterFrameDuringSysexHold()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t open[] = {
        kSysexStart,
        kSysexManufacturer,
        kMtcQuarterFrame,
        0x45,
        kSysexPayload};
    framer.Push(open, sizeof(open), sink);
    if (messages.size() != 1 || !expectQuarterFrame(messages[0], 0x45))
    {
        std::cerr << "Framer failed to emit quarter-frame during open SysEx\n";
        return false;
    }

    const uint8_t close[] = {kSysexEnd};
    framer.Push(close, sizeof(close), sink);
    if (messages.size() != 2 || !matchSysexWithoutQuarterFrame(messages[1]))
    {
        std::cerr << "Framer aborted or corrupted SysEx after quarter-frame interleave\n";
        return false;
    }
    return true;
}

bool testFramerFullFrameWithTimingClock()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const FullFrameTime time{0x10, 0x20, 0x30, 0x05};
    const uint8_t span[] = {
        kSysexStart,
        kMtcFullFrameDeviceId,
        kMtcFullFrameDeviceId,
        kMtcFullFrameSubId1,
        kMtcFullFrameSubId2,
        kTimingClock,
        time.hours,
        time.minutes,
        time.seconds,
        time.frames,
        kSysexEnd,
        kTimingClock};
    framer.Push(span, sizeof(span), sink);
    if (messages.size() != 3)
    {
        std::cerr << "Framer full-frame + Timing Clock message count failed\n";
        return false;
    }
    if (!expectSingleByte(messages[0], kTimingClock)
        || !expectFullFrame(messages[1], time)
        || !expectSingleByte(messages[2], kTimingClock))
    {
        std::cerr << "Framer corrupted full-frame around Timing Clock\n";
        return false;
    }
    return true;
}

bool testFramerQuarterFrameDuringRunningStatusNote()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t first[] = {kNoteOnStatus, kMiddleC, kNoteVelocity};
    framer.Push(first, sizeof(first), sink);

    // Next note under running status: data, then QF interrupt, then velocity.
    const uint8_t second[] = {
        kMiddleC,
        kMtcQuarterFrame,
        0x23,
        static_cast<uint8_t>(0x00)};
    framer.Push(second, sizeof(second), sink);

    if (messages.size() != 3)
    {
        std::cerr << "Framer running-status + quarter-frame message count failed\n";
        return false;
    }
    if (!expectQuarterFrame(messages[1], 0x23))
    {
        std::cerr << "Framer failed to emit quarter-frame during running-status note\n";
        return false;
    }
    if (messages[2].size() != 3 || messages[2][0] != kNoteOnStatus
        || messages[2][1] != kMiddleC || messages[2][2] != 0x00)
    {
        std::cerr << "Framer corrupted running-status note around quarter-frame\n";
        return false;
    }
    return true;
}

bool testFramerRealtimeBetweenQuarterFrameBytes()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t span[] = {
        kNoteOnStatus,
        kMtcQuarterFrame,
        kTimingClock,
        0x23,
        kMiddleC,
        kNoteVelocity};
    framer.Push(span, sizeof(span), sink);
    if (messages.size() != 3)
    {
        std::cerr << "Framer clock-between-quarter-frame message count failed\n";
        return false;
    }
    if (!expectSingleByte(messages[0], kTimingClock)
        || !expectQuarterFrame(messages[1], 0x23))
    {
        std::cerr << "Framer failed clock + quarter-frame interleave order\n";
        return false;
    }
    if (messages[2].size() != 3 || messages[2][0] != kNoteOnStatus
        || messages[2][1] != kMiddleC || messages[2][2] != kNoteVelocity)
    {
        std::cerr << "Framer corrupted note around clock-between-quarter-frame\n";
        return false;
    }
    return true;
}

bool testFramerRepeatedQuarterFrameReplaces()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t span[] = {kMtcQuarterFrame, kMtcQuarterFrame, 0x67};
    framer.Push(span, sizeof(span), sink);
    if (messages.size() != 1 || !expectQuarterFrame(messages[0], 0x67))
    {
        std::cerr << "Framer nested incomplete quarter-frame should replace, not double-emit\n";
        return false;
    }
    return true;
}

using FramerTestFn = bool (*)();
} // namespace

bool runFramerMtcTests()
{
    const FramerTestFn tests[] = {
        testFramerLoneQuarterFrame,
        testFramerEightQuarterFrameSequence,
        testFramerFullFrameSysex,
        testFramerQuarterFrameInterleavedMidNote,
        testFramerQuarterFrameDuringSysexHold,
        testFramerFullFrameWithTimingClock,
        testFramerQuarterFrameDuringRunningStatusNote,
        testFramerRealtimeBetweenQuarterFrameBytes,
        testFramerRepeatedQuarterFrameReplaces};
    for (FramerTestFn test : tests)
    {
        if (!test())
        {
            return false;
        }
    }
    return true;
}
