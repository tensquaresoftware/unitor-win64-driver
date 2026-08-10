// Pure MIDI Path latency/jitter summary (no WinMM). Host-winmm-qpc series only.

#pragma once

#include <cstddef>
#include <vector>

struct MidiPathLatencySummary
{
    double minUs = 0.0;
    double maxUs = 0.0;
    double meanUs = 0.0;
    double p99Us = 0.0;
    double medianUs = 0.0;
    // Classical jitter: absolute deviation from median of the latency series.
    double jitterUsMean = 0.0;
    double jitterUsP99 = 0.0;
    double jitterUsMax = 0.0;
    // Docs continuity: latency_us_p99 - latency_us_min (not classical jitter).
    double latencySpreadUs = 0.0;
};

// Empty input → all zeros. Same p99 index rule as published method: n*99/100.
MidiPathLatencySummary summarizeMidiPathLatenciesUs(const std::vector<double>& latencyUs);
