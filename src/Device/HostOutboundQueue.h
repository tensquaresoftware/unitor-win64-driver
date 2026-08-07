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
    // Design target: librarian bursts + long DIN loopback fixture (~14708 B).
    static constexpr std::size_t kMaxMessages = 128;
    static constexpr std::size_t kMaxQueuedBytes = 128 * 512;

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
