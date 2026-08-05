// Bridge process entry — user-session host (not a Windows Service).

#include "Profile/DeviceProfile.h"

namespace
{
bool matchesValidatedMt4Profile(const DeviceProfile& profile) noexcept
{
    return profile.vid == kEmagicVendorId
        && profile.pid == kMt4ProductId
        && profile.ifnum == kMt4InterfaceNumber
        && profile.inCables == kMt4InCables
        && profile.outCables == kMt4OutCables
        && !profile.patchMode
        && !profile.ltc
        && !profile.fastMode
        && countProductPorts(profile.inCables) == 2
        && countProductPorts(profile.outCables) == 4;
}

bool matchesMt4ProductCableOrder(const DeviceProfile& profile) noexcept
{
    uint8_t inIndices[kMaxEmagicCableCount] = {};
    uint8_t outIndices[kMaxEmagicCableCount] = {};

    const std::size_t inCount = collectProductCableIndices(
        profile.inCables, inIndices, kMaxEmagicCableCount);
    const std::size_t outCount = collectProductCableIndices(
        profile.outCables, outIndices, kMaxEmagicCableCount);

    return inCount == 2
        && inIndices[0] == 0
        && inIndices[1] == 1
        && outCount == 4
        && outIndices[0] == 0
        && outIndices[1] == 1
        && outIndices[2] == 2
        && outIndices[3] == 3;
}
} // namespace

int main()
{
    const DeviceProfile* mt4 = findDeviceProfile(kEmagicVendorId, kMt4ProductId);
    if (mt4 == nullptr
        || !matchesValidatedMt4Profile(*mt4)
        || !matchesMt4ProductCableOrder(*mt4))
    {
        return 1;
    }

    // Unknown PID fails closed — never invents an MT4 profile.
    if (findDeviceProfile(kEmagicVendorId, 0xFFFF) != nullptr)
    {
        return 1;
    }

    return 0;
}
