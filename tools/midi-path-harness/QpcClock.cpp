#include "QpcClock.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

bool QpcClock::initialize() noexcept
{
    LARGE_INTEGER frequency = {};
    if (QueryPerformanceFrequency(&frequency) == 0 || frequency.QuadPart <= 0)
    {
        frequencyHz_ = 0;
        return false;
    }
    frequencyHz_ = frequency.QuadPart;
    return true;
}

std::int64_t QpcClock::nowTicks() const noexcept
{
    LARGE_INTEGER counter = {};
    if (QueryPerformanceCounter(&counter) == 0)
    {
        return 0;
    }
    return counter.QuadPart;
}

double QpcClock::ticksToMicroseconds(std::int64_t tickDelta) const noexcept
{
    if (frequencyHz_ <= 0)
    {
        return 0.0;
    }
    return (static_cast<double>(tickDelta) * 1000000.0)
        / static_cast<double>(frequencyHz_);
}
