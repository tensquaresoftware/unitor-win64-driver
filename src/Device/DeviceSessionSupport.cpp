#include "Device/DeviceSessionSupport.h"

#include <sstream>

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
