#include "App/MidiSessionMultiHost.h"

#include "App/MidiSessionDiagnostics.h"
#include "App/MidiSessionMultiHostDetail.h"
#include "App/Mt4PresenceWait.h"
#include "App/Mt4WinUsbPresence.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <unordered_set>

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

void stopMissingUnits(
    const std::unordered_map<std::string, const Mt4WinUsbInterfaceInfo*>& presentByKey,
    std::vector<LiveUnitSession>& live)
{
    for (std::size_t index = 0; index < live.size();)
    {
        const std::string key =
            identityMapKey(live[index].identityKind, live[index].identityKey);
        if (presentByKey.find(key) == presentByKey.end())
        {
            std::cout << "MT4 unit K=" << live[index].unitOrdinalK
                      << " disconnected; tearing down that unit only\n";
            stopLiveUnit(live[index]);
            live.erase(live.begin() + static_cast<std::ptrdiff_t>(index));
            continue;
        }
        ++index;
    }
}

bool shouldAttemptHotPlugStart(
    const std::string& key,
    std::unordered_map<std::string, SteadyClock::time_point>& lastAttemptByKey)
{
    const Mt4PresenceWaitConfig hotPlug = makeHotPlugReplugPresenceWaitConfig();
    const auto now = SteadyClock::now();
    const auto found = lastAttemptByKey.find(key);
    if (found != lastAttemptByKey.end()
        && now - found->second < std::chrono::milliseconds(hotPlug.pollIntervalMs))
    {
        return false;
    }
    lastAttemptByKey[key] = now;
    return true;
}

struct StartMissingArgs
{
    MultiUnitHostContext* ctx = nullptr;
    const std::vector<Mt4WinUsbInterfaceInfo>* interfaces = nullptr;
    std::vector<LiveUnitSession>* live = nullptr;
    std::unordered_map<std::string, SteadyClock::time_point>* lastStartAttempt = nullptr;
};

bool tryStartOneMissingUnit(const StartMissingArgs& args, const Mt4WinUsbInterfaceInfo& iface)
{
    const std::string key = identityMapKey(iface.identityKind, iface.identityKey);
    if (!shouldAttemptHotPlugStart(key, *args.lastStartAttempt))
    {
        return false;
    }
    LiveUnitSession unit;
    if (!startResolvedUnit(*args.ctx, iface, unit, "Hot-plug DeviceSession start failed"))
    {
        std::cerr << "Hot-plug recovery: retrying Start later for identity " << key
                  << " (peer units keep running)\n";
        return false;
    }
    args.lastStartAttempt->erase(key);
    std::cout << "Hot-plug recovery: new DeviceSession started for K="
              << unit.unitOrdinalK << " (AD-6 ordinal preserved)\n";
    printUnitDiagnostics(unit);
    args.live->push_back(std::move(unit));
    return true;
}

bool startMissingUnitsSoft(const StartMissingArgs& args)
{
    if (args.ctx == nullptr || args.interfaces == nullptr || args.live == nullptr
        || args.lastStartAttempt == nullptr)
    {
        return false;
    }
    bool startedAny = false;
    std::unordered_set<std::string> liveKeys;
    for (const LiveUnitSession& unit : *args.live)
    {
        liveKeys.insert(identityMapKey(unit.identityKind, unit.identityKey));
    }
    for (const Mt4WinUsbInterfaceInfo& iface : *args.interfaces)
    {
        const std::string key = identityMapKey(iface.identityKind, iface.identityKey);
        if (liveKeys.count(key) != 0)
        {
            continue;
        }
        if (tryStartOneMissingUnit(args, iface))
        {
            startedAny = true;
        }
    }
    return startedAny;
}

void reconcileLiveUnits(
    MultiUnitHostContext& ctx,
    std::vector<LiveUnitSession>& live,
    std::unordered_map<std::string, SteadyClock::time_point>& lastStartAttempt)
{
    std::vector<Mt4WinUsbInterfaceInfo> interfaces;
    std::string listError;
    if (!listMt4WinUsbInterfaces(interfaces, listError))
    {
        std::cerr << "MT4 interface list failed (keeping live units): " << listError
                  << '\n';
        return;
    }
    std::unordered_map<std::string, const Mt4WinUsbInterfaceInfo*> presentByKey;
    std::string dupError;
    if (!indexPresentInterfaces(interfaces, presentByKey, dupError))
    {
        std::cerr << dupError << " (keeping live units)\n";
        return;
    }
    stopMissingUnits(presentByKey, live);
    const StartMissingArgs startArgs{&ctx, &interfaces, &live, &lastStartAttempt};
    if (startMissingUnitsSoft(startArgs) && !persistRegistry(*ctx.registry, *ctx.registryPath))
    {
        std::cerr << "Unit identity registry save failed after hot-plug start "
                     "(peer units keep running)\n";
    }
}

void dropFailedUnits(
    std::vector<LiveUnitSession>& live,
    std::unordered_map<std::string, DeviceHostCounterSnapshot>& lastCounters,
    std::unordered_map<std::string, SteadyClock::time_point>& lastHeartbeat)
{
    for (std::size_t index = 0; index < live.size();)
    {
        LiveUnitSession& unit = live[index];
        const std::string key = identityMapKey(unit.identityKind, unit.identityKey);
        if (!pollOneUnitOnce(unit, lastCounters[key], lastHeartbeat[key]))
        {
            stopLiveUnit(unit);
            live.erase(live.begin() + static_cast<std::ptrdiff_t>(index));
            continue;
        }
        ++index;
    }
}

void handleEmptyLiveSet(
    MultiUnitHostContext& ctx,
    std::vector<LiveUnitSession>& live,
    std::unordered_map<std::string, SteadyClock::time_point>& lastStartAttempt,
    bool recoverHotPlug)
{
    std::cout << "MIDI I/O stopped (no live units)\n";
    if (!recoverHotPlug)
    {
        return;
    }
    reconcileLiveUnits(ctx, live, lastStartAttempt);
    if (!live.empty())
    {
        printMultiUnitSessionBanner(live.size());
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
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
} // namespace mt4_multi

int runMultiUnitSessionLoop(
    MultiUnitHostContext& ctx,
    std::vector<LiveUnitSession>& live,
    bool recoverHotPlug,
    std::atomic<bool>& cancelRequested)
{
    using SteadyClock = std::chrono::steady_clock;
    std::unordered_map<std::string, DeviceHostCounterSnapshot> lastCounters;
    std::unordered_map<std::string, SteadyClock::time_point> lastHeartbeat;
    std::unordered_map<std::string, SteadyClock::time_point> lastStartAttempt;
    auto lastReconcile = SteadyClock::now();
    while (!cancelRequested.load())
    {
        mt4_multi::dropFailedUnits(live, lastCounters, lastHeartbeat);
        if (recoverHotPlug
            && SteadyClock::now() - lastReconcile >= std::chrono::milliseconds(500))
        {
            lastReconcile = SteadyClock::now();
            mt4_multi::reconcileLiveUnits(ctx, live, lastStartAttempt);
        }
        if (live.empty() && !recoverHotPlug)
        {
            return 1;
        }
        if (live.empty())
        {
            mt4_multi::handleEmptyLiveSet(ctx, live, lastStartAttempt, recoverHotPlug);
            if (live.empty())
            {
                continue;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    mt4_multi::printCancelCounters(live);
    stopAllLiveUnits(live);
    std::cout << "MIDI I/O stopped\n";
    return 0;
}
