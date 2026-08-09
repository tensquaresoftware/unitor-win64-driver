// Console diagnostics for MT4 MIDI session CLI hosts.

#include "App/MidiSessionDiagnostics.h"

#include <iostream>

void printExpectedPortDiagnostics(const PortNameSet& names)
{
    std::cout << "Expected Virtual Ports: " << names.inCount << " IN / " << names.outCount
              << " OUT\n";
    for (std::size_t index = 0; index < names.inCount; ++index)
    {
        std::cout << "  IN  " << names.inNames[index] << '\n';
    }
    for (std::size_t index = 0; index < names.outCount; ++index)
    {
        std::cout << "  OUT " << names.outNames[index] << '\n';
    }
}

void printDeviceHostCounters(const DeviceHostCounterSnapshot& snapshot)
{
    std::cout << "device-host counters: bulk_in_bytes=" << snapshot.bulkBytes
              << " demux_spans=" << snapshot.demuxSpans
              << " send_ok_msgs=" << snapshot.sendOk
              << " send_fail_msgs=" << snapshot.sendFail
              << " host_out_ok=" << snapshot.hostOutOk
              << " inquiry_out=" << snapshot.inquiryOut
              << " identity_reply_in=" << snapshot.identityReplyIn << '\n'
              << std::flush;
}

bool shouldPrintDeviceHostCounters(
    const DeviceHostCounterSnapshot& previous,
    const DeviceHostCounterSnapshot& current)
{
    if (current.sendFail != previous.sendFail)
    {
        return true;
    }
    if (current.inquiryOut != previous.inquiryOut
        || current.identityReplyIn != previous.identityReplyIn
        || current.hostOutOk != previous.hostOutOk)
    {
        return true;
    }
    if (previous.bulkBytes == 0 && current.bulkBytes > 0)
    {
        return true;
    }
    if (current.sendOk > previous.sendOk)
    {
        if (current.sendOk == 1)
        {
            return true;
        }
        return (current.sendOk / 25) > (previous.sendOk / 25);
    }
    return false;
}
