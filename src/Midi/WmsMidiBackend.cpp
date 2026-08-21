#include "Midi/WmsMidiBackend.h"

#include "Midi/MidiBackendSelect.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

#ifndef _WIN32

struct WmsMidiBackend::Impl
{
};

WmsMidiBackend::WmsMidiBackend()
    : impl_(std::make_unique<Impl>())
{
}

WmsMidiBackend::~WmsMidiBackend() = default;

bool WmsMidiBackend::CreatePortSet(const PortNameSet& /*names*/, std::string& errorOut)
{
    return rejectMissingWmsTransport(false, errorOut);
}

void WmsMidiBackend::DestroyPortSet() noexcept
{
    portsCreated_ = false;
}

bool WmsMidiBackend::SendToHost(
    std::size_t /*inPortIndex*/,
    const uint8_t* /*midiBytes*/,
    std::size_t /*byteCount*/,
    std::string& errorOut)
{
    errorOut = "WMS SendToHost is not available on this platform";
    return false;
}

void WmsMidiBackend::SetHostToDeviceSink(HostToDeviceSink sink, void* context) noexcept
{
    std::lock_guard<std::mutex> lock(hostToDeviceMutex_);
    hostToDeviceSink_ = sink;
    hostToDeviceContext_ = context;
}

bool WmsMidiBackend::createDirectionalEndpoints(
    const PortNameSet& /*names*/,
    std::string& errorOut)
{
    return rejectMissingWmsTransport(false, errorOut);
}

void WmsMidiBackend::destroyDirectionalEndpoints() noexcept
{
}

void WmsMidiBackend::rollbackFailedCreate() noexcept
{
}

bool WmsMidiBackend::ensureTransportOrFail(std::string& errorOut)
{
    return rejectMissingWmsTransport(false, errorOut);
}

void WmsMidiBackend::forwardHostToDevice(
    std::size_t /*outPortIndex*/,
    const uint8_t* /*midiBytes*/,
    std::size_t /*byteCount*/) noexcept
{
}

void WmsMidiBackend::acceptHostMidiBytes(
    std::size_t /*outPortIndex*/,
    const uint8_t* /*midiBytes*/,
    std::size_t /*byteCount*/) noexcept
{
}

#else

#include "Midi/WmsMidiBackendDetail.h"
#include "Midi/WmsMidiWinSupport.h"

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.Windows.Devices.Midi2.Messages.h>

namespace midicore = winrt::Microsoft::Windows::Devices::Midi2;
namespace wms_messages = winrt::Microsoft::Windows::Devices::Midi2::Messages;

