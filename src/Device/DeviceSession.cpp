#include "Device/DeviceSession.h"
#include "Device/DeviceSessionSupport.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <system_error>
#include <thread>

DeviceSession::~DeviceSession()
{
    Stop();
}

bool DeviceSession::sendComputerModeChannelKick(std::string& errorOut)
{
    if (mapper_ == nullptr)
    {
        errorOut = "Computer-mode channel kick requires EmagicCableMapper";
        return false;
    }

    // MT4 manual: channel MIDI on USB enters Computer Mode; SysEx/realtime do not.
    // SoundDiver sends a harmless controller at startup for the same reason.
    const uint8_t cc7Quiet[] = {0xB0, 0x07, 0x00};
    uint8_t framed[64] = {};
    EncodeBuffer buffer{framed, sizeof(framed), 0};
    EncodeRequest request{0, cc7Quiet, sizeof(cc7Quiet)};
    if (!mapper_->EncodeToDevice(request, buffer, errorOut))
    {
        return false;
    }
    if (!transport_.WriteEmagicHostMidi(framed, buffer.size, errorOut))
    {
        errorOut = "Computer-mode channel kick WriteBulk failed: " + errorOut;
        return false;
    }
    return true;
}

bool DeviceSession::sendInitMagic(std::string& errorOut)
{
    std::size_t drainedBytes = 0;
    if (!transport_.WriteEmagicInitSequence(errorOut, &drainedBytes))
    {
        return false;
    }
    if (!sendComputerModeChannelKick(errorOut))
    {
        return false;
    }
    // Kick has no Emagic reply framing we need; flush residual IN before the
    // async ring so the first host Inquiry burst keeps full INPUT_URBS depth.
    std::size_t kickDrained = 0;
    std::string drainError;
    if (!transport_.DrainBulkInBestEffort(8, kickDrained, drainError))
    {
        errorOut = "DeviceSession post-kick bulk IN drain failed: " + drainError;
        return false;
    }
    drainedBytes += kickDrained;
    if (mapper_ != nullptr)
    {
        mapper_->ResetInputState();
    }
    std::cout << "Emagic init/computer-mode: drained " << drainedBytes
              << " bulk IN byte(s); post-kick drain=" << kickDrained
              << "; read capacity="
              << transport_.BulkInReadCapacity()
              << "; channel CC kick sent (manual Computer Mode)\n"
              << std::flush;
    return true;
}

void DeviceSession::sendFinishMagicBestEffort() noexcept
{
    if (!transport_.IsOpen())
    {
        return;
    }

    std::string ignored;
    (void)transport_.WriteBulk(kEmagicFinishMagic, kEmagicFinishMagicSize, ignored);
}

void DeviceSession::destroyPortsBestEffort() noexcept
{
    if (midiBackend_ != nullptr)
    {
        midiBackend_->DestroyPortSet();
    }
}

bool DeviceSession::portNamesMatchProfile(
    const DeviceProfile& profile,
    const PortNameSet& portNames,
    std::string& errorOut) const
{
    const std::size_t expectedIn = countProductPorts(profile.inCables);
    const std::size_t expectedOut = countProductPorts(profile.outCables);
    if (portNames.inCount != expectedIn || portNames.outCount != expectedOut)
    {
        errorOut =
            "DeviceSession PortNameSet counts do not match DeviceProfile product ports";
        return false;
    }
    return true;
}

bool DeviceSession::buildCableMaps(const DeviceProfile& profile, std::string& errorOut)
{
    inPortCount_ = collectProductCableIndices(
        profile.inCables, inCableByPort_, kMaxMidiBackendInPorts);
    outPortCount_ = collectProductCableIndices(
        profile.outCables, outCableByPort_, kMaxMidiBackendOutPorts);

    if (inPortCount_ == 0 && outPortCount_ == 0)
    {
        errorOut = "DeviceSession profile has no product IN/OUT cables";
        return false;
    }
    return true;
}

std::size_t DeviceSession::findInPortIndex(uint8_t cableIndex) const noexcept
{
    for (std::size_t index = 0; index < inPortCount_; ++index)
    {
        if (inCableByPort_[index] == cableIndex)
        {
            return index;
        }
    }
    return inPortCount_;
}

void DeviceSession::recordPumpFailure(const std::string& message)
{
    {
        std::lock_guard<std::mutex> lock(pumpErrorMutex_);
        if (pumpError_.empty())
        {
            pumpError_ = message;
        }
    }
    stopPump_.store(true);
}

bool DeviceSession::TakePumpFailure(std::string& errorOut)
{
    std::lock_guard<std::mutex> lock(pumpErrorMutex_);
    if (pumpError_.empty())
    {
        return false;
    }
    errorOut = pumpError_;
    return true;
}

DeviceHostCounterSnapshot DeviceSession::CopyDeviceHostCounters() const noexcept
{
    return deviceHostCounters_.Snapshot();
}

