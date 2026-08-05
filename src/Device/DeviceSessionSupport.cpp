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
