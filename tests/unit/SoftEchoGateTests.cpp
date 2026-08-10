// SoftEchoGate truth table — CLI / force-off / env (lab software-loop only).

#include "Midi/SoftEchoGate.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>

namespace
{
void resetSoftEchoGate() noexcept
{
    softEchoEnabledFlag().store(false, std::memory_order_relaxed);
}

void clearSoftEchoEnv() noexcept
{
#if defined(_WIN32)
    _putenv("UNITOR_MIDI_SOFT_ECHO=");
#else
    unsetenv("UNITOR_MIDI_SOFT_ECHO");
#endif
}

void setSoftEchoEnvTruthy() noexcept
{
#if defined(_WIN32)
    _putenv("UNITOR_MIDI_SOFT_ECHO=1");
#else
    setenv("UNITOR_MIDI_SOFT_ECHO", "1", 1);
#endif
}
} // namespace

TEST_CASE("SoftEchoGate default and CLI enable", "[soft-echo-gate]")
{
    clearSoftEchoEnv();
    resetSoftEchoGate();
    configureSoftEchoGate(false, false);
    REQUIRE_FALSE(isSoftEchoEnabled());

    configureSoftEchoGate(true, false);
    REQUIRE(isSoftEchoEnabled());
}

TEST_CASE("SoftEchoGate --no-soft-echo wins over CLI and env", "[soft-echo-gate]")
{
    setSoftEchoEnvTruthy();
    resetSoftEchoGate();
    configureSoftEchoGate(true, true);
    REQUIRE_FALSE(isSoftEchoEnabled());
    clearSoftEchoEnv();
}

TEST_CASE("SoftEchoGate env alone enables when CLI off", "[soft-echo-gate]")
{
    setSoftEchoEnvTruthy();
    resetSoftEchoGate();
    configureSoftEchoGate(false, false);
    REQUIRE(isSoftEchoEnabled());
    clearSoftEchoEnv();
    resetSoftEchoGate();
}
