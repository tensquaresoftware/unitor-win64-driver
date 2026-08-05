// Librarian-sized SysEx opaque encode/decode vectors for EmagicCableMapper smoke.

#include "Protocol/EmagicMapperSmokeSupport.h"

#include "Protocol/EmagicCableMapper.h"

#include <cstdint>
#include <string>
#include <vector>

namespace
{
constexpr uint8_t kSysexStart = 0xF0;
constexpr uint8_t kSysexEnd = 0xF7;
constexpr uint8_t kOberheimId = 0x10;
constexpr uint8_t kMatrixDevice = 0x06;
constexpr uint8_t kPatchOpcode = 0x01;
constexpr uint8_t kMasterOpcode = 0x03;
constexpr std::size_t kPatchFrameBytes = 275;
constexpr std::size_t kMasterFrameBytes = 351;
constexpr std::size_t kEncodeScratchCapacity = 512;

struct OpaqueCase
{
    uint8_t cable = 0;
    const std::vector<uint8_t>* midi = nullptr;
    const char* label = "";
};

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

std::size_t midiOffsetAfterOptionalF5(const uint8_t* bulk, std::size_t bulkSize) noexcept
{
    if (bulkSize >= 2 && bulk[0] == kEmagicPortSwitch)
    {
        return 2;
    }
    return 0;
}

bool encodeOpaqueCarry(const DeviceProfile& profile, const OpaqueCase& testCase, std::ostream& err)
{
    EmagicCableMapper mapper(profile);
    uint8_t outBuffer[kEncodeScratchCapacity] = {};
    std::string error;
    EncodeRequest request{testCase.cable, testCase.midi->data(), testCase.midi->size()};
    EncodeBuffer buffer{outBuffer, sizeof(outBuffer), 0};
    if (!mapper.EncodeToDevice(request, buffer, error))
    {
        err << "Mapper encode failed (" << testCase.label << "): " << error << '\n';
        return false;
    }

    const std::size_t midiOffset = midiOffsetAfterOptionalF5(outBuffer, buffer.size);
    const std::size_t need = midiOffset + testCase.midi->size() + 1;
    if (buffer.size < need
        || !bytesEqual(
            outBuffer + midiOffset,
            testCase.midi->size(),
            testCase.midi->data(),
            testCase.midi->size())
        || outBuffer[midiOffset + testCase.midi->size()] != kEmagicEndOfValidData)
    {
        err << "Mapper encode rewrote or truncated SysEx (" << testCase.label << ")\n";
        return false;
    }
    return true;
}

bool decodeOpaqueCarry(const DeviceProfile& profile, const OpaqueCase& testCase, std::ostream& err)
{
    EmagicCableMapper mapper(profile);
    std::vector<uint8_t> bulk;
    bulk.push_back(kEmagicPortSwitch);
    bulk.push_back(0x01);
    bulk.insert(bulk.end(), testCase.midi->begin(), testCase.midi->end());
    bulk.push_back(kEmagicEndOfValidData);

    std::vector<std::vector<uint8_t>> seenMidi;
    std::string error;
    const bool ok = mapper.DecodeFromDevice(
        bulk.data(),
        bulk.size(),
        [&](uint8_t, const uint8_t* bytes, std::size_t n) {
            seenMidi.emplace_back(bytes, bytes + n);
        },
        error);

    if (!ok || seenMidi.size() != 1
        || !bytesEqual(
            seenMidi[0].data(),
            seenMidi[0].size(),
            testCase.midi->data(),
            testCase.midi->size()))
    {
        err << "Mapper decode rewrote or truncated SysEx (" << testCase.label << "): " << error
            << '\n';
        return false;
    }
    return true;
}

bool runOpaqueCases(
    const DeviceProfile& profile,
    bool (*runner)(const DeviceProfile&, const OpaqueCase&, std::ostream&),
    std::ostream& err)
{
    const std::vector<uint8_t> inquiry = {kSysexStart, 0x7E, 0x7F, 0x06, 0x01, kSysexEnd};
    const std::vector<uint8_t> patch = makeOberheimShapedFrame(kPatchFrameBytes, kPatchOpcode);
    const std::vector<uint8_t> master = makeOberheimShapedFrame(kMasterFrameBytes, kMasterOpcode);
    const OpaqueCase cases[] = {
        {1, &inquiry, "Device Inquiry"},
        {1, &patch, "275 B patch-shaped"},
        {2, &master, "351 B master-shaped"}};
    for (const OpaqueCase& testCase : cases)
    {
        if (!runner(profile, testCase, err))
        {
            return false;
        }
    }
    return true;
}
} // namespace

bool runEmagicMapperSmokeEncodeSysex(const DeviceProfile& profile, std::ostream& err)
{
    return runOpaqueCases(profile, encodeOpaqueCarry, err);
}

bool runEmagicMapperSmokeDecodeSysex(const DeviceProfile& profile, std::ostream& err)
{
    return runOpaqueCases(profile, decodeOpaqueCarry, err);
}
