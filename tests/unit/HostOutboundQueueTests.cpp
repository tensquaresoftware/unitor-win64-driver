// Catch2 unit tests for HostOutboundQueue (librarian-scale burst buffering).

#include <catch2/catch_test_macros.hpp>

#include "Device/HostOutboundQueue.h"

#include <cstdint>
#include <vector>

namespace
{
std::vector<uint8_t> makePatchShaped(std::size_t totalBytes)
{
    std::vector<uint8_t> frame(totalBytes, 0x20);
    frame[0] = 0xF0;
    frame[1] = 0x10;
    frame[2] = 0x06;
    frame[3] = 0x01;
    frame[totalBytes - 1] = 0xF7;
    return frame;
}
} // namespace

TEST_CASE("host outbound queue holds ~100 librarian-sized frames", "[queue][sysex]")
{
    HostOutboundQueue queue;
    const std::vector<uint8_t> frame = makePatchShaped(275);

    for (std::size_t index = 0; index < 100; ++index)
    {
        REQUIRE(queue.TryPush(0, frame.data(), frame.size()));
    }
    REQUIRE(queue.MessageCount() == 100);
    REQUIRE(queue.QueuedBytes() == 100 * 275);

    HostOutboundItem item;
    std::size_t popped = 0;
    while (queue.TryPop(item))
    {
        REQUIRE(item.midi == frame);
        ++popped;
    }
    REQUIRE(popped == 100);
    REQUIRE(queue.IsEmpty());
}

TEST_CASE("host outbound queue overflow is a visible failure", "[queue][sysex]")
{
    HostOutboundQueue queue;
    const std::vector<uint8_t> frame = makePatchShaped(275);

    for (std::size_t index = 0; index < HostOutboundQueue::kMaxMessages; ++index)
    {
        REQUIRE(queue.TryPush(1, frame.data(), frame.size()));
    }
    REQUIRE_FALSE(queue.TryPush(1, frame.data(), frame.size()));
    REQUIRE(queue.MessageCount() == HostOutboundQueue::kMaxMessages);
}

TEST_CASE("host outbound queue byte-cap overflow is a visible failure", "[queue][sysex]")
{
    HostOutboundQueue queue;
    std::vector<uint8_t> large(HostOutboundQueue::kMaxQueuedBytes - 2, 0x20);
    large[0] = 0xF0;
    large.back() = 0xF7;
    REQUIRE(queue.TryPush(0, large.data(), large.size()));

    const uint8_t extra[] = {0xF0, 0x7E, 0xF7};
    REQUIRE_FALSE(queue.TryPush(0, extra, sizeof(extra)));
    REQUIRE(queue.MessageCount() == 1);
    REQUIRE(queue.QueuedBytes() == large.size());

    std::vector<uint8_t> tooLargeAlone(HostOutboundQueue::kMaxQueuedBytes + 1, 0x21);
    tooLargeAlone[0] = 0xF0;
    tooLargeAlone.back() = 0xF7;
    HostOutboundQueue empty;
    REQUIRE_FALSE(empty.TryPush(0, tooLargeAlone.data(), tooLargeAlone.size()));
    REQUIRE(empty.IsEmpty());
}

TEST_CASE("host outbound queue rejects empty push", "[queue][sysex]")
{
    HostOutboundQueue queue;
    const uint8_t byte = 0xF8;
    REQUIRE_FALSE(queue.TryPush(0, nullptr, 1));
    REQUIRE_FALSE(queue.TryPush(0, &byte, 0));
    REQUIRE(queue.IsEmpty());
}
