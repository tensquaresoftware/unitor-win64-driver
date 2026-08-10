#include "MidiPathRunner.h"

#include "MidiPathStats.h"

#include <iostream>

namespace
{
const char* pathTypeLabel(MidiPathType type) noexcept
{
    return type == MidiPathType::HardwareLoop ? "hardware-loop" : "software-loop";
}

void summarizeSamples(MidiPathRunResult& result)
{
    std::vector<double> values;
    values.reserve(result.samples.size());
    for (const MidiPathSample& sample : result.samples)
    {
        values.push_back(sample.latencyUs);
    }
    const MidiPathLatencySummary summary = summarizeMidiPathLatenciesUs(values);
    result.minUs = summary.minUs;
    result.maxUs = summary.maxUs;
    result.meanUs = summary.meanUs;
    result.p99Us = summary.p99Us;
    result.medianUs = summary.medianUs;
    result.jitterUsMean = summary.jitterUsMean;
    result.jitterUsP99 = summary.jitterUsP99;
    result.jitterUsMax = summary.jitterUsMax;
    result.latencySpreadUs = summary.latencySpreadUs;
}

struct SampleAttempt
{
    WinMmMidiIo* io = nullptr;
    const QpcClock* clock = nullptr;
    unsigned sampleIndex = 0;
    unsigned timeoutMs = 0;
};

bool runOneSample(const SampleAttempt& attempt, MidiPathSample& sampleOut, std::string& errorOut)
{
    const auto note =
        static_cast<std::uint8_t>(36u + (attempt.sampleIndex % 48u));
    const auto velocity =
        static_cast<std::uint8_t>(1u + (attempt.sampleIndex % 126u));
    attempt.io->armExpectedNote(note, velocity);
    if (!attempt.io->injectNoteOn(*attempt.clock, note, velocity))
    {
        errorOut = "midiOutShortMsg failed on inject";
        return false;
    }
    std::int64_t observeTicks = 0;
    if (!attempt.io->waitForObserve(observeTicks, attempt.timeoutMs))
    {
        errorOut =
            "Timed out waiting for matching Note On on IN "
            "(software-loop needs Bridge --soft-echo; hardware-loop needs DIN)";
        return false;
    }
    const std::int64_t delta = observeTicks - attempt.io->lastInjectTicks();
    if (delta < 0)
    {
        errorOut = "Negative latency (clock anomaly)";
        return false;
    }
    sampleOut.latencyUs = attempt.clock->ticksToMicroseconds(delta);
    return true;
}
} // namespace

MidiPathRunResult runMidiPathMeasurement(const MidiPathRunConfig& config)
{
    MidiPathRunResult result;
    result.pathTypeLabel = pathTypeLabel(config.pathType);

    QpcClock clock;
    if (!clock.initialize())
    {
        result.error = "QueryPerformanceFrequency/Counter unavailable";
        return result;
    }

    WinMmMidiIo io;
    std::string openError;
    if (!io.openPorts(config.outPort, config.inPort, openError))
    {
        result.error = openError;
        return result;
    }
    result.outPortOpened = io.openedOutName();
    result.inPortOpened = io.openedInName();

    result.samples.reserve(config.sampleCount);
    for (unsigned index = 0; index < config.sampleCount; ++index)
    {
        SampleAttempt attempt;
        attempt.io = &io;
        attempt.clock = &clock;
        attempt.sampleIndex = index;
        attempt.timeoutMs = config.timeoutMs;
        MidiPathSample sample;
        std::string sampleError;
        if (!runOneSample(attempt, sample, sampleError))
        {
            result.error = sampleError;
            return result;
        }
        result.samples.push_back(sample);
    }

    summarizeSamples(result);
    result.ok = true;
    return result;
}

void printPlainSummary(const MidiPathRunResult& result)
{
    std::cout << "path_type=" << result.pathTypeLabel << '\n'
              << "out_port=" << result.outPortOpened << '\n'
              << "in_port=" << result.inPortOpened << '\n'
              << "samples=" << result.samples.size() << '\n'
              << "latency_us_min=" << result.minUs << '\n'
              << "latency_us_mean=" << result.meanUs << '\n'
              << "latency_us_median=" << result.medianUs << '\n'
              << "latency_us_p99=" << result.p99Us << '\n'
              << "latency_us_max=" << result.maxUs << '\n'
              << "latency_spread_us=" << result.latencySpreadUs << '\n'
              << "jitter_us_mean=" << result.jitterUsMean << '\n'
              << "jitter_us_p99=" << result.jitterUsP99 << '\n'
              << "jitter_us_max=" << result.jitterUsMax << '\n'
              << "jitter_def=p99_abs_dev_from_median\n"
              << "plane=host-winmm-qpc (not ASIO; run studio_done=false — see Gate decision doc)\n";
}

std::string jsonEscape(const std::string& text)
{
    std::string escaped;
    escaped.reserve(text.size() + 8);
    for (const char ch : text)
    {
        if (ch == '\\' || ch == '"')
        {
            escaped.push_back('\\');
        }
        escaped.push_back(ch);
    }
    return escaped;
}

void printJsonSummary(const MidiPathRunResult& result)
{
    std::cout << "{"
              << "\"path_type\":\"" << result.pathTypeLabel << "\","
              << "\"out_port\":\"" << jsonEscape(result.outPortOpened) << "\","
              << "\"in_port\":\"" << jsonEscape(result.inPortOpened) << "\","
              << "\"samples\":" << result.samples.size() << ","
              << "\"latency_us_min\":" << result.minUs << ","
              << "\"latency_us_mean\":" << result.meanUs << ","
              << "\"latency_us_median\":" << result.medianUs << ","
              << "\"latency_us_p99\":" << result.p99Us << ","
              << "\"latency_us_max\":" << result.maxUs << ","
              << "\"latency_spread_us\":" << result.latencySpreadUs << ","
              << "\"jitter_us_mean\":" << result.jitterUsMean << ","
              << "\"jitter_us_p99\":" << result.jitterUsP99 << ","
              << "\"jitter_us_max\":" << result.jitterUsMax << ","
              << "\"jitter_def\":\"p99_abs_dev_from_median\","
              << "\"plane\":\"host-winmm-qpc\","
              << "\"asio_buffer_proof\":false,"
              << "\"studio_done\":false"
              << "}\n";
}

void printMidiPathResult(const MidiPathRunResult& result, bool jsonSummary)
{
    if (!result.ok)
    {
        std::cerr << "midi-path-harness failed: " << result.error << '\n';
        return;
    }
    if (jsonSummary)
    {
        printJsonSummary(result);
        return;
    }
    printPlainSummary(result);
}
