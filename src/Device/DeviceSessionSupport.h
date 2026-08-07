// Shared DeviceSession helpers (English diagnostics).

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

std::string formatPortCableFailure(
    const char* direction,
    std::size_t portIndex,
    uint8_t cableIndex,
    const std::string& detail);

// Universal Device Inquiry: F0 7E xx 06 01 F7 (any device ID / channel byte).
bool isUniversalDeviceInquiry(const uint8_t* midiBytes, std::size_t byteCount) noexcept;

// Identity Reply: F0 7E xx 06 02 ...
bool isIdentityReply(const uint8_t* midiBytes, std::size_t byteCount) noexcept;
