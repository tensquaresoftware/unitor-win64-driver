#include "MidiPathStats.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace
{
std::size_t p99IndexForSortedSize(std::size_t count) noexcept
{
    if (count == 0)
    {
        return 0;
    }
    const std::size_t index = (count * 99) / 100;
    return index >= count ? count - 1 : index;
}

double medianOfSorted(const std::vector<double>& sorted)
{
    const std::size_t count = sorted.size();
    if (count == 0)
    {
        return 0.0;
    }
    if ((count % 2u) == 1u)
    {
        return sorted[count / 2u];
    }
    return (sorted[(count / 2u) - 1u] + sorted[count / 2u]) * 0.5;
}
} // namespace

MidiPathLatencySummary summarizeMidiPathLatenciesUs(const std::vector<double>& latencyUs)
{
    MidiPathLatencySummary summary;
    if (latencyUs.empty())
    {
        return summary;
    }

    for (const double sample : latencyUs)
    {
        if (!std::isfinite(sample))
        {
            return MidiPathLatencySummary{};
        }
    }

    std::vector<double> sorted = latencyUs;
    std::sort(sorted.begin(), sorted.end());

    summary.minUs = sorted.front();
    summary.maxUs = sorted.back();
    const double sum = std::accumulate(sorted.begin(), sorted.end(), 0.0);
    summary.meanUs = sum / static_cast<double>(sorted.size());
    summary.p99Us = sorted[p99IndexForSortedSize(sorted.size())];
    summary.medianUs = medianOfSorted(sorted);
    summary.latencySpreadUs = summary.p99Us - summary.minUs;

    std::vector<double> absDev;
    absDev.reserve(sorted.size());
    for (const double sample : sorted)
    {
        absDev.push_back(std::abs(sample - summary.medianUs));
    }
    std::sort(absDev.begin(), absDev.end());
    const double absSum = std::accumulate(absDev.begin(), absDev.end(), 0.0);
    summary.jitterUsMean = absSum / static_cast<double>(absDev.size());
    summary.jitterUsP99 = absDev[p99IndexForSortedSize(absDev.size())];
    summary.jitterUsMax = absDev.back();
    return summary;
}
