// VirtualMIDI directional port create/destroy (Windows teVirtualMIDI path).

#include "Midi/VirtualMidiBackend.h"

#ifdef _WIN32

#include "Midi/TeVirtualMidiLimits.h"
#include "Midi/VirtualMidiWinSupport.h"

bool VirtualMidiBackend::createDirectionalPort(
    const PortCreateRequest& request,
    std::string& errorOut)
{
    if (request.utf8Name == nullptr || request.handleOut == nullptr)
    {
        errorOut = "VirtualMIDI createDirectionalPort received null request fields";
        return false;
    }

    std::wstring wideName;
    if (!utf8ToWideVirtualMidiName(*request.utf8Name, wideName, errorOut))
    {
        return false;
    }

    SetLastError(0);
    *request.handleOut = createPortEx2_(
        wideName.c_str(),
        request.callback,
        request.callbackInstance,
        kTeVmDefaultMaxSysexLength,
        request.flags);
    if (*request.handleOut == nullptr)
    {
        errorOut = formatVirtualMidiLastError("virtualMIDICreatePortEx2");
        if (GetLastError() == ERROR_PATH_NOT_FOUND
            || errorOut.find("PATH_NOT_FOUND") != std::string::npos)
        {
            if (errorOut.find(kVirtualMidiMissingDriverFixPath) == std::string::npos)
            {
                errorOut += std::string(": ") + kVirtualMidiMissingDriverFixPath;
            }
        }
        return false;
    }
    return true;
}

bool VirtualMidiBackend::rejectSharedDirectionalHandles(
    std::size_t inCount,
    std::size_t outCount,
    std::string& errorOut) const
{
    for (std::size_t inIndex = 0; inIndex < inCount; ++inIndex)
    {
        for (std::size_t outIndex = 0; outIndex < outCount; ++outIndex)
        {
            if (inPorts_[inIndex] != nullptr
                && inPorts_[inIndex] == outPorts_[outIndex])
            {
                errorOut =
                    "VirtualMIDI CreatePortSet rejected shared Input/Output handle "
                    "(directional faces must be distinct teVirtualMIDI ports)";
                return false;
            }
        }
    }
    return true;
}

bool VirtualMidiBackend::createDirectionalPortSet(
    const PortNameSet& names,
    std::string& errorOut)
{
    inPortCount_ = names.inCount;
    outPortCount_ = names.outCount;

    for (std::size_t index = 0; index < names.inCount; ++index)
    {
        PortCreateRequest request;
        request.utf8Name = &names.inNames[index];
        request.flags = kVirtualMidiInPortFlags;
        request.handleOut = &inPorts_[index];
        if (!createDirectionalPort(request, errorOut))
        {
            return false;
        }
    }

    for (std::size_t index = 0; index < names.outCount; ++index)
    {
        // Cookie must be valid before create — callback may fire immediately.
        outCookies_[index].backend = this;
        outCookies_[index].outPortIndex = index;

        PortCreateRequest request;
        request.utf8Name = &names.outNames[index];
        request.flags = kVirtualMidiOutPortFlags;
        request.callback = &VirtualMidiBackend::outMidiDataCallback;
        request.callbackInstance = reinterpret_cast<DWORD_PTR>(&outCookies_[index]);
        request.handleOut = &outPorts_[index];
        if (!createDirectionalPort(request, errorOut))
        {
            outCookies_[index] = {};
            return false;
        }
    }

    return rejectSharedDirectionalHandles(names.inCount, names.outCount, errorOut);
}

bool VirtualMidiBackend::createDirectionalPortSetWithAliasBackoff(
    const PortNameSet& names,
    std::string& errorOut)
{
    // When DAW/MIDI-OX still hold an alias after DestroyPortSet, back off briefly.
    constexpr int kAliasBackoffAttempts = 20;
    constexpr int kAliasBackoffSleepMs = 250;
    for (int attempt = 0; attempt < kAliasBackoffAttempts; ++attempt)
    {
        if (createDirectionalPortSet(names, errorOut))
        {
            return true;
        }
        const DWORD lastError = GetLastError();
        closeAllPorts();
        if (!isVirtualMidiAliasBusyError(lastError, errorOut)
            || attempt + 1 >= kAliasBackoffAttempts)
        {
            return false;
        }
        Sleep(static_cast<DWORD>(kAliasBackoffSleepMs));
        errorOut.clear();
    }
    return false;
}

bool VirtualMidiBackend::CreatePortSet(const PortNameSet& names, std::string& errorOut)
{
    DestroyPortSet();

    if (!validateVirtualMidiPortNameSet(names, errorOut) || !ensureApiLoaded(errorOut))
    {
        return false;
    }

    // Directional Input/Output names — separate TX/RX faces (no shared handle).
    if (!createDirectionalPortSetWithAliasBackoff(names, errorOut))
    {
        return false;
    }

    if (inPortCount_ == 0 && outPortCount_ == 0)
    {
        errorOut = "VirtualMIDI CreatePortSet produced zero ports (fail closed)";
        return false;
    }

    portsCreated_ = true;
    errorOut.clear();
    return true;
}

void VirtualMidiBackend::DestroyPortSet() noexcept
{
    closeAllPorts();
    portsCreated_ = false;
}

#endif // _WIN32
