#include "Device/DeviceSession.h"
#include "Device/DeviceSessionSupport.h"

#include <iostream>
#include <sstream>
#include <system_error>

namespace
{
constexpr std::size_t kEncodeBufferCapacity = 4096;
}

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
    if (!transport_.WriteBulk(framed, buffer.size, errorOut))
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
    std::cout << "Emagic init/computer-mode: drained " << drainedBytes
              << " bulk IN byte(s); read capacity="
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

void DeviceSession::hostToDeviceThunk(
    void* context,
    std::size_t outPortIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount)
{
    auto* session = static_cast<DeviceSession*>(context);
    if (session == nullptr)
    {
        return;
    }
    session->handleHostMidi(outPortIndex, midiBytes, byteCount);
}

void DeviceSession::handleHostMidi(
    std::size_t outPortIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount)
{
    if (midiBytes == nullptr || byteCount == 0)
    {
        return;
    }

    // Hold usbIoMutex_ through encode + WriteBulk so concurrent OUT callbacks
    // cannot interleave Emagic F5 frames, and Stop cannot Close mid-write.
    uint8_t encodeBytes[kEncodeBufferCapacity] = {};
    HostEncodeScratch scratch{encodeBytes, sizeof(encodeBytes), 0, 0};
    std::lock_guard<std::mutex> lock(usbIoMutex_);
    if (!encodeHostMidiLocked(outPortIndex, midiBytes, byteCount, scratch))
    {
        return;
    }
    if (stopPump_.load() || !running_.load())
    {
        return;
    }

    std::string error;
    if (!transport_.WriteBulk(scratch.bytes, scratch.size, error))
    {
        recordPumpFailure(formatPortCableFailure(
            "Host→device WriteBulk", outPortIndex, scratch.cableIndex, error));
    }
}

bool DeviceSession::encodeHostMidiLocked(
    std::size_t outPortIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount,
    HostEncodeScratch& scratch)
{
    if (stopPump_.load() || !running_.load() || mapper_ == nullptr || midiBackend_ == nullptr)
    {
        return false;
    }
    if (outPortIndex >= outPortCount_)
    {
        std::ostringstream stream;
        stream << "Host→device rejected invalid OUT port index " << outPortIndex;
        recordPumpFailure(stream.str());
        return false;
    }

    scratch.cableIndex = outCableByPort_[outPortIndex];
    EncodeRequest request{scratch.cableIndex, midiBytes, byteCount};
    EncodeBuffer buffer{scratch.bytes, scratch.capacity, 0};
    std::string error;
    if (!mapper_->EncodeToDevice(request, buffer, error))
    {
        recordPumpFailure(formatPortCableFailure(
            "Host→device encode", outPortIndex, scratch.cableIndex, error));
        return false;
    }
    scratch.size = buffer.size;
    return true;
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

bool DeviceSession::startPump(std::string& errorOut)
{
    {
        std::lock_guard<std::mutex> lock(pumpErrorMutex_);
        pumpError_.clear();
    }

    deviceHostCounters_.Reset();
    resetInFramers();

    stopPump_.store(false);
    // Accept host→device as soon as the sink is live (before Start returns).
    running_.store(true);
    midiBackend_->SetHostToDeviceSink(&DeviceSession::hostToDeviceThunk, this);

    try
    {
        readerThread_ = std::thread([this]() { readerLoop(); });
    }
    catch (const std::system_error& ex)
    {
        stopPump_.store(true);
        running_.store(false);
        {
            std::lock_guard<std::mutex> lock(usbIoMutex_);
            midiBackend_->SetHostToDeviceSink(nullptr, nullptr);
        }
        errorOut = std::string("DeviceSession failed to start MIDI reader thread: ") + ex.what();
        return false;
    }

    return true;
}

void DeviceSession::stopPumpAndJoin() noexcept
{
    stopPump_.store(true);
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
    // Join reader before Close so in-flight ReadBulk can finish via IN timeout.
    stopPumpAndJoin();
    resetInFramers();

    {
        std::lock_guard<std::mutex> lock(usbIoMutex_);
        // Drop sink + running_ before DestroyPortSet so callbacks finish WriteBulk
        // under this lock or bail without touching a closed transport.
        if (midiBackend_ != nullptr)
        {
            midiBackend_->SetHostToDeviceSink(nullptr, nullptr);
        }
        running_.store(false);
    }

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