bool DeviceSession::openTransportOnly(
    const DeviceSessionStartRequest& request,
    std::string& errorOut)
{
    if (!transport_.Open(*request.profile, errorOut, request.openOptions))
    {
        return false;
    }

    mapper_ = std::make_unique<EmagicCableMapper>(*request.profile);
    return true;
}

bool DeviceSession::createPortsAndStartPump(
    const PortNameSet& portNames,
    std::string& errorOut)
{
    if (!midiBackend_->CreatePortSet(portNames, errorOut))
    {
        errorOut = "DeviceSession Virtual Port create failed: " + errorOut;
        return false;
    }
    return startPump(errorOut);
}

bool DeviceSession::armBulkInAsyncRing(std::string& errorOut)
{
    transport_.SetBulkInPacketHandler(&DeviceSession::bulkInPacketThunk, this);
    if (!transport_.StartBulkInAsyncRing(errorOut))
    {
        transport_.ClearBulkInPacketHandler();
        stopPump_.store(true);
        errorOut = "DeviceSession failed to arm bulk IN async ring: " + errorOut;
        return false;
    }
    const auto armedAt = std::chrono::steady_clock::now().time_since_epoch();
    bulkInRingArmedSteadyMs_.store(
        std::chrono::duration_cast<std::chrono::milliseconds>(armedAt).count(),
        std::memory_order_release);
    std::cout << "Bulk IN async ring armed (" << kBulkInAsyncSlotCount
              << " slots) before host MIDI sink\n"
              << std::flush;
    return true;
}

void DeviceSession::enableHostMidiSink()
{
    running_.store(true);
    // Keep host→device gated until waitPostStartInCalm opens the window after
    // the quiet-CC prime and IN ring is idle / fully pending.
    hostOutEarliestSteady_ = std::chrono::steady_clock::now() + std::chrono::hours(1);
    midiBackend_->SetHostToDeviceSink(&DeviceSession::hostToDeviceThunk, this);
    std::cout << "Host MIDI sink live (bulk IN ring already active; librarian OUT gated)\n"
              << std::flush;
}

void DeviceSession::queuePostStartPipePrime()
{
    // First host→device WriteBulk after Start often correlates with a dropped IN
    // head on the following librarian dump. Burn that on a quiet CC (SoundDiver-style).
    const uint8_t cc7Quiet[] = {0xB0, 0x07, 0x00};
    {
        std::lock_guard<std::mutex> queueLock(hostOutboundMutex_);
        if (!hostOutbound_.TryPush(0, cc7Quiet, sizeof(cc7Quiet)))
        {
            std::cerr << "Post-start pipe prime: outbound queue rejected quiet CC\n"
                      << std::flush;
            return;
        }
    }
    // Allow only the prime through the calm gate.
    hostOutEarliestSteady_ = std::chrono::steady_clock::now();
    std::cout << "Post-start pipe prime queued (quiet CC on Out1)\n" << std::flush;
    drainHostOutbound();
    // Re-gate until waitPostStartInCalm confirms IN idle + full pending depth.
    hostOutEarliestSteady_ = std::chrono::steady_clock::now() + std::chrono::hours(1);
}

void DeviceSession::waitPostStartInCalm() noexcept
{
    constexpr auto kMaxWait = std::chrono::milliseconds(500);
    constexpr auto kIdleNeed = std::chrono::milliseconds(40);
    constexpr auto kPoll = std::chrono::milliseconds(5);
    const auto deadline = std::chrono::steady_clock::now() + kMaxWait;

    while (std::chrono::steady_clock::now() < deadline)
    {
        if (stopPump_.load() || !running_.load())
        {
            openLibrarianOutboundGate(true);
            return;
        }
        if (postStartInIsCalm(kIdleNeed))
        {
            openLibrarianOutboundGate(false);
            return;
        }
        std::this_thread::sleep_for(kPoll);
    }
    openLibrarianOutboundGate(true);
}

bool DeviceSession::postStartInIsCalm(std::chrono::milliseconds idleNeed) noexcept
{
    std::size_t queued = 0;
    {
        std::lock_guard<std::mutex> lock(bulkInDeliverMutex_);
        queued = bulkInDeliverQueue_.size();
    }
    if (queued != 0 || transport_.CountPendingBulkInSlots() != kBulkInAsyncSlotCount)
    {
        return false;
    }
    const auto now = std::chrono::steady_clock::now();
    return lastBulkInPacketSteady_.time_since_epoch().count() == 0
        || (now - lastBulkInPacketSteady_ >= idleNeed);
}

void DeviceSession::openLibrarianOutboundGate(bool timedOut) noexcept
{
    const std::size_t pending = transport_.CountPendingBulkInSlots();
    std::size_t queued = 0;
    {
        std::lock_guard<std::mutex> lock(bulkInDeliverMutex_);
        queued = bulkInDeliverQueue_.size();
    }
    hostOutEarliestSteady_ = std::chrono::steady_clock::now();
    firstBurstDiagRemaining_.store(12, std::memory_order_relaxed);
    if (timedOut)
    {
        std::cerr << "Post-start IN calm timed out (pending=" << pending << "/"
                  << kBulkInAsyncSlotCount << " queued=" << queued
                  << "); opening librarian OUT anyway\n"
                  << std::flush;
        return;
    }
    std::cout << "Post-start IN calm ready (pending=" << pending << "/"
              << kBulkInAsyncSlotCount << " queued=" << queued << ")\n"
              << std::flush;
}

