// Catch2 unit tests for MidiMessageFramer realtime (clock / transport).

#include <catch2/catch_test_macros.hpp>

#include "Protocol/MidiMessageFramer.h"

#include <cstdint>
#include <vector>

namespace
{
constexpr uint8_t kNoteOnStatus = 0x90;
constexpr uint8_t kMiddleC = 0x3C;
constexpr uint8_t kNoteVelocity = 0x40;
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

bool isSingleByte(const std::vector<uint8_t>& message, uint8_t status)
{
    return message.size() == 1 && message[0] == status;
}
} // namespace

TEST_CASE("framer emits lone Timing Clock", "[framer]")
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t span[] = {kTimingClock};
    framer.Push(span, sizeof(span), sink);

    REQUIRE(messages.size() == 1);
    REQUIRE(isSingleByte(messages[0], kTimingClock));
}

TEST_CASE("framer emits Start Continue Stop as three messages", "[framer]")
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t span[] = {kStart, kContinue, kStop};
    framer.Push(span, sizeof(span), sink);

    REQUIRE(messages.size() == 3);
    REQUIRE(isSingleByte(messages[0], kStart));
    REQUIRE(isSingleByte(messages[1], kContinue));
    REQUIRE(isSingleByte(messages[2], kStop));
}

TEST_CASE("framer emits Timing Clock mid-note without corrupting note", "[framer]")
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t span[] = {kNoteOnStatus, kTimingClock, kMiddleC, kNoteVelocity};
    framer.Push(span, sizeof(span), sink);

    REQUIRE(messages.size() == 2);
    REQUIRE(isSingleByte(messages[0], kTimingClock));
    REQUIRE(messages[1].size() == 3);
    REQUIRE(messages[1][0] == kNoteOnStatus);
    REQUIRE(messages[1][1] == kMiddleC);
    REQUIRE(messages[1][2] == kNoteVelocity);
}

TEST_CASE("framer emits Timing Clock during SysEx without aborting SysEx", "[framer]")
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t open[] = {kSysexStart, kSysexManufacturer, kTimingClock, kSysexPayload};
    framer.Push(open, sizeof(open), sink);
    REQUIRE(messages.size() == 1);
    REQUIRE(isSingleByte(messages[0], kTimingClock));

    const uint8_t close[] = {kSysexEnd};
    framer.Push(close, sizeof(close), sink);
    REQUIRE(messages.size() == 2);

    const std::vector<uint8_t> expected = {
        kSysexStart,
        kSysexManufacturer,
        kSysexPayload,
        kSysexEnd};
    REQUIRE(messages[1] == expected);
}

TEST_CASE("framer preserves running status after Timing Clock", "[framer]")
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

    REQUIRE(messages.size() == 3);
    REQUIRE(isSingleByte(messages[1], kTimingClock));
    REQUIRE(messages[2].size() == 3);
    REQUIRE(messages[2][0] == kNoteOnStatus);
    REQUIRE(messages[2][1] == kMiddleC);
    REQUIRE(messages[2][2] == 0x00);
}
