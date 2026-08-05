#include "Device/HostOutboundQueue.h"

bool HostOutboundQueue::TryPush(
    std::size_t outPortIndex,
    const uint8_t* midi,
    std::size_t byteCount)
{
    if (midi == nullptr || byteCount == 0)
    {
        return false;
    }
    // Guard wrap: byteCount alone, then remaining capacity (not queuedBytes_ + byteCount).
    if (items_.size() >= kMaxMessages || byteCount > kMaxQueuedBytes
        || byteCount > (kMaxQueuedBytes - queuedBytes_))
    {
        return false;
    }

    HostOutboundItem item;
    item.outPortIndex = outPortIndex;
    item.midi.assign(midi, midi + byteCount);
    queuedBytes_ += byteCount;
    items_.push_back(std::move(item));
    return true;
}

bool HostOutboundQueue::TryCopyFront(HostOutboundItem& out) const
{
    if (items_.empty())
    {
        return false;
    }
    out = items_.front();
    return true;
}

bool HostOutboundQueue::TryPop(HostOutboundItem& out)
{
    if (items_.empty())
    {
        return false;
    }
    out = std::move(items_.front());
    items_.pop_front();
    queuedBytes_ -= out.midi.size();
    return true;
}

void HostOutboundQueue::Clear() noexcept
{
    items_.clear();
    queuedBytes_ = 0;
}

bool HostOutboundQueue::IsEmpty() const noexcept
{
    return items_.empty();
}

std::size_t HostOutboundQueue::MessageCount() const noexcept
{
    return items_.size();
}

std::size_t HostOutboundQueue::QueuedBytes() const noexcept
{
    return queuedBytes_;
}
