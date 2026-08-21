// Bridge process entry — user-session host (not a Windows Service).

#include "App/AutoStartRegistration.h"
#include "App/BridgeVersion.h"
#include "App/MapperSmoke.h"
#include "App/MidiSessionCli.h"
#include "App/WmsBackendSmoke.h"
#include "Device/DeviceSessionManager.h"
#include "Midi/MidiBackend.h"
#include "Midi/MidiBackendSelect.h"
#include "Midi/SoftEchoGate.h"
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

const char* flagValueAfterEquals(const char* arg, const char* prefix) noexcept
{
    const std::size_t prefixLen = std::strlen(prefix);
    if (std::strncmp(arg, prefix, prefixLen) != 0)
    {
        return nullptr;
    }
    return arg + prefixLen;
}

bool applyMidiBackendFlag(int argc, char* argv[], std::string& errorOut)
{
    const char* selected = nullptr;
    for (int index = 1; index < argc; ++index)
    {
        const char* value = flagValueAfterEquals(argv[index], "--midi-backend=");
        if (value == nullptr)
        {
            continue;
        }
        selected = value;
    }
    if (selected == nullptr)
    {
        return true;
    }
    MidiBackendKind kind = MidiBackendKind::Wms;
    if (!parseMidiBackendKind(selected, kind))
    {
        errorOut =
            "Invalid --midi-backend value (expected wms or virtualmidi)";
        return false;
    }
    setMidiBackendKindOverride(kind);
    return true;
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
               "MT4 In 1",
               "K=1 In 1")
        && expectExactName(
               formatPortDisplayName(1, 2, MidiPortDirection::In),
               "MT4 In 2",
               "K=1 In 2")
        && expectExactName(
               formatPortDisplayName(1, 1, MidiPortDirection::Out),
               "MT4 Out 1",
               "K=1 Out 1")
        && expectExactName(
               formatPortDisplayName(1, 4, MidiPortDirection::Out),
               "MT4 Out 4",
               "K=1 Out 4")
        && expectExactName(
               formatPortDisplayName(2, 3, MidiPortDirection::Out),
               "MT4 #2 Out 3",
               "K=2 Out 3")
        && expectExactName(
               formatPortDisplayName(2, 1, MidiPortDirection::In),
               "MT4 #2 In 1",
               "K=2 In 1");
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
    return expectExactName(names.inNames[0], "MT4 In 1", "IN 1")
        && expectExactName(names.inNames[1], "MT4 In 2", "IN 2")
        && expectExactName(names.outNames[0], "MT4 Out 1", "OUT 1")
        && expectExactName(names.outNames[1], "MT4 Out 2", "OUT 2")
        && expectExactName(names.outNames[2], "MT4 Out 3", "OUT 3")
        && expectExactName(names.outNames[3], "MT4 Out 4", "OUT 4");
}

bool expectMt4Unit2DirectionalPortNames(const PortNameSet& names)
{
    return expectExactName(names.inNames[0], "MT4 #2 In 1", "K2 IN 1")
        && expectExactName(names.inNames[1], "MT4 #2 In 2", "K2 IN 2")
        && expectExactName(names.outNames[0], "MT4 #2 Out 1", "K2 OUT 1")
        && expectExactName(names.outNames[1], "MT4 #2 Out 2", "K2 OUT 2")
        && expectExactName(names.outNames[2], "MT4 #2 Out 3", "K2 OUT 3")
        && expectExactName(names.outNames[3], "MT4 #2 Out 4", "K2 OUT 4");
}

