// Offline contract checks for Auto-Start constants (no Task Scheduler / hardware).

#include <catch2/catch_test_macros.hpp>

#include "App/AutoStartRegistration.h"
#include "Midi/MidiBackendSelect.h"

#include <string>

TEST_CASE("Auto-Start action arguments bake midi-backend (default WMS)", "[autostart]")
{
    clearMidiBackendKindOverride();
    REQUIRE(
        buildAutoStartActionArguments()
        == "--auto-session --midi-backend=wms");
    REQUIRE(std::string(kAutoSessionFlag) == "--auto-session");
}

TEST_CASE(
    "Auto-Start action arguments bake midi-backend after virtualMIDI override",
    "[autostart]")
{
    setMidiBackendKindOverride(MidiBackendKind::VirtualMidi);
    REQUIRE(
        buildAutoStartActionArguments()
        == "--auto-session --midi-backend=virtualmidi");
    clearMidiBackendKindOverride();
    REQUIRE(
        buildAutoStartActionArguments()
        == "--auto-session --midi-backend=wms");
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
