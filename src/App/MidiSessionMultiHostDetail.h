// Internal multi-unit host helpers shared by MidiSessionMultiHost*.cpp (not public API).

#pragma once

#include "App/MidiSessionDiagnostics.h"
#include "App/MidiSessionMultiHost.h"
#include "App/Mt4WinUsbPresence.h"

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

namespace mt4_multi
{
using SteadyClock = std::chrono::steady_clock;

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
void reconcileLiveUnits(
    MultiUnitHostContext& ctx,
    std::vector<LiveUnitSession>& live,
    std::unordered_map<std::string, SteadyClock::time_point>& lastStartAttempt);
void dropFailedUnits(
    std::vector<LiveUnitSession>& live,
    std::unordered_map<std::string, DeviceHostCounterSnapshot>& lastCounters,
    std::unordered_map<std::string, SteadyClock::time_point>& lastHeartbeat);
void handleEmptyLiveSet(
    MultiUnitHostContext& ctx,
    std::vector<LiveUnitSession>& live,
    std::unordered_map<std::string, SteadyClock::time_point>& lastStartAttempt,
    bool recoverHotPlug);
void printCancelCounters(std::vector<LiveUnitSession>& live);
} // namespace mt4_multi
