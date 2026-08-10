#include "App/MidiSessionMultiHost.h"

#include "App/MidiSessionDiagnostics.h"
#include "App/MidiSessionMultiHostDetail.h"
#include "App/Mt4WinUsbPresence.h"
#include "Device/DeviceSessionManager.h"

#include <iostream>
#include <unordered_map>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace mt4_multi
{
const char* identityKindLabel(UnitIdentityKind kind)
{
    return kind == UnitIdentityKind::Serial ? "serial" : "topology";
}

std::string identityMapKey(UnitIdentityKind kind, const std::string& key)
{
    return std::string(identityKindLabel(kind)) + '|' + key;
}

void printUnitDiagnostics(const LiveUnitSession& unit)
{
    std::cout << "Unit K=" << unit.unitOrdinalK
              << " identity=" << identityKindLabel(unit.identityKind)
              << " key=\"" << unit.identityKey << "\"\n";
    printExpectedPortDiagnostics(unit.names);
}

bool persistRegistry(const UnitIdentityRegistry& registry, const std::string& path)
{
    std::string saveError;
    if (!registry.saveToFile(path, saveError))
    {
        std::cerr << saveError << '\n';
        return false;
    }
    return true;
}

bool buildNamesForK(
    const DeviceProfile& profile,
    unsigned unitOrdinalK,
    PortNameSet& namesOut)
{
    DeviceSessionManager manager;
    std::string error;
    if (!manager.buildPortNameSet(profile, unitOrdinalK, namesOut, error))
    {
        std::cerr << "Port name build failed: " << error << '\n';
        return false;
    }
    return true;
}

void stopLiveUnit(LiveUnitSession& unit) noexcept
{
    if (unit.session)
    {
        unit.session->Stop();
        unit.session.reset();
    }
    unit.midiBackend.reset();
}

bool startLiveUnit(
    LiveUnitSession& unit,
    const DeviceProfile& profile,
    bool allowZadigFallback,
    std::string& errorOut)
{
    unit.midiBackend = std::make_unique<VirtualMidiBackend>();
    unit.session = std::make_unique<DeviceSession>();
    DeviceSessionStartRequest request;
    request.profile = &profile;
    request.midiBackend = unit.midiBackend.get();
    request.portNames = &unit.names;
    request.openOptions.allowZadigFallback = allowZadigFallback;
    request.openOptions.selectedDevicePath = unit.devicePathUtf8;
    if (!unit.session->Start(request, errorOut) || !unit.session->IsRunning())
    {
        if (errorOut.empty())
        {
            errorOut = "unknown error";
        }
        unit.session->Stop();
        unit.session.reset();
        unit.midiBackend.reset();
        return false;
    }
    return true;
}

struct ResolveNameArgs
{
    const Mt4WinUsbInterfaceInfo* iface = nullptr;
    const DeviceProfile* profile = nullptr;
    UnitIdentityRegistry* registry = nullptr;
    LiveUnitSession* unitOut = nullptr;
};

bool resolveAndNameUnit(const ResolveNameArgs& args, std::string& errorOut)
{
    if (args.iface == nullptr || args.profile == nullptr || args.registry == nullptr
        || args.unitOut == nullptr)
    {
        errorOut = "resolveAndNameUnit requires iface, profile, registry, unit";
        return false;
    }
    UnitIdentityResolveRequest resolve;
    resolve.kind = args.iface->identityKind;
    resolve.key = &args.iface->identityKey;
    resolve.topologyKey = &args.iface->topologyKey;
    unsigned k = 0;
    if (!args.registry->resolveOrAssign(resolve, k, errorOut))
    {
        return false;
    }
    args.unitOut->identityKey = args.iface->identityKey;
    args.unitOut->identityKind = args.iface->identityKind;
    args.unitOut->devicePathUtf8 = args.iface->devicePathUtf8;
    args.unitOut->unitOrdinalK = k;
    if (!buildNamesForK(*args.profile, k, args.unitOut->names))
    {
        errorOut = "failed to build PortNameSet";
        return false;
    }
    return true;
}

bool startResolvedUnit(
    MultiUnitHostContext& ctx,
    const Mt4WinUsbInterfaceInfo& iface,
    LiveUnitSession& unitOut,
    const char* failPrefix)
{
    std::string error;
    ResolveNameArgs resolve{&iface, ctx.profile, ctx.registry, &unitOut};
    if (!resolveAndNameUnit(resolve, error))
    {
        std::cerr << "Unit identity resolve failed: " << error << '\n';
        return false;
    }
    if (!startLiveUnit(unitOut, *ctx.profile, ctx.allowZadigFallback, error))
    {
        std::cerr << failPrefix << " (K=" << unitOut.unitOrdinalK << "): " << error
                  << '\n';
        return false;
    }
    return true;
}

bool indexPresentInterfaces(
    const std::vector<Mt4WinUsbInterfaceInfo>& interfaces,
    std::unordered_map<std::string, const Mt4WinUsbInterfaceInfo*>& presentByKey,
    std::string& errorOut)
{
    presentByKey.clear();
    for (const Mt4WinUsbInterfaceInfo& iface : interfaces)
    {
        const std::string key = identityMapKey(iface.identityKind, iface.identityKey);
        if (!presentByKey.emplace(key, &iface).second)
        {
            errorOut =
                "Duplicate MT4 identity key among present interfaces (refusing ambiguous "
                "multi-unit map): "
                + key;
            return false;
        }
    }
    errorOut.clear();
    return true;
}

bool quarantineCorruptRegistry(const std::string& path, const std::string& loadError)
{
    const std::string quarantinePath = path + ".corrupt";
#ifdef _WIN32
    if (MoveFileExA(path.c_str(), quarantinePath.c_str(), MOVEFILE_REPLACE_EXISTING) == 0)
    {
        std::cerr << loadError << '\n';
        std::cerr << "Unit identity registry quarantine failed; starting with empty map\n";
        return true;
    }
#else
    (void)path;
#endif
    std::cerr << loadError << '\n';
    std::cerr << "Unit identity registry unreadable; quarantined to \"" << quarantinePath
              << "\" and starting with an empty map (K may be reassigned)\n";
    return true;
}
} // namespace mt4_multi

