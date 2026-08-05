// Librarian-sized SysEx vectors for MidiMessageFramer smoke (Inquiry / Matrix-shaped).

#include "App/FramerSmoke.h"

#include "Protocol/MidiMessageFramer.h"

#include <cstdint>
#include <iostream>
#include <vector>

namespace
{
constexpr uint8_t kTimingClock = 0xF8;
constexpr uint8_t kMtcQuarterFrame = 0xF1;
constexpr uint8_t kSysexStart = 0xF0;
constexpr uint8_t kSysexEnd = 0xF7;
constexpr uint8_t kOberheimId = 0x10;
constexpr uint8_t kMatrixDevice = 0x06;
constexpr uint8_t kPatchOpcode = 0x01;
constexpr uint8_t kMasterOpcode = 0x03;
constexpr std::size_t kPatchFrameBytes = 275;
constexpr std::size_t kMasterFrameBytes = 351;

using MessageList = std::vector<std::vector<uint8_t>>;

MidiFramedMessageSink makeCollector(MessageList& messages)
{
    return [&](const uint8_t* midi, std::size_t n) {
        messages.emplace_back(midi, midi + n);
    };
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

bool expectExact(const std::vector<uint8_t>& actual, const std::vector<uint8_t>& expected)
{
    return actual == expected;
}

bool testFramerDeviceInquiry()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t span[] = {kSysexStart, 0x7E, 0x7F, 0x06, 0x01, kSysexEnd};
    framer.Push(span, sizeof(span), sink);
    if (messages.size() != 1 || messages[0].size() != 6)
    {
        std::cerr << "Framer Device Inquiry failed\n";
        return false;
    }
    return expectExact(messages[0], std::vector<uint8_t>(span, span + sizeof(span)));
}

bool testFramerInquiryReply()
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
    if (messages.size() != 1 || messages[0].size() != 15)
    {
        std::cerr << "Framer Inquiry reply failed\n";
        return false;
    }
    return expectExact(messages[0], std::vector<uint8_t>(span, span + sizeof(span)));
}

bool testFramerOberheimPatch275()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const std::vector<uint8_t> frame = makeOberheimShapedFrame(kPatchFrameBytes, kPatchOpcode);
    framer.Push(frame.data(), frame.size(), sink);
    if (messages.size() != 1 || !expectExact(messages[0], frame))
    {
        std::cerr << "Framer Oberheim-shaped patch (275 B) failed\n";
        return false;
    }
    return true;
}

bool testFramerOberheimMaster351()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const std::vector<uint8_t> frame = makeOberheimShapedFrame(kMasterFrameBytes, kMasterOpcode);
    framer.Push(frame.data(), frame.size(), sink);
    if (messages.size() != 1 || !expectExact(messages[0], frame))
    {
        std::cerr << "Framer Oberheim-shaped master (351 B) failed\n";
        return false;
    }
    return true;
}

bool testFramerPatchSplitAcrossPushes()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const std::vector<uint8_t> frame = makeOberheimShapedFrame(kPatchFrameBytes, kPatchOpcode);
    const std::size_t mid = 100;
    framer.Push(frame.data(), mid, sink);
    if (!messages.empty())
    {
        std::cerr << "Framer emitted before patch SysEx complete\n";
        return false;
    }
    framer.Push(frame.data() + mid, frame.size() - mid, sink);
    if (messages.size() != 1 || !expectExact(messages[0], frame))
    {
        std::cerr << "Framer patch split across Push spans failed\n";
        return false;
    }
    return true;
}

bool testFramerClockAndQuarterFrameMidLibrarianSysex()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const std::vector<uint8_t> frame = makeOberheimShapedFrame(kPatchFrameBytes, kPatchOpcode);
    std::vector<uint8_t> open(frame.begin(), frame.begin() + 50);
    open.push_back(kTimingClock);
    open.push_back(kMtcQuarterFrame);
    open.push_back(0x23);
    open.insert(open.end(), frame.begin() + 50, frame.end() - 1);

    framer.Push(open.data(), open.size(), sink);
    if (messages.size() != 2)
    {
        std::cerr << "Framer mid-SysEx clock/QF interleave count mismatch\n";
        return false;
    }
    if (messages[0].size() != 1 || messages[0][0] != kTimingClock)
    {
        std::cerr << "Framer mid-SysEx Timing Clock missing\n";
        return false;
    }
    if (messages[1].size() != 2 || messages[1][0] != kMtcQuarterFrame || messages[1][1] != 0x23)
    {
        std::cerr << "Framer mid-SysEx quarter-frame missing\n";
        return false;
    }

    const uint8_t close[] = {kSysexEnd};
    framer.Push(close, sizeof(close), sink);
    if (messages.size() != 3 || !expectExact(messages[2], frame))
    {
        std::cerr << "Framer mid-SysEx interleave corrupted librarian frame\n";
        return false;
    }
    return true;
}

bool testFramerOversizeSysexObservableFailure()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    std::vector<uint8_t> oversize(kMaxSysexHoldBytes + 8, 0x10);
    oversize[0] = kSysexStart;
    framer.Push(oversize.data(), oversize.size(), sink);

    if (!messages.empty())
    {
        std::cerr << "Framer oversize SysEx must not emit a partial frame\n";
        return false;
    }
    if (framer.OversizeSysexRejectCount() == 0)
    {
        std::cerr << "Framer oversize SysEx must bump reject counter\n";
        return false;
    }
    if (framer.ConsumeOversizeSysexRejectCount() == 0)
    {
        std::cerr << "Framer ConsumeOversizeSysexRejectCount failed\n";
        return false;
    }
    if (framer.OversizeSysexRejectCount() != 0)
    {
        std::cerr << "Framer reject counter should clear after Consume\n";
        return false;
    }
    return true;
}

bool testFramerNestedF0AbandonsOpenSysex()
{
    MidiMessageFramer framer;
    MessageList messages;
    auto sink = makeCollector(messages);

    const uint8_t openPatch[] = {kSysexStart, 0x10, 0x06, 0x01, 0x20, 0x21};
    framer.Push(openPatch, sizeof(openPatch), sink);
    if (!messages.empty() || framer.OversizeSysexRejectCount() != 0)
    {
        std::cerr << "Framer open SysEx must hold without emit or reject\n";
        return false;
    }

    const uint8_t restart[] = {kSysexStart, 0x7E, 0x7F, 0x06, 0x01, kSysexEnd};
    framer.Push(restart, sizeof(restart), sink);
    if (framer.OversizeSysexRejectCount() != 1)
    {
        std::cerr << "Framer nested F0 must bump reject counter for abandoned dump\n";
        return false;
    }
    if (messages.size() != 1 || messages[0].size() != sizeof(restart))
    {
        std::cerr << "Framer nested F0 must emit the replacement SysEx only\n";
        return false;
    }
    return true;
}

using FramerTestFn = bool (*)();
} // namespace

bool runFramerSysexTests()
{
    const FramerTestFn tests[] = {
        testFramerDeviceInquiry,
        testFramerInquiryReply,
        testFramerOberheimPatch275,
        testFramerOberheimMaster351,
        testFramerPatchSplitAcrossPushes,
        testFramerClockAndQuarterFrameMidLibrarianSysex,
        testFramerOversizeSysexObservableFailure,
        testFramerNestedF0AbandonsOpenSysex};
    for (FramerTestFn test : tests)
    {
        if (!test())
        {
            return false;
        }
    }
    return true;
}
