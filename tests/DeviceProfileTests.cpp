// Catch2 unit tests for declarative DeviceProfile (no hardware).

#include <catch2/catch_test_macros.hpp>

#include "Profile/DeviceProfile.h"

TEST_CASE("findDeviceProfile returns MT4 row for Emagic VID/PID", "[profile]")
{
    const DeviceProfile* profile = findDeviceProfile(kEmagicVendorId, kMt4ProductId);
    REQUIRE(profile != nullptr);
    REQUIRE(profile->vid == kEmagicVendorId);
    REQUIRE(profile->pid == kMt4ProductId);
    REQUIRE(profile->ifnum == kMt4InterfaceNumber);
    REQUIRE(profile->inCables == kMt4InCables);
    REQUIRE(profile->outCables == kMt4OutCables);
}

TEST_CASE("findDeviceProfile fails closed for unknown identity", "[profile]")
{
    REQUIRE(findDeviceProfile(0x1234, kMt4ProductId) == nullptr);
    REQUIRE(findDeviceProfile(kEmagicVendorId, 0xFFFF) == nullptr);
}

TEST_CASE("collectProductCableIndices lists MT4 OUT ports without Broadcast", "[profile]")
{
    uint8_t indices[kMaxEmagicCableCount] = {};
    const std::size_t count =
        collectProductCableIndices(kMt4OutCables, indices, kMaxEmagicCableCount);

    REQUIRE(count == 4);
    REQUIRE(indices[0] == 0);
    REQUIRE(indices[1] == 1);
    REQUIRE(indices[2] == 2);
    REQUIRE(indices[3] == 3);

    for (std::size_t index = 0; index < count; ++index)
    {
        REQUIRE(indices[index] != kEmagicBroadcastCableIndex);
    }
}

TEST_CASE("collectProductCableIndices lists MT4 IN ports without Broadcast", "[profile]")
{
    uint8_t indices[kMaxEmagicCableCount] = {};
    const std::size_t count =
        collectProductCableIndices(kMt4InCables, indices, kMaxEmagicCableCount);

    REQUIRE(count == 2);
    REQUIRE(indices[0] == 0);
    REQUIRE(indices[1] == 1);
}

TEST_CASE("collectProductCableIndices truncates when maxCount is small", "[profile]")
{
    uint8_t indices[2] = {0xFF, 0xFF};
    const std::size_t count = collectProductCableIndices(kMt4OutCables, indices, 2);
    REQUIRE(count == 2);
    REQUIRE(indices[0] == 0);
    REQUIRE(indices[1] == 1);
}

TEST_CASE("collectProductCableIndices returns 0 for null or empty output", "[profile]")
{
    uint8_t indices[4] = {};
    REQUIRE(collectProductCableIndices(kMt4OutCables, nullptr, 4) == 0);
    REQUIRE(collectProductCableIndices(kMt4OutCables, indices, 0) == 0);
}
