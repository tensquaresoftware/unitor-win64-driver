// WMS directional virtual-device create/destroy (Windows MIDI Services App SDK).

#include "Midi/WmsMidiBackend.h"

#ifdef _WIN32

#include "Midi/MidiBackendSelect.h"
#include "Midi/WmsMidiBackendDetail.h"
#include "Midi/WmsMidiWinSupport.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.Windows.Devices.Midi2.h>
#include <winrt/Microsoft.Windows.Devices.Midi2.Endpoints.Virtual.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

namespace midicore = winrt::Microsoft::Windows::Devices::Midi2;
namespace wms_virtual = winrt::Microsoft::Windows::Devices::Midi2::Endpoints::Virtual;

void wmsDeliverHostMessage(
    WmsMidiBackend* backend,
    std::size_t outPortIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount)
{
    if (backend == nullptr)
    {
        return;
    }
    backend->acceptHostMidiBytes(outPortIndex, midiBytes, byteCount);
}

namespace
{
enum class WmsEndpointFace
{
    DeviceToHost,
    HostToDevice,
};

struct CreateEndpointRequest
{
    const std::string* utf8Name = nullptr;
    WmsEndpointFace face = WmsEndpointFace::DeviceToHost;
    WmsEndpointSlot* slotOut = nullptr;
    midicore::MidiSession* session = nullptr;
    WmsMidiBackend* backend = nullptr;
    std::size_t outPortIndex = 0;
    const std::wstring* instanceSuffix = nullptr;
};

std::wstring makeProductInstanceId(
    const std::wstring& wideName,
    const std::wstring& instanceSuffix)
{
    return makeWmsProductInstanceId(wideName, instanceSuffix);
}

void clearEndpointSlot(WmsEndpointSlot& slot, midicore::MidiSession& /*session*/) noexcept
{
    // Skip Disconnect/revoke: both can stall with ghost midisrv endpoints.
    slot.hasMessageToken = false;
    slot.messageToken = {};
    slot.connection = nullptr;
    slot.device = nullptr;
    slot.backend = nullptr;
    slot.outPortIndex = 0;
}

midicore::MidiDeclaredEndpointInfo makeDeclaredEndpointInfo(
    const std::wstring& wideName,
    const std::wstring& instanceSuffix)
{
    midicore::MidiDeclaredEndpointInfo endpointInfo{};
    endpointInfo.Name = wideName;
    endpointInfo.ProductInstanceId = makeProductInstanceId(wideName, instanceSuffix);
    endpointInfo.SupportsMidi10Protocol = true;
    endpointInfo.SupportsMidi20Protocol = true;
    endpointInfo.SupportsReceivingJitterReductionTimestamps = false;
    endpointInfo.SupportsSendingJitterReductionTimestamps = false;
    endpointInfo.HasStaticFunctionBlocks = true;
    endpointInfo.DeclaredFunctionBlockCount = 1;
    endpointInfo.SpecificationVersionMajor = 1;
    endpointInfo.SpecificationVersionMinor = 1;
    return endpointInfo;
}

void applyFunctionBlockDirection(midicore::MidiFunctionBlock& block, WmsEndpointFace face)
{
    if (face == WmsEndpointFace::DeviceToHost)
    {
        block.Direction(midicore::MidiFunctionBlockDirection::BlockOutput);
        block.UIHint(midicore::MidiFunctionBlockUIHint::Sender);
        return;
    }
    block.Direction(midicore::MidiFunctionBlockDirection::BlockInput);
    block.UIHint(midicore::MidiFunctionBlockUIHint::Receiver);
}

wms_virtual::MidiVirtualDeviceCreationConfig makeCreationConfig(
    const std::wstring& wideName,
    WmsEndpointFace face,
    const std::wstring& instanceSuffix)
{
    auto config = wms_virtual::MidiVirtualDeviceCreationConfig(
        wideName,
        L"Unitor MT4 Bridge virtual endpoint",
        L"Ten Square Software",
        makeDeclaredEndpointInfo(wideName, instanceSuffix));
    config.CreateOnlyUmpEndpoints(false);

    midicore::MidiFunctionBlock block;
    block.Number(0);
    block.IsActive(true);
    block.Name(L"Port");
    block.FirstGroup(midicore::MidiGroup(static_cast<uint8_t>(0)));
    block.GroupCount(1);
    block.RepresentsMidi10Connection(
        midicore::MidiFunctionBlockRepresentsMidi10Connection::YesBandwidthUnrestricted);
    applyFunctionBlockDirection(block, face);
    config.FunctionBlocks().Append(block);
    return config;
}

bool wireHostMessageHandler(WmsEndpointSlot& slot, midicore::MidiEndpointConnection& connection)
{
    WmsEndpointSlot* slotPtr = &slot;
    slot.messageToken = connection.MessageReceived(
        [slotPtr](
            midicore::IMidiMessageReceivedEventSource const&,
            midicore::MidiMessageReceivedEventArgs const& args) {
            wmsOnHostMessageReceived(slotPtr, args);
        });
    slot.hasMessageToken = true;
    return true;
}

bool bindEndpointObjects(
    CreateEndpointRequest const& request,
    wms_virtual::MidiVirtualDevice const& device,
    midicore::MidiEndpointConnection const& connection)
{
    request.slotOut->device = device;
    request.slotOut->connection = connection;
    request.slotOut->backend = request.backend;
    request.slotOut->outPortIndex = request.outPortIndex;
    return true;
}

bool openEndpointConnection(
    CreateEndpointRequest const& request,
    wms_virtual::MidiVirtualDevice const& device,
    std::string& errorOut)
{
    auto connection = request.session->CreateEndpointConnection(device.DeviceEndpointDeviceId());
    if (connection == nullptr)
    {
        errorOut = "MidiSession::CreateEndpointConnection returned null";
        return false;
    }
    connection.AddMessageProcessingPlugin(device);
    bindEndpointObjects(request, device, connection);
    if (!connection.Open())
    {
        errorOut = "MidiEndpointConnection::Open failed for WMS virtual endpoint";
        return false;
    }
    if (request.face == WmsEndpointFace::HostToDevice)
    {
        wireHostMessageHandler(*request.slotOut, request.slotOut->connection);
    }
    return true;
}

bool createAndOpenEndpoint(CreateEndpointRequest const& request, std::string& errorOut)
{
    if (request.utf8Name == nullptr || request.slotOut == nullptr || request.session == nullptr
        || request.backend == nullptr || request.instanceSuffix == nullptr)
    {
        errorOut = "WMS createAndOpenEndpoint received null request fields";
        return false;
    }

    const std::wstring wideName = utf8ToWideWms(*request.utf8Name, errorOut);
    if (wideName.empty())
    {
        return false;
    }

    auto device = wms_virtual::MidiVirtualDeviceManager::CreateVirtualDevice(
        makeCreationConfig(wideName, request.face, *request.instanceSuffix));
    if (device == nullptr)
    {
        errorOut = "MidiVirtualDeviceManager::CreateVirtualDevice returned null";
        return false;
    }
    device.SuppressHandledMessages(true);
    return openEndpointConnection(request, device, errorOut);
}

std::wstring makeInstanceSuffix()
{
    wchar_t buffer[12] = {};
    const unsigned tick = static_cast<unsigned>(::GetTickCount64() & 0xFFFFu);
    const unsigned pid = static_cast<unsigned>(::GetCurrentProcessId() & 0xFFFFu);
    swprintf_s(buffer, L"%04X%04X", tick, pid);
    return std::wstring(buffer);
}

struct CreatePortFaceArgs
{
    WmsMidiBackend* backend = nullptr;
    midicore::MidiSession* session = nullptr;
    const std::wstring* instanceSuffix = nullptr;
    const std::string* names = nullptr;
    std::size_t count = 0;
    WmsEndpointSlot* slots = nullptr;
    WmsEndpointFace face = WmsEndpointFace::DeviceToHost;
};

bool createPortFace(const CreatePortFaceArgs& args, std::string& errorOut)
{
    for (std::size_t index = 0; index < args.count; ++index)
    {
        CreateEndpointRequest request;
        request.utf8Name = &args.names[index];
        request.face = args.face;
        request.slotOut = &args.slots[index];
        request.session = args.session;
        request.backend = args.backend;
        request.outPortIndex = (args.face == WmsEndpointFace::HostToDevice) ? index : 0;
        request.instanceSuffix = args.instanceSuffix;
        if (!createAndOpenEndpoint(request, errorOut))
        {
            return false;
        }
    }
    return true;
}
} // namespace

