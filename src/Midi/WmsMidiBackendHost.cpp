// WMS host→device MessageReceived convert/deliver (Windows MIDI Services App SDK).

#include "Midi/WmsMidiBackend.h"

#ifdef _WIN32

#include "Midi/WmsMidiBackendDetail.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.Windows.Devices.Midi2.h>
#include <winrt/Microsoft.Windows.Devices.Midi2.Messages.h>

#include <iostream>
#include <string>
#include <vector>

namespace midicore = winrt::Microsoft::Windows::Devices::Midi2;
namespace wms_messages = winrt::Microsoft::Windows::Devices::Midi2::Messages;

namespace
{
void logHostConvertFailure(const std::string& detail)
{
    std::cerr << "WMS host→device convert failed: " << detail << '\n' << std::flush;
}

bool deliverHostUmpMessage(
    WmsEndpointSlot* slot,
    midicore::MidiMessageReceivedEventArgs const& args)
{
    auto words = winrt::single_threaded_vector<uint32_t>();
    args.AppendWordsToList(words);
    if (words.Size() == 0)
    {
        return true;
    }
    auto bytes = wms_messages::MidiMessageConverter::
        ConvertSingleGroupCompleteMessageUmpWordsToMidi1Bytes(words);
    if (bytes.Size() == 0)
    {
        return true;
    }
    std::vector<uint8_t> buffer;
    buffer.reserve(bytes.Size());
    for (uint8_t value : bytes)
    {
        buffer.push_back(value);
    }
    wmsDeliverHostMessage(slot->backend, slot->outPortIndex, buffer.data(), buffer.size());
    return true;
}
} // namespace

void wmsOnHostMessageReceived(
    WmsEndpointSlot* slot,
    midicore::MidiMessageReceivedEventArgs const& args)
{
    if (slot == nullptr || slot->backend == nullptr)
    {
        return;
    }
    try
    {
        deliverHostUmpMessage(slot, args);
    }
    catch (const winrt::hresult_error& ex)
    {
        logHostConvertFailure(winrt::to_string(ex.message()));
    }
    catch (const std::exception& ex)
    {
        logHostConvertFailure(ex.what());
    }
    catch (...)
    {
        logHostConvertFailure("unknown exception");
    }
}

#endif // _WIN32