namespace
{
std::string formatInPortIndexError(const char* reason, std::size_t inPortIndex)
{
    std::ostringstream stream;
    stream << "WMS SendToHost " << reason << " for IN port index " << inPortIndex;
    return stream.str();
}

uint32_t umpWordCountFromFirstWord(uint32_t firstWord)
{
    // MIDI 2.0 UMP message type (top nibble) → word count.
    switch ((firstWord >> 28) & 0xFu)
    {
    case 0x0: // Utility
    case 0x1: // System Real Time / Common
    case 0x2: // MIDI 1.0 Channel Voice
        return 1;
    case 0x3: // Data (SysEx7)
    case 0x4: // MIDI 2.0 Channel Voice
        return 2;
    case 0x5: // Data (SysEx8 / MDS)
    case 0xD: // Flex Data
    case 0xF: // Stream
        return 4;
    default:
        return 0; // unknown type — caller must fail closed
    }
}

struct UmpWordSpan
{
    winrt::Windows::Foundation::Collections::IVector<uint32_t> const* words = nullptr;
    uint32_t offset = 0;
    uint32_t maxWords = 0;
};

bool nextUmpChunkEnd(const UmpWordSpan& span, uint32_t& chunkEndOut, std::string& errorOut)
{
    const auto& words = *span.words;
    const uint32_t total = words.Size();
    uint32_t chunkEnd = span.offset;
    while (chunkEnd < total)
    {
        const uint32_t msgWords = umpWordCountFromFirstWord(words.GetAt(chunkEnd));
        if (msgWords == 0 || chunkEnd + msgWords > total)
        {
            std::ostringstream stream;
            stream << "WMS SendToHost unsupported or truncated UMP at offset " << chunkEnd
                   << "/" << total;
            errorOut = stream.str();
            return false;
        }
        if (chunkEnd > span.offset && (chunkEnd - span.offset) + msgWords > span.maxWords)
        {
            break;
        }
        chunkEnd += msgWords;
        if ((chunkEnd - span.offset) >= span.maxWords)
        {
            break;
        }
    }
    chunkEndOut = chunkEnd;
    errorOut.clear();
    return true;
}

struct UmpChunkSend
{
    midicore::MidiEndpointConnection const* connection = nullptr;
    winrt::Windows::Foundation::Collections::IVector<uint32_t> const* chunk = nullptr;
    uint32_t offset = 0;
    uint32_t total = 0;
};

bool sendUmpChunkWithRetry(const UmpChunkSend& send, std::string& errorOut)
{
    constexpr int kMaxBufferFullRetries = 200;
    constexpr auto kBufferFullSleep = std::chrono::milliseconds(1);

    for (int retries = 0;; ++retries)
    {
        const auto result = send.connection->SendMultipleMessagesWordList(
            midicore::MidiClock::TimestampConstantSendImmediately(),
            *send.chunk);
        if (!midicore::MidiEndpointConnection::SendMessageFailed(result))
        {
            errorOut.clear();
            return true;
        }

        const auto flags = static_cast<uint32_t>(result);
        const bool bufferFull =
            (flags & static_cast<uint32_t>(midicore::MidiSendMessageResults::BufferFull))
            != 0;
        const bool partial =
            (flags
             & static_cast<uint32_t>(
                 midicore::MidiSendMessageResults::MessageListPartiallyProcessed))
            != 0;
        // Partial + BufferFull without a consumed-word count cannot be retried
        // safely (risk of duplicate UMPs). Fail loudly so lab scripts catch it.
        if (!bufferFull || partial || retries >= kMaxBufferFullRetries)
        {
            std::ostringstream stream;
            stream << "WMS SendMultipleMessagesWordList failed (result=" << flags
                   << " offset=" << send.offset << "/" << send.total << ")";
            errorOut = stream.str();
            return false;
        }
        std::this_thread::sleep_for(kBufferFullSleep);
    }
}

bool sendUmpWords(
    midicore::MidiEndpointConnection const& connection,
    winrt::Windows::Foundation::Collections::IVector<uint32_t> const& words,
    std::string& errorOut)
{
    if (words.Size() == 0)
    {
        errorOut = "WMS SendToHost produced zero UMP words from MIDI 1.0 bytes";
        return false;
    }

    // VirtualMIDI accepts a full SysEx blob in one call. WMS endpoint buffers are
    // smaller — blasting thousands of SysEx7 UMPs returns Failed|BufferFull
    // (0x11000000). Send paced whole-UMP chunks with BufferFull retry.
    constexpr uint32_t kMaxWordsPerChunk = 64; // ~32× SysEx7 packets

    const uint32_t total = words.Size();
    uint32_t offset = 0;
    while (offset < total)
    {
        uint32_t chunkEnd = 0;
        const UmpWordSpan span{&words, offset, kMaxWordsPerChunk};
        if (!nextUmpChunkEnd(span, chunkEnd, errorOut))
        {
            return false;
        }

        auto chunk = winrt::single_threaded_vector<uint32_t>();
        for (uint32_t index = offset; index < chunkEnd; ++index)
        {
            chunk.Append(words.GetAt(index));
        }
        const UmpChunkSend send{&connection, &chunk, offset, total};
        if (!sendUmpChunkWithRetry(send, errorOut))
        {
            return false;
        }
        offset = chunkEnd;
    }

    errorOut.clear();
    return true;
}
struct SendToHostValidation
{
    bool portsCreated = false;
    std::size_t inCount = 0;
    std::size_t inPortIndex = 0;
    const uint8_t* midiBytes = nullptr;
    std::size_t byteCount = 0;
    midicore::MidiEndpointConnection connection{nullptr};
};

bool validateSendToHostArgs(const SendToHostValidation& args, std::string& errorOut)
{
    if (!args.portsCreated)
    {
        errorOut = "WMS SendToHost requires an active port set";
        return false;
    }
    if (args.inPortIndex >= args.inCount)
    {
        errorOut = formatInPortIndexError("rejected out-of-range index", args.inPortIndex);
        return false;
    }
    if (args.midiBytes == nullptr || args.byteCount == 0)
    {
        errorOut = formatInPortIndexError("rejected empty MIDI payload", args.inPortIndex);
        return false;
    }
    if (args.connection == nullptr)
    {
        errorOut = formatInPortIndexError(
            "rejected null endpoint connection", args.inPortIndex);
        return false;
    }
    return true;
}
} // namespace

