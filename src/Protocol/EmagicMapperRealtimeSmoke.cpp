// Clock / Start-Stop realtime encode/decode vectors for EmagicCableMapper smoke.

#include "Protocol/EmagicMapperSmokeSupport.h"

#include "Protocol/EmagicCableMapper.h"

#include <cstdint>
#include <string>
#include <vector>

namespace
{
constexpr uint8_t kTimingClock = 0xF8;
constexpr uint8_t kStart = 0xFA;
constexpr uint8_t kContinue = 0xFB;
constexpr uint8_t kStop = 0xFC;

bool bytesEqual(
    const uint8_t* actual,
    std::size_t actualSize,
    const uint8_t* expected,
    std::size_t expectedSize) noexcept
{
    if (actualSize != expectedSize)
    {
        return false;
    }
    for (std::size_t index = 0; index < expectedSize; ++index)
    {
        if (actual[index] != expected[index])
        {
            return false;
        }
    }
    return true;
}
} // namespace

bool runEmagicMapperSmokeEncodeClockTransport(const DeviceProfile& profile, std::ostream& err)
{
    EmagicCableMapper mapper(profile);
    const uint8_t midi[] = {kTimingClock, kContinue, kStop};
    const uint8_t expected[] = {
        kEmagicPortSwitch,
        0x02,
        kTimingClock,
        kContinue,
        kStop,
        kEmagicEndOfValidData};

    uint8_t outBuffer[32] = {};
    std::string error;
    EncodeRequest request{1, midi, sizeof(midi)};
    EncodeBuffer buffer{outBuffer, sizeof(outBuffer), 0};
    if (!mapper.EncodeToDevice(request, buffer, error))
    {
        err << "Mapper encode failed (encode clock+continue+stop cable 1): " << error << '\n';
        return false;
    }
    if (!bytesEqual(outBuffer, buffer.size, expected, sizeof(expected)))
    {
        err << "Mapper test failed (encode clock+continue+stop cable 1): byte mismatch\n";
        return false;
    }
    return true;
}

bool runEmagicMapperSmokeDecodeClockTransport(const DeviceProfile& profile, std::ostream& err)
{
    EmagicCableMapper mapper(profile);
    const uint8_t bulk[] = {
        kEmagicPortSwitch,
        0x01,
        kTimingClock,
        kStart,
        kEmagicEndOfValidData};

    std::vector<uint8_t> seenCables;
    std::vector<std::vector<uint8_t>> seenMidi;
    std::string error;

    const bool ok = mapper.DecodeFromDevice(
        bulk,
        sizeof(bulk),
        [&](uint8_t cableIndex, const uint8_t* midi, std::size_t n) {
            seenCables.push_back(cableIndex);
            seenMidi.emplace_back(midi, midi + n);
        },
        error);

    if (!ok)
    {
        err << "Mapper clock/transport decode failed: " << error << '\n';
        return false;
    }
    if (seenCables.size() != 1 || seenCables[0] != 0 || seenMidi[0].size() != 2)
    {
        err << "Mapper clock/transport decode routed unexpected cable/MIDI\n";
        return false;
    }
    const uint8_t expectedMidi[] = {kTimingClock, kStart};
    if (!bytesEqual(seenMidi[0].data(), seenMidi[0].size(), expectedMidi, sizeof(expectedMidi)))
    {
        err << "Mapper test failed (decode clock+start): byte mismatch\n";
        return false;
    }
    return true;
}
