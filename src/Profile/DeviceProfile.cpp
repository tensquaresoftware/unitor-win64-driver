#include "Profile/DeviceProfile.h"

namespace
{
// Validated MT4 row — AD-3 / Linux quirks-table.h QUIRK_MIDI_EMAGIC.
constexpr DeviceProfile kMt4Profile = {
    kEmagicVendorId,
    kMt4ProductId,
    kMt4InterfaceNumber,
    kMt4InCables,
    kMt4OutCables,
    false,    // patchMode
    false,    // ltc
    false,    // fastMode
};

// Cousin stubs — data only, not product-validated (FR-16 / CAP-15).
constexpr DeviceProfile kAmt8StubProfile = {
    kEmagicVendorId,
    kAmt8ProductId,
    2,
    0x80ff,
    0x80ff,
    false,
    false,
    false,
};

constexpr DeviceProfile kUnitor8StubProfile = {
    kEmagicVendorId,
    kUnitor8ProductId,
    2,
    0x80ff,
    0x80ff,
    false,
    false,
    false,
};

constexpr DeviceProfile kDeviceProfiles[] = {
    kMt4Profile,
    kAmt8StubProfile,
    kUnitor8StubProfile,
};

constexpr std::size_t kMt4InProductPorts = countProductPorts(kMt4Profile.inCables);
constexpr std::size_t kMt4OutProductPorts = countProductPorts(kMt4Profile.outCables);

static_assert(kMt4InProductPorts == 2, "MT4 must expose 2 IN product ports (Broadcast excluded)");
static_assert(kMt4OutProductPorts == 4, "MT4 must expose 4 OUT product ports (Broadcast excluded)");
} // namespace

const DeviceProfile* findDeviceProfile(uint16_t vid, uint16_t pid) noexcept
{
    for (const DeviceProfile& profile : kDeviceProfiles)
    {
        if (profile.vid == vid && profile.pid == pid)
        {
            return &profile;
        }
    }

    return nullptr;
}

std::size_t collectProductCableIndices(
    uint16_t cableMask,
    uint8_t* outIndices,
    std::size_t maxCount) noexcept
{
    if (outIndices == nullptr || maxCount == 0)
    {
        return 0;
    }

    std::size_t written = 0;
    for (uint8_t cableIndex = 0; cableIndex < kMaxEmagicCableCount; ++cableIndex)
    {
        if (cableIndex == kEmagicBroadcastCableIndex)
        {
            // Bit 15 stays in the stored Linux mask for fidelity / future Broadcast work,
            // but is not exposed as MT4 In/Out N in V1.
            continue;
        }

        const uint16_t bit = static_cast<uint16_t>(1u << cableIndex);
        if ((cableMask & bit) == 0)
        {
            continue;
        }

        outIndices[written] = cableIndex;
        ++written;
        if (written == maxCount)
        {
            break;
        }
    }

    return written;
}
