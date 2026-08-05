// Bridge process entry — user-session host (not a Windows Service).

#include "Device/DeviceSession.h"
#include "Profile/DeviceProfile.h"
#include "Protocol/EmagicCableMapper.h"
#include "Usb/WinUsbTransport.h"

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace
{
constexpr uint8_t kNoteOnStatus = 0x90;
constexpr uint8_t kMiddleC = 0x3C;
constexpr uint8_t kNoteVelocity = 0x40;

bool matchesValidatedMt4Profile(const DeviceProfile& profile) noexcept
{
    return profile.vid == kEmagicVendorId
        && profile.pid == kMt4ProductId
        && profile.ifnum == kMt4InterfaceNumber
        && profile.inCables == kMt4InCables
        && profile.outCables == kMt4OutCables
        && !profile.patchMode
        && !profile.ltc
        && !profile.fastMode
        && countProductPorts(profile.inCables) == 2
        && countProductPorts(profile.outCables) == 4;
}

bool matchesMt4ProductCableOrder(const DeviceProfile& profile) noexcept
{
    uint8_t inIndices[kMaxEmagicCableCount] = {};
    uint8_t outIndices[kMaxEmagicCableCount] = {};

    const std::size_t inCount = collectProductCableIndices(
        profile.inCables, inIndices, kMaxEmagicCableCount);
    const std::size_t outCount = collectProductCableIndices(
        profile.outCables, outIndices, kMaxEmagicCableCount);

    return inCount == 2
        && inIndices[0] == 0
        && inIndices[1] == 1
        && outCount == 4
        && outIndices[0] == 0
        && outIndices[1] == 1
        && outIndices[2] == 2
        && outIndices[3] == 3;
}

bool hasFlag(int argc, char* argv[], const char* flag) noexcept
{
    for (int index = 1; index < argc; ++index)
    {
        if (std::strcmp(argv[index], flag) == 0)
        {
            return true;
        }
    }
    return false;
}

int runProfileSmoke()
{
    const DeviceProfile* mt4 = findDeviceProfile(kEmagicVendorId, kMt4ProductId);
    if (mt4 == nullptr
        || !matchesValidatedMt4Profile(*mt4)
        || !matchesMt4ProductCableOrder(*mt4))
    {
        return 1;
    }

    // Unknown PID fails closed — never invents an MT4 profile.
    if (findDeviceProfile(kEmagicVendorId, 0xFFFF) != nullptr)
    {
        return 1;
    }

    return 0;
}

struct ByteSpan
{
    const uint8_t* bytes = nullptr;
    std::size_t size = 0;
};

bool expectBytes(const ByteSpan& actual, const ByteSpan& expected, const char* label)
{
    if (actual.size != expected.size)
    {
        std::cerr << "Mapper test failed (" << label << "): size mismatch\n";
        return false;
    }
    for (std::size_t index = 0; index < expected.size; ++index)
    {
        if (actual.bytes[index] != expected.bytes[index])
        {
            std::cerr << "Mapper test failed (" << label << "): byte mismatch\n";
            return false;
        }
    }
    return true;
}

struct EncodeExpectCase
{
    uint8_t cable = 0;
    const uint8_t* expected = nullptr;
    std::size_t expectedSize = 0;
    const char* label = "";
};

bool encodeAndExpect(EmagicCableMapper& mapper, const EncodeExpectCase& testCase)
{
    const uint8_t noteOn[] = {kNoteOnStatus, kMiddleC, kNoteVelocity};
    uint8_t outBuffer[32] = {};
    std::string error;

    EncodeRequest request{testCase.cable, noteOn, sizeof(noteOn)};
    EncodeBuffer buffer{outBuffer, sizeof(outBuffer), 0};
    if (!mapper.EncodeToDevice(request, buffer, error))
    {
        std::cerr << "Mapper encode failed (" << testCase.label << "): " << error << '\n';
        return false;
    }

    return expectBytes(
        ByteSpan{outBuffer, buffer.size},
        ByteSpan{testCase.expected, testCase.expectedSize},
        testCase.label);
}

bool testEncodeOutCables(EmagicCableMapper& mapper)
{
    const uint8_t expectedCable0[] = {
        kNoteOnStatus, kMiddleC, kNoteVelocity, kEmagicEndOfValidData};
    if (!encodeAndExpect(
            mapper,
            EncodeExpectCase{0, expectedCable0, sizeof(expectedCable0), "encode cable 0"}))
    {
        return false;
    }

    for (uint8_t cable = 1; cable <= 3; ++cable)
    {
        const uint8_t expected[] = {
            kEmagicPortSwitch,
            static_cast<uint8_t>((cable + 1) & 15),
            kNoteOnStatus,
            kMiddleC,
            kNoteVelocity,
            kEmagicEndOfValidData};
        if (!encodeAndExpect(
                mapper,
                EncodeExpectCase{cable, expected, sizeof(expected), "encode switched cable"}))
        {
            return false;
        }
    }

    return true;
}

bool testDecodeSynthetic(EmagicCableMapper& mapper)
{
    // F5 02 → cable 1, then note-on, then FF pad.
    const uint8_t bulk[] = {
        kEmagicPortSwitch,
        0x02,
        kNoteOnStatus,
        kMiddleC,
        kNoteVelocity,
        kEmagicEndOfValidData,
        0x11,
        0x22};

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
        std::cerr << "Mapper decode failed: " << error << '\n';
        return false;
    }
    if (seenCables.size() != 1 || seenCables[0] != 1 || seenMidi[0].size() != 3)
    {
        std::cerr << "Mapper decode routed unexpected cable/MIDI\n";
        return false;
    }
    const uint8_t expectedMidi[] = {kNoteOnStatus, kMiddleC, kNoteVelocity};
    return expectBytes(
        ByteSpan{seenMidi[0].data(), seenMidi[0].size()},
        ByteSpan{expectedMidi, sizeof(expectedMidi)},
        "decode MIDI");
}

