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

bool namesCollideAcrossDirections(const PortNameSet& names)
{
    for (std::size_t inIndex = 0; inIndex < names.inCount; ++inIndex)
    {
        for (std::size_t outIndex = 0; outIndex < names.outCount; ++outIndex)
        {
            if (names.inNames[inIndex] == names.outNames[outIndex])
            {
                return true;
            }
        }
    }
    return false;
}

bool namesCollideWithinDirection(
    const std::string* names,
    std::size_t count)
{
    for (std::size_t left = 0; left < count; ++left)
    {
        for (std::size_t right = left + 1; right < count; ++right)
        {
            if (names[left] == names[right])
            {
                return true;
            }
        }
    }
    return false;
}

bool rejectBlankPortNames(
    const std::string* names,
    std::size_t count,
    const char* blankError,
    std::string& errorOut)
{
    for (std::size_t index = 0; index < count; ++index)
    {
        if (isBlankPortName(names[index]))
        {
            errorOut = blankError;
            return false;
        }
    }
    return true;
}

bool rejectNonUniquePortNames(const PortNameSet& names, std::string& errorOut)
{
    if (namesCollideWithinDirection(names.inNames, names.inCount)
        || namesCollideWithinDirection(names.outNames, names.outCount)
        || namesCollideAcrossDirections(names))
    {
        errorOut =
            "VirtualMIDI CreatePortSet rejected non-unique Input/Output display names "
            "(directional faces must not share a teVirtualMIDI alias)";
        return false;
    }
    return true;
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
    else if (code == ERROR_ALIAS_EXISTS)
    {
        stream << ": a VirtualMIDI port with this display name already exists "
                  "(close a leftover Bridge session — MT4 In/Out or old "
                  "MT4 Port N zombies — or conflicting loopMIDI entries, then retry)";
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
    return rejectBlankPortNames(
               names.inNames,
               names.inCount,
               "VirtualMIDI CreatePortSet rejected blank IN port display name",
               errorOut)
        && rejectBlankPortNames(
               names.outNames,
               names.outCount,
               "VirtualMIDI CreatePortSet rejected blank OUT port display name",
               errorOut)
        && rejectNonUniquePortNames(names, errorOut);
}

#endif // _WIN32
