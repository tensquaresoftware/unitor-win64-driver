#include "Midi/VirtualMidiBackend.h"

#include <sstream>

#ifndef _WIN32

VirtualMidiBackend::~VirtualMidiBackend() = default;

bool VirtualMidiBackend::CreatePortSet(const PortNameSet& /*names*/, std::string& errorOut)
{
    errorOut =
        "VirtualMIDI requires Windows with teVirtualMIDI.dll present "
        "(install loopMIDI or rtpMIDI so the VirtualMIDI driver is available)";
    return false;
}

void VirtualMidiBackend::DestroyPortSet() noexcept
{
    portsCreated_ = false;
}

bool VirtualMidiBackend::SendToHost(
    std::size_t /*inPortIndex*/,
    const uint8_t* /*midiBytes*/,
    std::size_t /*byteCount*/,
    std::string& errorOut)
{
    errorOut = "VirtualMIDI SendToHost is not available on this platform";
    return false;
}

void VirtualMidiBackend::SetHostToDeviceSink(HostToDeviceSink sink, void* context) noexcept
{
    hostToDeviceSink_ = sink;
    hostToDeviceContext_ = context;
}

#else

namespace
{
constexpr const char* kVirtualMidiDllName = "teVirtualMIDI.dll";
constexpr const char* kMissingDriverFixPath =
    "VirtualMIDI driver/DLL missing (ERROR_PATH_NOT_FOUND-class). "
    "Install loopMIDI or rtpMIDI so the VirtualMIDI driver is present, "
    "then retry.";

std::string formatLastError(const char* action)
{
    const DWORD code = GetLastError();
    std::ostringstream stream;
    stream << action << " failed (Win32=" << code << ")";
    if (code == ERROR_PATH_NOT_FOUND || code == ERROR_MOD_NOT_FOUND)
    {
        stream << ": " << kMissingDriverFixPath;
    }
    return stream.str();
}

bool utf8ToWide(const std::string& utf8, std::wstring& wideOut, std::string& errorOut)
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
        errorOut = formatLastError("UTF-8 to wide conversion size query");
        return false;
    }

    wideOut.assign(static_cast<std::size_t>(needed), L'\0');
    const int written = MultiByteToWideChar(
        CP_UTF8, 0, utf8.c_str(), -1, wideOut.data(), needed);
    if (written <= 0)
    {
        errorOut = formatLastError("UTF-8 to wide conversion");
        return false;
    }

    // MultiByteToWideChar includes the trailing null in `needed`.
    if (!wideOut.empty() && wideOut.back() == L'\0')
    {
        wideOut.pop_back();
    }
    return true;
}

bool isBlankPortName(const std::string& name)
{
    return name.find_first_not_of(" \t\r\n") == std::string::npos;
}

bool validatePortNameSet(const PortNameSet& names, std::string& errorOut)
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
} // namespace

VirtualMidiBackend::~VirtualMidiBackend()
{
    DestroyPortSet();
    unloadApi();
}

