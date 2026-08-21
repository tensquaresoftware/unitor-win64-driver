// Offline contract checks for Auto-Start constants (no Task Scheduler / hardware).

#include <catch2/catch_test_macros.hpp>

#include "App/AutoStartRegistration.h"
#include "Midi/MidiBackendSelect.h"

#include <cstdlib>
#include <string>

namespace
{
void clearMidiBackendEnv() noexcept
{
#if defined(_WIN32)
    _putenv("UNITOR_MIDI_BACKEND=");
#else
    unsetenv("UNITOR_MIDI_BACKEND");
#endif
}

void setMidiBackendEnv(const char* value) noexcept
{
#if defined(_WIN32)
    std::string entry = "UNITOR_MIDI_BACKEND=";
    entry += value != nullptr ? value : "";
    _putenv(entry.c_str());
#else
    if (value == nullptr || value[0] == '\0')
    {
        unsetenv("UNITOR_MIDI_BACKEND");
        return;
    }
    setenv("UNITOR_MIDI_BACKEND", value, 1);
#endif
}

struct MidiBackendOverrideGuard
{
    ~MidiBackendOverrideGuard()
    {
        clearMidiBackendKindOverride();
        clearMidiBackendEnv();
    }
};
} // namespace

TEST_CASE("Auto-Start action arguments bake midi-backend (default WMS)", "[autostart]")
{
    MidiBackendOverrideGuard guard;
    clearMidiBackendKindOverride();
    clearMidiBackendEnv();
    REQUIRE(
        buildAutoStartActionArguments()
        == "--auto-session --midi-backend=wms");
    REQUIRE(std::string(kAutoSessionFlag) == "--auto-session");
}

TEST_CASE(
    "Auto-Start action arguments bake midi-backend after virtualMIDI override",
    "[autostart]")
{
    MidiBackendOverrideGuard guard;
    clearMidiBackendEnv();
    setMidiBackendKindOverride(MidiBackendKind::VirtualMidi);
    REQUIRE(
        buildAutoStartActionArguments()
        == "--auto-session --midi-backend=virtualmidi");
    clearMidiBackendKindOverride();
    REQUIRE(
        buildAutoStartActionArguments()
        == "--auto-session --midi-backend=wms");
}

TEST_CASE(
    "Auto-Start action arguments bake midi-backend from UNITOR_MIDI_BACKEND env",
    "[autostart]")
{
    MidiBackendOverrideGuard guard;
    clearMidiBackendKindOverride();
    setMidiBackendEnv("virtualmidi");
    REQUIRE(
        buildAutoStartActionArguments()
        == "--auto-session --midi-backend=virtualmidi");
}

TEST_CASE("Auto-Start task name is stable and non-empty", "[autostart]")
{
    REQUIRE(std::string(kAutoStartTaskName) == "UnitorMt4BridgeAutoStart");
    REQUIRE(kAutoStartTaskName[0] != '\0');
}

TEST_CASE("Auto-session wait bound is finite and documented cadence", "[autostart]")
{
    REQUIRE(kAutoSessionWaitTimeoutSeconds == 900);
    REQUIRE(kAutoSessionPollIntervalMs == 2000);
    REQUIRE(kAutoSessionProgressIntervalSeconds == 30);
    REQUIRE(kAutoSessionWaitTimeoutSeconds > 0);
    REQUIRE(kAutoSessionPollIntervalMs > 0);
}
