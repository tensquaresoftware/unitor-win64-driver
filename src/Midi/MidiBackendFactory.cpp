#include "Midi/MidiBackendSelect.h"

#include "Midi/VirtualMidiBackend.h"
#include "Midi/WmsMidiBackend.h"

std::unique_ptr<MidiBackend> createMidiBackend(MidiBackendKind kind)
{
    if (kind == MidiBackendKind::VirtualMidi)
    {
        return std::make_unique<VirtualMidiBackend>();
    }
    return std::make_unique<WmsMidiBackend>();
}
