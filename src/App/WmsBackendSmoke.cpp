// WMS MidiBackend Create/Destroy smoke without a live MT4 USB session.

#include "App/WmsBackendSmoke.h"

#include "Device/DeviceSessionManager.h"
#include "Midi/MidiBackend.h"
#include "Midi/MidiBackendSelect.h"
#include "Profile/DeviceProfile.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

namespace
{
bool buildUnitOnePortNames(PortNameSet& namesOut, std::string& errorOut)
{
    const DeviceProfile* mt4 = findDeviceProfile(kEmagicVendorId, kMt4ProductId);
    if (mt4 == nullptr)
    {
        errorOut = "MT4 DeviceProfile missing";
        return false;
    }
    DeviceSessionManager manager;
    return manager.buildPortNameSet(*mt4, 1, namesOut, errorOut);
}

bool createPortsOrFail(MidiBackend& backend, const PortNameSet& names, std::string& errorOut)
{
    if (!backend.CreatePortSet(names, errorOut))
    {
        return false;
    }
    std::cout << "WMS CreatePortSet ok (" << names.inCount << " IN / " << names.outCount
              << " OUT)" << std::endl;
    return true;
}
} // namespace

int runWmsBackendLifecycleSmoke()
{
    setMidiBackendKindOverride(MidiBackendKind::Wms);
    PortNameSet names;
    std::string error;
    if (!buildUnitOnePortNames(names, error))
    {
        std::cerr << "WMS lifecycle smoke failed: " << error << '\n';
        return 1;
    }

    std::unique_ptr<MidiBackend> backend = createMidiBackend(MidiBackendKind::Wms);
    if (backend == nullptr)
    {
        std::cerr << "WMS lifecycle smoke failed: createMidiBackend returned null\n";
        return 1;
    }

    if (!createPortsOrFail(*backend, names, error))
    {
        std::cerr << "WMS lifecycle smoke failed: CreatePortSet: " << error << '\n';
        return 1;
    }

    // Skip SendToHost here: on this lab build it can stall indefinitely with no
    // MIDI client attached. Create/Destroy covers the Story 6.1 lifecycle gate.

    std::cout << "WMS: DestroyPortSet..." << std::endl;
    backend->DestroyPortSet();
    backend->DestroyPortSet();
    std::cout << "WMS DestroyPortSet ok (idempotent)" << std::endl;
    std::cout << "WMS backend lifecycle smoke passed" << std::endl;
    return 0;
}
