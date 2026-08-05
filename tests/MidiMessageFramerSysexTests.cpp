// Catch2 unit tests for MidiMessageFramer librarian-sized SysEx.

#include <catch2/catch_test_macros.hpp>

#include "Protocol/MidiMessageFramer.h"

#include <cstdint>
#include <vector>

namespace
{
constexpr uint8_t kTimingClock = 0xF8;
constexpr uint8_t kMtcQuarterFrame = 0xF1;
constexpr uint8_t kSysexStart = 0xF0;
constexpr uint8_t kSysexEnd = 0xF7;
constexpr uint8_t kOberheimId = 0x10;
constexpr uint8_t kMatrixDevice = 0x06;
constexpr std::size_t kPatchFrameBytes = 275;
constexpr std::size_t kMasterFrameBytes = 351;

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

std::vector<uint8_t> makeOberheimShapedFrame(std::size_t totalBytes, uint8_t opcode)
{
    std::vector<uint8_t> frame(totalBytes, 0);
    frame[0] = kSysexStart;
    frame[1] = kOberheimId;
    frame[2] = kMatrixDevice;
    frame[3] = opcode;
    for (std::size_t index = 4; index + 1 < totalBytes; ++index)
    {
        frame[index] = static_cast<uint8_t>(index & 0x7F);
    }
    frame[totalBytes - 1] = kSysexEnd;
    return frame;
}
} // namespace

TEST_CASE("framer emits Device Inquiry SysEx", "[framer][sysex]")
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t span[] = {kSysexStart, 0x7E, 0x7F, 0x06, 0x01, kSysexEnd};
    framer.Push(span, sizeof(span), sink);

    REQUIRE(messages.size() == 1);
    REQUIRE(messages[0] == std::vector<uint8_t>(span, span + sizeof(span)));
}

TEST_CASE("framer emits Inquiry reply SysEx", "[framer][sysex]")
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t span[] = {
        kSysexStart,
        0x7E,
        0x00,
        0x06,
        0x02,
        0x00,
        0x00,
        0x0E,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
        0x00,
        kSysexEnd};
    framer.Push(span, sizeof(span), sink);

    REQUIRE(messages.size() == 1);
    REQUIRE(messages[0].size() == 15);
    REQUIRE(messages[0] == std::vector<uint8_t>(span, span + sizeof(span)));
}

TEST_CASE("framer emits Oberheim-shaped patch SysEx (275 B)", "[framer][sysex]")
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const std::vector<uint8_t> frame = makeOberheimShapedFrame(kPatchFrameBytes, 0x01);
    framer.Push(frame.data(), frame.size(), sink);

    REQUIRE(messages.size() == 1);
    REQUIRE(messages[0] == frame);
    REQUIRE(frame.size() <= kMaxSysexHoldBytes);
}

TEST_CASE("framer emits Oberheim-shaped master SysEx (351 B)", "[framer][sysex]")
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const std::vector<uint8_t> frame = makeOberheimShapedFrame(kMasterFrameBytes, 0x03);
    framer.Push(frame.data(), frame.size(), sink);

    REQUIRE(messages.size() == 1);
    REQUIRE(messages[0] == frame);
}

TEST_CASE("framer assembles patch SysEx split across Push spans", "[framer][sysex]")
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const std::vector<uint8_t> frame = makeOberheimShapedFrame(kPatchFrameBytes, 0x01);
    framer.Push(frame.data(), 120, sink);
    REQUIRE(messages.empty());
    framer.Push(frame.data() + 120, frame.size() - 120, sink);

    REQUIRE(messages.size() == 1);
    REQUIRE(messages[0] == frame);
}

TEST_CASE(
    "framer preserves librarian SysEx around Timing Clock and MTC quarter-frame",
    "[framer][sysex]")
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const std::vector<uint8_t> frame = makeOberheimShapedFrame(kPatchFrameBytes, 0x01);
    std::vector<uint8_t> open(frame.begin(), frame.begin() + 40);
    open.push_back(kTimingClock);
    open.push_back(kMtcQuarterFrame);
    open.push_back(0x11);
    open.insert(open.end(), frame.begin() + 40, frame.end() - 1);
    framer.Push(open.data(), open.size(), sink);

    REQUIRE(messages.size() == 2);
    REQUIRE(isSingleByte(messages[0], kTimingClock));
    REQUIRE(messages[1].size() == 2);
    REQUIRE(messages[1][0] == kMtcQuarterFrame);

    const uint8_t close[] = {kSysexEnd};
    framer.Push(close, sizeof(close), sink);
    REQUIRE(messages.size() == 3);
    REQUIRE(messages[2] == frame);
}

TEST_CASE("framer oversize SysEx bumps reject counter without emit", "[framer][sysex]")
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    std::vector<uint8_t> oversize(kMaxSysexHoldBytes + 16, 0x22);
    oversize[0] = kSysexStart;
    framer.Push(oversize.data(), oversize.size(), sink);

    REQUIRE(messages.empty());
    REQUIRE(framer.OversizeSysexRejectCount() > 0);
    REQUIRE(framer.ConsumeOversizeSysexRejectCount() > 0);
    REQUIRE(framer.OversizeSysexRejectCount() == 0);
}

TEST_CASE("framer nested F0 abandons open SysEx as reject", "[framer][sysex]")
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t openPatch[] = {kSysexStart, 0x10, 0x06, 0x01, 0x20, 0x21};
    framer.Push(openPatch, sizeof(openPatch), sink);
    REQUIRE(messages.empty());
    REQUIRE(framer.OversizeSysexRejectCount() == 0);

    const uint8_t restart[] = {kSysexStart, 0x7E, 0x7F, 0x06, 0x01, kSysexEnd};
    framer.Push(restart, sizeof(restart), sink);

    REQUIRE(framer.OversizeSysexRejectCount() == 1);
    REQUIRE(messages.size() == 1);
    REQUIRE(messages[0] == std::vector<uint8_t>(restart, restart + sizeof(restart)));
}
