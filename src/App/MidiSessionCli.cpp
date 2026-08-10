// Long-running MT4 DeviceSession CLI (--start-session / --run-midi / --auto-session).

#include "App/MidiSessionCli.h"

#include "App/BridgeVersion.h"
#include "App/MidiSessionMultiHost.h"
#include "App/Mt4PresenceWait.h"
#include "Profile/DeviceProfile.h"

#include <atomic>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <csignal>
#endif

namespace
{
std::atomic<bool> g_cancelRequested{false};

#ifdef _WIN32
BOOL WINAPI onConsoleCtrl(DWORD controlType)
{
    if (controlType == CTRL_C_EVENT
        || controlType == CTRL_BREAK_EVENT
        || controlType == CTRL_CLOSE_EVENT)
    {
        g_cancelRequested.store(true);
        return TRUE;
    }
    return FALSE;
}

bool installCancelHandler()
{
    static bool installed = false;
    if (installed)
    {
        return true;
    }
    if (SetConsoleCtrlHandler(onConsoleCtrl, TRUE) != TRUE)
    {
        return false;
    }
    installed = true;
    return true;
}
#else
void onSignalCancel(int /*signal*/)
{
    g_cancelRequested.store(true);
}

bool installCancelHandler()
{
    static bool installed = false;
    if (installed)
    {
        return true;
    }
    std::signal(SIGINT, onSignalCancel);
    std::signal(SIGTERM, onSignalCancel);
    installed = true;
    return true;
}
#endif

bool armMidiSessionCancel(bool preserveCancel)
{
    if (!installCancelHandler())
    {
        std::cerr << "Failed to install Ctrl+C handler for MIDI session\n";
        return false;
    }
    if (preserveCancel)
    {
        if (g_cancelRequested.load())
        {
            std::cerr << "Auto-session cancelled before DeviceSession start\n";
            return false;
        }
        return true;
    }
    g_cancelRequested.store(false);
    return true;
}

void printBridgeVersionLine()
{
    std::cout << kBridgeProductName << " " << kBridgeVersionString << '\n';
}

void printAutoSessionHostBanner()
{
    printBridgeVersionLine();
    std::cout << "Auto-session host (user-session Bridge; not a Windows Service)\n";
    std::cout << "Prefer clean exit with Ctrl+C so Virtual Ports tear down "
                 "(closing the console window may leave orphan ports).\n";
    std::cout << "Hot-plug: mid-session unplug tears down that unit's ports only; "
                 "Bridge keeps peer units and starts a new DeviceSession for "
                 "returning identities under the same ordinal K (no Windows reboot). "
                 "Hosts may need a MIDI rescan; supervised Bridge restart is allowed.\n";
}

int runMt4SessionsHost(bool allowZadigFallback, bool preserveCancel, bool recoverHotPlug)
{
    const DeviceProfile* mt4 = findDeviceProfile(kEmagicVendorId, kMt4ProductId);
    if (mt4 == nullptr)
    {
        std::cerr << "MT4 DeviceProfile not found\n";
        return 1;
    }
    if (!armMidiSessionCancel(preserveCancel))
    {
        return 1;
    }

    UnitIdentityRegistry registry;
    std::string registryPath;
    if (!loadOrCreateUnitRegistry(registry, registryPath))
    {
        return 1;
    }

    MultiUnitHostContext ctx{mt4, &registry, &registryPath, allowZadigFallback};
    std::vector<LiveUnitSession> live;
    if (!startAllPresentUnits(ctx, live))
    {
        return 1;
    }

    printMultiUnitSessionBanner(live.size());
    return runMultiUnitSessionLoop(ctx, live, recoverHotPlug, g_cancelRequested);
}
} // namespace

int runMt4MidiSession(bool allowZadigFallback, bool preserveCancel)
{
    printBridgeVersionLine();
    return runMt4SessionsHost(allowZadigFallback, preserveCancel, false);
}

int runMt4AutoSession()
{
    printAutoSessionHostBanner();
    if (!installCancelHandler())
    {
        std::cerr << "Failed to install Ctrl+C handler for auto-session\n";
        return 1;
    }
    g_cancelRequested.store(false);

    if (!waitForMt4WinUsbOrTimeout(makeAutoSessionPresenceWaitConfig(), g_cancelRequested))
    {
        return 1;
    }
    if (g_cancelRequested.load())
    {
        std::cerr << "Auto-session cancelled after MT4 appeared\n";
        return 1;
    }

    return runMt4SessionsHost(false, true, true);
}
