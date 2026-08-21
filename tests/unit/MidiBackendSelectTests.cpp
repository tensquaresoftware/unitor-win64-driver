#include <catch2/catch_test_macros.hpp>

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
} // namespace

TEST_CASE("parseMidiBackendKind accepts wms and virtualmidi aliases", "[midi-backend]")
{
    MidiBackendKind kind = MidiBackendKind::VirtualMidi;
    REQUIRE(parseMidiBackendKind("wms", kind));
    REQUIRE(kind == MidiBackendKind::Wms);
    REQUIRE(parseMidiBackendKind("Windows-MIDI-Services", kind));
    REQUIRE(kind == MidiBackendKind::Wms);
    REQUIRE(parseMidiBackendKind("virtualmidi", kind));
    REQUIRE(kind == MidiBackendKind::VirtualMidi);
    REQUIRE(parseMidiBackendKind("virtual-midi", kind));
    REQUIRE(kind == MidiBackendKind::VirtualMidi);
    REQUIRE_FALSE(parseMidiBackendKind("loopmidi", kind));
}

TEST_CASE("rejectMissingWmsTransport fail-closed messaging", "[midi-backend]")
{
    std::string error;
    REQUIRE(rejectMissingWmsTransport(true, error));
    REQUIRE(error.empty());

    REQUIRE_FALSE(rejectMissingWmsTransport(false, error));
    REQUIRE(error.find("Windows MIDI Services") != std::string::npos);
    REQUIRE(error.find("virtualmidi") != std::string::npos);
}

TEST_CASE("midiBackendKindOverride controls resolveMidiBackendKind", "[midi-backend]")
{
    clearMidiBackendKindOverride();
    clearMidiBackendEnv();
    REQUIRE(resolveMidiBackendKind() == MidiBackendKind::Wms);

    setMidiBackendKindOverride(MidiBackendKind::VirtualMidi);
    REQUIRE(hasMidiBackendKindOverride());
    REQUIRE(resolveMidiBackendKind() == MidiBackendKind::VirtualMidi);
    REQUIRE(std::string(midiBackendKindLabel(resolveMidiBackendKind())) == "virtualmidi");

    setMidiBackendKindOverride(MidiBackendKind::Wms);
    REQUIRE(resolveMidiBackendKind() == MidiBackendKind::Wms);
    REQUIRE(std::string(midiBackendKindLabel(resolveMidiBackendKind())) == "wms");

    clearMidiBackendKindOverride();
    REQUIRE_FALSE(hasMidiBackendKindOverride());
    REQUIRE(resolveMidiBackendKind() == MidiBackendKind::Wms);
}
