#include "Midi/MidiBackendSelect.h"

#include <cstdlib>
#include <cstring>

namespace
{
bool g_hasOverride = false;
MidiBackendKind g_overrideKind = MidiBackendKind::Wms;

char toLowerAscii(char value) noexcept
{
    if (value >= 'A' && value <= 'Z')
    {
        return static_cast<char>(value - 'A' + 'a');
    }
    return value;
}

bool equalsIgnoreCaseAscii(const char* left, const char* right) noexcept
{
    if (left == nullptr || right == nullptr)
    {
        return false;
    }
    while (*left != '\0' && *right != '\0')
    {
        if (toLowerAscii(*left) != toLowerAscii(*right))
        {
            return false;
        }
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}
} // namespace

const char* midiBackendKindLabel(MidiBackendKind kind) noexcept
{
    return kind == MidiBackendKind::VirtualMidi ? "virtualmidi" : "wms";
}

bool parseMidiBackendKind(const char* text, MidiBackendKind& kindOut) noexcept
{
    if (equalsIgnoreCaseAscii(text, "wms")
        || equalsIgnoreCaseAscii(text, "windows-midi-services"))
    {
        kindOut = MidiBackendKind::Wms;
        return true;
    }
    if (equalsIgnoreCaseAscii(text, "virtualmidi")
        || equalsIgnoreCaseAscii(text, "virtual-midi"))
    {
        kindOut = MidiBackendKind::VirtualMidi;
        return true;
    }
    return false;
}

void setMidiBackendKindOverride(MidiBackendKind kind) noexcept
{
    g_overrideKind = kind;
    g_hasOverride = true;
}

void clearMidiBackendKindOverride() noexcept
{
    g_hasOverride = false;
    g_overrideKind = MidiBackendKind::Wms;
}

bool hasMidiBackendKindOverride() noexcept
{
    return g_hasOverride;
}

MidiBackendKind resolveMidiBackendKind() noexcept
{
    if (g_hasOverride)
    {
        return g_overrideKind;
    }
    const char* env = std::getenv("UNITOR_MIDI_BACKEND");
    MidiBackendKind fromEnv = MidiBackendKind::Wms;
    if (env != nullptr && env[0] != '\0' && parseMidiBackendKind(env, fromEnv))
    {
        return fromEnv;
    }
    return MidiBackendKind::Wms;
}

bool rejectMissingWmsTransport(bool transportAvailable, std::string& errorOut)
{
    if (transportAvailable)
    {
        return true;
    }
    errorOut =
        "Windows MIDI Services virtual-device transport is unavailable "
        "(WMS MidiBackend prerequisite missing). Install Windows MIDI Services "
        "and the App SDK runtime on Windows 11, or pass --midi-backend=virtualmidi "
        "for the lab virtualMIDI path.";
    return false;
}
