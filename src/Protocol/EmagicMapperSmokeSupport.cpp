// Shared synthetic EmagicCableMapper encode/decode checks (CLI smoke + Catch2).

#include "Protocol/EmagicMapperSmokeSupport.h"

#include "Protocol/EmagicCableMapper.h"

#include <cstdint>
#include <cstdio>
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

bool expectBytes(
    const ByteSpan& actual,
    const ByteSpan& expected,
    const char* label,
    std::ostream& err)
{
    if (actual.size != expected.size)
    {
        err << "Mapper test failed (" << label << "): size mismatch (actual "
            << actual.size << ", expected " << expected.size << ")\n";
        return false;
    }
    for (std::size_t index = 0; index < expected.size; ++index)
    {
        if (actual.bytes[index] != expected.bytes[index])
        {
            err << "Mapper test failed (" << label << "): byte mismatch at index "
                << index << "\n";
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

bool encodeAndExpect(
    EmagicCableMapper& mapper,
    const EncodeExpectCase& testCase,
    std::ostream& err)
{
    uint8_t outBuffer[32] = {};
    std::string error;

    EncodeRequest request{testCase.cable, testCase.midi, testCase.midiSize};
    EncodeBuffer buffer{outBuffer, sizeof(outBuffer), 0};
    if (!mapper.EncodeToDevice(request, buffer, error))
    {
        err << "Mapper encode failed (" << testCase.label << "): " << error << '\n';
        return false;
    }

    return expectBytes(
        ByteSpan{outBuffer, buffer.size},
        ByteSpan{testCase.expected, testCase.expectedSize},
        testCase.label,
        err);
}

bool testEncodeSwitchedNote(EmagicCableMapper& mapper, uint8_t cable, std::ostream& err)
{
    const uint8_t noteOn[] = {kNoteOnStatus, kMiddleC, kNoteVelocity};
    const uint8_t expected[] = {
        kEmagicPortSwitch,
        static_cast<uint8_t>((cable + 1) & 15),
        kNoteOnStatus,
        kMiddleC,
        kNoteVelocity,
        kEmagicEndOfValidData};
    char label[40] = {};
    std::snprintf(
        label,
        sizeof(label),
        "encode note switched cable %u",
        static_cast<unsigned>(cable));
    return encodeAndExpect(
        mapper,
        EncodeExpectCase{
            cable,
            noteOn,
            sizeof(noteOn),
            expected,
            sizeof(expected),
            label},
        err);
}

bool testEncodeFirstCable0HasF5(EmagicCableMapper& mapper, std::ostream& err)
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
            "encode note cable 0 first F5"},
        err);
}

bool testEncodeStickyCable0OmitsF5(EmagicCableMapper& mapper, std::ostream& err)
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
            "encode note cable 0 sticky"},
        err);
}

bool testEncodeReturnToCable0HasF5(EmagicCableMapper& mapper, std::ostream& err)
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
            "encode note cable 0 after other out"},
        err);
}

bool testEncodeOutCables(EmagicCableMapper& mapper, std::ostream& err)
{
    if (!testEncodeFirstCable0HasF5(mapper, err)
        || !testEncodeStickyCable0OmitsF5(mapper, err))
    {
        return false;
    }
    for (uint8_t cable = 1; cable <= 3; ++cable)
    {
        if (!testEncodeSwitchedNote(mapper, cable, err))
        {
            return false;
        }
    }
    return testEncodeReturnToCable0HasF5(mapper, err);
}

bool testEncodeControlChange(EmagicCableMapper& mapper, std::ostream& err)
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
                0, cc, sizeof(cc), expectedCable0, sizeof(expectedCable0), "encode CC cable 0"},
            err))
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
            "encode CC cable 2"},
        err);
}

bool testDecodeSynthetic(EmagicCableMapper& mapper, std::ostream& err)
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
        err << "Mapper decode failed: " << error << '\n';
        return false;
    }
    if (seenCables.size() != 1 || seenCables[0] != 1 || seenMidi[0].size() != 3)
    {
        err << "Mapper decode routed unexpected cable/MIDI\n";
        return false;
    }
    const uint8_t expectedMidi[] = {kNoteOnStatus, kMiddleC, kNoteVelocity};
    return expectBytes(
        ByteSpan{seenMidi[0].data(), seenMidi[0].size()},
        ByteSpan{expectedMidi, sizeof(expectedMidi)},
        "decode MIDI",
        err);
}

