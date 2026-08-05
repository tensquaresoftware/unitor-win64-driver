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
