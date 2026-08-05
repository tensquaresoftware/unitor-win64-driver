// Windows-only helpers for VirtualMidiBackend (UTF-8 names, diagnostics).

#include "Midi/VirtualMidiWinSupport.h"

#ifdef _WIN32

#include "Midi/MidiBackend.h"

#include <sstream>

namespace
{
constexpr const char* kMissingDriverFixPath = kVirtualMidiMissingDriverFixPath;

bool isBlankPortName(const std::string& name)
{
    return name.find_first_not_of(" \t\r\n") == std::string::npos;
}
} // namespace

std::string formatVirtualMidiLastError(const char* action)
{
    const DWORD code = GetLastError();
    std::ostringstream stream;
    stream << action << " failed (Win32=" << code << ")";
    if (code == ERROR_PATH_NOT_FOUND || code == ERROR_MOD_NOT_FOUND)
    {
        stream << ": " << kMissingDriverFixPath;
    }
    else if (code == ERROR_ALIAS_EXISTS)
    {
        stream << ": a VirtualMIDI port with this display name already exists "
                  "(close loopMIDI entries or a leftover Bridge session, then retry)";
    }
    return stream.str();
}

bool utf8ToWideVirtualMidiName(
    const std::string& utf8,
    std::wstring& wideOut,
    std::string& errorOut)
{
    if (utf8.empty())
    {
        errorOut = "VirtualMIDI port name must not be empty";
        return false;
    }

    const int needed = MultiByteToWideChar(
        CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (needed <= 0)
    {
        errorOut = formatVirtualMidiLastError("UTF-8 to wide conversion size query");
        return false;
    }

    wideOut.assign(static_cast<std::size_t>(needed), L'\0');
    const int written = MultiByteToWideChar(
        CP_UTF8, 0, utf8.c_str(), -1, wideOut.data(), needed);
    if (written <= 0)
    {
        errorOut = formatVirtualMidiLastError("UTF-8 to wide conversion");
        return false;
    }

    if (!wideOut.empty() && wideOut.back() == L'\0')
    {
        wideOut.pop_back();
    }
    return true;
}

bool validateVirtualMidiPortNameSet(const PortNameSet& names, std::string& errorOut)
{
    if (names.inCount == 0 && names.outCount == 0)
    {
        errorOut = "VirtualMIDI CreatePortSet rejected empty PortNameSet (fail closed)";
        return false;
    }
    if (names.inCount > kMaxMidiBackendInPorts || names.outCount > kMaxMidiBackendOutPorts)
    {
        errorOut = "VirtualMIDI CreatePortSet port counts exceed backend limits";
        return false;
    }
    for (std::size_t index = 0; index < names.inCount; ++index)
    {
        if (isBlankPortName(names.inNames[index]))
        {
            errorOut = "VirtualMIDI CreatePortSet rejected blank IN port display name";
            return false;
        }
    }
    for (std::size_t index = 0; index < names.outCount; ++index)
    {
        if (isBlankPortName(names.outNames[index]))
        {
            errorOut = "VirtualMIDI CreatePortSet rejected blank OUT port display name";
            return false;
        }
    }
    return true;
}

namespace
{
constexpr DWORD kInPortFlags = kTeVmFlagsParseTx | kTeVmFlagsInstantiateTx;
constexpr DWORD kOutPortFlags = kTeVmFlagsParseRx | kTeVmFlagsInstantiateRx;

std::size_t findMergedPlanIndex(
    const MergedVirtualMidiPlan* plans,
    std::size_t planCount,
    const std::string& name)
{
    for (std::size_t index = 0; index < planCount; ++index)
    {
        if (plans[index].name != nullptr && *plans[index].name == name)
        {
            return index;
        }
    }
    return planCount;
}

struct AppendMergedPlanArgs
{
    MergedVirtualMidiPlan* plans = nullptr;
    std::size_t* planCount = nullptr;
    const std::string* name = nullptr;
    DWORD flags = 0;
    int inIndex = -1;
    int outIndex = -1;
};

bool appendMergedPlan(AppendMergedPlanArgs& args, std::string& errorOut)
{
    if (args.plans == nullptr || args.planCount == nullptr || args.name == nullptr)
    {
        errorOut = "VirtualMIDI appendMergedPlan received null fields";
        return false;
    }
    if (*args.planCount >= kMaxMergedVirtualMidiPlans)
    {
        errorOut = "VirtualMIDI merged port plan exceeds capacity";
        return false;
    }
    args.plans[*args.planCount].name = args.name;
    args.plans[*args.planCount].flags = args.flags;
    args.plans[*args.planCount].inIndex = args.inIndex;
    args.plans[*args.planCount].outIndex = args.outIndex;
    ++(*args.planCount);
    return true;
}

bool mergeInPortNames(
    const PortNameSet& names,
    MergedVirtualMidiPlan* plans,
    std::size_t& planCount,
    std::string& errorOut)
{
    for (std::size_t index = 0; index < names.inCount; ++index)
    {
        const std::size_t planIndex =
            findMergedPlanIndex(plans, planCount, names.inNames[index]);
        if (planIndex == planCount)
        {
            AppendMergedPlanArgs args;
            args.plans = plans;
            args.planCount = &planCount;
            args.name = &names.inNames[index];
            args.flags = kInPortFlags;
            args.inIndex = static_cast<int>(index);
            if (!appendMergedPlan(args, errorOut))
            {
                return false;
            }
            continue;
        }
        plans[planIndex].flags |= kInPortFlags;
        plans[planIndex].inIndex = static_cast<int>(index);
    }
    return true;
}

bool mergeOutPortNames(
    const PortNameSet& names,
    MergedVirtualMidiPlan* plans,
    std::size_t& planCount,
    std::string& errorOut)
{
    for (std::size_t index = 0; index < names.outCount; ++index)
    {
        const std::size_t planIndex =
            findMergedPlanIndex(plans, planCount, names.outNames[index]);
        if (planIndex == planCount)
        {
            AppendMergedPlanArgs args;
            args.plans = plans;
            args.planCount = &planCount;
            args.name = &names.outNames[index];
            args.flags = kOutPortFlags;
            args.outIndex = static_cast<int>(index);
            if (!appendMergedPlan(args, errorOut))
            {
                return false;
            }
            continue;
        }
        plans[planIndex].flags |= kOutPortFlags;
        plans[planIndex].outIndex = static_cast<int>(index);
    }
    return true;
}
} // namespace

bool buildMergedVirtualMidiPlans(
    const PortNameSet& names,
    MergedVirtualMidiPlan* plans,
    std::size_t& planCount,
    std::string& errorOut)
{
    planCount = 0;
    return mergeInPortNames(names, plans, planCount, errorOut)
        && mergeOutPortNames(names, plans, planCount, errorOut);
}

#endif // _WIN32
