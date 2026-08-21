// Per-user Auto-Start registration for the Bridge (Task Scheduler logon; AD-20).
// Not a Session-0 Windows Service. Daily register/unregister must not require elevation.

#pragma once

#include <string>

// Fixed Task Scheduler task name (current interactive user folder).
inline constexpr const char* kAutoStartTaskName = "UnitorMt4BridgeAutoStart";

// Flag written into the registered action arguments (must match Main.cpp).
// buildAutoStartActionArguments() also appends --midi-backend=wms|virtualmidi
// from resolveMidiBackendKind() at register time (frozen until re-register).
inline constexpr const char* kAutoSessionFlag = "--auto-session";

// Wait/rescan bound when MT4 is absent at Auto-Start launch (plug-after-login).
inline constexpr int kAutoSessionWaitTimeoutSeconds = 900;
inline constexpr int kAutoSessionPollIntervalMs = 2000;
inline constexpr int kAutoSessionProgressIntervalSeconds = 30;

// Mid-session hot-plug replug wait (Story 3.2) — same bound/cadence as first-availability.
inline constexpr int kHotPlugReplugWaitTimeoutSeconds = kAutoSessionWaitTimeoutSeconds;
inline constexpr int kHotPlugReplugPollIntervalMs = kAutoSessionPollIntervalMs;
inline constexpr int kHotPlugReplugProgressIntervalSeconds =
    kAutoSessionProgressIntervalSeconds;

// Builds the argument string for the registered Exec action.
std::string buildAutoStartActionArguments();

// Absolute path of this Bridge.exe (Windows). Empty + English error on failure.
bool resolveBridgeExecutablePath(std::string& pathOut, std::string& errorOut);

// Register/unregister Auto-Start for the current interactive user (no admin).
// On success, writes a short English summary to messageOut.
bool registerAutoStart(std::string& messageOut, std::string& errorOut);
bool unregisterAutoStart(std::string& messageOut, std::string& errorOut);
