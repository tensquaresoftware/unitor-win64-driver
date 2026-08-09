// Long-running MT4 DeviceSession CLI (--start-session / --run-midi / --auto-session).

#include "App/MidiSessionCli.h"

#include "App/MidiSessionDiagnostics.h"
#include "App/Mt4PresenceWait.h"
#include "Device/DeviceSession.h"
#include "Device/DeviceSessionManager.h"
#include "Midi/VirtualMidiBackend.h"
#include "Profile/DeviceProfile.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

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

enum class MidiSessionWaitResult
{
    Cancelled,
    Disconnected
};

struct Mt4SessionStartArgs
{
    DeviceSession* session = nullptr;
    VirtualMidiBackend* midiBackend = nullptr;
    const DeviceProfile* profile = nullptr;
    const PortNameSet* names = nullptr;
    bool allowZadigFallback = false;
};

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

bool pollMidiSessionOnce(
    DeviceSession& session,
    DeviceHostCounterSnapshot& lastCounters,
    std::chrono::steady_clock::time_point& lastHeartbeat)
{
    std::string pumpError;
    if (session.TakePumpFailure(pumpError))
    {
        printDeviceHostCounters(session.CopyDeviceHostCounters());
        std::cerr << "MIDI I/O pump failed: " << pumpError << '\n';
        return false;
    }
    if (!session.IsRunning())
    {
        printDeviceHostCounters(session.CopyDeviceHostCounters());
        std::cerr << "MIDI I/O session ended unexpectedly\n";
        return false;
    }

    const DeviceHostCounterSnapshot counters = session.CopyDeviceHostCounters();
    if (shouldPrintDeviceHostCounters(lastCounters, counters))
    {
        printDeviceHostCounters(counters);
        lastCounters = counters;
    }

    const auto now = std::chrono::steady_clock::now();
    if (now - lastHeartbeat >= std::chrono::seconds(3))
    {
        printDeviceHostCounters(counters);
        lastCounters = counters;
        lastHeartbeat = now;
    }
    return true;
}

