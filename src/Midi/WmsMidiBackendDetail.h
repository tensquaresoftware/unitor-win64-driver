// Internal WMS endpoint slot layout shared by WmsMidiBackend*.cpp (not public API).

#pragma once

#ifdef _WIN32

#include "Midi/Midi1StreamAssembler.h"
#include "Midi/WmsMidiBackend.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.Windows.Devices.Midi2.h>
#include <winrt/Microsoft.Windows.Devices.Midi2.Endpoints.Virtual.h>

#include <mutex>
#include <string>

struct WmsEndpointSlot
{
    winrt::Microsoft::Windows::Devices::Midi2::Endpoints::Virtual::MidiVirtualDevice device{
        nullptr};
    winrt::Microsoft::Windows::Devices::Midi2::MidiEndpointConnection connection{nullptr};
    winrt::event_token messageToken{};
    bool hasMessageToken = false;
    std::size_t outPortIndex = 0;
    WmsMidiBackend* backend = nullptr;
};

struct WmsMidiBackendImpl
{
    winrt::Microsoft::Windows::Devices::Midi2::MidiSession session{nullptr};
    WmsEndpointSlot inSlots[kMaxMidiBackendInPorts];
    WmsEndpointSlot outSlots[kMaxMidiBackendOutPorts];
    // WMS may deliver SysEx7 as many small MIDI 1.0 chunks; reassemble per OUT.
    Midi1StreamAssembler hostAssemblers[kMaxMidiBackendOutPorts];
    std::mutex hostAssembleMutex;
    std::size_t inCount = 0;
    std::size_t outCount = 0;
    bool runtimeHeld = false;
    // Distinguishes CreatePortSet runs so ghost endpoints from hung sessions do not collide.
    std::wstring instanceSuffix;
};

struct WmsMidiBackend::Impl : WmsMidiBackendImpl
{
};

void wmsOnHostMessageReceived(
    WmsEndpointSlot* slot,
    winrt::Microsoft::Windows::Devices::Midi2::MidiMessageReceivedEventArgs const& args);

#endif // _WIN32
