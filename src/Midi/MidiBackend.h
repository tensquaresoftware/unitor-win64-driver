// Abstract host MIDI backend seam (AD-2 / NFR-Q3).
// Backends must not invent unit ordinal K — display names arrive ready-made (AD-5 / AD-6).

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

inline constexpr std::size_t kMaxMidiBackendInPorts = 8;
inline constexpr std::size_t kMaxMidiBackendOutPorts = 8;

// Ready-made AD-5 display strings + direction counts. No USB serial / topology inventing.
struct PortNameSet
{
    std::string inNames[kMaxMidiBackendInPorts];
    std::size_t inCount = 0;
    std::string outNames[kMaxMidiBackendOutPorts];
    std::size_t outCount = 0;
};

class MidiBackend
{
public:
    virtual ~MidiBackend() = default;

    // Create the unit's Virtual Port set from ready-made names.
    virtual bool CreatePortSet(const PortNameSet& names, std::string& errorOut) = 0;

    // Destroy that set. Idempotent if never created or already destroyed.
    virtual void DestroyPortSet() noexcept = 0;

    // Device→host / host→device data path (Story 1.6).
    virtual bool SendToHost(
        std::size_t inPortIndex,
        const uint8_t* midiBytes,
        std::size_t byteCount,
        std::string& errorOut) = 0;

    using HostToDeviceSink = void (*)(
        void* context,
        std::size_t outPortIndex,
        const uint8_t* midiBytes,
        std::size_t byteCount);

    virtual void SetHostToDeviceSink(HostToDeviceSink sink, void* context) noexcept = 0;
};
