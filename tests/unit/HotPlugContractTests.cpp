// Offline contract checks for hot-plug replug wait constants (no hardware).

#include <catch2/catch_test_macros.hpp>

#include "App/AutoStartRegistration.h"

TEST_CASE("Hot-plug replug wait matches Auto-Start cadence", "[hotplug]")
{
    REQUIRE(kHotPlugReplugWaitTimeoutSeconds == kAutoSessionWaitTimeoutSeconds);
    REQUIRE(kHotPlugReplugPollIntervalMs == kAutoSessionPollIntervalMs);
    REQUIRE(
        kHotPlugReplugProgressIntervalSeconds == kAutoSessionProgressIntervalSeconds);
    REQUIRE(kHotPlugReplugWaitTimeoutSeconds == 900);
    REQUIRE(kHotPlugReplugPollIntervalMs == 2000);
    REQUIRE(kHotPlugReplugProgressIntervalSeconds == 30);
    REQUIRE(kHotPlugReplugWaitTimeoutSeconds > 0);
    REQUIRE(kHotPlugReplugPollIntervalMs > 0);
}
