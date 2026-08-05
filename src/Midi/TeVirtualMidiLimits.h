// teVirtualMIDI create-port / SendToHost SysEx ceiling (platform-neutral).

#pragma once

#include <cstddef>

// Matches teVirtualMIDI default maxSysexLength used at port create.
inline constexpr std::size_t kTeVmDefaultMaxSysexLengthSize = 65535;

inline bool exceedsTeVmDefaultMaxSysexLength(std::size_t byteCount) noexcept
{
    return byteCount > kTeVmDefaultMaxSysexLengthSize;
}
