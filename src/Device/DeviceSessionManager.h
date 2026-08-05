// Naming authority for unit ordinal K and AD-5 Virtual Port display strings (AD-6).
// MidiBackend must never invent K or re-format these names.

#pragma once

#include "Midi/MidiBackend.h"
#include "Profile/DeviceProfile.h"

#include <string>

// Sole formatting SSOT for Virtual Port display names.
// K == 1 → "MT4 Port N"; K >= 2 → "MT4 #K Port N".
std::string formatPortDisplayName(unsigned unitOrdinalK, unsigned portN);

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
