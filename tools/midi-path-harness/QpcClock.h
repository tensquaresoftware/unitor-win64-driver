// QueryPerformanceCounter-based interval clock (AD-11). No Sleep-as-timer.

#pragma once

#include <cstdint>

class QpcClock
{
public:
    // Caches QueryPerformanceFrequency once. Returns false if QPC unavailable.
    bool initialize() noexcept;

    // Raw tick count (QueryPerformanceCounter).
    std::int64_t nowTicks() const noexcept;

    // Convert tick delta to microseconds (double for sub-us display).
    double ticksToMicroseconds(std::int64_t tickDelta) const noexcept;

    std::int64_t frequencyHz() const noexcept { return frequencyHz_; }

private:
    std::int64_t frequencyHz_ = 0;
};