bool printPortNameSet(const char* title, const PortNameSet& names)
{
    std::cout << title << " (" << names.inCount << " IN / " << names.outCount << " OUT):\n";
    for (std::size_t index = 0; index < names.inCount; ++index)
    {
        std::cout << "  IN  " << names.inNames[index] << '\n';
    }
    for (std::size_t index = 0; index < names.outCount; ++index)
    {
        std::cout << "  OUT " << names.outNames[index] << '\n';
    }
    return !portNameSetHasInOutCollision(names);
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
    PortNameSet namesK2;
    std::string error;
    if (!manager.buildPortNameSet(*mt4, 1, names, error)
        || !manager.buildPortNameSet(*mt4, 2, namesK2, error))
    {
        std::cerr << "buildPortNameSet failed: " << error << '\n';
        return false;
    }
    if (names.inCount != 2 || names.outCount != 4)
    {
        std::cerr << "PortNameSet counts mismatch (expected 2 IN / 4 OUT)\n";
        return false;
    }

    return printPortNameSet("Unit 1 Virtual Ports", names)
        && printPortNameSet("Unit 2 Virtual Ports", namesK2)
        && expectMt4DirectionalPortNames(names)
        && expectMt4Unit2DirectionalPortNames(namesK2)
        && testFormatPortDisplayNames();
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

int runAutoStartRegisterCommand()
{
    std::string message;
    std::string error;
    if (!registerAutoStart(message, error))
    {
        std::cerr << "Auto-Start register failed: "
                  << (error.empty() ? "unknown error" : error) << '\n';
        return 1;
    }
    std::cout << message << '\n';
    return 0;
}

int runAutoStartUnregisterCommand()
{
    std::string message;
    std::string error;
    if (!unregisterAutoStart(message, error))
    {
        std::cerr << "Auto-Start unregister failed: "
                  << (error.empty() ? "unknown error" : error) << '\n';
        return 1;
    }
    std::cout << message << '\n';
    return 0;
}

int runProbeUsbCommand()
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

int dispatchSessionFlags(int argc, char* argv[])
{
    // Auto-Start must never pick up a leftover UNITOR_MIDI_SOFT_ECHO (studio fail-closed).
    const bool autoSession = hasFlag(argc, argv, kAutoSessionFlag);
    configureSoftEchoGate(
        !autoSession && hasFlag(argc, argv, "--soft-echo"),
        autoSession || hasFlag(argc, argv, "--no-soft-echo"));
    const bool allowZadigFallback = hasFlag(argc, argv, "--dev-zadig");
    if (hasFlag(argc, argv, kAutoSessionFlag))
    {
        return runMt4AutoSession();
    }
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

int dispatchBridgeFlags(int argc, char* argv[])
{
    if (hasFlag(argc, argv, "--test-mapper"))
    {
        return runMapperTests();
    }
    if (hasFlag(argc, argv, "--test-port-names"))
    {
        return runPortNameTests();
    }
    if (hasFlag(argc, argv, "--test-wms-ports"))
    {
        return runWmsBackendLifecycleSmoke();
    }
    if (hasFlag(argc, argv, "--probe-usb"))
    {
        return runProbeUsbCommand();
    }
    if (hasFlag(argc, argv, "--register-auto-start"))
    {
        return runAutoStartRegisterCommand();
    }
    if (hasFlag(argc, argv, "--unregister-auto-start"))
    {
        return runAutoStartUnregisterCommand();
    }
    return dispatchSessionFlags(argc, argv);
}
} // namespace

int main(int argc, char* argv[])
{
    // Answer --version before profile smoke so the version is always available.
    if (hasFlag(argc, argv, "--version"))
    {
        std::cout << kBridgeProductName << " " << kBridgeVersionString << '\n';
        return 0;
    }
    if (hasFlag(argc, argv, "--help") || hasFlag(argc, argv, "-h"))
    {
        std::cout
            << kBridgeProductName << " " << kBridgeVersionString << "\n"
            << "Usage (selected flags):\n"
            << "  --version\n"
            << "  --help\n"
            << "  --start-session | --run-midi | --auto-session\n"
            << "  --dev-zadig\n"
            << "  --soft-echo | --no-soft-echo   Lab software-loop only;\n"
            << "      default OFF. Env UNITOR_MIDI_SOFT_ECHO=1|true|yes also enables\n"
            << "      unless --no-soft-echo. --auto-session always forces OFF (ignores env).\n"
            << "  --midi-backend=wms|virtualmidi\n"
            << "      Default wms (Win11 community / fail closed if WMS missing).\n"
            << "      Win10 Setup registers --midi-backend=virtualmidi via Auto-Start.\n"
            << "      Env UNITOR_MIDI_BACKEND=wms|virtualmidi also selects when flag omitted.\n"
            << "  --register-auto-start | --unregister-auto-start\n"
            << "  --probe-usb | --test-mapper | --test-port-names | --test-wms-ports\n";
        return 0;
    }

    std::string backendError;
    if (!applyMidiBackendFlag(argc, argv, backendError))
    {
        std::cerr << backendError << '\n';
        return 1;
    }
    const int profileResult = runProfileSmoke();
    if (profileResult != 0)
    {
        return profileResult;
    }
    return dispatchBridgeFlags(argc, argv);
}