bool loadOrCreateUnitRegistry(UnitIdentityRegistry& registry, std::string& pathOut)
{
    std::string pathError;
    pathOut = UnitIdentityRegistry::defaultPersistencePath(pathError);
    if (pathOut.empty())
    {
        std::cerr << pathError << '\n';
        return false;
    }
    std::string loadError;
    if (!registry.loadFromFile(pathOut, loadError))
    {
        mt4_multi::quarantineCorruptRegistry(pathOut, loadError);
        registry.clear();
        return true;
    }
    return true;
}

bool startAllPresentUnits(MultiUnitHostContext& ctx, std::vector<LiveUnitSession>& liveOut)
{
    liveOut.clear();
    std::vector<Mt4WinUsbInterfaceInfo> interfaces;
    std::string listError;
    if (!listMt4WinUsbInterfaces(interfaces, listError) || interfaces.empty())
    {
        std::cerr << (listError.empty() ? "No present MT4 WinUSB interfaces to start"
                                        : listError)
                  << '\n';
        return false;
    }
    std::unordered_map<std::string, const Mt4WinUsbInterfaceInfo*> presentByKey;
    std::string dupError;
    if (!mt4_multi::indexPresentInterfaces(interfaces, presentByKey, dupError))
    {
        std::cerr << dupError << '\n';
        return false;
    }
    for (const Mt4WinUsbInterfaceInfo& iface : interfaces)
    {
        LiveUnitSession unit;
        if (!mt4_multi::startResolvedUnit(ctx, iface, unit, "DeviceSession start failed"))
        {
            stopAllLiveUnits(liveOut);
            return false;
        }
        mt4_multi::printUnitDiagnostics(unit);
        liveOut.push_back(std::move(unit));
    }
    if (!mt4_multi::persistRegistry(*ctx.registry, *ctx.registryPath))
    {
        stopAllLiveUnits(liveOut);
        return false;
    }
    return true;
}

void stopAllLiveUnits(std::vector<LiveUnitSession>& live) noexcept
{
    for (LiveUnitSession& unit : live)
    {
        mt4_multi::stopLiveUnit(unit);
    }
    live.clear();
}

void printMultiUnitSessionBanner(std::size_t unitCount)
{
    std::cout << "DeviceSession started for " << unitCount
              << " MT4 unit(s) with Virtual Ports\n";
    std::cout << "MIDI I/O running - notes/CC smoke ready (Ctrl+C to stop)\n";
    std::cout << "device-host counters will print in this window on USB IN activity"
                 " (same thread as this message)\n";
    std::cout << "Device Inquiry lab: watch inquiry_out vs identity_reply_in"
                 " (expect them to stay close)\n";
}
