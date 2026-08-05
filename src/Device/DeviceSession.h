// One live usermode session per MT4: WinUSB + Emagic mapper + MidiBackend ports (AD-4 / AD-9).
// Only a live DeviceSession creates/destroys that unit's Virtual Ports and owns the I/O pump.

#pragma once

#include "Midi/MidiBackend.h"
#include "Profile/DeviceProfile.h"
#include "Protocol/EmagicCableMapper.h"
#include "Usb/WinUsbTransport.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

struct DeviceSessionStartRequest
{
    const DeviceProfile* profile = nullptr;
    MidiBackend* midiBackend = nullptr;
    const PortNameSet* portNames = nullptr;
    WinUsbOpenOptions openOptions = {};
};

class DeviceSession
{
public:
    DeviceSession() = default;
    ~DeviceSession();

    DeviceSession(const DeviceSession&) = delete;
    DeviceSession& operator=(const DeviceSession&) = delete;

    // Normative: Open → init → CreatePortSet → sink → reader thread.
    bool Start(const DeviceSessionStartRequest& request, std::string& errorOut);

    // Normative Stop: signal → join reader → clear sink → DestroyPortSet → finish → Close.
    void Stop() noexcept;
    bool IsRunning() const noexcept;

    // True when the device→host pump recorded a fatal English diagnostic.
    bool TakePumpFailure(std::string& errorOut);

private:
    bool sendInitMagic(std::string& errorOut);
    void sendFinishMagicBestEffort() noexcept;
    void destroyPortsBestEffort() noexcept;
    bool portNamesMatchProfile(
        const DeviceProfile& profile,
        const PortNameSet& portNames,
        std::string& errorOut) const;
    bool buildCableMaps(const DeviceProfile& profile, std::string& errorOut);
    bool openTransportAndInit(
        const DeviceSessionStartRequest& request,
        std::string& errorOut);
    bool createPortsAndStartPump(
        const PortNameSet& portNames,
        std::string& errorOut);
    bool startPump(std::string& errorOut);
    void stopPumpAndJoin() noexcept;
    void readerLoop();
    bool processBulkRead(
        const uint8_t* readBuffer,
        std::size_t bytesRead,
        std::string& errorOut);
    struct HostEncodeScratch
    {
        uint8_t* bytes = nullptr;
        std::size_t capacity = 0;
        std::size_t size = 0;
        uint8_t cableIndex = 0;
    };
    bool encodeHostMidiLocked(
        std::size_t outPortIndex,
        const uint8_t* midiBytes,
        std::size_t byteCount,
        HostEncodeScratch& scratch);
    void appendPendingProductMidi(
        std::vector<std::pair<uint8_t, std::vector<uint8_t>>>& pending,
        uint8_t cableIndex,
        const uint8_t* midi,
        std::size_t n);
    void handleHostMidi(
        std::size_t outPortIndex,
        const uint8_t* midiBytes,
        std::size_t byteCount);
    void forwardDeviceMidi(
        uint8_t cableIndex,
        const uint8_t* midiBytes,
        std::size_t byteCount);
    void recordPumpFailure(const std::string& message);
    std::size_t findInPortIndex(uint8_t cableIndex) const noexcept;

    static void hostToDeviceThunk(
        void* context,
        std::size_t outPortIndex,
        const uint8_t* midiBytes,
        std::size_t byteCount);

    WinUsbTransport transport_;
    std::unique_ptr<EmagicCableMapper> mapper_;
    MidiBackend* midiBackend_ = nullptr;
    bool running_ = false;

    std::thread readerThread_;
    std::atomic<bool> stopPump_{true};
    std::mutex usbIoMutex_;
    std::mutex pumpErrorMutex_;
    std::string pumpError_;

    uint8_t inCableByPort_[kMaxMidiBackendInPorts] = {};
    uint8_t outCableByPort_[kMaxMidiBackendOutPorts] = {};
    std::size_t inPortCount_ = 0;
    std::size_t outPortCount_ = 0;
};
