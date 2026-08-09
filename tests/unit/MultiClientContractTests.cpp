// Offline contract: VirtualMIDI create-flag mask has no exclusive-open policy (AD-8 / SM-7).
// Hardware SM-7 (DAW + MIDI-OX concurrent) remains the product gate — see
// docs/tests/smoke-epic3-multiclient-mt4.md.

#include <catch2/catch_test_macros.hpp>

#include "Midi/TeVirtualMidiApi.h"
#include "Midi/VirtualMidiWinSupport.h"

TEST_CASE("VirtualMIDI create flags are PARSE/INSTANTIATE only (no exclusive-open)",
          "[midi][multiclient]")
{
    // Public teVirtualMIDI flag bit set owned by this repo (AQ-3 unpinned).
    REQUIRE(kTeVmFlagsParseRx == 1u);
    REQUIRE(kTeVmFlagsParseTx == 2u);
    REQUIRE(kTeVmFlagsInstantiateRx == 4u);
    REQUIRE(kTeVmFlagsInstantiateTx == 8u);

    constexpr DWORD kKnownFlagMask =
        kTeVmFlagsParseRx | kTeVmFlagsParseTx | kTeVmFlagsInstantiateRx
        | kTeVmFlagsInstantiateTx;

    // Directional faces used by VirtualMidiBackend::createDirectionalPortSet.
    REQUIRE(
        kVirtualMidiInPortFlags
        == (kTeVmFlagsParseTx | kTeVmFlagsInstantiateTx));
    REQUIRE(
        kVirtualMidiOutPortFlags
        == (kTeVmFlagsParseRx | kTeVmFlagsInstantiateRx));

    // No extra exclusive / single-client bits layered on create.
    REQUIRE((kVirtualMidiInPortFlags & ~kKnownFlagMask) == 0u);
    REQUIRE((kVirtualMidiOutPortFlags & ~kKnownFlagMask) == 0u);
    REQUIRE(kVirtualMidiInPortFlags != 0u);
    REQUIRE(kVirtualMidiOutPortFlags != 0u);
}
