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

bool DeviceSession::Start(
    const DeviceProfile& profile,
    std::string& errorOut,
    WinUsbOpenOptions options)
{
    Stop();

    if (!transport_.Open(profile, errorOut, options))
    {
        return false;
    }

    mapper_ = std::make_unique<EmagicCableMapper>(profile);

    if (!sendInitMagic(errorOut))
    {
        const std::string initError = errorOut;
        Stop();
        errorOut = "DeviceSession init magic write failed: " + initError;
        return false;
    }

    running_ = true;
    errorOut.clear();
    return true;
}

void DeviceSession::Stop() noexcept
{
    if (running_ || transport_.IsOpen())
    {
        sendFinishMagicBestEffort();
    }

    transport_.Close();
    mapper_.reset();
    running_ = false;
}

bool DeviceSession::IsRunning() const noexcept
{
    return running_ && transport_.IsOpen() && mapper_ != nullptr;
}
