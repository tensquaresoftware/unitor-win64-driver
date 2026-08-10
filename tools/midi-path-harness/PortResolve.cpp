#include "PortResolve.h"

#include "PortNameNormalize.h"

#include <climits>
#include <vector>

namespace
{
std::string narrowDeviceName(const char* name)
{
    return name == nullptr ? std::string() : std::string(name);
}

int findBestDeviceIndex(
    const std::vector<std::string>& names,
    const std::string& needle)
{
    int bestIndex = -1;
    int bestRank = INT_MAX;
    bool tied = false;
    for (std::size_t index = 0; index < names.size(); ++index)
    {
        const int rank = portMatchRank(names[index], needle);
        if (rank < 0)
        {
            continue;
        }
        if (rank < bestRank)
        {
            bestRank = rank;
            bestIndex = static_cast<int>(index);
            tied = false;
            continue;
        }
        if (rank == bestRank)
        {
            tied = true;
        }
    }
    if (tied)
    {
        return -2;
    }
    return bestIndex;
}

bool enumerateOutNames(std::vector<std::string>& namesOut, std::string& errorOut)
{
    namesOut.clear();
    const UINT count = midiOutGetNumDevs();
    for (UINT index = 0; index < count; ++index)
    {
        MIDIOUTCAPSA caps = {};
        if (midiOutGetDevCapsA(index, &caps, sizeof(caps)) != MMSYSERR_NOERROR)
        {
            errorOut = "midiOutGetDevCapsA failed during enumeration";
            return false;
        }
        namesOut.push_back(narrowDeviceName(caps.szPname));
    }
    return true;
}

bool enumerateInNames(std::vector<std::string>& namesOut, std::string& errorOut)
{
    namesOut.clear();
    const UINT count = midiInGetNumDevs();
    for (UINT index = 0; index < count; ++index)
    {
        MIDIINCAPSA caps = {};
        if (midiInGetDevCapsA(index, &caps, sizeof(caps)) != MMSYSERR_NOERROR)
        {
            errorOut = "midiInGetDevCapsA failed during enumeration";
            return false;
        }
        namesOut.push_back(narrowDeviceName(caps.szPname));
    }
    return true;
}

bool matchIndexOrError(
    int found,
    const std::string& needle,
    const char* roleLabel,
    std::string& errorOut)
{
    if (found == -2)
    {
        errorOut = std::string("Ambiguous MIDI ") + roleLabel + " port match for \""
            + needle + "\" (multiple devices at best rank; narrow the needle)";
        return false;
    }
    if (found < 0)
    {
        errorOut = std::string("No MIDI ") + roleLabel + " port matched \"" + needle + "\"";
        return false;
    }
    return true;
}
} // namespace

bool resolvePortIndices(
    const std::string& outNeedle,
    const std::string& inNeedle,
    ResolvedPorts& resolved,
    std::string& errorOut)
{
    std::vector<std::string> outNames;
    std::vector<std::string> inNames;
    if (!enumerateOutNames(outNames, errorOut) || !enumerateInNames(inNames, errorOut))
    {
        return false;
    }
    const int outFound = findBestDeviceIndex(outNames, outNeedle);
    const int inFound = findBestDeviceIndex(inNames, inNeedle);
    if (!matchIndexOrError(outFound, outNeedle, "OUT", errorOut)
        || !matchIndexOrError(inFound, inNeedle, "IN", errorOut))
    {
        return false;
    }
    resolved.outIndex = static_cast<UINT>(outFound);
    resolved.inIndex = static_cast<UINT>(inFound);
    resolved.outName = outNames[static_cast<std::size_t>(outFound)];
    resolved.inName = inNames[static_cast<std::size_t>(inFound)];
    return true;
}
