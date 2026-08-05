// Synthetic EmagicCableMapper encode/decode smoke (--test-mapper).

#include "App/MapperSmoke.h"
#include "App/FramerSmoke.h"

#include "Profile/DeviceProfile.h"
#include "Protocol/EmagicCableMapper.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace
{
constexpr uint8_t kNoteOnStatus = 0x90;
constexpr uint8_t kMiddleC = 0x3C;
constexpr uint8_t kNoteVelocity = 0x40;
constexpr uint8_t kControlChangeStatus = 0xB0;
constexpr uint8_t kCcModulation = 0x01;
constexpr uint8_t kCcValue = 0x40;

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
    const uint8_t* midi = nullptr;
    std::size_t midiSize = 0;
    const uint8_t* expected = nullptr;
    std::size_t expectedSize = 0;
    const char* label = "";
};

bool encodeAndExpect(EmagicCableMapper& mapper, const EncodeExpectCase& testCase)
{
    uint8_t outBuffer[32] = {};
    std::string error;

    EncodeRequest request{testCase.cable, testCase.midi, testCase.midiSize};
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

bool testEncodeSwitchedNote(EmagicCableMapper& mapper, uint8_t cable)
{
    const uint8_t noteOn[] = {kNoteOnStatus, kMiddleC, kNoteVelocity};
    const uint8_t expected[] = {
        kEmagicPortSwitch,
        static_cast<uint8_t>((cable + 1) & 15),
        kNoteOnStatus,
        kMiddleC,
        kNoteVelocity,
        kEmagicEndOfValidData};
    return encodeAndExpect(
        mapper,
        EncodeExpectCase{
            cable,
            noteOn,
            sizeof(noteOn),
            expected,
            sizeof(expected),
            "encode note switched cable"});
}

bool testEncodeFirstCable0HasF5(EmagicCableMapper& mapper)
{
    const uint8_t noteOn[] = {kNoteOnStatus, kMiddleC, kNoteVelocity};
    const uint8_t expectedFirstCable0[] = {
        kEmagicPortSwitch,
        0x01,
        kNoteOnStatus,
        kMiddleC,
        kNoteVelocity,
        kEmagicEndOfValidData};
    return encodeAndExpect(
        mapper,
        EncodeExpectCase{
            0,
            noteOn,
            sizeof(noteOn),
            expectedFirstCable0,
            sizeof(expectedFirstCable0),
            "encode note cable 0 first F5"});
}

bool testEncodeStickyCable0OmitsF5(EmagicCableMapper& mapper)
{
    const uint8_t noteOn[] = {kNoteOnStatus, kMiddleC, kNoteVelocity};
    const uint8_t expectedStickyCable0[] = {
        kNoteOnStatus, kMiddleC, kNoteVelocity, kEmagicEndOfValidData};
    return encodeAndExpect(
        mapper,
        EncodeExpectCase{
            0,
            noteOn,
            sizeof(noteOn),
            expectedStickyCable0,
            sizeof(expectedStickyCable0),
            "encode note cable 0 sticky"});
}

bool testEncodeReturnToCable0HasF5(EmagicCableMapper& mapper)
{
    const uint8_t noteOn[] = {kNoteOnStatus, kMiddleC, kNoteVelocity};
    const uint8_t expectedReturnCable0[] = {
        kEmagicPortSwitch,
        0x01,
        kNoteOnStatus,
        kMiddleC,
        kNoteVelocity,
        kEmagicEndOfValidData};
    return encodeAndExpect(
        mapper,
        EncodeExpectCase{
            0,
            noteOn,
            sizeof(noteOn),
            expectedReturnCable0,
            sizeof(expectedReturnCable0),
            "encode note cable 0 after other out"});
}

bool testEncodeOutCables(EmagicCableMapper& mapper)
{
    if (!testEncodeFirstCable0HasF5(mapper) || !testEncodeStickyCable0OmitsF5(mapper))
    {
        return false;
    }
    for (uint8_t cable = 1; cable <= 3; ++cable)
    {
        if (!testEncodeSwitchedNote(mapper, cable))
        {
            return false;
        }
    }
    return testEncodeReturnToCable0HasF5(mapper);
}

bool testEncodeControlChange(EmagicCableMapper& mapper)
{
    const uint8_t cc[] = {kControlChangeStatus, kCcModulation, kCcValue};
    const uint8_t expectedCable0[] = {
        kEmagicPortSwitch,
        0x01,
        kControlChangeStatus,
        kCcModulation,
        kCcValue,
        kEmagicEndOfValidData};
    if (!encodeAndExpect(
            mapper,
            EncodeExpectCase{
                0, cc, sizeof(cc), expectedCable0, sizeof(expectedCable0), "encode CC cable 0"}))
    {
        return false;
    }

    const uint8_t expectedCable2[] = {
        kEmagicPortSwitch,
        0x03,
        kControlChangeStatus,
        kCcModulation,
        kCcValue,
        kEmagicEndOfValidData};
    return encodeAndExpect(
        mapper,
        EncodeExpectCase{
            2,
            cc,
            sizeof(cc),
            expectedCable2,
            sizeof(expectedCable2),
            "encode CC cable 2"});
}

bool testDecodeSynthetic(EmagicCableMapper& mapper)
{
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

bool testDecodeControlChange(EmagicCableMapper& mapper)
{
    const uint8_t bulk[] = {
        kEmagicPortSwitch,
        0x02,
        kControlChangeStatus,
        kCcModulation,
        kCcValue,
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
        std::cerr << "Mapper CC decode failed: " << error << '\n';
        return false;
    }
    if (seenCables.size() != 1 || seenCables[0] != 1 || seenMidi[0].size() != 3)
    {
        std::cerr << "Mapper CC decode routed unexpected cable/MIDI\n";
        return false;
    }
    const uint8_t expectedMidi[] = {kControlChangeStatus, kCcModulation, kCcValue};
    return expectBytes(
        ByteSpan{seenMidi[0].data(), seenMidi[0].size()},
        ByteSpan{expectedMidi, sizeof(expectedMidi)},
        "decode CC");
}

bool testDecodeSplitF5(EmagicCableMapper& mapper)
{
    const uint8_t firstChunk[] = {kNoteOnStatus, kMiddleC, kEmagicPortSwitch};
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
} // namespace

int runMapperTests()
{
    const DeviceProfile* mt4 = findDeviceProfile(kEmagicVendorId, kMt4ProductId);
    if (mt4 == nullptr)
    {
        std::cerr << "MT4 DeviceProfile not found for mapper tests\n";
        return 1;
    }

    EmagicCableMapper encodeMapper(*mt4);
    EmagicCableMapper ccEncodeMapper(*mt4);
    EmagicCableMapper decodeMapper(*mt4);
    EmagicCableMapper ccDecodeMapper(*mt4);
    EmagicCableMapper splitMapper(*mt4);
    if (!testEncodeOutCables(encodeMapper) || !testEncodeControlChange(ccEncodeMapper)
        || !testDecodeSynthetic(decodeMapper) || !testDecodeControlChange(ccDecodeMapper)
        || !testDecodeSplitF5(splitMapper) || !runFramerTests())
    {
        return 1;
    }

    std::cout << "Mapper synthetic tests passed\n";
    return 0;
}
