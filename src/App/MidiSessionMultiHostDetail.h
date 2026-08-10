// Internal multi-unit host helpers shared by MidiSessionMultiHost*.cpp (not public API).

#pragma once

#include "App/MidiSessionDiagnostics.h"
#include "App/MidiSessionMultiHost.h"
#include "App/Mt4WinUsbPresence.h"

#include <atomic>
#include <chrono>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mt4_multi
{
using SteadyClock = std::chrono::steady_clock;

struct HotPlugLoopState
{
    std::unordered_map<std::string, SteadyClock::time_point> lastStartAttempt;
    // After pump/surprise failure, wait until this identity leaves the present set
    // before retrying Start (clears stale Present).
    std::unordered_set<std::string> needsAbsentBeforeStart;
    // Debounce SetupAPI blips before tearing down a still-plugged unit.
    std::unordered_map<std::string, int> consecutiveMissingPolls;
    // After a full Absent→Present wait, soft-retry Start until the same 900s budget.
    bool completedReplugWait = false;
    SteadyClock::time_point emptySince{};
};

std::string identityMapKey(UnitIdentityKind kind, const std::string& key);
void printUnitDiagnostics(const LiveUnitSession& unit);
bool persistRegistry(const UnitIdentityRegistry& registry, const std::string& path);
void stopLiveUnit(LiveUnitSession& unit) noexcept;
bool startResolvedUnit(
    MultiUnitHostContext& ctx,
    const Mt4WinUsbInterfaceInfo& iface,
    LiveUnitSession& unitOut,
    const char* failPrefix);
bool indexPresentInterfaces(
    const std::vector<Mt4WinUsbInterfaceInfo>& interfaces,
    std::unordered_map<std::string, const Mt4WinUsbInterfaceInfo*>& presentByKey,
    std::string& errorOut);
bool pollOneUnitOnce(
    LiveUnitSession& unit,
    DeviceHostCounterSnapshot& lastCounters,
    SteadyClock::time_point& lastHeartbeat);
bool tryLookupIfaceK(
    const UnitIdentityRegistry& registry,
    const Mt4WinUsbInterfaceInfo& iface,
    unsigned& kOut);
const Mt4WinUsbInterfaceInfo* findMatchingPresentIface(
    const LiveUnitSession& unit,
    const std::vector<Mt4WinUsbInterfaceInfo>& interfaces,
    const UnitIdentityRegistry& registry);
void markNeedsAbsentSettle(HotPlugLoopState& hotPlug, const LiveUnitSession& unit);
void clearAbsentSettleWhenGone(
    HotPlugLoopState& hotPlug,
    const std::unordered_map<std::string, const Mt4WinUsbInterfaceInfo*>& presentByKey);
bool restartLiveUnitOnPathChange(
    MultiUnitHostContext& ctx,
    LiveUnitSession& unit,
    const Mt4WinUsbInterfaceInfo& iface);
void adoptIdentityMigration(LiveUnitSession& unit, const Mt4WinUsbInterfaceInfo& iface);

void reconcileLiveUnits(
    MultiUnitHostContext& ctx,
    std::vector<LiveUnitSession>& live,
    HotPlugLoopState& hotPlug);
void dropFailedUnits(
    std::vector<LiveUnitSession>& live,
    HotPlugLoopState& hotPlug,
    std::unordered_map<std::string, DeviceHostCounterSnapshot>& lastCounters,
    std::unordered_map<std::string, SteadyClock::time_point>& lastHeartbeat);

struct EmptyLiveHandleArgs
{
    MultiUnitHostContext* ctx = nullptr;
    std::vector<LiveUnitSession>* live = nullptr;
    HotPlugLoopState* hotPlug = nullptr;
    std::atomic<bool>* cancelRequested = nullptr;
    bool recoverHotPlug = false;
};

// Returns false on hot-plug fail-closed timeout/error (caller should exit 1).
// Cancel during wait returns true with live still empty (caller exits 0 on cancel).
bool handleEmptyLiveSet(EmptyLiveHandleArgs& args);
void printCancelCounters(std::vector<LiveUnitSession>& live);
} // namespace mt4_multi
