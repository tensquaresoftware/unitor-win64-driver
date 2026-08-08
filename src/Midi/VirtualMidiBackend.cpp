#include "Midi/VirtualMidiBackend.h"

#include <atomic>
#include <cstdint>
#include <iostream>
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
    std::lock_guard<std::mutex> lock(hostToDeviceMutex_);
    hostToDeviceSink_ = sink;
    hostToDeviceContext_ = context;
}

#else

#include "Midi/VirtualMidiWinSupport.h"
#include "Midi/TeVirtualMidiLimits.h"

namespace
{
constexpr const char* kVirtualMidiDllName = "teVirtualMIDI.dll";

std::string formatInPortIndexError(const char* reason, std::size_t inPortIndex)
{
    std::ostringstream stream;
    stream << "VirtualMIDI SendToHost " << reason << " for IN port index " << inPortIndex;
    return stream.str();
}

struct SendToHostValidation
{
    bool portsCreated = false;
    std::size_t inPortCount = 0;
    std::size_t inPortIndex = 0;
    const uint8_t* midiBytes = nullptr;
    std::size_t byteCount = 0;
    TeVmMidiPortHandle portHandle = nullptr;
    TeVmSendDataFn sendData = nullptr;
};

bool validateSendToHostArgs(const SendToHostValidation& args, std::string& errorOut)
{
    if (!args.portsCreated)
    {
        errorOut = "VirtualMIDI SendToHost requires an active port set";
        return false;
    }
    if (args.inPortIndex >= args.inPortCount)
    {
        errorOut = formatInPortIndexError("rejected out-of-range index", args.inPortIndex);
        return false;
    }
    if (args.midiBytes == nullptr || args.byteCount == 0)
    {
        errorOut = formatInPortIndexError("rejected empty MIDI payload", args.inPortIndex);
        return false;
    }
    if (exceedsTeVmDefaultMaxSysexLength(args.byteCount))
    {
        errorOut = formatInPortIndexError(
            "rejected payload above teVirtualMIDI max SysEx length (65535)",
            args.inPortIndex);
        return false;
    }
    if (args.portHandle == nullptr)
    {
        errorOut = formatInPortIndexError("rejected null port handle", args.inPortIndex);
        return false;
    }
    if (args.sendData == nullptr)
    {
        errorOut =
            "VirtualMIDI SendToHost missing virtualMIDISendData export; "
            "reinstall loopMIDI or rtpMIDI";
        return false;
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
    if (dllModule_ != nullptr
        && createPortEx2_ != nullptr
        && closePort_ != nullptr
        && sendData_ != nullptr)
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
        errorOut = formatVirtualMidiLastError("LoadLibraryEx(teVirtualMIDI.dll, SYSTEM32)");
        if (errorOut.find(kVirtualMidiMissingDriverFixPath) == std::string::npos)
        {
            errorOut += std::string(": ") + kVirtualMidiMissingDriverFixPath;
        }
        return false;
    }

    createPortEx2_ = reinterpret_cast<TeVmCreatePortEx2Fn>(
        GetProcAddress(dllModule_, "virtualMIDICreatePortEx2"));
    closePort_ = reinterpret_cast<TeVmClosePortFn>(
        GetProcAddress(dllModule_, "virtualMIDIClosePort"));
    sendData_ = reinterpret_cast<TeVmSendDataFn>(
        GetProcAddress(dllModule_, "virtualMIDISendData"));

    if (createPortEx2_ == nullptr || closePort_ == nullptr || sendData_ == nullptr)
    {
        unloadApi();
        errorOut =
            "teVirtualMIDI.dll loaded but required exports are missing "
            "(virtualMIDICreatePortEx2 / virtualMIDIClosePort / virtualMIDISendData); "
            "reinstall loopMIDI or rtpMIDI so the VirtualMIDI driver matches the DLL";
        return false;
    }

    return true;
}

void VirtualMidiBackend::unloadApi() noexcept
{
    createPortEx2_ = nullptr;
    closePort_ = nullptr;
    sendData_ = nullptr;
    if (dllModule_ != nullptr)
    {
        FreeLibrary(dllModule_);
        dllModule_ = nullptr;
    }
}

void VirtualMidiBackend::closeAllPorts() noexcept
{
    // Never call closePort_ from outMidiDataCallback — teVirtualMIDI deadlock risk.
    // Deduplicate before close in case a handle was ever stored on both faces.
    if (closePort_ != nullptr)
    {
        TeVmMidiPortHandle unique[
            kMaxMidiBackendInPorts + kMaxMidiBackendOutPorts] = {};
        std::size_t uniqueCount = 0;
        auto remember = [&](TeVmMidiPortHandle handle) {
            if (handle == nullptr)
            {
                return;
            }
            for (std::size_t index = 0; index < uniqueCount; ++index)
            {
                if (unique[index] == handle)
                {
                    return;
                }
            }
            unique[uniqueCount++] = handle;
        };
        for (std::size_t index = 0; index < inPortCount_; ++index)
        {
            remember(inPorts_[index]);
            inPorts_[index] = nullptr;
        }
        for (std::size_t index = 0; index < outPortCount_; ++index)
        {
            remember(outPorts_[index]);
            outPorts_[index] = nullptr;
            outCookies_[index] = {};
        }
        for (std::size_t index = 0; index < uniqueCount; ++index)
        {
            closePort_(unique[index]);
        }
    }

    inPortCount_ = 0;
    outPortCount_ = 0;
}

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

void VirtualMidiBackend::forwardHostToDevice(
    std::size_t outPortIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount) noexcept
{
    HostToDeviceSink sink = nullptr;
    void* context = nullptr;
    {
        // Copy under the mutex, then release before calling into DeviceSession so
        // Stop (usbIoMutex_ → SetHostToDeviceSink) cannot deadlock with this path.
        std::lock_guard<std::mutex> lock(hostToDeviceMutex_);
        sink = hostToDeviceSink_;
        context = hostToDeviceContext_;
    }
    if (sink == nullptr || midiBytes == nullptr || byteCount == 0)
    {
        if (sink == nullptr && midiBytes != nullptr && byteCount > 0)
        {
            const std::uint64_t drops =
                nullSinkDrops_.fetch_add(1, std::memory_order_relaxed) + 1;
            if (drops <= 8 || (drops % 256) == 0)
            {
                std::cerr << "VirtualMIDI host→device dropped (sink unset) out_port="
                          << (outPortIndex + 1) << " bytes=" << byteCount
                          << " drops=" << drops << "\n"
                          << std::flush;
            }
        }
        return;
    }
    sink(context, outPortIndex, midiBytes, byteCount);
}

void CALLBACK VirtualMidiBackend::outMidiDataCallback(
    TeVmMidiPortHandle /*midiPort*/,
    LPBYTE midiDataBytes,
    DWORD length,
    DWORD_PTR callbackInstance)
{
    auto* cookie = reinterpret_cast<OutPortCookie*>(callbackInstance);
    if (cookie == nullptr || cookie->backend == nullptr)
    {
        return;
    }
    cookie->backend->forwardHostToDevice(
        cookie->outPortIndex,
        reinterpret_cast<const uint8_t*>(midiDataBytes),
        static_cast<std::size_t>(length));
}

bool VirtualMidiBackend::CreatePortSet(const PortNameSet& names, std::string& errorOut)
{
    DestroyPortSet();

    if (!validateVirtualMidiPortNameSet(names, errorOut) || !ensureApiLoaded(errorOut))
    {
        return false;
    }

    // Directional Input/Output names are unique — create separate TX-only and
    // RX-only teVirtualMIDI faces (no shared bidirectional handle / local echo).
    if (!createDirectionalPortSet(names, errorOut))
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
    std::size_t inPortIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount,
    std::string& errorOut)
{
    SendToHostValidation args;
    args.portsCreated = portsCreated_;
    args.inPortCount = inPortCount_;
    args.inPortIndex = inPortIndex;
    args.midiBytes = midiBytes;
    args.byteCount = byteCount;
    args.portHandle =
        (inPortIndex < inPortCount_) ? inPorts_[inPortIndex] : nullptr;
    args.sendData = sendData_;

    if (!validateSendToHostArgs(args, errorOut))
    {
        return false;
    }

    SetLastError(0);
    const BOOL ok = sendData_(
        inPorts_[inPortIndex],
        const_cast<LPBYTE>(reinterpret_cast<const BYTE*>(midiBytes)),
        static_cast<DWORD>(byteCount));
    if (!ok)
    {
        errorOut = formatVirtualMidiLastError("virtualMIDISendData");
        std::ostringstream stream;
        stream << errorOut << " (IN port index " << inPortIndex << ")";
        errorOut = stream.str();
        return false;
    }

    errorOut.clear();
    return true;
}

void VirtualMidiBackend::SetHostToDeviceSink(HostToDeviceSink sink, void* context) noexcept
{
    std::lock_guard<std::mutex> lock(hostToDeviceMutex_);
    hostToDeviceSink_ = sink;
    hostToDeviceContext_ = context;
}

#endif // _WIN32
