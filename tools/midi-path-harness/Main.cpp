// MIDI Path harness entry — measures Bridge Virtual Port path with QPC (AD-11).

#include "MidiPathRunner.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace
{
struct ArgCursor
{
    int argc = 0;
    char** argv = nullptr;
    int index = 0;
};

void printHelp()
{
    std::cout
        << "MidiPathHarness — measure MIDI Path latency/jitter (QPC inject/observe)\n"
        << "Jitter: p99 of |sample - median| (jitter_us_p99); not ASIO buffer size.\n"
        << "\n"
        << "Usage:\n"
        << "  MidiPathHarness --path software-loop [options]\n"
        << "  MidiPathHarness --path hardware-loop --confirm-soft-echo-off [options]\n"
        << "\n"
        << "Options:\n"
        << "  --help                      Show this help\n"
        << "  --path <type>               software-loop (required first) | hardware-loop\n"
        << "  --out <name>                Bridge OUT port (default: MT4 Out 1)\n"
        << "  --in <name>                 Bridge IN port (default: MT4 In 1)\n"
        << "  --samples <n>               Sample count (default: 100)\n"
        << "  --timeout-ms <n>            Per-sample wait (default: 2000; min 1)\n"
        << "  --json                      JSON summary on stdout\n"
        << "  --confirm-soft-echo-off     Required for hardware-loop (operator asserts\n"
        << "                              Bridge soft-echo is OFF; prevents fake Pass)\n"
        << "\n"
        << "Measurement plane (locked):\n"
        << "  Inject: QueryPerformanceCounter immediately before midiOutShortMsg\n"
        << "  Observe: QueryPerformanceCounter in midiIn callback on matching Note On\n"
        << "  Never uses ASIO buffer size as MIDI Path proof.\n"
        << "\n"
        << "software-loop: Bridge must run with --soft-echo (or UNITOR_MIDI_SOFT_ECHO=1).\n"
        << "  Soft-echo copies host→OUT to matching IN without USB/DIN claim.\n"
        << "  Bridge --no-soft-echo forces the gate OFF even if the env is set.\n"
        << "hardware-loop: DIN Out→In required; Bridge soft-echo must be OFF.\n"
        << "  Requires --confirm-soft-echo-off. Missing DIN must not be reported as Pass.\n"
        << "\n"
        << "Lab teardown: prefer Ctrl+C on Bridge (CTRL_CLOSE can orphan Virtual Ports).\n"
        << "JSON studio_done=false: one harness run is not the Studio-Done Gate decision.\n"
        << "Gate timing claim (if any): docs/dev/measurements/studio-done-gate-decision.md.\n";
}

bool parseUnsigned(const char* text, unsigned& outValue)
{
    if (text == nullptr || text[0] == '\0')
    {
        return false;
    }
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' || parsed > 1000000ul)
    {
        return false;
    }
    outValue = static_cast<unsigned>(parsed);
    return true;
}

bool takeNextValue(ArgCursor& cursor, const char* flag, const char*& valueOut)
{
    if (cursor.index + 1 >= cursor.argc)
    {
        std::cerr << "Missing value for " << flag << '\n';
        return false;
    }
    valueOut = cursor.argv[++cursor.index];
    return true;
}

bool applyPathValue(const char* value, MidiPathRunConfig& config)
{
    if (std::strcmp(value, "software-loop") == 0)
    {
        config.pathType = MidiPathType::SoftwareLoop;
        return true;
    }
    if (std::strcmp(value, "hardware-loop") == 0)
    {
        config.pathType = MidiPathType::HardwareLoop;
        return true;
    }
    std::cerr << "Unknown --path (use software-loop|hardware-loop)\n";
    return false;
}

bool applyPathFlag(ArgCursor& cursor, MidiPathRunConfig& config, bool& pathSet)
{
    const char* value = nullptr;
    if (!takeNextValue(cursor, "--path", value) || !applyPathValue(value, config))
    {
        return false;
    }
    pathSet = true;
    return true;
}

bool applyPortFlag(ArgCursor& cursor, std::string& target, const char* flag)
{
    const char* value = nullptr;
    if (!takeNextValue(cursor, flag, value))
    {
        return false;
    }
    target = value;
    return true;
}

bool applyUnsignedFlag(ArgCursor& cursor, unsigned& target, const char* flag)
{
    const char* value = nullptr;
    if (!takeNextValue(cursor, flag, value) || !parseUnsigned(value, target))
    {
        std::cerr << "Invalid " << flag << '\n';
        return false;
    }
    return true;
}

bool applyFlag(ArgCursor& cursor, MidiPathRunConfig& config, bool& pathSet)
{
    const char* arg = cursor.argv[cursor.index];
    if (std::strcmp(arg, "--json") == 0)
    {
        config.jsonSummary = true;
        return true;
    }
    if (std::strcmp(arg, "--confirm-soft-echo-off") == 0)
    {
        config.confirmSoftEchoOff = true;
        return true;
    }
    if (std::strcmp(arg, "--path") == 0)
    {
        return applyPathFlag(cursor, config, pathSet);
    }
    if (std::strcmp(arg, "--out") == 0)
    {
        return applyPortFlag(cursor, config.outPort, "--out");
    }
    if (std::strcmp(arg, "--in") == 0)
    {
        return applyPortFlag(cursor, config.inPort, "--in");
    }
    if (std::strcmp(arg, "--samples") == 0)
    {
        return applyUnsignedFlag(cursor, config.sampleCount, "--samples");
    }
    if (std::strcmp(arg, "--timeout-ms") == 0)
    {
        return applyUnsignedFlag(cursor, config.timeoutMs, "--timeout-ms");
    }
    std::cerr << "Unknown argument: " << arg << '\n';
    return false;
}

bool finishParse(const MidiPathRunConfig& config, bool showHelp, bool pathSet)
{
    if (!showHelp && !pathSet)
    {
        std::cerr << "Required: --path software-loop|hardware-loop (see --help)\n";
        return false;
    }
    if (config.sampleCount == 0)
    {
        std::cerr << "--samples must be >= 1\n";
        return false;
    }
    if (config.timeoutMs == 0)
    {
        std::cerr << "--timeout-ms must be >= 1\n";
        return false;
    }
    if (config.pathType == MidiPathType::HardwareLoop && !config.confirmSoftEchoOff)
    {
        std::cerr
            << "hardware-loop requires --confirm-soft-echo-off "
               "(assert Bridge soft-echo is OFF; prevents soft-echo fake Pass)\n";
        return false;
    }
    return true;
}

bool parseArgs(int argc, char* argv[], MidiPathRunConfig& config, bool& showHelp)
{
    showHelp = false;
    bool pathSet = false;
    ArgCursor cursor;
    cursor.argc = argc;
    cursor.argv = argv;
    for (cursor.index = 1; cursor.index < argc; ++cursor.index)
    {
        const char* arg = argv[cursor.index];
        if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0)
        {
            showHelp = true;
            return true;
        }
        if (!applyFlag(cursor, config, pathSet))
        {
            return false;
        }
    }
    return finishParse(config, showHelp, pathSet);
}
} // namespace

int main(int argc, char* argv[])
{
    MidiPathRunConfig config;
    bool showHelp = false;
    if (!parseArgs(argc, argv, config, showHelp))
    {
        return 2;
    }
    if (showHelp)
    {
        printHelp();
        return 0;
    }

    const MidiPathRunResult result = runMidiPathMeasurement(config);
    printMidiPathResult(result, config.jsonSummary);
    return result.ok ? 0 : 1;
}
