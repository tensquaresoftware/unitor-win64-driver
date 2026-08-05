// One live usermode session per MT4: WinUSB + Emagic mapper + MidiBackend ports (AD-4 / AD-9).
// Only a live DeviceSession creates/destroys that unit's Virtual Ports.

#pragma once

#include "Midi/MidiBackend.h"
#include "Profile/DeviceProfile.h"
#include "Protocol/EmagicCableMapper.h"
#include "Usb/WinUsbTransport.h"

#include <memory>
#include <string>

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

    // Normative sequence: WinUSB open → Emagic init → MidiBackend::CreatePortSet.
    bool Start(const DeviceSessionStartRequest& request, std::string& errorOut);

    void Stop() noexcept;
    bool IsRunning() const noexcept;

private:
    bool sendInitMagic(std::string& errorOut);
    void sendFinishMagicBestEffort() noexcept;
    void destroyPortsBestEffort() noexcept;
    bool portNamesMatchProfile(
        const DeviceProfile& profile,
        const PortNameSet& portNames,
        std::string& errorOut) const;

    WinUsbTransport transport_;
    std::unique_ptr<EmagicCableMapper> mapper_;
    MidiBackend* midiBackend_ = nullptr;
    bool running_ = false;
};
