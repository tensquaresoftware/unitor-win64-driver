// One live usermode session per MT4: WinUSB transport + Emagic cable mapper (AD-4).
// Virtual ports / MidiBackend arrive in Story 1.5 — only a live session will own that lifecycle.

#pragma once

#include "Profile/DeviceProfile.h"
#include "Protocol/EmagicCableMapper.h"
#include "Usb/WinUsbTransport.h"

#include <memory>
#include <string>

class DeviceSession
{
public:
    DeviceSession() = default;
    ~DeviceSession();

    DeviceSession(const DeviceSession&) = delete;
    DeviceSession& operator=(const DeviceSession&) = delete;

    bool Start(
        const DeviceProfile& profile,
        std::string& errorOut,
        WinUsbOpenOptions options = {});

    void Stop() noexcept;
    bool IsRunning() const noexcept;

private:
    bool sendInitMagic(std::string& errorOut);
    void sendFinishMagicBestEffort() noexcept;

    WinUsbTransport transport_;
    std::unique_ptr<EmagicCableMapper> mapper_;
    bool running_ = false;
};
