// V1 MidiBackend: Tobias Erichsen VirtualMIDI via runtime teVirtualMIDI.dll (AD-7).

#pragma once

#include "Midi/MidiBackend.h"

#ifdef _WIN32
#include "Midi/TeVirtualMidiApi.h"
#endif

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>

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
    struct OutPortCookie
    {
        VirtualMidiBackend* backend = nullptr;
        std::size_t outPortIndex = 0;
    };

    struct PortCreateRequest
    {
        const std::string* utf8Name = nullptr;
        DWORD flags = 0;
        TeVmMidiDataCallback callback = nullptr;
        DWORD_PTR callbackInstance = 0;
        TeVmMidiPortHandle* handleOut = nullptr;
    };

    bool ensureApiLoaded(std::string& errorOut);
    bool createDirectionalPort(const PortCreateRequest& request, std::string& errorOut);
    bool createDirectionalPortSet(const PortNameSet& names, std::string& errorOut);
    bool createDirectionalPortSetWithAliasBackoff(
        const PortNameSet& names,
        std::string& errorOut);
    bool rejectSharedDirectionalHandles(
        std::size_t inCount,
        std::size_t outCount,
        std::string& errorOut) const;
    void closeAllPorts() noexcept;
    void unloadApi() noexcept;
    void forwardHostToDevice(
        std::size_t outPortIndex,
        const uint8_t* midiBytes,
        std::size_t byteCount) noexcept;

    static void CALLBACK outMidiDataCallback(
        TeVmMidiPortHandle midiPort,
        LPBYTE midiDataBytes,
        DWORD length,
        DWORD_PTR callbackInstance);

    HMODULE dllModule_ = nullptr;
    TeVmCreatePortEx2Fn createPortEx2_ = nullptr;
    TeVmClosePortFn closePort_ = nullptr;
    TeVmSendDataFn sendData_ = nullptr;

    TeVmMidiPortHandle inPorts_[kMaxMidiBackendInPorts] = {};
    TeVmMidiPortHandle outPorts_[kMaxMidiBackendOutPorts] = {};
    OutPortCookie outCookies_[kMaxMidiBackendOutPorts] = {};
    std::size_t inPortCount_ = 0;
    std::size_t outPortCount_ = 0;
#endif

    HostToDeviceSink hostToDeviceSink_ = nullptr;
    void* hostToDeviceContext_ = nullptr;
    // Guards sink + context pair (session thread vs teVirtualMIDI OUT callbacks).
    mutable std::mutex hostToDeviceMutex_;
    std::atomic<std::uint64_t> nullSinkDrops_{0};
    bool portsCreated_ = false;
};