bool DeviceSession::startPump(std::string& errorOut)
{
    {
        std::lock_guard<std::mutex> lock(pumpErrorMutex_);
        pumpError_.clear();
    }
    resetPumpRuntimeState();
    stopPump_.store(false);

    // Arm INPUT_URBS, start the reader Wait loop, then open the host sink so
    // completions are harvested before any host→device WriteBulk.
    if (!armBulkInAsyncRing(errorOut))
    {
        return false;
    }

    try
    {
        readerThread_ = std::thread([this]() { readerLoop(); });
    }
    catch (const std::system_error& ex)
    {
        stopPump_.store(true);
        transport_.StopBulkInAsyncRing();
        errorOut = std::string("DeviceSession failed to start MIDI reader thread: ") + ex.what();
        return false;
    }

    enableHostMidiSink();
    queuePostStartPipePrime();
    waitPostStartInCalm();
    return true;
}

void DeviceSession::resetPumpRuntimeState() noexcept
{
    deviceHostCounters_.Reset();
    resetInFramers();
    (void)clearHostOutboundQueue();
    {
        std::lock_guard<std::mutex> lock(bulkInDeliverMutex_);
        bulkInDeliverQueue_.clear();
    }
    firstHostInquiryLogged_.store(false);
    firstBurstDiagRemaining_.store(0, std::memory_order_relaxed);
    bulkInRingArmedSteadyMs_.store(-1, std::memory_order_relaxed);
    lastBulkInPacketSteady_ = {};
    clearExpectInBurst();
    lastDumpRequest_.clear();
    lastDumpOutPort_ = 0;
}

void DeviceSession::stopPumpAndJoin() noexcept
{
    stopPump_.store(true);
    // Wake WaitForMultipleObjects on infinite-timeout IN URBs.
    transport_.AbortBulkInAsyncRing();
    if (readerThread_.joinable())
    {
        readerThread_.join();
    }
}

bool DeviceSession::Start(const DeviceSessionStartRequest& request, std::string& errorOut)
{
    Stop();

    if (request.profile == nullptr || request.midiBackend == nullptr || request.portNames == nullptr)
    {
        errorOut = "DeviceSession Start requires profile, MidiBackend, and PortNameSet";
        return false;
    }

    if (!portNamesMatchProfile(*request.profile, *request.portNames, errorOut)
        || !buildCableMaps(*request.profile, errorOut))
    {
        return false;
    }

    midiBackend_ = request.midiBackend;

    // Init + drain + Set Computer Mode BEFORE the reader thread so the reply is
    // not racing a second ReadBulk (ALSA: MT4 enters computer mode after reply,
    // or immediately via Set Computer Mode SysEx).
    if (!openTransportOnly(request, errorOut))
    {
        Stop();
        return false;
    }

    if (!sendInitMagic(errorOut))
    {
        errorOut = "DeviceSession init magic write failed: " + errorOut;
        Stop();
        return false;
    }

    if (!createPortsAndStartPump(*request.portNames, errorOut))
    {
        Stop();
        return false;
    }

    errorOut.clear();
    return true;
}

void DeviceSession::Stop() noexcept
{
    // Join reader before Close so AbortBulkInAsyncRing can finish in-flight IN.
    stopPumpAndJoin();
    resetInFramers();

    {
        std::lock_guard<std::mutex> lock(usbIoMutex_);
        // Drop sink + running_ before clearing the queue / DestroyPortSet so a late
        // VirtualMIDI callback cannot enqueue work that will never drain.
        if (midiBackend_ != nullptr)
        {
            midiBackend_->SetHostToDeviceSink(nullptr, nullptr);
        }
        running_.store(false);
    }
    (void)clearHostOutboundQueue();
    {
        std::lock_guard<std::mutex> deliverLock(bulkInDeliverMutex_);
        bulkInDeliverQueue_.clear();
    }
    deferredHostSends_.clear();
    deferHostSendDuringOut_ = false;
    betweenOutChunkDemuxFailed_ = false;
    clearExpectInBurst();

    // DestroyPortSet outside usbIoMutex_: teVirtualMIDI may wait for OUT callbacks.
    destroyPortsBestEffort();

    {
        std::lock_guard<std::mutex> lock(usbIoMutex_);
        if (transport_.IsOpen())
        {
            sendFinishMagicBestEffort();
        }
        transport_.Close();
        mapper_.reset();
        midiBackend_ = nullptr;
        inPortCount_ = 0;
        outPortCount_ = 0;
    }
}

bool DeviceSession::IsRunning() const noexcept
{
    return running_.load() && !stopPump_.load() && transport_.IsOpen() && mapper_ != nullptr
        && midiBackend_ != nullptr;
}
