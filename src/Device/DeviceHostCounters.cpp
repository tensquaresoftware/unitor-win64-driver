#include "Device/DeviceHostCounters.h"

#include <iostream>

void DeviceHostCounters::Reset() noexcept
{
    bulkBytes_.store(0);
    demuxSpans_.store(0);
    sendOk_.store(0);
    sendFail_.store(0);
    firstBulkLogged_.store(false);
}

void DeviceHostCounters::AddBulkAndDemux(
    std::size_t bulkBytes,
    std::size_t demuxSpans) noexcept
{
    if (bulkBytes > 0)
    {
        bulkBytes_.fetch_add(bulkBytes);
    }
    if (demuxSpans > 0)
    {
        demuxSpans_.fetch_add(demuxSpans);
    }
}

void DeviceHostCounters::AddSendOk() noexcept
{
    sendOk_.fetch_add(1);
}

void DeviceHostCounters::AddSendFail() noexcept
{
    sendFail_.fetch_add(1);
}

DeviceHostCounterSnapshot DeviceHostCounters::Snapshot() const noexcept
{
    DeviceHostCounterSnapshot snapshot;
    snapshot.bulkBytes = bulkBytes_.load();
    snapshot.demuxSpans = demuxSpans_.load();
    snapshot.sendOk = sendOk_.load();
    snapshot.sendFail = sendFail_.load();
    return snapshot;
}

bool DeviceHostCounters::ConsumeFirstBulkLog() noexcept
{
    return !firstBulkLogged_.exchange(true);
}

bool DeviceHostCounters::ShouldLogSendOk(std::uint64_t sendOk) noexcept
{
    return sendOk == 1 || (sendOk % 25) == 0;
}

void DeviceHostCounters::PrintLine(const DeviceHostCounterSnapshot& snapshot)
{
    // cerr + flush: reader-thread cout can stay invisible in PowerShell until exit.
    std::cerr << "device-host counters: bulk_in_bytes=" << snapshot.bulkBytes
              << " demux_spans=" << snapshot.demuxSpans
              << " send_ok_msgs=" << snapshot.sendOk
              << " send_fail_msgs=" << snapshot.sendFail << '\n'
              << std::flush;
}