bool WmsMidiBackend::createDirectionalEndpoints(const PortNameSet& names, std::string& errorOut)
{
    impl_->session = midicore::MidiSession::Create(L"Unitor MT4 Bridge");
    if (impl_->session == nullptr)
    {
        errorOut = "MidiSession::Create returned null";
        return false;
    }

    impl_->instanceSuffix = makeInstanceSuffix();
    impl_->inCount = names.inCount;
    impl_->outCount = names.outCount;

    CreatePortFaceArgs inArgs;
    inArgs.backend = this;
    inArgs.session = &impl_->session;
    inArgs.instanceSuffix = &impl_->instanceSuffix;
    inArgs.names = names.inNames;
    inArgs.count = names.inCount;
    inArgs.slots = impl_->inSlots;
    inArgs.face = WmsEndpointFace::DeviceToHost;

    CreatePortFaceArgs outArgs = inArgs;
    outArgs.names = names.outNames;
    outArgs.count = names.outCount;
    outArgs.slots = impl_->outSlots;
    outArgs.face = WmsEndpointFace::HostToDevice;

    if (!createPortFace(inArgs, errorOut) || !createPortFace(outArgs, errorOut))
    {
        return false;
    }

    errorOut.clear();
    return true;
}

void WmsMidiBackend::destroyDirectionalEndpoints() noexcept
{
    if (impl_ == nullptr)
    {
        return;
    }
    // Null slots first, then abandon the session without Close(): Close() can stall
    // indefinitely when midisrv retains ghost virtual endpoints from prior hangs.
    if (impl_->session != nullptr)
    {
        for (std::size_t index = 0; index < impl_->inCount; ++index)
        {
            clearEndpointSlot(impl_->inSlots[index], impl_->session);
        }
        for (std::size_t index = 0; index < impl_->outCount; ++index)
        {
            clearEndpointSlot(impl_->outSlots[index], impl_->session);
        }
        winrt::detach_abi(impl_->session);
    }
    impl_->inCount = 0;
    impl_->outCount = 0;
    impl_->instanceSuffix.clear();
    {
        std::lock_guard<std::mutex> assembleLock(impl_->hostAssembleMutex);
        for (Midi1StreamAssembler& assembler : impl_->hostAssemblers)
        {
            assembler.Clear();
        }
    }
}

