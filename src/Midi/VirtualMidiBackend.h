// V1 MidiBackend: Tobias Erichsen VirtualMIDI via runtime teVirtualMIDI.dll (AD-7).

#pragma once

#include "Midi/MidiBackend.h"

#ifdef _WIN32
#include "Midi/TeVirtualMidiApi.h"
#endif

#include <cstddef>

class VirtualMidiBackend : public MidiBackend
{
public:
    VirtualMidiBackend() = default;
    ~VirtualMidiBackend() override;

    VirtualMidiBackend(const VirtualMidiBackend&) = delete;
    VirtualMidiBackend& operator=(const VirtualMidiBackend&) = delete;

    bool CreatePortSet(const PortNameSet& names, std::string& errorOut) override;
    void DestroyPortSet() noexcept override;

    bool SendToHost(
        std::size_t inPortIndex,
        const uint8_t* midiBytes,
        std::size_t byteCount,
        std::string& errorOut) override;

    void SetHostToDeviceSink(HostToDeviceSink sink, void* context) noexcept override;

private:
#ifdef _WIN32
    bool ensureApiLoaded(std::string& errorOut);
    bool createDirectionalPort(
        const std::string& utf8Name,
        DWORD flags,
        TeVmMidiPortHandle& handleOut,
        std::string& errorOut);
    struct PortGroupCreate
    {
        const std::string* names = nullptr;
        std::size_t count = 0;
        DWORD flags = 0;
        TeVmMidiPortHandle* handlesOut = nullptr;
    };
    bool createPortGroup(
        const PortGroupCreate& group,
        std::size_t& countOut,
        std::string& errorOut);
    void closeAllPorts() noexcept;
    void unloadApi() noexcept;

    HMODULE dllModule_ = nullptr;
    TeVmCreatePortEx2Fn createPortEx2_ = nullptr;
    TeVmClosePortFn closePort_ = nullptr;

    TeVmMidiPortHandle inPorts_[kMaxMidiBackendInPorts] = {};
    TeVmMidiPortHandle outPorts_[kMaxMidiBackendOutPorts] = {};
    std::size_t inPortCount_ = 0;
    std::size_t outPortCount_ = 0;
#endif

    HostToDeviceSink hostToDeviceSink_ = nullptr;
    void* hostToDeviceContext_ = nullptr;
    bool portsCreated_ = false;
};
