// Windows-only helpers for VirtualMidiBackend (UTF-8 names, diagnostics).

#pragma once

#ifdef _WIN32

#include "Midi/MidiBackend.h"
#include "Midi/TeVirtualMidiApi.h"

#include <string>

std::string formatVirtualMidiLastError(const char* action);

bool utf8ToWideVirtualMidiName(
    const std::string& utf8,
    std::wstring& wideOut,
    std::string& errorOut);

bool validateVirtualMidiPortNameSet(const PortNameSet& names, std::string& errorOut);

inline constexpr const char* kVirtualMidiMissingDriverFixPath =
    "VirtualMIDI driver/DLL missing (ERROR_PATH_NOT_FOUND-class). "
    "Install loopMIDI or rtpMIDI so the VirtualMIDI driver is present, "
    "then retry.";

#endif // _WIN32
