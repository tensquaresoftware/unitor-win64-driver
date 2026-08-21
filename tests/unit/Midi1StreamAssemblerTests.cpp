// Catch2 unit tests for Midi1StreamAssembler (WMS SysEx fragment reassembly).

#include <catch2/catch_test_macros.hpp>

#include "Midi/Midi1StreamAssembler.h"

#include <cstdint>
#include <vector>

namespace
{
std::vector<std::vector<uint8_t>> collectAppend(
    Midi1StreamAssembler& assembler,
    const std::vector<uint8_t>& chunk)
{
    std::vector<std::vector<uint8_t>> emitted;
    assembler.Append(chunk.data(), chunk.size(), [&](const uint8_t* midi, std::size_t n) {
        emitted.emplace_back(midi, midi + n);
    });
    return emitted;
}
} // namespace

TEST_CASE("midi1 assembler forwards complete non-sysex chunk", "[midi1][assembler]")
{
    Midi1StreamAssembler assembler;
    const std::vector<uint8_t> note{0x90, 0x3C, 0x40};
    const auto emitted = collectAppend(assembler, note);
    REQUIRE(emitted.size() == 1);
    REQUIRE(emitted[0] == note);
    REQUIRE_FALSE(assembler.HoldingSysex());
}

TEST_CASE("midi1 assembler reunites fragmented sysex", "[midi1][assembler]")
{
    Midi1StreamAssembler assembler;
    const std::vector<uint8_t> part1{0xF0, 0x10, 0x06, 0x01};
    const std::vector<uint8_t> part2{0x20, 0x30, 0xF7};
    auto emitted = collectAppend(assembler, part1);
    REQUIRE(emitted.empty());
    REQUIRE(assembler.HoldingSysex());
    emitted = collectAppend(assembler, part2);
    REQUIRE(emitted.size() == 1);
    REQUIRE(emitted[0] == (std::vector<uint8_t>{0xF0, 0x10, 0x06, 0x01, 0x20, 0x30, 0xF7}));
    REQUIRE_FALSE(assembler.HoldingSysex());
}

TEST_CASE("midi1 assembler emits realtime during sysex hold", "[midi1][assembler]")
{
    Midi1StreamAssembler assembler;
    const std::vector<uint8_t> open{0xF0, 0x7D};
    auto emitted = collectAppend(assembler, open);
    REQUIRE(emitted.empty());
    const uint8_t clock = 0xF8;
    emitted = collectAppend(assembler, std::vector<uint8_t>{clock});
    REQUIRE(emitted.size() == 1);
    REQUIRE(emitted[0] == std::vector<uint8_t>{0xF8});
    REQUIRE(assembler.HoldingSysex());
    emitted = collectAppend(assembler, std::vector<uint8_t>{0x01, 0xF7});
    REQUIRE(emitted.size() == 1);
    REQUIRE(emitted[0].front() == 0xF0);
    REQUIRE(emitted[0].back() == 0xF7);
}

TEST_CASE("midi1 assembler drops oversize sysex hold", "[midi1][assembler]")
{
    Midi1StreamAssembler assembler;
    std::vector<uint8_t> huge(Midi1StreamAssembler::kMaxSysexHoldBytes + 1, 0x10);
    huge[0] = 0xF0;
    auto emitted = collectAppend(assembler, huge);
    REQUIRE(emitted.empty());
    REQUIRE_FALSE(assembler.HoldingSysex());
}

TEST_CASE("midi1 assembler aborts sysex on channel status", "[midi1][assembler]")
{
    Midi1StreamAssembler assembler;
    auto emitted = collectAppend(assembler, std::vector<uint8_t>{0xF0, 0x7D, 0x01});
    REQUIRE(emitted.empty());
    REQUIRE(assembler.HoldingSysex());
    emitted = collectAppend(assembler, std::vector<uint8_t>{0x90, 0x3C, 0x40});
    REQUIRE(emitted.size() == 1);
    REQUIRE(emitted[0] == (std::vector<uint8_t>{0x90, 0x3C, 0x40}));
    REQUIRE_FALSE(assembler.HoldingSysex());
}
