// MIDI Path software/hardware loop runner — QPC inject/observe plane only.

#pragma once

#include "QpcClock.h"
#include "WinMmMidiIo.h"

#include <string>
#include <vector>

enum class MidiPathType
{
    SoftwareLoop,
    HardwareLoop
};

struct MidiPathRunConfig
{
    MidiPathType pathType = MidiPathType::SoftwareLoop;
    std::string outPort = "MT4 Out 1";
    std::string inPort = "MT4 In 1";
    unsigned sampleCount = 100;
    unsigned timeoutMs = 2000;
    bool jsonSummary = false;
    // Required for hardware-loop: operator asserts Bridge soft-echo is OFF.
    bool confirmSoftEchoOff = false;
};

struct MidiPathSample
{
    double latencyUs = 0.0;
};

struct MidiPathRunResult
{
    bool ok = false;
    std::string error;
    std::string pathTypeLabel;
    std::string outPortOpened;
    std::string inPortOpened;
    std::vector<MidiPathSample> samples;
    double minUs = 0.0;
    double maxUs = 0.0;
    double meanUs = 0.0;
    double p99Us = 0.0;
    double medianUs = 0.0;
    // Classical jitter: p99/max/mean of |sample - median| (µs).
    double jitterUsMean = 0.0;
    double jitterUsP99 = 0.0;
    double jitterUsMax = 0.0;
    // latency_us_p99 - latency_us_min — not classical jitter.
    double latencySpreadUs = 0.0;
};

MidiPathRunResult runMidiPathMeasurement(const MidiPathRunConfig& config);
void printMidiPathResult(const MidiPathRunResult& result, bool jsonSummary);
