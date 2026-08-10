// Fail-closed Bridge soft-echo gate (Epic 5 software-loop). Default OFF.

#pragma once

#include <atomic>

// Process-wide gate. Studio / Auto-Start paths must leave this false
// (--auto-session forces OFF and ignores UNITOR_MIDI_SOFT_ECHO).
inline std::atomic<bool>& softEchoEnabledFlag() noexcept
{
    static std::atomic<bool> enabled{false};
    return enabled;
}

inline bool isSoftEchoEnabled() noexcept
{
    return softEchoEnabledFlag().load(std::memory_order_relaxed);
}

// True when UNITOR_MIDI_SOFT_ECHO is 1/true/yes (case-insensitive).
bool softEchoEnvRequestsEnable() noexcept;

// Resolve gate: --no-soft-echo wins over --soft-echo and env. Else CLI OR env.
// Never silently ON without an explicit enable source.
void configureSoftEchoGate(bool enabledFromCli, bool forceOffFromCli) noexcept;
