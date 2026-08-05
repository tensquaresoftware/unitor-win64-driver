// Windows-only helpers for VirtualMidiBackend (UTF-8 names, diagnostics).

#include "Midi/VirtualMidiWinSupport.h"

#ifdef _WIN32

#include "Midi/MidiBackend.h"

#include <sstream>

namespace
{
constexpr const char* kMissingDriverFixPath = kVirtualMidiMissingDriverFixPath;

bool isBlankPortName(const std::string& name)
{
    return name.find_first_not_of(" \t\r\n") == std::string::npos;
}
} // namespace

std::string formatVirtualMidiLastError(const char* action)
{
    const DWORD code = GetLastError();
    std::ostringstream stream;
    stream << action << " failed (Win32=" << code << ")";
    if (code == ERROR_PATH_NOT_FOUND || code == ERROR_MOD_NOT_FOUND)
    {
        stream << ": " << kMissingDriverFixPath;
    }
    return stream.str();
}

bool utf8ToWideVirtualMidiName(
    const std::string& utf8,
    std::wstring& wideOut,
    std::string& errorOut)
{
    if (utf8.empty())
    {
        errorOut = "VirtualMIDI port name must not be empty";
        return false;
    }

    const int needed = MultiByteToWideChar(
        CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (needed <= 0)
    {
        errorOut = formatVirtualMidiLastError("UTF-8 to wide conversion size query");
        return false;
    }

    wideOut.assign(static_cast<std::size_t>(needed), L'\0');
    const int written = MultiByteToWideChar(
        CP_UTF8, 0, utf8.c_str(), -1, wideOut.data(), needed);
    if (written <= 0)
    {
        errorOut = formatVirtualMidiLastError("UTF-8 to wide conversion");
        return false;
    }

    if (!wideOut.empty() && wideOut.back() == L'\0')
    {
        wideOut.pop_back();
    }
    return true;
}

bool validateVirtualMidiPortNameSet(const PortNameSet& names, std::string& errorOut)
{
    if (names.inCount == 0 && names.outCount == 0)
    {
        errorOut = "VirtualMIDI CreatePortSet rejected empty PortNameSet (fail closed)";
        return false;
    }
    if (names.inCount > kMaxMidiBackendInPorts || names.outCount > kMaxMidiBackendOutPorts)
    {
        errorOut = "VirtualMIDI CreatePortSet port counts exceed backend limits";
        return false;
    }
    for (std::size_t index = 0; index < names.inCount; ++index)
    {
        if (isBlankPortName(names.inNames[index]))
        {
            errorOut = "VirtualMIDI CreatePortSet rejected blank IN port display name";
            return false;
        }
    }
    for (std::size_t index = 0; index < names.outCount; ++index)
    {
        if (isBlankPortName(names.outNames[index]))
        {
            errorOut = "VirtualMIDI CreatePortSet rejected blank OUT port display name";
            return false;
        }
    }
    return true;
}

#endif // _WIN32
