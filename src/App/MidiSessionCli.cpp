// Long-running MT4 DeviceSession CLI for notes/CC smoke (--start-session / --run-midi).

#include "App/MidiSessionCli.h"

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
    return SetConsoleCtrlHandler(onConsoleCtrl, TRUE) == TRUE;
}
#else
void onSignalCancel(int /*signal*/)
{
    g_cancelRequested.store(true);
}

bool installCancelHandler()
{
    std::signal(SIGINT, onSignalCancel);
    std::signal(SIGTERM, onSignalCancel);
    return true;
}
#endif

void printExpectedPortDiagnostics(const PortNameSet& names)
{
    std::cout << "Expected Virtual Ports: " << names.inCount << " IN / " << names.outCount
              << " OUT\n";
    for (std::size_t index = 0; index < names.inCount; ++index)
    {
        std::cout << "  IN  " << names.inNames[index] << '\n';
    }
    for (std::size_t index = 0; index < names.outCount; ++index)
    {
        std::cout << "  OUT " << names.outNames[index] << '\n';
    }
}

void printDeviceHostCounters(const DeviceHostCounterSnapshot& snapshot)
{
    std::cout << "device-host counters: bulk_in_bytes=" << snapshot.bulkBytes
              << " demux_spans=" << snapshot.demuxSpans
              << " send_ok_msgs=" << snapshot.sendOk
              << " send_fail_msgs=" << snapshot.sendFail << '\n'
              << std::flush;
}

bool shouldPrintDeviceHostCounters(
    const DeviceHostCounterSnapshot& previous,
    const DeviceHostCounterSnapshot& current)
{
    if (current.sendFail != previous.sendFail)
    {
        return true;
    }
    if (previous.bulkBytes == 0 && current.bulkBytes > 0)
    {
        return true;
    }
    if (current.sendOk > previous.sendOk)
    {
        if (current.sendOk == 1)
        {
            return true;
        }
        return (current.sendOk / 25) > (previous.sendOk / 25);
    }
    return false;
}

bool pollMidiSessionOnce(
    DeviceSession& session,
    DeviceHostCounterSnapshot& lastCounters,
    std::chrono::steady_clock::time_point& lastHeartbeat,
    int& exitCode)
{
    std::string pumpError;
    if (session.TakePumpFailure(pumpError))
    {
        printDeviceHostCounters(session.CopyDeviceHostCounters());
        std::cerr << "MIDI I/O pump failed: " << pumpError << '\n';
        exitCode = 1;
        return false;
    }
    if (!session.IsRunning())
    {
        printDeviceHostCounters(session.CopyDeviceHostCounters());
        std::cerr << "MIDI I/O session ended unexpectedly\n";
        exitCode = 1;
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

int waitForMidiSessionCancel(DeviceSession& session)
{
    DeviceHostCounterSnapshot lastCounters = {};
    auto lastHeartbeat = std::chrono::steady_clock::now();
    int exitCode = 0;
    while (!g_cancelRequested.load())
    {
        if (!pollMidiSessionOnce(session, lastCounters, lastHeartbeat, exitCode))
        {
            session.Stop();
            std::cout << "MIDI I/O stopped\n";
            return exitCode;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    printDeviceHostCounters(session.CopyDeviceHostCounters());
    session.Stop();
    std::cout << "MIDI I/O stopped\n";
    return 0;
}
} // namespace

void printSessionStartedBanner()
{
    std::cout << "DeviceSession started for MT4 with Virtual Ports\n";
    std::cout << "MIDI I/O running - notes/CC smoke ready (Ctrl+C to stop)\n";
    std::cout << "device-host counters will print in this window on USB IN activity"
                 " (same thread as this message)\n";
}

int runMt4MidiSession(bool allowZadigFallback)
{
    const DeviceProfile* mt4 = findDeviceProfile(kEmagicVendorId, kMt4ProductId);
    if (mt4 == nullptr)
    {
        std::cerr << "MT4 DeviceProfile not found\n";
        return 1;
    }

    DeviceSessionManager manager;
    PortNameSet names;
    std::string error;
    if (!manager.buildPortNameSet(*mt4, names, error))
    {
        std::cerr << "Port name build failed: " << error << '\n';
        return 1;
    }

    printExpectedPortDiagnostics(names);

    if (!installCancelHandler())
    {
        std::cerr << "Failed to install Ctrl+C handler for MIDI session\n";
        return 1;
    }
    g_cancelRequested.store(false);

    VirtualMidiBackend midiBackend;
    DeviceSession session;

    DeviceSessionStartRequest request;
    request.profile = mt4;
    request.midiBackend = &midiBackend;
    request.portNames = &names;
    request.openOptions.allowZadigFallback = allowZadigFallback;

    if (!session.Start(request, error) || !session.IsRunning())
    {
        std::cerr << "DeviceSession start failed: "
                  << (error.empty() ? "unknown error" : error) << '\n';
        return 1;
    }

    printSessionStartedBanner();
    return waitForMidiSessionCancel(session);
}
