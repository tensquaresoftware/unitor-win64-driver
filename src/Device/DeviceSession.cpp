#include "Device/DeviceSession.h"

DeviceSession::~DeviceSession()
{
    Stop();
}

bool DeviceSession::sendInitMagic(std::string& errorOut)
{
    // Linux sends the Unitor "get version" magic twice after OUT endpoint open.
    if (!transport_.WriteBulk(kEmagicInitMagic, kEmagicInitMagicSize, errorOut))
    {
        return false;
    }
    if (!transport_.WriteBulk(kEmagicInitMagic, kEmagicInitMagicSize, errorOut))
    {
        return false;
    }
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

bool DeviceSession::Start(const DeviceSessionStartRequest& request, std::string& errorOut)
{
    Stop();

    if (request.profile == nullptr || request.midiBackend == nullptr || request.portNames == nullptr)
    {
        errorOut = "DeviceSession Start requires profile, MidiBackend, and PortNameSet";
        return false;
    }

    if (!portNamesMatchProfile(*request.profile, *request.portNames, errorOut))
    {
        return false;
    }

    midiBackend_ = request.midiBackend;

    if (!transport_.Open(*request.profile, errorOut, request.openOptions))
    {
        midiBackend_ = nullptr;
        return false;
    }

    mapper_ = std::make_unique<EmagicCableMapper>(*request.profile);

    if (!sendInitMagic(errorOut))
    {
        const std::string initError = errorOut;
        Stop();
        errorOut = "DeviceSession init magic write failed: " + initError;
        return false;
    }

    if (!midiBackend_->CreatePortSet(*request.portNames, errorOut))
    {
        const std::string portError = errorOut;
        Stop();
        errorOut = "DeviceSession Virtual Port create failed: " + portError;
        return false;
    }

    running_ = true;
    errorOut.clear();
    return true;
}

void DeviceSession::Stop() noexcept
{
    // AD-9: destroy ports first, then Emagic finish + transport close.
    destroyPortsBestEffort();

    if (running_ || transport_.IsOpen())
    {
        sendFinishMagicBestEffort();
    }

    transport_.Close();
    mapper_.reset();
    midiBackend_ = nullptr;
    running_ = false;
}

bool DeviceSession::IsRunning() const noexcept
{
    return running_ && transport_.IsOpen() && mapper_ != nullptr && midiBackend_ != nullptr;
}