bool VirtualMidiBackend::ensureApiLoaded(std::string& errorOut)
{
    if (dllModule_ != nullptr && createPortEx2_ != nullptr && closePort_ != nullptr)
    {
        return true;
    }

    // Restrict search to System32 — avoid CWD / PATH DLL hijack of teVirtualMIDI.dll.
    dllModule_ = LoadLibraryExA(
        kVirtualMidiDllName,
        nullptr,
        LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (dllModule_ == nullptr)
    {
        errorOut = formatLastError("LoadLibraryEx(teVirtualMIDI.dll, SYSTEM32)");
        if (errorOut.find(kMissingDriverFixPath) == std::string::npos)
        {
            errorOut += std::string(": ") + kMissingDriverFixPath;
        }
        return false;
    }

    createPortEx2_ = reinterpret_cast<TeVmCreatePortEx2Fn>(
        GetProcAddress(dllModule_, "virtualMIDICreatePortEx2"));
    closePort_ = reinterpret_cast<TeVmClosePortFn>(
        GetProcAddress(dllModule_, "virtualMIDIClosePort"));

    if (createPortEx2_ == nullptr || closePort_ == nullptr)
    {
        unloadApi();
        errorOut =
            "teVirtualMIDI.dll loaded but required exports are missing; "
            "reinstall loopMIDI or rtpMIDI so the VirtualMIDI driver matches the DLL";
        return false;
    }

    return true;
}

void VirtualMidiBackend::unloadApi() noexcept
{
    createPortEx2_ = nullptr;
    closePort_ = nullptr;
    if (dllModule_ != nullptr)
    {
        FreeLibrary(dllModule_);
        dllModule_ = nullptr;
    }
}

void VirtualMidiBackend::closeAllPorts() noexcept
{
    if (closePort_ != nullptr)
    {
        for (std::size_t index = 0; index < inPortCount_; ++index)
        {
            if (inPorts_[index] != nullptr)
            {
                closePort_(inPorts_[index]);
                inPorts_[index] = nullptr;
            }
        }
        for (std::size_t index = 0; index < outPortCount_; ++index)
        {
            if (outPorts_[index] != nullptr)
            {
                closePort_(outPorts_[index]);
                outPorts_[index] = nullptr;
            }
        }
    }

    inPortCount_ = 0;
    outPortCount_ = 0;
}

bool VirtualMidiBackend::createDirectionalPort(
    const std::string& utf8Name,
    DWORD flags,
    TeVmMidiPortHandle& handleOut,
    std::string& errorOut)
{
    std::wstring wideName;
    if (!utf8ToWide(utf8Name, wideName, errorOut))
    {
        return false;
    }

    SetLastError(0);
    handleOut = createPortEx2_(
        wideName.c_str(),
        nullptr,
        0,
        kTeVmDefaultMaxSysexLength,
        flags);
    if (handleOut == nullptr)
    {
        errorOut = formatLastError("virtualMIDICreatePortEx2");
        if (GetLastError() == ERROR_PATH_NOT_FOUND
            || errorOut.find("PATH_NOT_FOUND") != std::string::npos)
        {
            // Keep explicit fix path for driver-not-present failures.
            if (errorOut.find(kMissingDriverFixPath) == std::string::npos)
            {
                errorOut += std::string(": ") + kMissingDriverFixPath;
            }
        }
        return false;
    }
    return true;
}

bool VirtualMidiBackend::createPortGroup(
    const PortGroupCreate& group,
    std::size_t& countOut,
    std::string& errorOut)
{
    if (group.names == nullptr || group.handlesOut == nullptr)
    {
        errorOut = "VirtualMIDI createPortGroup received null buffers";
        return false;
    }

    for (std::size_t index = 0; index < group.count; ++index)
    {
        TeVmMidiPortHandle handle = nullptr;
        if (!createDirectionalPort(group.names[index], group.flags, handle, errorOut))
        {
            return false;
        }
        group.handlesOut[countOut++] = handle;
    }
    return true;
}

bool VirtualMidiBackend::CreatePortSet(const PortNameSet& names, std::string& errorOut)
{
    DestroyPortSet();

    if (!validatePortNameSet(names, errorOut) || !ensureApiLoaded(errorOut))
    {
        return false;
    }

    // Product IN → apps see MIDI IN (TX). Product OUT → MIDI OUT (RX).
    constexpr DWORD kInFlags = kTeVmFlagsParseTx | kTeVmFlagsInstantiateTx;
    constexpr DWORD kOutFlags = kTeVmFlagsParseRx | kTeVmFlagsInstantiateRx;

    const PortGroupCreate inGroup{names.inNames, names.inCount, kInFlags, inPorts_};
    const PortGroupCreate outGroup{names.outNames, names.outCount, kOutFlags, outPorts_};
    if (!createPortGroup(inGroup, inPortCount_, errorOut)
        || !createPortGroup(outGroup, outPortCount_, errorOut))
    {
        closeAllPorts();
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

bool VirtualMidiBackend::SendToHost(
    std::size_t /*inPortIndex*/,
    const uint8_t* /*midiBytes*/,
    std::size_t /*byteCount*/,
    std::string& errorOut)
{
    errorOut = "VirtualMIDI SendToHost is reserved for Story 1.6";
    return false;
}

void VirtualMidiBackend::SetHostToDeviceSink(HostToDeviceSink sink, void* context) noexcept
{
    hostToDeviceSink_ = sink;
    hostToDeviceContext_ = context;
}

#endif // _WIN32
