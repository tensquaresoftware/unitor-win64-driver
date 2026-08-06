// Bridge process entry — user-session host (not a Windows Service).

#include "App/MapperSmoke.h"
#include "App/MidiSessionCli.h"
#include "Device/DeviceSessionManager.h"
#include "Midi/MidiBackend.h"
#include "Profile/DeviceProfile.h"
#include "Usb/WinUsbBulkProbe.h"
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
    return expectExactName(
               formatPortDisplayName(1, 1, MidiPortDirection::In),
               "MT4 Input 1",
               "K=1 Input 1")
        && expectExactName(
               formatPortDisplayName(1, 2, MidiPortDirection::In),
               "MT4 Input 2",
               "K=1 Input 2")
        && expectExactName(
               formatPortDisplayName(1, 1, MidiPortDirection::Out),
               "MT4 Output 1",
               "K=1 Output 1")
        && expectExactName(
               formatPortDisplayName(1, 4, MidiPortDirection::Out),
               "MT4 Output 4",
               "K=1 Output 4")
        && expectExactName(
               formatPortDisplayName(2, 3, MidiPortDirection::Out),
               "MT4 #2 Output 3",
               "K=2 Output 3")
        && expectExactName(
               formatPortDisplayName(2, 1, MidiPortDirection::In),
               "MT4 #2 Input 1",
               "K=2 Input 1");
}

bool portNameSetHasInOutCollision(const PortNameSet& names)
{
    for (std::size_t inIndex = 0; inIndex < names.inCount; ++inIndex)
    {
        for (std::size_t outIndex = 0; outIndex < names.outCount; ++outIndex)
        {
            if (names.inNames[inIndex] == names.outNames[outIndex])
            {
                std::cerr << "PortNameSet IN/OUT alias collision: \""
                          << names.inNames[inIndex] << "\"\n";
                return true;
            }
        }
    }
    return false;
}

bool expectMt4DirectionalPortNames(const PortNameSet& names)
{
    return expectExactName(names.inNames[0], "MT4 Input 1", "IN 1")
        && expectExactName(names.inNames[1], "MT4 Input 2", "IN 2")
        && expectExactName(names.outNames[0], "MT4 Output 1", "OUT 1")
        && expectExactName(names.outNames[1], "MT4 Output 2", "OUT 2")
        && expectExactName(names.outNames[2], "MT4 Output 3", "OUT 3")
        && expectExactName(names.outNames[3], "MT4 Output 4", "OUT 4");
}

bool printAndCheckBuiltMt4PortNameSet()
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

    std::cout << "Unit 1 Virtual Ports (" << names.inCount << " IN / " << names.outCount
              << " OUT):\n";
    for (std::size_t index = 0; index < names.inCount; ++index)
    {
        std::cout << "  IN  " << names.inNames[index] << '\n';
    }
    for (std::size_t index = 0; index < names.outCount; ++index)
    {
        std::cout << "  OUT " << names.outNames[index] << '\n';
    }

    std::cout << "Multi-unit samples:\n";
    std::cout << "  IN  " << formatPortDisplayName(2, 1, MidiPortDirection::In) << '\n';
    std::cout << "  OUT " << formatPortDisplayName(2, 3, MidiPortDirection::Out) << '\n';

    if (portNameSetHasInOutCollision(names))
    {
        return false;
    }

    return expectMt4DirectionalPortNames(names) && testFormatPortDisplayNames();
}

int runPortNameTests()
{
    if (!printAndCheckBuiltMt4PortNameSet())
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

    if (hasFlag(argc, argv, "--probe-usb"))
    {
        std::string probeError;
        const int probeResult = runMt4UsbBulkProbe(probeError);
        if (probeResult != 0)
        {
            std::cerr << "USB bulk probe failed: "
                      << (probeError.empty() ? "unknown error" : probeError) << '\n';
        }
        return probeResult;
    }

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