MidiSessionWaitResult waitForMidiSessionCancel(DeviceSession& session)
{
    DeviceHostCounterSnapshot lastCounters = {};
    auto lastHeartbeat = std::chrono::steady_clock::now();
    while (!g_cancelRequested.load())
    {
        if (!pollMidiSessionOnce(session, lastCounters, lastHeartbeat))
        {
            session.Stop();
            std::cout << "MIDI I/O stopped\n";
            return MidiSessionWaitResult::Disconnected;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    printDeviceHostCounters(session.CopyDeviceHostCounters());
    session.Stop();
    std::cout << "MIDI I/O stopped\n";
    return MidiSessionWaitResult::Cancelled;
}

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

void printSessionStartedBanner()
{
    std::cout << "DeviceSession started for MT4 with Virtual Ports\n";
    std::cout << "MIDI I/O running - notes/CC smoke ready (Ctrl+C to stop)\n";
    std::cout << "device-host counters will print in this window on USB IN activity"
                 " (same thread as this message)\n";
    std::cout << "Device Inquiry lab: watch inquiry_out vs identity_reply_in"
                 " (expect them to stay close)\n";
}

bool startMt4DeviceSession(const Mt4SessionStartArgs& args, std::string& errorOut)
{
    if (args.session == nullptr || args.midiBackend == nullptr || args.profile == nullptr
        || args.names == nullptr)
    {
        errorOut = "DeviceSession start requires session, backend, profile, and names";
        return false;
    }
    DeviceSessionStartRequest request;
    request.profile = args.profile;
    request.midiBackend = args.midiBackend;
    request.portNames = args.names;
    request.openOptions.allowZadigFallback = args.allowZadigFallback;
    if (!args.session->Start(request, errorOut) || !args.session->IsRunning())
    {
        if (errorOut.empty())
        {
            errorOut = "unknown error";
        }
        // Start may return true with IsRunning false — always tear down.
        args.session->Stop();
        return false;
    }
    return true;
}

int exitAfterHotPlugWaitFailed()
{
    if (g_cancelRequested.load())
    {
        std::cerr << "Hot-plug recovery cancelled while waiting for replug\n";
        return 0;
    }
    return 1;
}

// After Absent→Present, retry Start until timeout (PnP settle race).
bool startMt4AfterReplugOrTimeout(
    const Mt4SessionStartArgs& startArgs,
    const Mt4PresenceWaitConfig& config)
{
    const auto deadline = std::chrono::steady_clock::now()
        + std::chrono::seconds(config.timeoutSeconds);
    while (!g_cancelRequested.load())
    {
        std::string error;
        if (startMt4DeviceSession(startArgs, error))
        {
            return true;
        }
        std::cerr << "Hot-plug recovery: DeviceSession start failed (" << error
                  << "); retrying...\n";
        if (std::chrono::steady_clock::now() >= deadline)
        {
            std::cerr << "Hot-plug recovery: DeviceSession start retries exhausted\n";
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(config.pollIntervalMs));
    }
    std::cerr << "Hot-plug recovery cancelled before DeviceSession restart\n";
    return false;
}

int recoverAfterDisconnect(const Mt4SessionStartArgs& startArgs)
{
    std::cout << "MT4 disconnected; waiting for replug...\n";
    const Mt4PresenceWaitConfig replugConfig = makeHotPlugReplugPresenceWaitConfig();
    if (!waitForMt4WinUsbReplugOrTimeout(replugConfig, g_cancelRequested))
    {
        return exitAfterHotPlugWaitFailed();
    }
    if (g_cancelRequested.load())
    {
        std::cerr << "Hot-plug recovery cancelled before DeviceSession restart\n";
        return 0;
    }
    std::cout << "MT4 replugged; starting new DeviceSession...\n";
    if (!startMt4AfterReplugOrTimeout(startArgs, replugConfig))
    {
        return g_cancelRequested.load() ? 0 : 1;
    }
    return -1; // continue host loop
}

void printHotPlugSessionStarted(bool firstStart)
{
    if (!firstStart)
    {
        std::cout << "Hot-plug recovery: new DeviceSession started "
                     "(AD-6 names unchanged for single unit)\n";
    }
    printSessionStartedBanner();
}

// Product host: Stop already destroyed ports; Absent→Present; Start with retry.
int runAutoSessionHotPlugLoop(const Mt4SessionStartArgs& startArgs)
{
    bool firstStart = true;
    while (!g_cancelRequested.load())
    {
        if (firstStart)
        {
            std::string error;
            if (!startMt4DeviceSession(startArgs, error))
            {
                std::cerr << "DeviceSession start failed: " << error << '\n';
                return 1;
            }
        }
        printHotPlugSessionStarted(firstStart);
        firstStart = false;
        if (waitForMidiSessionCancel(*startArgs.session)
            == MidiSessionWaitResult::Cancelled)
        {
            return 0;
        }
        const int recoverExit = recoverAfterDisconnect(startArgs);
        if (recoverExit >= 0)
        {
            return recoverExit;
        }
    }
    return 0;
}

bool prepareMt4PortNames(PortNameSet& namesOut, const DeviceProfile*& profileOut)
{
    profileOut = findDeviceProfile(kEmagicVendorId, kMt4ProductId);
    if (profileOut == nullptr)
    {
        std::cerr << "MT4 DeviceProfile not found\n";
        return false;
    }
    DeviceSessionManager manager;
    std::string error;
    if (!manager.buildPortNameSet(*profileOut, namesOut, error))
    {
        std::cerr << "Port name build failed: " << error << '\n';
        return false;
    }
    printExpectedPortDiagnostics(namesOut);
    return true;
}

void printAutoSessionHostBanner()
{
    std::cout << "Auto-session host (user-session Bridge; not a Windows Service)\n";
    std::cout << "Prefer clean exit with Ctrl+C so Virtual Ports tear down "
                 "(closing the console window may leave orphan ports).\n";
    std::cout << "Hot-plug: mid-session unplug tears down ports; Bridge waits for "
                 "replug and starts a new DeviceSession (no Windows reboot). "
                 "Hosts may need a MIDI rescan; supervised Bridge restart is allowed.\n";
}
} // namespace

int runMt4MidiSession(bool allowZadigFallback, bool preserveCancel)
{
    PortNameSet names;
    const DeviceProfile* mt4 = nullptr;
    if (!prepareMt4PortNames(names, mt4))
    {
        return 1;
    }
    if (!armMidiSessionCancel(preserveCancel))
    {
        return 1;
    }

    VirtualMidiBackend midiBackend;
    DeviceSession session;
    std::string error;
    const Mt4SessionStartArgs startArgs{
        &session, &midiBackend, mt4, &names, allowZadigFallback};
    if (!startMt4DeviceSession(startArgs, error))
    {
        std::cerr << "DeviceSession start failed: " << error << '\n';
        return 1;
    }

    printSessionStartedBanner();
    // Lab one-shot: exit on USB loss so spawners that expect process exit keep working.
    const MidiSessionWaitResult waitResult = waitForMidiSessionCancel(session);
    return waitResult == MidiSessionWaitResult::Cancelled ? 0 : 1;
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

    PortNameSet names;
    const DeviceProfile* mt4 = nullptr;
    if (!prepareMt4PortNames(names, mt4) || !armMidiSessionCancel(true))
    {
        return 1;
    }

    VirtualMidiBackend midiBackend;
    DeviceSession session;
    const Mt4SessionStartArgs startArgs{&session, &midiBackend, mt4, &names, false};
    return runAutoSessionHotPlugLoop(startArgs);
}
