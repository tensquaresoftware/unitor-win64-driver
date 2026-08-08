// Shared DeviceSession helpers (English diagnostics).

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

std::string formatPortCableFailure(
    const char* direction,
    std::size_t portIndex,
    uint8_t cableIndex,
    const std::string& detail);

// Universal Device Inquiry: F0 7E xx 06 01 F7 (any device ID / channel byte).
bool isUniversalDeviceInquiry(const uint8_t* midiBytes, std::size_t byteCount) noexcept;

// Identity Reply: F0 7E xx 06 02 ...
bool isIdentityReply(const uint8_t* midiBytes, std::size_t byteCount) noexcept;

// Compact uppercase hex for lab dumps (e.g. "F0 7E 00"); empty if no bytes.
std::string formatMidiBytesHex(const uint8_t* midiBytes, std::size_t byteCount);

void logLongSysexSendToHost(
    std::size_t inPortIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount,
    std::size_t deliverHighWater);

struct DeviceInquiryHostOutDiag
{
    std::size_t outPortIndex = 0;
    std::size_t midiBytes = 0;
    std::size_t encodedBytes = 0;
    bool includedF5 = false;
    bool ringActive = false;
    long long msSinceRing = -1;
    std::size_t pendingUrbs = 0;
    std::size_t urbSlotCount = 0;
    bool firstAfterStart = false;
};

void logDeviceInquiryHostOut(const DeviceInquiryHostOutDiag& diag);

struct MidiPushView
{
    const uint8_t* bytes = nullptr;
    std::size_t count = 0;
};

// Narrow Matrix dump guard: prepend F0 when the first IN URB dropped it.
// Under expect, Emagic demux may deliver a lone 0x10 span (then 0x06…) or a
// multi-byte span starting 10 06. repairStorage must outlive the returned view.
MidiPushView maybePrependLostLeadingF0(
    bool armRepair,
    const uint8_t* midiBytes,
    std::size_t byteCount,
    std::vector<uint8_t>& repairStorage);

struct FirstBurstDiag
{
    uint8_t cableIndex = 0;
    const uint8_t* midiBytes = nullptr;
    std::size_t byteCount = 0;
    bool holding = false;
    std::size_t heldSize = 0;
    std::size_t pendingUrbs = 0;
};

void logFirstBurstSpan(const FirstBurstDiag& diag);

// Matrix-Control dump reply: F0 10 06 01… (patch) or F0 10 06 03… (master).
bool isMatrixDumpReply(const uint8_t* midiBytes, std::size_t byteCount) noexcept;
bool isExactMatrixDumpLength(std::size_t byteCount) noexcept;
// Matrix dump request: F0 10 06 04 <type> <slot> F7 (7 bytes).
bool isMatrixDumpRequest(const uint8_t* midiBytes, std::size_t byteCount) noexcept;
