// Catch2 coverage for teVirtualMIDI SendToHost max SysEx length guard.

#include <catch2/catch_test_macros.hpp>

#include "Midi/TeVirtualMidiLimits.h"

TEST_CASE("teVirtualMIDI max SysEx length reject helper", "[midi][sysex]")
{
    REQUIRE(kTeVmDefaultMaxSysexLengthSize == 65535);
    REQUIRE_FALSE(exceedsTeVmDefaultMaxSysexLength(0));
    REQUIRE_FALSE(exceedsTeVmDefaultMaxSysexLength(351));
    REQUIRE_FALSE(exceedsTeVmDefaultMaxSysexLength(kTeVmDefaultMaxSysexLengthSize));
    REQUIRE(exceedsTeVmDefaultMaxSysexLength(kTeVmDefaultMaxSysexLengthSize + 1));
}
