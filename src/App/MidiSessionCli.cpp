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

int waitForMidiSessionCancel(DeviceSession& session)
{
    while (!g_cancelRequested.load())
    {
        std::string pumpError;
        if (session.TakePumpFailure(pumpError))
        {
            std::cerr << "MIDI I/O pump failed: " << pumpError << '\n';
            session.Stop();
            std::cout << "MIDI I/O stopped\n";
            return 1;
        }
        if (!session.IsRunning())
        {
            std::cerr << "MIDI I/O session ended unexpectedly\n";
            session.Stop();
            std::cout << "MIDI I/O stopped\n";
            return 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    session.Stop();
    std::cout << "MIDI I/O stopped\n";
    return 0;
}
} // namespace

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

    std::cout << "DeviceSession started for MT4 with Virtual Ports\n";
    std::cout << "MIDI I/O running — notes/CC smoke ready (Ctrl+C to stop)\n";
    return waitForMidiSessionCancel(session);
}
