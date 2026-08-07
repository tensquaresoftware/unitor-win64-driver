// Catch2 unit tests for EmagicCableMapper (synthetic, no hardware).

#include <catch2/catch_test_macros.hpp>

#include "Profile/DeviceProfile.h"
#include "Protocol/EmagicCableMapper.h"
#include "Protocol/EmagicMapperSmokeSupport.h"

#include <sstream>
#include <string>

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
