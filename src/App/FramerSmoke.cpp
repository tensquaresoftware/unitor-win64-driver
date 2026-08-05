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
constexpr uint8_t kControlChangeStatus = 0xB0;
constexpr uint8_t kCcModulation = 0x01;
constexpr uint8_t kCcValue = 0x40;
constexpr uint8_t kTimingClock = 0xF8;
constexpr uint8_t kStart = 0xFA;
constexpr uint8_t kContinue = 0xFB;
constexpr uint8_t kStop = 0xFC;
constexpr uint8_t kSysexStart = 0xF0;
constexpr uint8_t kSysexEnd = 0xF7;
constexpr uint8_t kSysexManufacturer = 0x7D;
constexpr uint8_t kSysexPayload = 0x10;

using MessageList = std::vector<std::vector<uint8_t>>;

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

bool testFramerPartialNoteAcrossPushes()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t first[] = {kNoteOnStatus, kMiddleC};
    const uint8_t second[] = {kNoteVelocity};
    framer.Push(first, sizeof(first), sink);
    if (!messages.empty())
    {
        std::cerr << "Framer emitted before note was complete\n";
        return false;
    }
    framer.Push(second, sizeof(second), sink);
    if (messages.size() != 1 || messages[0].size() != 3)
    {
        std::cerr << "Framer failed to emit one complete note\n";
        return false;
    }
    return messages[0][0] == kNoteOnStatus && messages[0][1] == kMiddleC
        && messages[0][2] == kNoteVelocity;
}

bool testFramerTwoNotesOnePush()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t span[] = {
        kNoteOnStatus,
        kMiddleC,
        kNoteVelocity,
        kControlChangeStatus,
        kCcModulation,
        kCcValue};
    framer.Push(span, sizeof(span), sink);
    return messages.size() == 2 && messages[0].size() == 3 && messages[1].size() == 3;
}

bool testFramerRunningStatus()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t first[] = {kNoteOnStatus, kMiddleC, kNoteVelocity};
    const uint8_t second[] = {kMiddleC, static_cast<uint8_t>(0x00)};
    framer.Push(first, sizeof(first), sink);
    framer.Push(second, sizeof(second), sink);
    if (messages.size() != 2 || messages[1].size() != 3)
    {
        std::cerr << "Framer running-status continuation failed\n";
        return false;
    }
    return messages[1][0] == kNoteOnStatus && messages[1][1] == kMiddleC
        && messages[1][2] == 0x00;
}

bool testFramerLoneTimingClock()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t span[] = {kTimingClock};
    framer.Push(span, sizeof(span), sink);
    if (messages.size() != 1 || !expectSingleByte(messages[0], kTimingClock))
    {
        std::cerr << "Framer failed to emit lone Timing Clock\n";
        return false;
    }
    return true;
}

bool testFramerTransportRealtime()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t span[] = {kStart, kContinue, kStop};
    framer.Push(span, sizeof(span), sink);
    if (messages.size() != 3)
    {
        std::cerr << "Framer failed to emit Start/Continue/Stop as three messages\n";
        return false;
    }
    if (!expectSingleByte(messages[0], kStart) || !expectSingleByte(messages[1], kContinue)
        || !expectSingleByte(messages[2], kStop))
    {
        std::cerr << "Framer Start/Continue/Stop byte values mismatched\n";
        return false;
    }
    return true;
}

bool testFramerClockInterleavedMidNote()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    // Realtime between status and data must emit immediately without aborting the note.
    const uint8_t span[] = {kNoteOnStatus, kTimingClock, kMiddleC, kNoteVelocity};
    framer.Push(span, sizeof(span), sink);
    if (messages.size() != 2)
    {
        std::cerr << "Framer mid-note clock interleave message count failed\n";
        return false;
    }
    if (!expectSingleByte(messages[0], kTimingClock))
    {
        std::cerr << "Framer mid-note clock was not emitted first\n";
        return false;
    }
    if (messages[1].size() != 3 || messages[1][0] != kNoteOnStatus
        || messages[1][1] != kMiddleC || messages[1][2] != kNoteVelocity)
    {
        std::cerr << "Framer mid-note clock corrupted the note\n";
        return false;
    }
    return true;
}

bool testFramerRealtimeDuringSysexHold()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    // Realtime during an open SysEx must not abort assembly.
    const uint8_t open[] = {kSysexStart, kSysexManufacturer, kTimingClock, kSysexPayload};
    framer.Push(open, sizeof(open), sink);
    if (messages.size() != 1 || !expectSingleByte(messages[0], kTimingClock))
    {
        std::cerr << "Framer failed to emit clock during open SysEx\n";
        return false;
    }

    const uint8_t close[] = {kSysexEnd};
    framer.Push(close, sizeof(close), sink);
    if (messages.size() != 2)
    {
        std::cerr << "Framer aborted SysEx after realtime interleave\n";
        return false;
    }

    const std::vector<uint8_t>& sysex = messages[1];
    const uint8_t expected[] = {
        kSysexStart,
        kSysexManufacturer,
        kSysexPayload,
        kSysexEnd};
    if (sysex.size() != sizeof(expected))
    {
        std::cerr << "Framer SysEx size wrong after realtime interleave\n";
        return false;
    }
    for (std::size_t index = 0; index < sizeof(expected); ++index)
    {
        if (sysex[index] != expected[index])
        {
            std::cerr << "Framer SysEx bytes wrong after realtime interleave\n";
            return false;
        }
    }
    return true;
}

bool testFramerRealtimePreservesRunningStatus()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t first[] = {kNoteOnStatus, kMiddleC, kNoteVelocity};
    framer.Push(first, sizeof(first), sink);

    const uint8_t clock[] = {kTimingClock};
    framer.Push(clock, sizeof(clock), sink);

    const uint8_t second[] = {kMiddleC, static_cast<uint8_t>(0x00)};
    framer.Push(second, sizeof(second), sink);

    if (messages.size() != 3)
    {
        std::cerr << "Framer running-status after clock message count failed\n";
        return false;
    }
    if (!expectSingleByte(messages[1], kTimingClock))
    {
        std::cerr << "Framer failed to emit clock between running-status notes\n";
        return false;
    }
    if (messages[2].size() != 3 || messages[2][0] != kNoteOnStatus
        || messages[2][1] != kMiddleC || messages[2][2] != 0x00)
    {
        std::cerr << "Framer cleared running status after realtime\n";
        return false;
    }
    return true;
}

using FramerTestFn = bool (*)();

bool runFramerCoreTests()
{
    const FramerTestFn tests[] = {
        testFramerPartialNoteAcrossPushes,
        testFramerTwoNotesOnePush,
        testFramerRunningStatus,
        testFramerLoneTimingClock,
        testFramerTransportRealtime,
        testFramerClockInterleavedMidNote,
        testFramerRealtimeDuringSysexHold,
        testFramerRealtimePreservesRunningStatus};
    for (FramerTestFn test : tests)
    {
        if (!test())
        {
            return false;
        }
    }
    return true;
}
} // namespace

bool runFramerMtcTests();

bool runFramerTests()
{
    return runFramerCoreTests() && runFramerMtcTests();
}
