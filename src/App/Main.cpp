// Bridge process entry — user-session host (not a Windows Service).

#include "App/MapperSmoke.h"
#include "App/MidiSessionCli.h"
#include "Device/DeviceSessionManager.h"
#include "Midi/MidiBackend.h"
#include "Profile/DeviceProfile.h"
#include "Usb/WinUsbTransport.h"

#include <cstring>
#include <iostream>
#include <string>

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

    // IN 0→0,1→1 ; OUT 0..3→0..3 (Story 1.6 cable↔port tables).
    return inCount == 2
        && inIndices[0] == 0
        && inIndices[1] == 1
        && outCount == 4
        && outIndices[0] == 0
        && outIndices[1] == 1
        && outIndices[2] == 2
        && outIndices[3] == 3;
}

bool hasFlag(int argc, char* argv[], const char* flag) noexcept
{
    for (int index = 1; index < argc; ++index)
    {
        if (std::strcmp(argv[index], flag) == 0)
        {
            return true;
        }
    }
    return false;
}

int runProfileSmoke()
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

bool expectExactName(const std::string& actual, const char* expected, const char* label)
{
    if (actual != expected)
    {
        std::cerr << "Port name test failed (" << label << "): got \"" << actual
                  << "\", expected \"" << expected << "\"\n";
        return false;
    }
    return true;
}

bool testFormatPortDisplayNames()
{
    return expectExactName(formatPortDisplayName(1, 1), "MT4 Port 1", "K=1 Port 1")
        && expectExactName(formatPortDisplayName(1, 2), "MT4 Port 2", "K=1 Port 2")
        && expectExactName(formatPortDisplayName(1, 3), "MT4 Port 3", "K=1 Port 3")
        && expectExactName(formatPortDisplayName(1, 4), "MT4 Port 4", "K=1 Port 4")
        && expectExactName(formatPortDisplayName(2, 3), "MT4 #2 Port 3", "K=2 Port 3");
}

bool testBuiltMt4PortNameSet()
{
    const DeviceProfile* mt4 = findDeviceProfile(kEmagicVendorId, kMt4ProductId);
    if (mt4 == nullptr)
    {
        std::cerr << "MT4 DeviceProfile not found for port-name tests\n";
        return false;
    }

    DeviceSessionManager manager;
    PortNameSet names;
    std::string error;
    if (!manager.buildPortNameSet(*mt4, names, error))
    {
        std::cerr << "buildPortNameSet failed: " << error << '\n';
        return false;
    }

    if (names.inCount != 2 || names.outCount != 4)
    {
        std::cerr << "PortNameSet counts mismatch (expected 2 IN / 4 OUT)\n";
        return false;
    }

    return expectExactName(names.inNames[0], "MT4 Port 1", "IN 1")
        && expectExactName(names.inNames[1], "MT4 Port 2", "IN 2")
        && expectExactName(names.outNames[0], "MT4 Port 1", "OUT 1")
        && expectExactName(names.outNames[1], "MT4 Port 2", "OUT 2")
        && expectExactName(names.outNames[2], "MT4 Port 3", "OUT 3")
        && expectExactName(names.outNames[3], "MT4 Port 4", "OUT 4");
}

int runPortNameTests()
{
    if (!testFormatPortDisplayNames() || !testBuiltMt4PortNameSet())
    {
        return 1;
    }

    std::cout << "Port name tests passed\n";
    return 0;
}

int openMt4DeviceWithOptions(bool allowZadigFallback)
{
    const DeviceProfile* mt4 = findDeviceProfile(kEmagicVendorId, kMt4ProductId);
    if (mt4 == nullptr)
    {
        std::cerr << "MT4 DeviceProfile not found\n";
        return 1;
    }

    WinUsbTransport transport;
    WinUsbOpenOptions options;
    options.allowZadigFallback = allowZadigFallback;

    std::string error;
    if (!transport.Open(*mt4, error, options) || !transport.IsOpen())
    {
        std::cerr << "WinUSB open failed: "
                  << (error.empty() ? "unknown error" : error) << '\n';
        return 1;
    }

    std::cout << "WinUSB open succeeded\n";
    return 0;
}
} // namespace

int main(int argc, char* argv[])
{
    const int profileResult = runProfileSmoke();
    if (profileResult != 0)
    {
        return profileResult;
    }

    if (hasFlag(argc, argv, "--test-mapper"))
    {
        return runMapperTests();
    }

    if (hasFlag(argc, argv, "--test-port-names"))
    {
        return runPortNameTests();
    }

    const bool allowZadigFallback = hasFlag(argc, argv, "--dev-zadig");

    // --run-midi is an alias kept behind the same Start wiring as --start-session.
    if (hasFlag(argc, argv, "--start-session") || hasFlag(argc, argv, "--run-midi"))
    {
        return runMt4MidiSession(allowZadigFallback);
    }

    if (hasFlag(argc, argv, "--open-device"))
    {
        return openMt4DeviceWithOptions(allowZadigFallback);
    }

    return 0;
}
