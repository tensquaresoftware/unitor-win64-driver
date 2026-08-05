// Emagic F5 cable multiplex/demultiplex — Protocol layer (Profile + STL only).
// MIT-original reimplementation of documented Emagic USB-MIDI wire behavior.

#pragma once

#include "Profile/DeviceProfile.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

inline constexpr uint8_t kEmagicEndOfValidData = 0xFF;
inline constexpr uint8_t kEmagicPortSwitch = 0xF5;

// Session lifecycle SysEx magics (send raw on OUT; not product Patch Mode).
inline constexpr uint8_t kEmagicInitMagic[] = {
    0xF0, 0x00, 0x20, 0x31, 0x64, 0x0B, 0x00, 0x00, 0xF7};
inline constexpr std::size_t kEmagicInitMagicSize =
    sizeof(kEmagicInitMagic) / sizeof(kEmagicInitMagic[0]);

inline constexpr uint8_t kEmagicFinishMagic[] = {
    0xF0, 0x00, 0x20, 0x31, 0x64, 0x10, 0x00, 0x7F, 0x40, 0xF7};
inline constexpr std::size_t kEmagicFinishMagicSize =
    sizeof(kEmagicFinishMagic) / sizeof(kEmagicFinishMagic[0]);

struct EncodeRequest
{
    uint8_t cableIndex = 0;
    const uint8_t* midiBytes = nullptr;
    std::size_t midiSize = 0;
};

struct EncodeBuffer
{
    uint8_t* bytes = nullptr;
    std::size_t capacity = 0;
    std::size_t size = 0;
};

// Per-cable MIDI sink for device→host demux (Story 1.5 can feed MidiBackend here).
using MidiCableSink =
    std::function<void(uint8_t cableIndex, const uint8_t* midi, std::size_t n)>;

class EmagicCableMapper
{
public:
    explicit EmagicCableMapper(const DeviceProfile& profile);

    // Host MIDI for one Emagic cable → framed bulk OUT bytes (F5 switch + pad).
    bool EncodeToDevice(
        const EncodeRequest& request,
        EncodeBuffer& buffer,
        std::string& errorOut);

    // One bulk IN buffer → demux into per-cable sinks; preserves seen_f5 state.
    bool DecodeFromDevice(
        const uint8_t* bulkBytes,
        std::size_t bulkSize,
        const MidiCableSink& sink,
        std::string& errorOut);

    bool IsProductOutCable(uint8_t cableIndex) const noexcept;
    bool IsProductInCable(uint8_t cableIndex) const noexcept;

private:
    bool appendPortSwitch(uint8_t cableIndex, EncodeBuffer& buffer, std::string& errorOut);
    bool appendMidiBytes(const EncodeRequest& request, EncodeBuffer& buffer, std::string& errorOut);
    void appendTrailingPad(EncodeBuffer& buffer) noexcept;
    bool consumePendingPortSwitch(const uint8_t*& cursor, std::size_t& remaining) noexcept;
    bool demuxUntilPortSwitch(
        const uint8_t*& cursor,
        std::size_t& remaining,
        const MidiCableSink& sink);

    DeviceProfile profile_;
    uint8_t currentOutCable_ = 0;
    uint8_t currentInCable_ = 0;
    bool seenF5_ = false;
};
