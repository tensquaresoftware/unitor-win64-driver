// Lab/community MidiBackend selection (CLI override, else env, else WMS).
// Setup flavors bake the choice by calling --register-auto-start with the
// resolved backend (see buildAutoStartActionArguments) — not a separate resolve path.

#pragma once

#include "Midi/MidiBackend.h"

#include <memory>
#include <string>

enum class MidiBackendKind
{
    Wms,
    VirtualMidi,
};

const char* midiBackendKindLabel(MidiBackendKind kind) noexcept;

bool parseMidiBackendKind(const char* text, MidiBackendKind& kindOut) noexcept;

// Explicit CLI/env override. Empty/cleared ⇒ default WMS.
void setMidiBackendKindOverride(MidiBackendKind kind) noexcept;
void clearMidiBackendKindOverride() noexcept;
bool hasMidiBackendKindOverride() noexcept;

// Reads override, else UNITOR_MIDI_BACKEND, else WMS.
MidiBackendKind resolveMidiBackendKind() noexcept;

std::unique_ptr<MidiBackend> createMidiBackend(MidiBackendKind kind);

// Fail-closed helper used by WMS CreatePortSet (unit-testable without WinRT).
bool rejectMissingWmsTransport(bool transportAvailable, std::string& errorOut);
