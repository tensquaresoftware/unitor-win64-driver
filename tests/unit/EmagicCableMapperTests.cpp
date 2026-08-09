// Catch2 unit tests for EmagicCableMapper (synthetic, no hardware).

#include <catch2/catch_test_macros.hpp>

#include "Profile/DeviceProfile.h"
#include "Protocol/EmagicCableMapper.h"
#include "Protocol/EmagicMapperSmokeSupport.h"

#include <sstream>
#include <string>
#include <vector>

namespace
{
const DeviceProfile& requireMt4Profile()
{
    const DeviceProfile* profile = findDeviceProfile(kEmagicVendorId, kMt4ProductId);
    REQUIRE(profile != nullptr);
    return *profile;
}

void requireSmokeOk(bool ok, const std::ostringstream& err)
{
    INFO(err.str());
    REQUIRE(ok);
}
} // namespace

TEST_CASE("mapper smoke encode note on OUT cables 0..3", "[mapper]")
{
    std::ostringstream err;
    requireSmokeOk(runEmagicMapperSmokeEncodeOutCables(requireMt4Profile(), err), err);
}

TEST_CASE("mapper smoke encode control change", "[mapper]")
{
    std::ostringstream err;
    requireSmokeOk(runEmagicMapperSmokeEncodeControlChange(requireMt4Profile(), err), err);
}

TEST_CASE("mapper smoke decode F5 note and ignore pad after FF", "[mapper]")
{
    std::ostringstream err;
    requireSmokeOk(runEmagicMapperSmokeDecodeSynthetic(requireMt4Profile(), err), err);
}

TEST_CASE("mapper smoke decode control change", "[mapper]")
{
    std::ostringstream err;
    requireSmokeOk(runEmagicMapperSmokeDecodeControlChange(requireMt4Profile(), err), err);
}

TEST_CASE("mapper smoke split F5 across two DecodeFromDevice calls", "[mapper]")
{
    std::ostringstream err;
    requireSmokeOk(runEmagicMapperSmokeDecodeSplitF5(requireMt4Profile(), err), err);
}

TEST_CASE("mapper sticky F5 then mid-SysEx data byte is not a cable index", "[mapper]")
{
    EmagicCableMapper mapper(requireMt4Profile());
    std::string error;
    std::vector<uint8_t> cables;
    std::vector<std::vector<uint8_t>> spans;

    auto sink = [&](uint8_t cableIndex, const uint8_t* midi, std::size_t n) {
        cables.push_back(cableIndex);
        spans.emplace_back(midi, midi + n);
    };

    const uint8_t first[] = {0xF5}; // cable byte arrives later / missing
    REQUIRE(mapper.DecodeFromDevice(first, sizeof(first), sink, error));

    // Empty/pad-only follow-up (truncate at FF → zero remaining) keeps sticky F5.
    const uint8_t padOnly[] = {0xFF};
    REQUIRE(mapper.DecodeFromDevice(padOnly, sizeof(padOnly), sink, error));

    // Mid-SysEx data 0x10 must NOT become cable 15 ((0x10-1)&15); stay on cable 0.
    const uint8_t data[] = {0x10, 0x20, 0xF7, 0xFF};
    REQUIRE(mapper.DecodeFromDevice(data, sizeof(data), sink, error));
    REQUIRE(cables.size() == 1);
    REQUIRE(cables[0] == 0);
    REQUIRE(spans.size() == 1);
    REQUIRE(spans[0].size() == 3);
    REQUIRE(spans[0][0] == 0x10);
}

TEST_CASE("mapper smoke encode Timing Clock Continue and Stop", "[mapper]")
{
    std::ostringstream err;
    requireSmokeOk(runEmagicMapperSmokeEncodeClockTransport(requireMt4Profile(), err), err);
}

TEST_CASE("mapper smoke decode Timing Clock and Start", "[mapper]")
{
    std::ostringstream err;
    requireSmokeOk(runEmagicMapperSmokeDecodeClockTransport(requireMt4Profile(), err), err);
}

TEST_CASE("mapper smoke encode MTC quarter-frame and full-frame", "[mapper][mtc]")
{
    std::ostringstream err;
    requireSmokeOk(runEmagicMapperSmokeEncodeMtc(requireMt4Profile(), err), err);
}

TEST_CASE("mapper smoke decode MTC quarter-frame and full-frame", "[mapper][mtc]")
{
    std::ostringstream err;
    requireSmokeOk(runEmagicMapperSmokeDecodeMtc(requireMt4Profile(), err), err);
}

TEST_CASE("mapper smoke encode librarian-sized SysEx opaque carry", "[mapper][sysex]")
{
    std::ostringstream err;
    requireSmokeOk(runEmagicMapperSmokeEncodeSysex(requireMt4Profile(), err), err);
}

TEST_CASE("mapper smoke decode librarian-sized SysEx opaque carry", "[mapper][sysex]")
{
    std::ostringstream err;
    requireSmokeOk(runEmagicMapperSmokeDecodeSysex(requireMt4Profile(), err), err);
}

TEST_CASE("EncodeToDevice rejects non-product OUT cable", "[mapper]")
{
    const DeviceProfile& mt4 = requireMt4Profile();
    EmagicCableMapper mapper(mt4);

    const uint8_t noteOn[] = {0x90, 0x3C, 0x40};
    uint8_t outBuffer[32] = {};
    std::string error;
    EncodeRequest request{4, noteOn, sizeof(noteOn)};
    EncodeBuffer buffer{outBuffer, sizeof(outBuffer), 0};

    REQUIRE_FALSE(mapper.EncodeToDevice(request, buffer, error));
    REQUIRE_FALSE(error.empty());
}