WmsMidiBackend::WmsMidiBackend()
    : impl_(std::make_unique<Impl>())
{
}

WmsMidiBackend::~WmsMidiBackend()
{
    DestroyPortSet();
}

void WmsMidiBackend::SetHostToDeviceSink(HostToDeviceSink sink, void* context) noexcept
{
    std::lock_guard<std::mutex> lock(hostToDeviceMutex_);
    hostToDeviceSink_ = sink;
    hostToDeviceContext_ = context;
}

void WmsMidiBackend::forwardHostToDevice(
    std::size_t outPortIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount) noexcept
{
    HostToDeviceSink sink = nullptr;
    void* context = nullptr;
    {
        std::lock_guard<std::mutex> lock(hostToDeviceMutex_);
        sink = hostToDeviceSink_;
        context = hostToDeviceContext_;
    }
    if (sink == nullptr || midiBytes == nullptr || byteCount == 0)
    {
        if (sink == nullptr && midiBytes != nullptr && byteCount > 0)
        {
            const std::uint64_t drops =
                nullSinkDrops_.fetch_add(1, std::memory_order_relaxed) + 1;
            if (drops <= 8 || (drops % 256) == 0)
            {
                std::cerr << "WMS host→device dropped (sink unset) out_port="
                          << (outPortIndex + 1) << " bytes=" << byteCount
                          << " drops=" << drops << "\n"
                          << std::flush;
            }
        }
        return;
    }
    sink(context, outPortIndex, midiBytes, byteCount);
}

void WmsMidiBackend::acceptHostMidiBytes(
    std::size_t outPortIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount) noexcept
{
    if (impl_ == nullptr || midiBytes == nullptr || byteCount == 0)
    {
        return;
    }
    if (outPortIndex >= kMaxMidiBackendOutPorts || outPortIndex >= impl_->outCount)
    {
        return;
    }

    // Reassemble before DeviceSession queue so WMS SysEx7 fragments do not burn
    // HostOutboundQueue message slots the way a single teVirtualMIDI callback would not.
    std::vector<std::vector<uint8_t>> completeMessages;
    {
        std::lock_guard<std::mutex> assembleLock(impl_->hostAssembleMutex);
        impl_->hostAssemblers[outPortIndex].Append(
            midiBytes,
            byteCount,
            [&completeMessages](const uint8_t* complete, std::size_t completeCount) {
                completeMessages.emplace_back(complete, complete + completeCount);
            });
    }
    for (const std::vector<uint8_t>& complete : completeMessages)
    {
        forwardHostToDevice(outPortIndex, complete.data(), complete.size());
    }
}

bool WmsMidiBackend::SendToHost(
    std::size_t inPortIndex,
    const uint8_t* midiBytes,
    std::size_t byteCount,
    std::string& errorOut)
{
    SendToHostValidation args;
    args.portsCreated = portsCreated_ && impl_ != nullptr;
    args.inCount = args.portsCreated ? impl_->inCount : 0;
    args.inPortIndex = inPortIndex;
    args.midiBytes = midiBytes;
    args.byteCount = byteCount;
    args.connection = (args.portsCreated && inPortIndex < args.inCount)
        ? impl_->inSlots[inPortIndex].connection
        : midicore::MidiEndpointConnection{nullptr};
    if (!validateSendToHostArgs(args, errorOut))
    {
        return false;
    }

    try
    {
        std::vector<uint8_t> bytes(midiBytes, midiBytes + byteCount);
        auto words = wms_messages::MidiMessageConverter::ConvertMidi1CompleteMessageBytesToUmpWords(
            midicore::MidiGroup(static_cast<uint8_t>(0)),
            bytes,
            false);
        return sendUmpWords(args.connection, words, errorOut);
    }
    catch (const winrt::hresult_error& ex)
    {
        std::ostringstream stream;
        stream << "WMS SendToHost failed: " << winrt::to_string(ex.message())
               << " (IN port index " << inPortIndex << ")";
        errorOut = stream.str();
        return false;
    }
}

#endif // _WIN32
