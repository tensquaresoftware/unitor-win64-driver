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

struct MergedVirtualMidiPlan
{
    const std::string* name = nullptr;
    DWORD flags = 0;
    int inIndex = -1;
    int outIndex = -1;
};

inline constexpr std::size_t kMaxMergedVirtualMidiPlans =
    kMaxMidiBackendInPorts + kMaxMidiBackendOutPorts;

bool buildMergedVirtualMidiPlans(
    const PortNameSet& names,
    MergedVirtualMidiPlan* plans,
    std::size_t& planCount,
    std::string& errorOut);

inline constexpr const char* kVirtualMidiMissingDriverFixPath =
    "VirtualMIDI driver/DLL missing (ERROR_PATH_NOT_FOUND-class). "
    "Install loopMIDI or rtpMIDI so the VirtualMIDI driver is present, "
    "then retry.";

#endif // _WIN32
