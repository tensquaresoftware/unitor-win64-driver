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

bool testFramerPartialNoteAcrossPushes()
{
    MidiMessageFramer framer;
    std::vector<std::vector<uint8_t>> messages;
    auto sink = [&](const uint8_t* midi, std::size_t n) {
        messages.emplace_back(midi, midi + n);
    };

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
    std::vector<std::vector<uint8_t>> messages;
    auto sink = [&](const uint8_t* midi, std::size_t n) {
        messages.emplace_back(midi, midi + n);
    };

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
    std::vector<std::vector<uint8_t>> messages;
    auto sink = [&](const uint8_t* midi, std::size_t n) {
        messages.emplace_back(midi, midi + n);
    };

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
} // namespace

bool runFramerTests()
{
    return testFramerPartialNoteAcrossPushes() && testFramerTwoNotesOnePush()
        && testFramerRunningStatus();
}
