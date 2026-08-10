// WinMM port enumerate + needle match (exact / normalized ranks).

#pragma once

#include <string>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>

struct ResolvedPorts
{
    UINT outIndex = 0;
    UINT inIndex = 0;
    std::string outName;
    std::string inName;
};

bool resolvePortIndices(
    const std::string& outNeedle,
    const std::string& inNeedle,
    ResolvedPorts& resolved,
    std::string& errorOut);
