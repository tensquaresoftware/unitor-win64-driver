// One live usermode session per MT4: WinUSB + Emagic mapper + MidiBackend ports (AD-4 / AD-9).
// Only a live DeviceSession creates/destroys that unit's Virtual Ports and owns the I/O pump.

#pragma once

#include "Midi/MidiBackend.h"
#include "Profile/DeviceProfile.h"
#include "Protocol/EmagicCableMapper.h"
#include "Protocol/MidiMessageFramer.h"
#include "Device/DeviceHostCounters.h"
#include "Device/HostOutboundQueue.h"
#include "Usb/WinUsbTransport.h"

#include <atomic>
#include <chrono>
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

    // Normative: Open → Emagic init/drain/computer-mode → CreatePortSet → reader.
    bool Start(const DeviceSessionStartRequest& request, std::string& errorOut);

    // Normative Stop: signal → join reader → clear sink → DestroyPortSet → finish → Close.
    void Stop() noexcept;
    bool IsRunning() const noexcept;

    // True when the device→host pump recorded a fatal English diagnostic.
    bool TakePumpFailure(std::string& errorOut);

    // Snapshot for CLI-thread printing (reader thread must not write the console).
    DeviceHostCounterSnapshot CopyDeviceHostCounters() const noexcept;

private:
    bool sendInitMagic(std::string& errorOut);
    bool sendComputerModeChannelKick(std::string& errorOut);
    void sendFinishMagicBestEffort() noexcept;
    void destroyPortsBestEffort() noexcept;
    bool portNamesMatchProfile(
        const DeviceProfile& profile,
        const PortNameSet& portNames,
        std::string& errorOut) const;
    bool buildCableMaps(const DeviceProfile& profile, std::string& errorOut);
    bool openTransportOnly(
        const DeviceSessionStartRequest& request,
        std::string& errorOut);
    bool createPortsAndStartPump(
        const PortNameSet& portNames,
        std::string& errorOut);
    bool startPump(std::string& errorOut);
    bool armBulkInAsyncRing(std::string& errorOut);
    void enableHostMidiSink();
    void stopPumpAndJoin() noexcept;
    void readerLoop();
    void failReaderOnReadBulkError(const std::string& error);
    // Decode into pending while usbIoMutex_ is already held.
    bool collectBulkReadPending(
        const uint8_t* readBuffer,
        std::size_t bytesRead,
        std::vector<std::pair<uint8_t, std::vector<uint8_t>>>& pending,
        std::string& errorOut);
    void forwardPendingProductMidi(
        const std::vector<std::pair<uint8_t, std::vector<uint8_t>>>& pending);
    // Decode + forward while usbIoMutex_ is already held.
    bool processBulkReadLocked(
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
    void drainHostOutbound();
    void drainHostOutboundLocked();
    void failHostOutboundDrain(const std::string& reason);
    bool writeHostOutboundItem(const HostOutboundItem& item, HostEncodeScratch& scratch);
    bool encodeWritePopOneHostOutbound(HostEncodeScratch& scratch);
    bool hostOutboundPending() const;
    bool processAsyncBulkInPacket(const BulkInAsyncPacket& packet);
    bool processAndResubmitAsyncPacket(const BulkInAsyncPacket& packet);
    // 1=continue, 0=stop, -1=fatal. Starts from an already-completed packet.
    int harvestReadyAsyncPackets(BulkInAsyncPacket firstPacket);
    // One Wait (+ optional demux/outbound). 1=continue, 0=stop, -1=fatal.
    int readerWaitOnceAsync();
    // Clears queued host→device work; returns how many messages were discarded.
    std::size_t clearHostOutboundQueue() noexcept;
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
    void sendFramedToHost(
        std::size_t inPortIndex,
        uint8_t cableIndex,
        const uint8_t* midiBytes,
        std::size_t byteCount);
    void noteFramerOversizeRejects(
        std::size_t inPortIndex,
        uint8_t cableIndex,
        std::uint64_t rejectCount);
    void noteBulkReadCounters(std::size_t bulkBytes, std::size_t demuxSpans);
    void noteHostOutboundCounters(
        const HostOutboundItem& item,
        std::size_t encodedBytes) noexcept;
    void recordPumpFailure(const std::string& message);
    std::size_t findInPortIndex(uint8_t cableIndex) const noexcept;
    void resetInFramers() noexcept;

    static void hostToDeviceThunk(
        void* context,
        std::size_t outPortIndex,
        const uint8_t* midiBytes,
        std::size_t byteCount);

    WinUsbTransport transport_;
    std::unique_ptr<EmagicCableMapper> mapper_;
    MidiBackend* midiBackend_ = nullptr;
    // Polled from the CLI thread while Start/Stop/reader mutate session state.
    std::atomic<bool> running_{false};

    std::thread readerThread_;
    std::atomic<bool> stopPump_{true};
    std::mutex usbIoMutex_;
    std::mutex pumpErrorMutex_;
    std::string pumpError_;

    uint8_t inCableByPort_[kMaxMidiBackendInPorts] = {};
    uint8_t outCableByPort_[kMaxMidiBackendOutPorts] = {};
    std::size_t inPortCount_ = 0;
    std::size_t outPortCount_ = 0;

    MidiMessageFramer inFramers_[kMaxMidiBackendInPorts];
    DeviceHostCounters deviceHostCounters_;
    HostOutboundQueue hostOutbound_;
    mutable std::mutex hostOutboundMutex_;
    // Steady-clock ms at ring arm (-1 = not armed); read from host MIDI callbacks.
    std::atomic<std::int64_t> bulkInRingArmedSteadyMs_{-1};
    std::atomic<bool> firstHostInquiryLogged_{false};
};
