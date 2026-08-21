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
namespace
{
constexpr int kMissingPollsBeforeTeardown = 2;
} // namespace

struct ReconcileLiveArgs
{
    MultiUnitHostContext* ctx = nullptr;
    const std::vector<Mt4WinUsbInterfaceInfo>* interfaces = nullptr;
    std::vector<LiveUnitSession>* live = nullptr;
    HotPlugLoopState* hotPlug = nullptr;
};

// Returns true when the live entry at index was erased.
bool reconcileOneLiveUnit(ReconcileLiveArgs& args, std::size_t index)
{
    LiveUnitSession& unit = (*args.live)[index];
    const std::string key = identityMapKey(unit.identityKind, unit.identityKey);
    const Mt4WinUsbInterfaceInfo* matched =
        findMatchingPresentIface(unit, *args.interfaces, *args.ctx->registry);
    if (matched != nullptr)
    {
        args.hotPlug->consecutiveMissingPolls.erase(key);
        if (matched->devicePathUtf8 != unit.devicePathUtf8)
        {
            if (!restartLiveUnitOnPathChange(*args.ctx, unit, *matched))
            {
                markNeedsAbsentSettle(*args.hotPlug, unit);
                stopLiveUnit(unit);
                args.live->erase(args.live->begin() + static_cast<std::ptrdiff_t>(index));
                return true;
            }
        }
        else if (matched->identityKey != unit.identityKey
            || matched->identityKind != unit.identityKind)
        {
            adoptIdentityMigration(unit, *matched);
        }
        return false;
    }

    if (++args.hotPlug->consecutiveMissingPolls[key] < kMissingPollsBeforeTeardown)
    {
        return false;
    }
    args.hotPlug->consecutiveMissingPolls.erase(key);
    std::cout << "MT4 unit K=" << unit.unitOrdinalK
              << " disconnected; tearing down that unit only\n";
    markNeedsAbsentSettle(*args.hotPlug, unit);
    stopLiveUnit(unit);
    args.live->erase(args.live->begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

void stopMissingUnits(
    MultiUnitHostContext& ctx,
    const std::vector<Mt4WinUsbInterfaceInfo>& interfaces,
    std::vector<LiveUnitSession>& live,
    HotPlugLoopState& hotPlug)
{
    ReconcileLiveArgs args{&ctx, &interfaces, &live, &hotPlug};
    for (std::size_t index = 0; index < live.size();)
    {
        if (reconcileOneLiveUnit(args, index))
        {
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
    HotPlugLoopState* hotPlug = nullptr;
};

bool tryStartOneMissingUnit(const StartMissingArgs& args, const Mt4WinUsbInterfaceInfo& iface)
{
    const std::string key = identityMapKey(iface.identityKind, iface.identityKey);
    if (args.hotPlug->needsAbsentBeforeStart.count(key) != 0)
    {
        return false;
    }
    if (!shouldAttemptHotPlugStart(key, args.hotPlug->lastStartAttempt))
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
    args.hotPlug->lastStartAttempt.erase(key);
    std::cout << "Hot-plug recovery: new DeviceSession started for K="
              << unit.unitOrdinalK << " (AD-6 ordinal preserved)\n";
    printUnitDiagnostics(unit);
    args.live->push_back(std::move(unit));
    return true;
}

bool ifaceAlreadyHosted(
    const StartMissingArgs& args,
    const Mt4WinUsbInterfaceInfo& iface,
    const std::unordered_set<unsigned>& liveOrdinals)
{
    unsigned presentK = 0;
    if (tryLookupIfaceK(*args.ctx->registry, iface, presentK)
        && liveOrdinals.count(presentK) != 0)
    {
        return true;
    }
    const std::string key = identityMapKey(iface.identityKind, iface.identityKey);
    for (const LiveUnitSession& unit : *args.live)
    {
        if (identityMapKey(unit.identityKind, unit.identityKey) == key)
        {
            return true;
        }
    }
    return false;
}

bool startMissingUnitsSoft(const StartMissingArgs& args)
{
    if (args.ctx == nullptr || args.interfaces == nullptr || args.live == nullptr
        || args.hotPlug == nullptr)
    {
        return false;
    }
    bool startedAny = false;
    std::unordered_set<unsigned> liveOrdinals;
    for (const LiveUnitSession& unit : *args.live)
    {
        liveOrdinals.insert(unit.unitOrdinalK);
    }
    for (const Mt4WinUsbInterfaceInfo& iface : *args.interfaces)
    {
        if (ifaceAlreadyHosted(args, iface, liveOrdinals))
        {
            continue;
        }
        if (tryStartOneMissingUnit(args, iface))
        {
            startedAny = true;
            liveOrdinals.insert(args.live->back().unitOrdinalK);
        }
    }
    return startedAny;
}

bool persistAfterHotPlugStarts(
    MultiUnitHostContext& ctx,
    std::vector<LiveUnitSession>& live,
    std::size_t liveCountBeforeStart)
{
    if (persistRegistry(*ctx.registry, *ctx.registryPath))
    {
        return true;
    }
    std::cerr << "Unit identity registry save failed after hot-plug start; retrying\n";
    if (persistRegistry(*ctx.registry, *ctx.registryPath))
    {
        return true;
    }
    std::cerr << "Unit identity registry save failed again; stopping units started "
                 "in this hot-plug pass (peers that were already live keep running)\n";
    while (live.size() > liveCountBeforeStart)
    {
        stopLiveUnit(live.back());
        live.pop_back();
    }
    return false;
}

void reconcileLiveUnits(
    MultiUnitHostContext& ctx,
    std::vector<LiveUnitSession>& live,
    HotPlugLoopState& hotPlug)
{
    std::vector<Mt4WinUsbInterfaceInfo> interfaces;
    std::string listError;
    if (!listMt4WinUsbInterfaces(interfaces, listError, ctx.allowZadigFallback))
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
    clearAbsentSettleWhenGone(hotPlug, presentByKey);
    stopMissingUnits(ctx, interfaces, live, hotPlug);
    const std::size_t liveBefore = live.size();
    const StartMissingArgs startArgs{&ctx, &interfaces, &live, &hotPlug};
    if (startMissingUnitsSoft(startArgs))
    {
        (void)persistAfterHotPlugStarts(ctx, live, liveBefore);
    }
}

bool runInitialReplugWait(
    EmptyLiveHandleArgs& args,
    const Mt4PresenceWaitConfig& waitConfig)
{
    args.hotPlug->emptySince = SteadyClock::now();
    std::cout << "MT4 disconnected; waiting for replug...\n";
    if (!waitForMt4WinUsbReplugOrTimeout(waitConfig, *args.cancelRequested))
    {
        if (!args.cancelRequested->load())
        {
            std::cerr << "Hot-plug recovery timed out or failed after "
                      << waitConfig.timeoutSeconds
                      << "s (fail closed; peer sessions were already down)\n";
        }
        return false;
    }
    args.hotPlug->completedReplugWait = true;
    return true;
}

bool continueEmptySoftRetry(
    EmptyLiveHandleArgs& args,
    const Mt4PresenceWaitConfig& waitConfig)
{
    if (SteadyClock::now() - args.hotPlug->emptySince
        >= std::chrono::seconds(waitConfig.timeoutSeconds))
    {
        std::cerr << "Hot-plug recovery timed out after " << waitConfig.timeoutSeconds
                  << "s waiting for a healthy DeviceSession Start (fail closed)\n";
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(waitConfig.pollIntervalMs));
    return true;
}

bool prepareEmptyLiveRecovery(EmptyLiveHandleArgs& args)
{
    const Mt4PresenceWaitConfig waitConfig = makeHotPlugReplugPresenceWaitConfig();
    if (!args.hotPlug->completedReplugWait)
    {
        return runInitialReplugWait(args, waitConfig);
    }
    return continueEmptySoftRetry(args, waitConfig);
}

bool handleEmptyLiveSet(EmptyLiveHandleArgs& args)
{
    std::cout << "MIDI I/O stopped (no live units)\n";
    if (!args.recoverHotPlug)
    {
        return true;
    }
    if (!prepareEmptyLiveRecovery(args))
    {
        return args.cancelRequested->load();
    }
    if (args.cancelRequested->load())
    {
        return true;
    }
    reconcileLiveUnits(*args.ctx, *args.live, *args.hotPlug);
    if (!args.live->empty())
    {
        args.hotPlug->completedReplugWait = false;
        printMultiUnitSessionBanner(args.live->size());
    }
    return true;
}

} // namespace mt4_multi

struct MultiUnitLoopState
{
    MultiUnitHostContext* ctx = nullptr;
    std::vector<LiveUnitSession>* live = nullptr;
    mt4_multi::HotPlugLoopState hotPlug;
    bool recoverHotPlug = false;
    std::unordered_map<std::string, DeviceHostCounterSnapshot> lastCounters;
    std::unordered_map<std::string, mt4_multi::SteadyClock::time_point> lastHeartbeat;
    mt4_multi::SteadyClock::time_point lastReconcile = mt4_multi::SteadyClock::now();
    std::atomic<bool>* cancelRequested = nullptr;
};

int pollMultiUnitOnce(MultiUnitLoopState& state)
{
    mt4_multi::dropFailedUnits(
        *state.live, state.hotPlug, state.lastCounters, state.lastHeartbeat);
    if (state.recoverHotPlug
        && mt4_multi::SteadyClock::now() - state.lastReconcile
            >= std::chrono::milliseconds(500))
    {
        state.lastReconcile = mt4_multi::SteadyClock::now();
        if (!state.live->empty())
        {
            mt4_multi::reconcileLiveUnits(*state.ctx, *state.live, state.hotPlug);
        }
    }
    if (state.live->empty() && !state.recoverHotPlug)
    {
        return 1;
    }
    if (state.live->empty())
    {
        mt4_multi::EmptyLiveHandleArgs emptyArgs{state.ctx,
            state.live,
            &state.hotPlug,
            state.cancelRequested,
            state.recoverHotPlug};
        if (!mt4_multi::handleEmptyLiveSet(emptyArgs))
        {
            return 1;
        }
    }
    return -1; // continue loop
}

int runMultiUnitSessionLoop(
    MultiUnitHostContext& ctx,
    std::vector<LiveUnitSession>& live,
    bool recoverHotPlug,
    std::atomic<bool>& cancelRequested)
{
    MultiUnitLoopState state;
    state.ctx = &ctx;
    state.live = &live;
    state.recoverHotPlug = recoverHotPlug;
    state.cancelRequested = &cancelRequested;
    while (!cancelRequested.load())
    {
        const int step = pollMultiUnitOnce(state);
        if (step >= 0)
        {
            return step;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    mt4_multi::printCancelCounters(live);
    stopAllLiveUnits(live);
    std::cout << "MIDI I/O stopped\n";
    return 0;
}
