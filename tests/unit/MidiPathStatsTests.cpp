#include "MidiPathStats.h"

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <limits>
#include <vector>

TEST_CASE("summarizeMidiPathLatenciesUs empty is zero", "[midi-path-stats]")
{
    const MidiPathLatencySummary summary = summarizeMidiPathLatenciesUs({});
    REQUIRE(summary.minUs == 0.0);
    REQUIRE(summary.jitterUsP99 == 0.0);
    REQUIRE(summary.latencySpreadUs == 0.0);
}

TEST_CASE("summarizeMidiPathLatenciesUs classical jitter from median", "[midi-path-stats]")
{
    // Sorted: 100, 110, 120, 130, 1000 — median 120
    // absdevs: 20, 10, 0, 10, 880 → sorted 0,10,10,20,880
    const std::vector<double> samples{100.0, 110.0, 120.0, 130.0, 1000.0};
    const MidiPathLatencySummary summary = summarizeMidiPathLatenciesUs(samples);
    REQUIRE(summary.minUs == 100.0);
    REQUIRE(summary.maxUs == 1000.0);
    REQUIRE(summary.medianUs == 120.0);
    REQUIRE(summary.p99Us == 1000.0); // n=5 → index 4
    REQUIRE(summary.latencySpreadUs == 900.0);
    REQUIRE(summary.jitterUsMax == 880.0);
    REQUIRE(summary.jitterUsP99 == 880.0);
    REQUIRE(std::abs(summary.jitterUsMean - 184.0) < 1e-9);
}

TEST_CASE("summarizeMidiPathLatenciesUs even-count median average", "[midi-path-stats]")
{
    const std::vector<double> samples{10.0, 20.0, 30.0, 40.0};
    const MidiPathLatencySummary summary = summarizeMidiPathLatenciesUs(samples);
    REQUIRE(summary.medianUs == 25.0);
    // absdevs: 15, 5, 5, 15 → p99 index 3 → 15
    REQUIRE(summary.jitterUsP99 == 15.0);
    REQUIRE(summary.jitterUsMax == 15.0);
}

TEST_CASE("summarizeMidiPathLatenciesUs rejects non-finite samples", "[midi-path-stats]")
{
    const MidiPathLatencySummary nanSummary =
        summarizeMidiPathLatenciesUs({100.0, std::numeric_limits<double>::quiet_NaN(), 120.0});
    REQUIRE(nanSummary.minUs == 0.0);
    REQUIRE(nanSummary.p99Us == 0.0);
    REQUIRE(nanSummary.jitterUsP99 == 0.0);

    const MidiPathLatencySummary infSummary =
        summarizeMidiPathLatenciesUs({100.0, std::numeric_limits<double>::infinity()});
    REQUIRE(infSummary.maxUs == 0.0);
    REQUIRE(infSummary.latencySpreadUs == 0.0);
}

TEST_CASE("summarizeMidiPathLatenciesUs n100 p99 equals max under method index", "[midi-path-stats]")
{
    std::vector<double> samples(100, 1000.0);
    samples[99] = 2000.0; // sorted index 99 = max; n*99/100 = 99
    const MidiPathLatencySummary summary = summarizeMidiPathLatenciesUs(samples);
    REQUIRE(summary.p99Us == 2000.0);
    REQUIRE(summary.maxUs == 2000.0);
    REQUIRE(summary.p99Us == summary.maxUs);
}
