#include "Device/DeviceSessionSupport.h"

#include <cstdio>
#include <iostream>
#include <sstream>
#include <vector>

std::string formatPortCableFailure(
    const char* direction,
    std::size_t portIndex,
    uint8_t cableIndex,
    const std::string& detail)
{
    std::ostringstream stream;
    stream << direction << " failed on Port " << (portIndex + 1)
           << " (cable index " << static_cast<unsigned>(cableIndex) << "): " << detail;
    return stream.str();
}

bool isUniversalDeviceInquiry(const uint8_t* midiBytes, std::size_t byteCount) noexcept
{
    // F0 7E <channel> 06 01 F7
    if (midiBytes == nullptr || byteCount < 6)
    {
        return false;
    }
    return midiBytes[0] == 0xF0 && midiBytes[1] == 0x7E && midiBytes[3] == 0x06
        && midiBytes[4] == 0x01 && midiBytes[byteCount - 1] == 0xF7;
}

bool isIdentityReply(const uint8_t* midiBytes, std::size_t byteCount) noexcept
{
    // F0 7E <channel> 06 02 ...
    if (midiBytes == nullptr || byteCount < 5)
    {
        return false;
    }
    return midiBytes[0] == 0xF0 && midiBytes[1] == 0x7E && midiBytes[3] == 0x06
        && midiBytes[4] == 0x02;
}

std::string formatMidiBytesHex(const uint8_t* midiBytes, std::size_t byteCount)
{
    if (midiBytes == nullptr || byteCount == 0)
    {
        return {};
    }

    // Lab dumps stay short; Identity Reply is 15 bytes — cap avoids log floods.
    constexpr std::size_t kMaxHexBytes = 64;
    const std::size_t shown = (byteCount < kMaxHexBytes) ? byteCount : kMaxHexBytes;

    std::string out;
    out.reserve(shown * 3 + 16);
    char token[4] = {};
    for (std::size_t index = 0; index < shown; ++index)
    {
        if (index > 0)
        {
            out.push_back(' ');
        }
        std::snprintf(
            token,
            sizeof(token),
            "%02X",
            static_cast<unsigned>(midiBytes[index]));
        out.append(token);
    }
    if (byteCount > kMaxHexBytes)
    {
        out.append(" ...");
    }
    return out;
}

void logLongSysexSendToHost(
    std::size_t inPortIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount,
    std::size_t deliverHighWater)
{
    if (byteCount < 512 || midiBytes == nullptr || midiBytes[0] != 0xF0
        || midiBytes[byteCount - 1] != 0xF7)
    {
        return;
    }
    const std::size_t edge = (byteCount < 4) ? byteCount : 4;
    std::cerr << "device→host: long SysEx SendToHost ok (in_port=" << (inPortIndex + 1)
              << " bytes=" << byteCount << " head=" << formatMidiBytesHex(midiBytes, edge)
              << " tail=" << formatMidiBytesHex(midiBytes + byteCount - edge, edge)
              << " deliver_hw=" << deliverHighWater << ")\n"
              << std::flush;
}

void logDeviceInquiryHostOut(const DeviceInquiryHostOutDiag& diag)
{
    std::cerr << "host→device: Device Inquiry WriteBulk ok (out_port="
              << (diag.outPortIndex + 1) << " midi_bytes=" << diag.midiBytes
              << " encoded_bytes=" << diag.encodedBytes
              << " f5_switch=" << (diag.includedF5 ? "yes" : "no")
              << " ring_active=" << (diag.ringActive ? "yes" : "no")
              << " ms_since_ring_arm=" << diag.msSinceRing
              << " pending_urbs=" << diag.pendingUrbs << "/" << diag.urbSlotCount
              << " first_after_start=" << (diag.firstAfterStart ? "yes" : "no") << ")\n"
              << std::flush;
}

MidiPushView maybePrependLostLeadingF0(
    bool armRepair,
    const uint8_t* midiBytes,
    std::size_t byteCount,
    std::vector<uint8_t>& repairStorage)
{
    // Matrix manuf id follows a lost F0: body starts 10 06 …
    if (!armRepair || midiBytes == nullptr || byteCount < 2 || midiBytes[0] != 0x10
        || midiBytes[1] != 0x06)
    {
        return {midiBytes, byteCount};
    }
    repairStorage.clear();
    repairStorage.push_back(0xF0);
    repairStorage.insert(repairStorage.end(), midiBytes, midiBytes + byteCount);
    std::cerr << "SysEx leading-F0 repair: prepended F0 (span_len=" << byteCount
              << " head_was=10 06)\n"
              << std::flush;
    return {repairStorage.data(), repairStorage.size()};
}

void logFirstBurstSpan(const FirstBurstDiag& diag)
{
    if (diag.midiBytes == nullptr || diag.byteCount == 0)
    {
        return;
    }
    std::cerr << "first-burst IN: cable=" << static_cast<unsigned>(diag.cableIndex)
              << " span_len=" << diag.byteCount
              << " head=" << formatMidiBytesHex(diag.midiBytes, diag.byteCount)
              << " has_f0=" << ((diag.midiBytes[0] == 0xF0) ? "yes" : "no")
              << " holding=" << (diag.holding ? "yes" : "no")
              << " held=" << diag.heldSize << " pending_urbs=" << diag.pendingUrbs
              << "\n"
              << std::flush;
}

bool isMatrixDumpReply(const uint8_t* midiBytes, std::size_t byteCount) noexcept
{
    if (midiBytes == nullptr || byteCount < 5 || midiBytes[0] != 0xF0)
    {
        return false;
    }
    return midiBytes[1] == 0x10 && midiBytes[2] == 0x06
        && (midiBytes[3] == 0x01 || midiBytes[3] == 0x03);
}

bool isExactMatrixDumpLength(std::size_t byteCount) noexcept
{
    return byteCount == 275 || byteCount == 351;
}

bool isMatrixDumpRequest(const uint8_t* midiBytes, std::size_t byteCount) noexcept
{
    return midiBytes != nullptr && byteCount == 7 && midiBytes[0] == 0xF0
        && midiBytes[1] == 0x10 && midiBytes[2] == 0x06 && midiBytes[3] == 0x04
        && midiBytes[6] == 0xF7;
}
