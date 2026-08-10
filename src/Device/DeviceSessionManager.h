// Naming authority for unit ordinal K and Virtual Port display strings (AD-6).
// MidiBackend must never invent K or re-format these names.

#pragma once

#include "Midi/MidiBackend.h"
#include "Profile/DeviceProfile.h"

#include <string>

// Directional Virtual Port face (host ← DIN vs host → DIN).
enum class MidiPortDirection
{
    In,
    Out,
};

// Sole formatting SSOT for Virtual Port display names.
// Unit 1: "MT4 In N" / "MT4 Out N"; unit K>=2: "MT4 #K In N" / "MT4 #K Out N".
// Matches MT4 chassis silkscreen (In/Out), not the longer Input/Output words.
std::string formatPortDisplayName(
    unsigned unitOrdinalK,
    unsigned portN,
    MidiPortDirection direction);

class DeviceSessionManager
{
public:
    DeviceSessionManager() = default;

    // Build ready-made PortNameSet from Profile product port counts (2 IN / 4 OUT for MT4).
    // unitOrdinalK comes from UnitIdentityRegistry (AD-6); never invent K here from USB.
    bool buildPortNameSet(
        const DeviceProfile& profile,
        unsigned unitOrdinalK,
        PortNameSet& namesOut,
        std::string& errorOut) const;
};
