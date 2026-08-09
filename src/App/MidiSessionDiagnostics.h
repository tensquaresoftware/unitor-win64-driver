// Console diagnostics for MT4 MIDI session CLI hosts.

#pragma once

#include "Device/DeviceHostCounters.h"
#include "Device/DeviceSessionManager.h"

void printExpectedPortDiagnostics(const PortNameSet& names);
void printDeviceHostCounters(const DeviceHostCounterSnapshot& snapshot);
bool shouldPrintDeviceHostCounters(
    const DeviceHostCounterSnapshot& previous,
    const DeviceHostCounterSnapshot& current);
