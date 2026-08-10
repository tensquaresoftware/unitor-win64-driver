#include "App/MidiSessionMultiHostDetail.h"

#include "App/MidiSessionDiagnostics.h"

#include <iostream>

namespace mt4_multi
{
bool pollOneUnitOnce(
    LiveUnitSession& unit,
    DeviceHostCounterSnapshot& lastCounters,
    SteadyClock::time_point& lastHeartbeat)
{
    if (!unit.session)
    {
        return false;
    }
    std::string pumpError;
    if (unit.session->TakePumpFailure(pumpError))
    {
        printDeviceHostCounters(unit.session->CopyDeviceHostCounters());
        std::cerr << "MIDI I/O pump failed (K=" << unit.unitOrdinalK << "): "
                  << pumpError << '\n';
        return false;
    }
    if (!unit.session->IsRunning())
    {
        printDeviceHostCounters(unit.session->CopyDeviceHostCounters());
        std::cerr << "MIDI I/O session ended unexpectedly (K=" << unit.unitOrdinalK
                  << ")\n";
        return false;
    }

    const DeviceHostCounterSnapshot counters = unit.session->CopyDeviceHostCounters();
    const bool dueHeartbeat =
        SteadyClock::now() - lastHeartbeat >= std::chrono::seconds(3);
    if (shouldPrintDeviceHostCounters(lastCounters, counters) || dueHeartbeat)
    {
        std::cout << "[K=" << unit.unitOrdinalK << "] ";
        printDeviceHostCounters(counters);
        lastCounters = counters;
        lastHeartbeat = SteadyClock::now();
    }
    return true;
}

void dropFailedUnits(
    std::vector<LiveUnitSession>& live,
    HotPlugLoopState& hotPlug,
    std::unordered_map<std::string, DeviceHostCounterSnapshot>& lastCounters,
    std::unordered_map<std::string, SteadyClock::time_point>& lastHeartbeat)
{
    for (std::size_t index = 0; index < live.size();)
    {
        LiveUnitSession& unit = live[index];
        const std::string key = identityMapKey(unit.identityKind, unit.identityKey);
        if (!pollOneUnitOnce(unit, lastCounters[key], lastHeartbeat[key]))
        {
            markNeedsAbsentSettle(hotPlug, unit);
            stopLiveUnit(unit);
            live.erase(live.begin() + static_cast<std::ptrdiff_t>(index));
            continue;
        }
        ++index;
    }
}

void printCancelCounters(std::vector<LiveUnitSession>& live)
{
    for (LiveUnitSession& unit : live)
    {
        if (unit.session)
        {
            printDeviceHostCounters(unit.session->CopyDeviceHostCounters());
        }
    }
}

bool tryLookupIfaceK(
    const UnitIdentityRegistry& registry,
    const Mt4WinUsbInterfaceInfo& iface,
    unsigned& kOut)
{
    if (registry.tryLookup(iface.identityKind, iface.identityKey, kOut))
    {
        return true;
    }
    if (!iface.topologyKey.empty())
    {
        return registry.tryLookup(UnitIdentityKind::Topology, iface.topologyKey, kOut);
    }
    return false;
}

const Mt4WinUsbInterfaceInfo* findMatchingPresentIface(
    const LiveUnitSession& unit,
    const std::vector<Mt4WinUsbInterfaceInfo>& interfaces,
    const UnitIdentityRegistry& registry)
{
    const std::string liveKey = identityMapKey(unit.identityKind, unit.identityKey);
    for (const Mt4WinUsbInterfaceInfo& iface : interfaces)
    {
        if (identityMapKey(iface.identityKind, iface.identityKey) == liveKey)
        {
            return &iface;
        }
    }
    for (const Mt4WinUsbInterfaceInfo& iface : interfaces)
    {
        unsigned presentK = 0;
        if (tryLookupIfaceK(registry, iface, presentK) && presentK == unit.unitOrdinalK)
        {
            return &iface;
        }
    }
    return nullptr;
}

void markNeedsAbsentSettle(HotPlugLoopState& hotPlug, const LiveUnitSession& unit)
{
    hotPlug.needsAbsentBeforeStart.insert(
        identityMapKey(unit.identityKind, unit.identityKey));
}

void clearAbsentSettleWhenGone(
    HotPlugLoopState& hotPlug,
    const std::unordered_map<std::string, const Mt4WinUsbInterfaceInfo*>& presentByKey)
{
    for (auto it = hotPlug.needsAbsentBeforeStart.begin();
         it != hotPlug.needsAbsentBeforeStart.end();)
    {
        if (presentByKey.find(*it) == presentByKey.end())
        {
            it = hotPlug.needsAbsentBeforeStart.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

bool restartLiveUnitOnPathChange(
    MultiUnitHostContext& ctx,
    LiveUnitSession& unit,
    const Mt4WinUsbInterfaceInfo& iface)
{
    std::cout << "MT4 unit K=" << unit.unitOrdinalK
              << " device path changed; restarting that unit only\n";
    stopLiveUnit(unit);
    LiveUnitSession restarted;
    if (!startResolvedUnit(ctx, iface, restarted, "Hot-plug path-change Start failed"))
    {
        return false;
    }
    unit = std::move(restarted);
    printUnitDiagnostics(unit);
    return true;
}

void adoptIdentityMigration(LiveUnitSession& unit, const Mt4WinUsbInterfaceInfo& iface)
{
    unit.identityKey = iface.identityKey;
    unit.identityKind = iface.identityKind;
}
} // namespace mt4_multi