bool testDecodeControlChange(EmagicCableMapper& mapper, std::ostream& err)
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
        err << "Mapper CC decode failed: " << error << '\n';
        return false;
    }
    if (seenCables.size() != 1 || seenCables[0] != 1 || seenMidi[0].size() != 3)
    {
        err << "Mapper CC decode routed unexpected cable/MIDI\n";
        return false;
    }
    const uint8_t expectedMidi[] = {kControlChangeStatus, kCcModulation, kCcValue};
    return expectBytes(
        ByteSpan{seenMidi[0].data(), seenMidi[0].size()},
        ByteSpan{expectedMidi, sizeof(expectedMidi)},
        "decode CC",
        err);
}

bool testDecodeSplitF5(EmagicCableMapper& mapper, std::ostream& err)
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
        err << "Mapper split-F5 first decode failed: " << error << '\n';
        return false;
    }
    if (!mapper.DecodeFromDevice(secondChunk, sizeof(secondChunk), sink, error))
    {
        err << "Mapper split-F5 second decode failed: " << error << '\n';
        return false;
    }

    if (cables.size() != 2 || cables[0] != 0 || cables[1] != 1)
    {
        err << "Mapper split-F5 cable routing failed\n";
        return false;
    }
    if (sizes.size() != 2 || sizes[0] != 2 || sizes[1] != 1)
    {
        err << "Mapper split-F5 MIDI sizes failed\n";
        return false;
    }
    return true;
}
} // namespace

bool runEmagicMapperSmokeEncodeOutCables(const DeviceProfile& profile, std::ostream& err)
{
    EmagicCableMapper mapper(profile);
    return testEncodeOutCables(mapper, err);
}

bool runEmagicMapperSmokeEncodeControlChange(const DeviceProfile& profile, std::ostream& err)
{
    EmagicCableMapper mapper(profile);
    return testEncodeControlChange(mapper, err);
}

bool runEmagicMapperSmokeDecodeSynthetic(const DeviceProfile& profile, std::ostream& err)
{
    EmagicCableMapper mapper(profile);
    return testDecodeSynthetic(mapper, err);
}

bool runEmagicMapperSmokeDecodeControlChange(const DeviceProfile& profile, std::ostream& err)
{
    EmagicCableMapper mapper(profile);
    return testDecodeControlChange(mapper, err);
}

bool runEmagicMapperSmokeDecodeSplitF5(const DeviceProfile& profile, std::ostream& err)
{
    EmagicCableMapper mapper(profile);
    return testDecodeSplitF5(mapper, err);
}

bool runAllEmagicMapperSmokeTests(std::ostream& out, std::ostream& err)
{
    const DeviceProfile* mt4 = findDeviceProfile(kEmagicVendorId, kMt4ProductId);
    if (mt4 == nullptr)
    {
        err << "MT4 DeviceProfile not found for mapper tests\n";
        return false;
    }

    if (!runEmagicMapperSmokeEncodeOutCables(*mt4, err))
    {
        return false;
    }
    if (!runEmagicMapperSmokeEncodeControlChange(*mt4, err))
    {
        return false;
    }
    if (!runEmagicMapperSmokeDecodeSynthetic(*mt4, err))
    {
        return false;
    }
    if (!runEmagicMapperSmokeDecodeControlChange(*mt4, err))
    {
        return false;
    }
    if (!runEmagicMapperSmokeDecodeSplitF5(*mt4, err))
    {
        return false;
    }
    if (!runEmagicMapperSmokeEncodeClockTransport(*mt4, err))
    {
        return false;
    }
    if (!runEmagicMapperSmokeDecodeClockTransport(*mt4, err))
    {
        return false;
    }

    out << "Mapper synthetic tests passed\n";
    return true;
}