void WmsMidiBackend::rollbackFailedCreate() noexcept
{
    destroyDirectionalEndpoints();
    if (impl_ != nullptr && impl_->runtimeHeld)
    {
        releaseWmsRuntime();
        impl_->runtimeHeld = false;
    }
}

bool WmsMidiBackend::ensureTransportOrFail(std::string& errorOut)
{
    std::string transportError;
    if (rejectMissingWmsTransport(queryWmsTransportAvailable(transportError), errorOut))
    {
        return true;
    }
    if (!transportError.empty() && errorOut.find(transportError) == std::string::npos)
    {
        errorOut += " (";
        errorOut += transportError;
        errorOut += ")";
    }
    rollbackFailedCreate();
    return false;
}

bool WmsMidiBackend::createPortsGuarded(const PortNameSet& names, std::string& errorOut)
{
    try
    {
        if (!createDirectionalEndpoints(names, errorOut))
        {
            rollbackFailedCreate();
            return false;
        }
        return true;
    }
    catch (const winrt::hresult_error& ex)
    {
        rollbackFailedCreate();
        errorOut = std::string("WMS CreatePortSet failed: ") + winrt::to_string(ex.message());
        return false;
    }
    catch (const std::exception& ex)
    {
        rollbackFailedCreate();
        errorOut = std::string("WMS CreatePortSet failed: ") + ex.what();
        return false;
    }
    catch (...)
    {
        rollbackFailedCreate();
        errorOut = "WMS CreatePortSet failed: unknown exception";
        return false;
    }
}

bool WmsMidiBackend::CreatePortSet(const PortNameSet& names, std::string& errorOut)
{
    DestroyPortSet();
    if (!validateWmsPortNameSet(names, errorOut) || !ensureWmsRuntimeReady(errorOut))
    {
        return false;
    }
    impl_->runtimeHeld = true;
    if (!ensureTransportOrFail(errorOut) || !createPortsGuarded(names, errorOut))
    {
        return false;
    }
    if (impl_->inCount == 0 && impl_->outCount == 0)
    {
        rollbackFailedCreate();
        errorOut = "WMS CreatePortSet produced zero ports (fail closed)";
        return false;
    }
    portsCreated_ = true;
    errorOut.clear();
    return true;
}

void WmsMidiBackend::DestroyPortSet() noexcept
{
    destroyDirectionalEndpoints();
    portsCreated_ = false;
    if (impl_ != nullptr && impl_->runtimeHeld)
    {
        releaseWmsRuntime();
        impl_->runtimeHeld = false;
    }
}

#endif // _WIN32
