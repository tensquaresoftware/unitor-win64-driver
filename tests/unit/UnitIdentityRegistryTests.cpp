// Offline contracts for dual-MT4 naming + durable ordinal K registry (AD-5 / AD-6 / FR-10).
// Hardware / honest simulated dual smoke remains the product gate — see
// docs/tests/smoke-epic3-dual-mt4-mt4.md.

#include <catch2/catch_test_macros.hpp>

#include "Device/DeviceSessionManager.h"
#include "Device/UnitIdentityRegistry.h"
#include "Profile/DeviceProfile.h"

#include <cstdio>
#include <fstream>
#include <string>

namespace
{
UnitIdentityResolveRequest makeResolve(
    UnitIdentityKind kind,
    const std::string& key,
    const std::string* topologyKey = nullptr)
{
    UnitIdentityResolveRequest request;
    request.kind = kind;
    request.key = &key;
    request.topologyKey = topologyKey;
    return request;
}
} // namespace

TEST_CASE("formatPortDisplayName keeps directional K=1 and K>=2 spelling", "[device][naming]")
{
    REQUIRE(formatPortDisplayName(1, 1, MidiPortDirection::In) == "MT4 In 1");
    REQUIRE(formatPortDisplayName(1, 4, MidiPortDirection::Out) == "MT4 Out 4");
    REQUIRE(formatPortDisplayName(2, 1, MidiPortDirection::In) == "MT4 #2 In 1");
    REQUIRE(formatPortDisplayName(2, 3, MidiPortDirection::Out) == "MT4 #2 Out 3");
}

TEST_CASE("buildPortNameSet uses assigned K (not a hard-wired unit-1-only path)",
          "[device][naming]")
{
    const DeviceProfile* mt4 = findDeviceProfile(kEmagicVendorId, kMt4ProductId);
    REQUIRE(mt4 != nullptr);

    DeviceSessionManager manager;
    PortNameSet namesK1;
    PortNameSet namesK2;
    std::string error;
    REQUIRE(manager.buildPortNameSet(*mt4, 1, namesK1, error));
    REQUIRE(manager.buildPortNameSet(*mt4, 2, namesK2, error));
    REQUIRE(namesK1.inNames[0] == "MT4 In 1");
    REQUIRE(namesK1.outNames[0] == "MT4 Out 1");
    REQUIRE(namesK2.inNames[0] == "MT4 #2 In 1");
    REQUIRE(namesK2.outNames[2] == "MT4 #2 Out 3");
}

TEST_CASE("UnitIdentityRegistry keeps known K when peer identity leaves",
          "[device][registry]")
{
    UnitIdentityRegistry registry;
    std::string error;
    unsigned kA = 0;
    unsigned kB = 0;
    const std::string keyA = "A";
    const std::string keyB = "B";
    REQUIRE(registry.resolveOrAssign(makeResolve(UnitIdentityKind::Serial, keyA), kA, error));
    REQUIRE(kA == 1);
    REQUIRE(registry.resolveOrAssign(makeResolve(UnitIdentityKind::Topology, keyB), kB, error));
    REQUIRE(kB == 2);

    unsigned kAAgain = 0;
    REQUIRE(registry.resolveOrAssign(makeResolve(UnitIdentityKind::Serial, keyA), kAAgain, error));
    REQUIRE(kAAgain == 1);

    char tempPath[L_tmpnam] = {};
#if defined(_MSC_VER)
    REQUIRE(tmpnam_s(tempPath, L_tmpnam) == 0);
#else
    REQUIRE(std::tmpnam(tempPath) != nullptr);
#endif
    REQUIRE(registry.saveToFile(tempPath, error));

    UnitIdentityRegistry reloaded;
    REQUIRE(reloaded.loadFromFile(tempPath, error));
    std::remove(tempPath);

    unsigned kBReload = 0;
    REQUIRE(reloaded.tryLookup(UnitIdentityKind::Topology, "B", kBReload));
    REQUIRE(kBReload == 2);

    unsigned kC = 0;
    const std::string keyC = "C";
    REQUIRE(reloaded.resolveOrAssign(makeResolve(UnitIdentityKind::Serial, keyC), kC, error));
    // Next free after 1 and 2 is 3 (B still reserved even if offline).
    REQUIRE(kC == 3);
}

TEST_CASE("UnitIdentityRegistry migrates topology binding to serial without new K",
          "[device][registry]")
{
    UnitIdentityRegistry registry;
    std::string error;
    unsigned kTopo = 0;
    const std::string topology = "USB\\VID_0A4A&PID_1000\\INSTANCE";
    REQUIRE(registry.resolveOrAssign(
        makeResolve(UnitIdentityKind::Topology, topology), kTopo, error));
    REQUIRE(kTopo == 1);

    unsigned kSerial = 0;
    const std::string serial = "MT4SERIAL1";
    REQUIRE(registry.resolveOrAssign(
        makeResolve(UnitIdentityKind::Serial, serial, &topology), kSerial, error));
    REQUIRE(kSerial == 1);

    unsigned lookup = 0;
    REQUIRE(registry.tryLookup(UnitIdentityKind::Serial, serial, lookup));
    REQUIRE(lookup == 1);
    REQUIRE(registry.tryLookup(UnitIdentityKind::Topology, topology, lookup));
    REQUIRE(lookup == 1);
}

TEST_CASE("UnitIdentityRegistry rejects partial ordinal tails on load", "[device][registry]")
{
    char tempPath[L_tmpnam] = {};
#if defined(_MSC_VER)
    REQUIRE(tmpnam_s(tempPath, L_tmpnam) == 0);
#else
    REQUIRE(std::tmpnam(tempPath) != nullptr);
#endif
    {
        std::ofstream out(tempPath, std::ios::trunc);
        REQUIRE(out);
        out << "unitor-unit-identity-v1\n";
        out << "serial|ABC|12abc\n";
    }
    UnitIdentityRegistry registry;
    std::string error;
    REQUIRE_FALSE(registry.loadFromFile(tempPath, error));
    REQUIRE(error.find("invalid ordinal") != std::string::npos);
    std::remove(tempPath);
}