bool testDecodeSplitF5(EmagicCableMapper& mapper)
{
    const uint8_t firstChunk[] = {kNoteOnStatus, kMiddleC, kEmagicPortSwitch};
    // Wire port 2 → internal cable 1 (MT4 product IN).
    const uint8_t secondChunk[] = {0x02, kNoteVelocity, kEmagicEndOfValidData};

    std::vector<uint8_t> cables;
    std::vector<std::size_t> sizes;
    std::string error;

    auto sink = [&](uint8_t cableIndex, const uint8_t* /*midi*/, std::size_t n) {
        cables.push_back(cableIndex);
        sizes.push_back(n);
    };

    if (!mapper.DecodeFromDevice(firstChunk, sizeof(firstChunk), sink, error))
    {
        std::cerr << "Mapper split-F5 first decode failed: " << error << '\n';
        return false;
    }
    if (!mapper.DecodeFromDevice(secondChunk, sizeof(secondChunk), sink, error))
    {
        std::cerr << "Mapper split-F5 second decode failed: " << error << '\n';
        return false;
    }

    // First chunk: two MIDI bytes on cable 0; second: one MIDI byte after switch to cable 1.
    if (cables.size() != 2 || cables[0] != 0 || cables[1] != 1)
    {
        std::cerr << "Mapper split-F5 cable routing failed\n";
        return false;
    }
    if (sizes.size() != 2 || sizes[0] != 2 || sizes[1] != 1)
    {
        std::cerr << "Mapper split-F5 MIDI sizes failed\n";
        return false;
    }
    return true;
}

int runMapperTests()
{
    const DeviceProfile* mt4 = findDeviceProfile(kEmagicVendorId, kMt4ProductId);
    if (mt4 == nullptr)
    {
        std::cerr << "MT4 DeviceProfile not found for mapper tests\n";
        return 1;
    }

    EmagicCableMapper encodeMapper(*mt4);
    if (!testEncodeOutCables(encodeMapper))
    {
        return 1;
    }

    EmagicCableMapper decodeMapper(*mt4);
    if (!testDecodeSynthetic(decodeMapper))
    {
        return 1;
    }

    EmagicCableMapper splitMapper(*mt4);
    if (!testDecodeSplitF5(splitMapper))
    {
        return 1;
    }

    std::cout << "Mapper synthetic tests passed\n";
    return 0;
}

int startMt4Session(bool allowZadigFallback)
{
    const DeviceProfile* mt4 = findDeviceProfile(kEmagicVendorId, kMt4ProductId);
    if (mt4 == nullptr)
    {
        std::cerr << "MT4 DeviceProfile not found\n";
        return 1;
    }

    DeviceSession session;
    WinUsbOpenOptions options;
    options.allowZadigFallback = allowZadigFallback;

    std::string error;
    if (!session.Start(*mt4, error, options) || !session.IsRunning())
    {
        std::cerr << "DeviceSession start failed: "
                  << (error.empty() ? "unknown error" : error) << '\n';
        return 1;
    }

    std::cout << "DeviceSession started for MT4\n";
    session.Stop();
    std::cout << "DeviceSession stopped\n";
    return 0;
}

int openMt4DeviceWithOptions(bool allowZadigFallback)
{
    const DeviceProfile* mt4 = findDeviceProfile(kEmagicVendorId, kMt4ProductId);
    if (mt4 == nullptr)
    {
        std::cerr << "MT4 DeviceProfile not found\n";
        return 1;
    }

    WinUsbTransport transport;
    WinUsbOpenOptions options;
    options.allowZadigFallback = allowZadigFallback;

    std::string error;
    if (!transport.Open(*mt4, error, options) || !transport.IsOpen())
    {
        std::cerr << "WinUSB open failed: "
                  << (error.empty() ? "unknown error" : error) << '\n';
        return 1;
    }

    std::cout << "WinUSB open succeeded\n";
    return 0;
}
} // namespace

int main(int argc, char* argv[])
{
    const int profileResult = runProfileSmoke();
    if (profileResult != 0)
    {
        return profileResult;
    }

    if (hasFlag(argc, argv, "--test-mapper"))
    {
        return runMapperTests();
    }

    const bool allowZadigFallback = hasFlag(argc, argv, "--dev-zadig");

    if (hasFlag(argc, argv, "--start-session"))
    {
        return startMt4Session(allowZadigFallback);
    }

    if (hasFlag(argc, argv, "--open-device"))
    {
        return openMt4DeviceWithOptions(allowZadigFallback);
    }

    return 0;
}
