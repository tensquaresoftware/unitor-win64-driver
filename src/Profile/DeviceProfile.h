// Declarative per-PID hardware profile (USB identity, Emagic cable masks, capabilities).
// Profile layer must not depend on WinUSB or VirtualMIDI headers.

#pragma once

#include <cstddef>
#include <cstdint>

// Emagic USB vendor ID shared by MT4 / AMT8 / Unitor8.
inline constexpr uint16_t kEmagicVendorId = 0x086A;

inline constexpr uint16_t kMt4ProductId = 0x0003;
inline constexpr uint16_t kAmt8ProductId = 0x0002;
inline constexpr uint16_t kUnitor8ProductId = 0x0001;

// Validated MT4 AD-3 / Linux QUIRK_MIDI_EMAGIC row (single source of truth).
inline constexpr uint8_t kMt4InterfaceNumber = 2;
inline constexpr uint16_t kMt4InCables = 0x8003;   // bits 0,1 + Broadcast 15
inline constexpr uint16_t kMt4OutCables = 0x800f;  // bits 0..3 + Broadcast 15

// Linux QUIRK_MIDI_EMAGIC stores bit 15 as Emagic "Broadcast".
// Keep it in stored masks for Linux fidelity; V1 product Port N ignores cable 15.
inline constexpr uint8_t kEmagicBroadcastCableIndex = 15;
inline constexpr uint16_t kEmagicBroadcastCableBit = static_cast<uint16_t>(1u << kEmagicBroadcastCableIndex);

inline constexpr std::size_t kMaxEmagicCableCount = 16;

struct DeviceProfile
{
    uint16_t vid;
    uint16_t pid;
    uint8_t ifnum;
    uint16_t inCables;   // Linux quirk in_cables bitmask (may include 0x8000)
    uint16_t outCables;  // Linux quirk out_cables bitmask (may include 0x8000)
    bool patchMode;
    bool ltc;
    bool fastMode;
};

// Find profile by USB identity. Missing PID fails closed (nullptr) — never invents MT4.
const DeviceProfile* findDeviceProfile(uint16_t vid, uint16_t pid) noexcept;

// Enumerate Emagic cable indices for V1 product Virtual Ports (Port N).
// Stores ascending set-bit indices from cableMask, excluding cable 15 (Broadcast).
// Returns the number of product ports written (capped by maxCount).
std::size_t collectProductCableIndices(
    uint16_t cableMask,
    uint8_t* outIndices,
    std::size_t maxCount) noexcept;

inline constexpr std::size_t countProductPorts(uint16_t cableMask) noexcept
{
    std::size_t count = 0;
    for (uint8_t cableIndex = 0; cableIndex < kMaxEmagicCableCount; ++cableIndex)
    {
        if (cableIndex == kEmagicBroadcastCableIndex)
        {
            // Bit 15 remains in stored Linux masks; V1 Port N skips Broadcast.
            continue;
        }

        const uint16_t bit = static_cast<uint16_t>(1u << cableIndex);
        if ((cableMask & bit) != 0)
        {
            ++count;
        }
    }

    return count;
}
