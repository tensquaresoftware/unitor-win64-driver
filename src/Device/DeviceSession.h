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
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
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
    void resetPumpRuntimeState() noexcept;
    bool armBulkInAsyncRing(std::string& errorOut);
    void enableHostMidiSink();
    void queuePostStartPipePrime();
    // After post-start prime: wait until deliver queue is idle and IN ring is full
    // before allowing librarian host→device traffic.
    void waitPostStartInCalm() noexcept;
    bool postStartInIsCalm(std::chrono::milliseconds idleNeed) noexcept;
    void openLibrarianOutboundGate(bool timedOut) noexcept;
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
    void drainHostOutboundLocked(std::unique_lock<std::mutex>& usbIoLock);
    void failHostOutboundDrain(const std::string& reason);
    bool writeHostOutboundItem(
        const HostOutboundItem& item,
        HostEncodeScratch& scratch,
        std::unique_lock<std::mutex>& usbIoLock);
    struct HostOutWriteFinishArgs
    {
        const HostOutboundItem* item = nullptr;
        uint8_t cableIndex = 0;
        std::size_t encodedBytes = 0;
        bool wrote = false;
        std::string* error = nullptr;
        std::chrono::steady_clock::time_point outStarted{};
        std::size_t deliverDepthAtStart = 0;
    };
    bool finishHostOutboundWrite(const HostOutWriteFinishArgs& finish);
    bool encodeWritePopOneHostOutbound(
        HostEncodeScratch& scratch,
        std::unique_lock<std::mutex>& usbIoLock);
    void restoreMapperOutCable(uint8_t previousOutCable) noexcept;
    bool hostOutboundPending() const;
    // One Wait (+ idle finalize / outbound). 1=continue, 0=stop, -1=fatal.
    int readerWaitOnceAsync();
    static bool bulkInPacketThunk(void* context, const uint8_t* data, std::size_t size);
    bool enqueueBulkInPacket(const uint8_t* data, std::size_t size);
    int drainQueuedBulkInPackets();
    // Demux deliver-queue while usbIoMutex_ is already held (OUT between-chunks).
    // Returns false on demux failure (caller sets betweenOutChunkDemuxFailed_ if needed).
    bool drainQueuedBulkInPacketsHoldingUsbIo();
    static void betweenOutChunksDrainIn(void* context);
    std::size_t bulkInDeliverQueueDepth();
    void logLongSysexHostOutWrite(
        const HostOutboundItem& item,
        std::size_t encodedBytes,
        std::chrono::milliseconds outMs,
        std::size_t deliverDepthAtStart);
    bool failHostOutboundBetweenChunkDemux(std::size_t outPortIndex, uint8_t cableIndex);
    bool rewriteLastDumpRequestLocked();
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
    void maybeLogFirstBurst(
        uint8_t cableIndex,
        std::size_t inPortIndex,
        const uint8_t* midiBytes,
        std::size_t byteCount);
    void sendFramedToHost(
        std::size_t inPortIndex,
        uint8_t cableIndex,
        const uint8_t* midiBytes,
        std::size_t byteCount);
    void noteSendToHostSuccess(
        std::size_t inPortIndex,
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
    bool anyInFramerHoldingSysex() const noexcept;
    bool hostOutboundWriteBlocked() const noexcept;
    bool expectInBurstActive() const noexcept;
    void armExpectInBurstAfterHostSysex(
        std::size_t outPortIndex,
        const uint8_t* midiBytes,
        std::size_t midiBytesCount) noexcept;
    void clearExpectInBurst() noexcept;
    bool rejectShortMatrixDumpAndRetry(std::size_t gotLength);
    void finalizeIdleHeldSysex();
    void finalizeOneHeldSysex(std::size_t inPortIndex);
    void abandonIdlePartialSysexHold(std::size_t inPortIndex);
    void abandonIdlePartialSysexHoldUnlocked(std::size_t inPortIndex);

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
    std::chrono::steady_clock::time_point lastBulkInPacketSteady_{};
    std::chrono::steady_clock::time_point hostOutEarliestSteady_{};
    // After a short host SysEx (dump request), block further WriteBulk until the
    // expected device→host SysEx completes or this deadline passes.
    std::chrono::steady_clock::time_point expectInBurstUntil_{};
    std::vector<uint8_t> lastDumpRequest_{};
    std::size_t lastDumpOutPort_ = 0;
    unsigned dumpRequestRetryRemaining_ = 0;
    // Temporary lab diag: log first product IN spans after Start (F0 / hold).
    std::atomic<unsigned> firstBurstDiagRemaining_{0};

    // Completion thread enqueues USB packets; reader demuxes + SendToHost (teVirtualMIDI
    // is not safe to call from the WinUSB completion thread — lab saw send_ok with
    // host TIMEOUT last=none).
    struct QueuedBulkInPacket
    {
        std::array<uint8_t, 512> data{};
        std::size_t size = 0;
    };
    // While WriteEmagicHostMidi holds usbIoMutex_, IN cannot demux — completion
    // only enqueues. Long SysEx + DIN loopback noise can land thousands of 32 B
    // URBs before the next reader drain (lab: 4096 overflowed mid-4096/14 KiB).
    static constexpr std::size_t kMaxQueuedBulkInPackets = 32768;
    std::mutex bulkInDeliverMutex_;
    std::deque<QueuedBulkInPacket> bulkInDeliverQueue_;
    std::atomic<std::size_t> bulkInDeliverHighWater_{0};
    // Set when between-chunk IN demux fails while WriteEmagicHostMidi is in flight.
    bool betweenOutChunkDemuxFailed_ = false;
    // Between OUT USB chunks: demux+frame IN, but defer SendToHost until OUT ends
    // (teVirtualMIDI must not run nested under an in-flight long WriteBulk).
    bool deferHostSendDuringOut_ = false;
    struct DeferredHostSend
    {
        std::size_t inPortIndex = 0;
        uint8_t cableIndex = 0;
        std::vector<uint8_t> midi;
    };
    std::vector<DeferredHostSend> deferredHostSends_;
    void flushDeferredHostSends();
};
