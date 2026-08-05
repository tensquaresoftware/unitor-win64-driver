// MTC quarter-frame / full-frame encode/decode vectors for EmagicCableMapper smoke.

#include "Protocol/EmagicMapperSmokeSupport.h"

#include "Protocol/EmagicCableMapper.h"

#include <cstdint>
#include <string>
#include <vector>

namespace
{
constexpr uint8_t kMtcQuarterFrame = 0xF1;
constexpr uint8_t kSysexStart = 0xF0;
constexpr uint8_t kSysexEnd = 0xF7;
constexpr uint8_t kMtcFullFrameDeviceId = 0x7F;
constexpr uint8_t kMtcFullFrameSubId1 = 0x01;
constexpr uint8_t kMtcFullFrameSubId2 = 0x01;

struct ByteSpan
{
    const uint8_t* bytes = nullptr;
    std::size_t size = 0;
};

struct EncodeCase
{
    uint8_t cable = 0;
    ByteSpan midi{};
    ByteSpan expected{};
    const char* label = "";
};

struct DecodeCase
{
    ByteSpan bulk{};
    ByteSpan expectedMidi{};
    const char* label = "";
};

bool bytesEqual(const ByteSpan& actual, const ByteSpan& expected) noexcept
{
    if (actual.size != expected.size)
    {
        return false;
    }
    for (std::size_t index = 0; index < expected.size; ++index)
    {
        if (actual.bytes[index] != expected.bytes[index])
        {
            return false;
        }
    }
    return true;
}

bool encodeSpanAndExpect(
    const DeviceProfile& profile,
    const EncodeCase& testCase,
    std::ostream& err)
{
    EmagicCableMapper mapper(profile);
    uint8_t outBuffer[32] = {};
    std::string error;
    EncodeRequest request{testCase.cable, testCase.midi.bytes, testCase.midi.size};
    EncodeBuffer buffer{outBuffer, sizeof(outBuffer), 0};
    if (!mapper.EncodeToDevice(request, buffer, error))
    {
        err << "Mapper encode failed (" << testCase.label << "): " << error << '\n';
        return false;
    }
    if (!bytesEqual(ByteSpan{outBuffer, buffer.size}, testCase.expected))
    {
        err << "Mapper test failed (" << testCase.label << "): byte mismatch\n";
        return false;
    }
    return true;
}

bool decodeBulkAndExpect(
    const DeviceProfile& profile,
    const DecodeCase& testCase,
    std::ostream& err)
{
    EmagicCableMapper mapper(profile);
    std::vector<uint8_t> seenCables;
    std::vector<std::vector<uint8_t>> seenMidi;
    std::string error;

    const bool ok = mapper.DecodeFromDevice(
        testCase.bulk.bytes,
        testCase.bulk.size,
        [&](uint8_t cableIndex, const uint8_t* midi, std::size_t n) {
            seenCables.push_back(cableIndex);
            seenMidi.emplace_back(midi, midi + n);
        },
        error);

    if (!ok)
    {
        err << "Mapper " << testCase.label << " decode failed: " << error << '\n';
        return false;
    }
    if (seenCables.size() != 1 || seenCables[0] != 0
        || seenMidi[0].size() != testCase.expectedMidi.size)
    {
        err << "Mapper " << testCase.label << " decode routed unexpected cable/MIDI\n";
        return false;
    }
    if (!bytesEqual(
            ByteSpan{seenMidi[0].data(), seenMidi[0].size()},
            testCase.expectedMidi))
    {
        err << "Mapper test failed (decode " << testCase.label << "): byte mismatch\n";
        return false;
    }
    return true;
}

bool encodeQuarterFrame(const DeviceProfile& profile, std::ostream& err)
{
    const uint8_t midi[] = {kMtcQuarterFrame, 0x23};
    const uint8_t expected[] = {
        kEmagicPortSwitch,
        0x02,
        kMtcQuarterFrame,
        0x23,
        kEmagicEndOfValidData};
    return encodeSpanAndExpect(
        profile,
        EncodeCase{
            1,
            ByteSpan{midi, sizeof(midi)},
            ByteSpan{expected, sizeof(expected)},
            "encode MTC quarter-frame cable 1"},
        err);
}

bool encodeFullFrame(const DeviceProfile& profile, std::ostream& err)
{
    const uint8_t midi[] = {
        kSysexStart,
        kMtcFullFrameDeviceId,
        kMtcFullFrameDeviceId,
        kMtcFullFrameSubId1,
        kMtcFullFrameSubId2,
        0x20,
        0x15,
        0x30,
        0x10,
        kSysexEnd};
    const uint8_t expected[] = {
        kEmagicPortSwitch,
        0x02,
        kSysexStart,
        kMtcFullFrameDeviceId,
        kMtcFullFrameDeviceId,
        kMtcFullFrameSubId1,
        kMtcFullFrameSubId2,
        0x20,
        0x15,
        0x30,
        0x10,
        kSysexEnd,
        kEmagicEndOfValidData};
    return encodeSpanAndExpect(
        profile,
        EncodeCase{
            1,
            ByteSpan{midi, sizeof(midi)},
            ByteSpan{expected, sizeof(expected)},
            "encode MTC full-frame cable 1"},
        err);
}

bool decodeQuarterFrame(const DeviceProfile& profile, std::ostream& err)
{
    const uint8_t bulk[] = {
        kEmagicPortSwitch,
        0x01,
        kMtcQuarterFrame,
        0x17,
        kEmagicEndOfValidData};
    const uint8_t expected[] = {kMtcQuarterFrame, 0x17};
    return decodeBulkAndExpect(
        profile,
        DecodeCase{
            ByteSpan{bulk, sizeof(bulk)},
            ByteSpan{expected, sizeof(expected)},
            "MTC quarter-frame"},
        err);
}

bool decodeFullFrame(const DeviceProfile& profile, std::ostream& err)
{
    const uint8_t bulk[] = {
        kEmagicPortSwitch,
        0x01,
        kSysexStart,
        kMtcFullFrameDeviceId,
        kMtcFullFrameDeviceId,
        kMtcFullFrameSubId1,
        kMtcFullFrameSubId2,
        0x10,
        0x20,
        0x30,
        0x05,
        kSysexEnd,
        kEmagicEndOfValidData};
    const uint8_t expected[] = {
        kSysexStart,
        kMtcFullFrameDeviceId,
        kMtcFullFrameDeviceId,
        kMtcFullFrameSubId1,
        kMtcFullFrameSubId2,
        0x10,
        0x20,
        0x30,
        0x05,
        kSysexEnd};
    return decodeBulkAndExpect(
        profile,
        DecodeCase{
            ByteSpan{bulk, sizeof(bulk)},
            ByteSpan{expected, sizeof(expected)},
            "MTC full-frame"},
        err);
}
} // namespace

bool runEmagicMapperSmokeEncodeMtc(const DeviceProfile& profile, std::ostream& err)
{
    return encodeQuarterFrame(profile, err) && encodeFullFrame(profile, err);
}

bool runEmagicMapperSmokeDecodeMtc(const DeviceProfile& profile, std::ostream& err)
{
    return decodeQuarterFrame(profile, err) && decodeFullFrame(profile, err);
}
