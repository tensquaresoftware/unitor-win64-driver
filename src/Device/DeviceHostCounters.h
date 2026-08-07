// ASCII device→host counter lines for lab smoke (bulk IN / demux / SendToHost).

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

struct DeviceHostCounterSnapshot
{
    std::uint64_t bulkBytes = 0;
    std::uint64_t demuxSpans = 0;
    std::uint64_t sendOk = 0;
    std::uint64_t sendFail = 0;
    std::uint64_t hostOutOk = 0;
    std::uint64_t inquiryOut = 0;
    std::uint64_t identityReplyIn = 0;
};

class DeviceHostCounters
{
public:
    void Reset() noexcept;
    void AddBulkAndDemux(std::size_t bulkBytes, std::size_t demuxSpans) noexcept;
    void AddSendOk() noexcept;
    void AddSendFail() noexcept;
    void AddHostOutOk() noexcept;
    void AddInquiryOut() noexcept;
    void AddIdentityReplyIn() noexcept;
    DeviceHostCounterSnapshot Snapshot() const noexcept;
    bool ConsumeFirstBulkLog() noexcept;
    static bool ShouldLogSendOk(std::uint64_t sendOk) noexcept;
    static void PrintLine(const DeviceHostCounterSnapshot& snapshot);

private:
    std::atomic<std::uint64_t> bulkBytes_{0};
    std::atomic<std::uint64_t> demuxSpans_{0};
    std::atomic<std::uint64_t> sendOk_{0};
    std::atomic<std::uint64_t> sendFail_{0};
    std::atomic<std::uint64_t> hostOutOk_{0};
    std::atomic<std::uint64_t> inquiryOut_{0};
    std::atomic<std::uint64_t> identityReplyIn_{0};
    std::atomic<bool> firstBulkLogged_{false};
};
