// Multi-unit MT4 session host helpers (AD-4 / AD-6 / AD-9).

#pragma once

#include "Device/DeviceSession.h"
#include "Device/UnitIdentityRegistry.h"
#include "Midi/MidiBackend.h"
#include "Midi/MidiBackendSelect.h"
#include "Profile/DeviceProfile.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

struct LiveUnitSession
{
    std::string identityKey;
    UnitIdentityKind identityKind = UnitIdentityKind::Topology;
    std::string devicePathUtf8;
    unsigned unitOrdinalK = 0;
    PortNameSet names;
    std::unique_ptr<MidiBackend> midiBackend;
    std::unique_ptr<DeviceSession> session;
};

struct MultiUnitHostContext
{
    const DeviceProfile* profile = nullptr;
    UnitIdentityRegistry* registry = nullptr;
    const std::string* registryPath = nullptr;
    bool allowZadigFallback = false;
};

bool loadOrCreateUnitRegistry(UnitIdentityRegistry& registry, std::string& pathOut);
bool startAllPresentUnits(MultiUnitHostContext& ctx, std::vector<LiveUnitSession>& liveOut);
void stopAllLiveUnits(std::vector<LiveUnitSession>& live) noexcept;
void printMultiUnitSessionBanner(std::size_t unitCount);

// recoverHotPlug=true keeps reconciling Absent/Present per unit.
// Returns 0 on Ctrl+C cancel, 1 on fatal disconnect without recovery path.
int runMultiUnitSessionLoop(
    MultiUnitHostContext& ctx,
    std::vector<LiveUnitSession>& live,
    bool recoverHotPlug,
    std::atomic<bool>& cancelRequested);
