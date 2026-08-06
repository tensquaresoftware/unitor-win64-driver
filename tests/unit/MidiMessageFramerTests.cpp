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
constexpr uint8_t kMtcQuarterFrame = 0xF1;
constexpr uint8_t kSysexStart = 0xF0;
constexpr uint8_t kSysexEnd = 0xF7;
constexpr uint8_t kSysexManufacturer = 0x7D;
constexpr uint8_t kSysexPayload = 0x10;
constexpr uint8_t kMtcFullFrameDeviceId = 0x7F;
constexpr uint8_t kMtcFullFrameSubId1 = 0x01;
constexpr uint8_t kMtcFullFrameSubId2 = 0x01;

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

TEST_CASE("framer emits lone MTC quarter-frame", "[framer][mtc]")
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t span[] = {kMtcQuarterFrame, 0x01};
    framer.Push(span, sizeof(span), sink);

    REQUIRE(messages.size() == 1);
    REQUIRE(messages[0].size() == 2);
    REQUIRE(messages[0][0] == kMtcQuarterFrame);
    REQUIRE(messages[0][1] == 0x01);
}

TEST_CASE("framer emits eight MTC quarter-frames in sequence", "[framer][mtc]")
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

    REQUIRE(messages.size() == 8);
    for (uint8_t type = 0; type < 8; ++type)
    {
        REQUIRE(messages[type].size() == 2);
        REQUIRE(messages[type][0] == kMtcQuarterFrame);
        REQUIRE(
            messages[type][1]
            == static_cast<uint8_t>(static_cast<uint8_t>(type << 4) | 0x05));
    }
}

TEST_CASE("framer emits MTC full-frame SysEx", "[framer][mtc]")
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t span[] = {
        kSysexStart,
        kMtcFullFrameDeviceId,
        kMtcFullFrameDeviceId,
        kMtcFullFrameSubId1,
        kMtcFullFrameSubId2,
        0x20,
        0x15,
        0x30,
        0x10,
        kSysexEnd};
    framer.Push(span, sizeof(span), sink);

    REQUIRE(messages.size() == 1);
    const std::vector<uint8_t> expected = {
        kSysexStart,
        kMtcFullFrameDeviceId,
        kMtcFullFrameDeviceId,
        kMtcFullFrameSubId1,
        kMtcFullFrameSubId2,
        0x20,
        0x15,
        0x30,
        0x10,
        kSysexEnd};
    REQUIRE(messages[0] == expected);
}

TEST_CASE("framer emits MTC quarter-frame mid-note without corrupting note", "[framer][mtc]")
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

    REQUIRE(messages.size() == 2);
    REQUIRE(messages[0].size() == 2);
    REQUIRE(messages[0][0] == kMtcQuarterFrame);
    REQUIRE(messages[0][1] == 0x23);
    REQUIRE(messages[1].size() == 3);
    REQUIRE(messages[1][0] == kNoteOnStatus);
    REQUIRE(messages[1][1] == kMiddleC);
    REQUIRE(messages[1][2] == kNoteVelocity);
}

TEST_CASE(
    "framer emits MTC quarter-frame during SysEx without aborting SysEx",
    "[framer][mtc]")
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
    REQUIRE(messages.size() == 1);
    REQUIRE(messages[0].size() == 2);
    REQUIRE(messages[0][0] == kMtcQuarterFrame);
    REQUIRE(messages[0][1] == 0x45);

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

TEST_CASE("framer preserves MTC full-frame around Timing Clock", "[framer][mtc]")
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t span[] = {
        kSysexStart,
        kMtcFullFrameDeviceId,
        kMtcFullFrameDeviceId,
        kMtcFullFrameSubId1,
        kMtcFullFrameSubId2,
        kTimingClock,
        0x10,
        0x20,
        0x30,
        0x05,
        kSysexEnd,
        kTimingClock};
    framer.Push(span, sizeof(span), sink);

    REQUIRE(messages.size() == 3);
    REQUIRE(isSingleByte(messages[0], kTimingClock));
    const std::vector<uint8_t> expected = {
        kSysexStart,
        kMtcFullFrameDeviceId,
        kMtcFullFrameDeviceId,
        kMtcFullFrameSubId1,
        kMtcFullFrameSubId2,
        0x10,
        0x20,
        0x30,
        0x05,
        kSysexEnd};
    REQUIRE(messages[1] == expected);
    REQUIRE(isSingleByte(messages[2], kTimingClock));
}

TEST_CASE(
    "framer emits MTC quarter-frame during running-status note without corrupting it",
    "[framer][mtc]")
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t first[] = {kNoteOnStatus, kMiddleC, kNoteVelocity};
    framer.Push(first, sizeof(first), sink);

    const uint8_t second[] = {
        kMiddleC,
        kMtcQuarterFrame,
        0x23,
        static_cast<uint8_t>(0x00)};
    framer.Push(second, sizeof(second), sink);

    REQUIRE(messages.size() == 3);
    REQUIRE(messages[1].size() == 2);
    REQUIRE(messages[1][0] == kMtcQuarterFrame);
    REQUIRE(messages[1][1] == 0x23);
    REQUIRE(messages[2].size() == 3);
    REQUIRE(messages[2][0] == kNoteOnStatus);
    REQUIRE(messages[2][1] == kMiddleC);
    REQUIRE(messages[2][2] == 0x00);
}

TEST_CASE(
    "framer emits Timing Clock between MTC quarter-frame status and data",
    "[framer][mtc]")
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

    REQUIRE(messages.size() == 3);
    REQUIRE(isSingleByte(messages[0], kTimingClock));
    REQUIRE(messages[1].size() == 2);
    REQUIRE(messages[1][0] == kMtcQuarterFrame);
    REQUIRE(messages[1][1] == 0x23);
    REQUIRE(messages[2].size() == 3);
    REQUIRE(messages[2][0] == kNoteOnStatus);
    REQUIRE(messages[2][1] == kMiddleC);
    REQUIRE(messages[2][2] == kNoteVelocity);
}

TEST_CASE(
    "framer replaces incomplete MTC quarter-frame instead of nesting",
    "[framer][mtc]")
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t span[] = {kMtcQuarterFrame, kMtcQuarterFrame, 0x67};
    framer.Push(span, sizeof(span), sink);

    REQUIRE(messages.size() == 1);
    REQUIRE(messages[0].size() == 2);
    REQUIRE(messages[0][0] == kMtcQuarterFrame);
    REQUIRE(messages[0][1] == 0x67);
}

