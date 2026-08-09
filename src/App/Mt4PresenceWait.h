// Bounded wait/rescan for project WinUSB GUID (Auto-Start + hot-plug).

#pragma once

#include <atomic>
#include <string>

struct Mt4PresenceWaitConfig
{
    const char* contextLabel = "Presence wait";
    int timeoutSeconds = 900;
    int pollIntervalMs = 2000;
    int progressIntervalSeconds = 30;
};

// Present immediately → true. Absent → poll until present, cancel, timeout, or Error.
// Writes English diagnostics to stdout/stderr. Does not open WinUSB.
bool waitForMt4WinUsbOrTimeout(
    const Mt4PresenceWaitConfig& config,
    const std::atomic<bool>& cancelRequested);
