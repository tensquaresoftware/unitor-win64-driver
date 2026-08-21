// Windows MIDI Services MidiBackend (Win11 App SDK virtual devices).

#pragma once

#include "Midi/MidiBackend.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

class WmsMidiBackend;

void wmsDeliverHostMessage(
    WmsMidiBackend* backend,
    std::size_t outPortIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount);

class WmsMidiBackend : public MidiBackend
{
public:
    WmsMidiBackend();
    ~WmsMidiBackend() override;

    WmsMidiBackend(const WmsMidiBackend&) = delete;
    WmsMidiBackend& operator=(const WmsMidiBackend&) = delete;

    bool CreatePortSet(const PortNameSet& names, std::string& errorOut) override;
    void DestroyPortSet() noexcept override;

    bool SendToHost(
        std::size_t inPortIndex,
        const uint8_t* midiBytes,
        std::size_t byteCount,
        std::string& errorOut) override;

    void SetHostToDeviceSink(HostToDeviceSink sink, void* context) noexcept override;

private:
    struct Impl;

    bool createDirectionalEndpoints(const PortNameSet& names, std::string& errorOut);
    bool createPortsGuarded(const PortNameSet& names, std::string& errorOut);
    void destroyDirectionalEndpoints() noexcept;
    void rollbackFailedCreate() noexcept;
    bool ensureTransportOrFail(std::string& errorOut);
    void forwardHostToDevice(
        std::size_t outPortIndex,
        const uint8_t* midiBytes,
        std::size_t byteCount) noexcept;
    void acceptHostMidiBytes(
        std::size_t outPortIndex,
        const uint8_t* midiBytes,
        std::size_t byteCount) noexcept;

    // Host→device callback entry used by WmsMidiBackendPorts.cpp.
    friend void wmsDeliverHostMessage(
        WmsMidiBackend* backend,
        std::size_t outPortIndex,
        const uint8_t* midiBytes,
        std::size_t byteCount);

    std::unique_ptr<Impl> impl_;
    HostToDeviceSink hostToDeviceSink_ = nullptr;
    void* hostToDeviceContext_ = nullptr;
    mutable std::mutex hostToDeviceMutex_;
    std::atomic<std::uint64_t> nullSinkDrops_{0};
    bool portsCreated_ = false;
};
