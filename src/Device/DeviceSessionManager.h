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
// Unit 1: "MT4 Input N" / "MT4 Output N"; unit K>=2: "MT4 #K Input N" / "MT4 #K Output N".
std::string formatPortDisplayName(
    unsigned unitOrdinalK,
    unsigned portN,
    MidiPortDirection direction);

class DeviceSessionManager
{
public:
    DeviceSessionManager() = default;

    // V1 hard-wires single-unit K = 1 (multi-unit persistence = Story 3.4).
    unsigned unitOrdinalK() const noexcept { return unitOrdinalK_; }

    // Build ready-made PortNameSet from Profile product port counts (2 IN / 4 OUT for MT4).
    bool buildPortNameSet(
        const DeviceProfile& profile,
        PortNameSet& namesOut,
        std::string& errorOut) const;

private:
    unsigned unitOrdinalK_ = 1;
};