TEST_CASE("EncodeToDevice rejects tiny buffer", "[mapper]")
{
    const DeviceProfile& mt4 = requireMt4Profile();
    EmagicCableMapper mapper(mt4);

    const uint8_t noteOn[] = {0x90, 0x3C, 0x40};
    uint8_t outBuffer[2] = {};
    std::string error;
    // Cable 2 needs F5 switch (2 bytes) + MIDI (3) + pad — capacity 2 is too small.
    EncodeRequest request{2, noteOn, sizeof(noteOn)};
    EncodeBuffer buffer{outBuffer, sizeof(outBuffer), 0};

    REQUIRE_FALSE(mapper.EncodeToDevice(request, buffer, error));
    REQUIRE_FALSE(error.empty());
}

TEST_CASE("EncodeToDevice appends a single trailing 0xFF pad", "[mapper]")
{
    const DeviceProfile& mt4 = requireMt4Profile();
    EmagicCableMapper mapper(mt4);

    const uint8_t inquiry[] = {0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7};
    uint8_t outBuffer[64] = {};
    std::string error;
    // Cable 0: first encode emits F5 01 + inquiry + one 0xFF (Linux short URB).
    EncodeRequest request{0, inquiry, sizeof(inquiry)};
    EncodeBuffer buffer{outBuffer, sizeof(outBuffer), 0};

    REQUIRE(mapper.EncodeToDevice(request, buffer, error));
    REQUIRE(error.empty());
    REQUIRE(buffer.size == sizeof(inquiry) + 2 + 1);
    REQUIRE(outBuffer[buffer.size - 1] == 0xFF);
}

TEST_CASE("OUT encode to IN-capable cable hints demux when IN omits F5", "[mapper]")
{
    // Repro: physical Out2→In2 loopback arrives as F0/…/FF with no F5 02 → was In 1.
    EmagicCableMapper mapper(requireMt4Profile());
    std::string error;
    const uint8_t noteOn[] = {0x90, 0x3C, 0x40};
    uint8_t outBuffer[32] = {};
    EncodeRequest request{1, noteOn, sizeof(noteOn)}; // Out 2
    EncodeBuffer buffer{outBuffer, sizeof(outBuffer), 0};
    REQUIRE(mapper.EncodeToDevice(request, buffer, error));
    REQUIRE(mapper.CurrentInCable() == 1);

    std::vector<uint8_t> cables;
    auto sink = [&](uint8_t cableIndex, const uint8_t* midi, std::size_t n) {
        cables.push_back(cableIndex);
        REQUIRE(n == sizeof(noteOn));
        REQUIRE(midi[0] == noteOn[0]);
    };
    const uint8_t unlabeledIn[] = {0x90, 0x3C, 0x40, 0xFF};
    REQUIRE(mapper.DecodeFromDevice(unlabeledIn, sizeof(unlabeledIn), sink, error));
    REQUIRE(cables.size() == 1);
    REQUIRE(cables[0] == 1);
}

TEST_CASE("IN F5 still overrides OUT-hinted demux cable", "[mapper]")
{
    EmagicCableMapper mapper(requireMt4Profile());
    std::string error;
    const uint8_t noteOn[] = {0x90, 0x3C, 0x40};
    uint8_t outBuffer[32] = {};
    EncodeRequest outReq{1, noteOn, sizeof(noteOn)};
    EncodeBuffer buffer{outBuffer, sizeof(outBuffer), 0};
    REQUIRE(mapper.EncodeToDevice(outReq, buffer, error));
    REQUIRE(mapper.CurrentInCable() == 1);

    std::vector<uint8_t> cables;
    auto sink = [&](uint8_t cableIndex, const uint8_t*, std::size_t) {
        cables.push_back(cableIndex);
    };
    const uint8_t taggedIn1[] = {0xF5, 0x01, 0x90, 0x3C, 0x40, 0xFF};
    REQUIRE(mapper.DecodeFromDevice(taggedIn1, sizeof(taggedIn1), sink, error));
    REQUIRE(cables.size() == 1);
    REQUIRE(cables[0] == 0);
    REQUIRE(mapper.CurrentInCable() == 0);
}

TEST_CASE("OUT encode beyond IN mask does not move IN sticky", "[mapper]")
{
    EmagicCableMapper mapper(requireMt4Profile());
    std::string error;
    const uint8_t noteOn[] = {0x90, 0x3C, 0x40};
    uint8_t outBuffer[32] = {};
    // Prime IN sticky via Out 2 (cable 1).
    {
        EncodeRequest request{1, noteOn, sizeof(noteOn)};
        EncodeBuffer buffer{outBuffer, sizeof(outBuffer), 0};
        REQUIRE(mapper.EncodeToDevice(request, buffer, error));
        REQUIRE(mapper.CurrentInCable() == 1);
    }
    // Out 3 (cable 2) is OUT-only on MT4 — must not clobber IN sticky.
    {
        EncodeRequest request{2, noteOn, sizeof(noteOn)};
        EncodeBuffer buffer{outBuffer, sizeof(outBuffer), 0};
        REQUIRE(mapper.EncodeToDevice(request, buffer, error));
        REQUIRE(mapper.CurrentInCable() == 1);
    }
}
