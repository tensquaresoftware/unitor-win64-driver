// Bounded host→device MIDI queue for librarian-scale SysEx bursts (AD-18).

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

struct HostOutboundItem
{
    std::size_t outPortIndex = 0;
    std::vector<uint8_t> midi;
};

class HostOutboundQueue
{
public:
    // Story 6.1 WMS prerequisite: librarian bursts + concurrent long DIN fixtures
    // (~14708 B) while Emagic USB OUT still drains prior frames (WMS + virtualMIDI).
    // Raised from 128 / (128*512) so fragmented WMS SysEx does not trip the queue early.
    static constexpr std::size_t kMaxMessages = 256;
    static constexpr std::size_t kMaxQueuedBytes = 8 * 16384;

    bool TryPush(std::size_t outPortIndex, const uint8_t* midi, std::size_t byteCount);
    bool TryCopyFront(HostOutboundItem& out) const;
    bool TryPop(HostOutboundItem& out);
    void Clear() noexcept;
    bool IsEmpty() const noexcept;
    std::size_t MessageCount() const noexcept;
    std::size_t QueuedBytes() const noexcept;

private:
    std::deque<HostOutboundItem> items_;
    std::size_t queuedBytes_ = 0;
};
