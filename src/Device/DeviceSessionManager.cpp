#include "Device/DeviceSessionManager.h"

#include <sstream>

std::string formatPortDisplayName(unsigned unitOrdinalK, unsigned portN)
{
    std::ostringstream stream;
    if (unitOrdinalK <= 1)
    {
        stream << "MT4 Port " << portN;
    }
    else
    {
        stream << "MT4 #" << unitOrdinalK << " Port " << portN;
    }
    return stream.str();
}

namespace
{
struct DirectionalNameFill
{
    uint16_t cableMask = 0;
    unsigned unitOrdinalK = 1;
    std::string* namesOut = nullptr;
    std::size_t maxNames = 0;
};

bool fillDirectionalNames(
    const DirectionalNameFill& fill,
    std::size_t& countOut,
    std::string& errorOut)
{
    if (fill.namesOut == nullptr || fill.maxNames == 0)
    {
        errorOut = "DeviceSessionManager: invalid PortNameSet destination";
        return false;
    }

    uint8_t cableIndices[kMaxEmagicCableCount] = {};
    const std::size_t cableCount = collectProductCableIndices(
        fill.cableMask, cableIndices, kMaxEmagicCableCount);

    if (cableCount == 0 || cableCount > fill.maxNames)
    {
        errorOut = "DeviceSessionManager: product port count is invalid for PortNameSet";
        return false;
    }

    for (std::size_t index = 0; index < cableCount; ++index)
    {
        // Port N is 1-based display order matching product cable ascending order.
        const unsigned portN = static_cast<unsigned>(index + 1);
        fill.namesOut[index] = formatPortDisplayName(fill.unitOrdinalK, portN);
    }

    countOut = cableCount;
    return true;
}
} // namespace

bool DeviceSessionManager::buildPortNameSet(
    const DeviceProfile& profile,
    PortNameSet& namesOut,
    std::string& errorOut) const
{
    PortNameSet built;
    DirectionalNameFill inFill{
        profile.inCables, unitOrdinalK_, built.inNames, kMaxMidiBackendInPorts};
    if (!fillDirectionalNames(inFill, built.inCount, errorOut))
    {
        return false;
    }

    DirectionalNameFill outFill{
        profile.outCables, unitOrdinalK_, built.outNames, kMaxMidiBackendOutPorts};
    if (!fillDirectionalNames(outFill, built.outCount, errorOut))
    {
        return false;
    }

    namesOut = std::move(built);
    errorOut.clear();
    return true;
}
