// Lightweight present-check for the project WinUSB DeviceInterfaceGUID (no open claim).

#pragma once

#include <string>

enum class Mt4WinUsbPresence
{
    Present,
    Absent,
    Error
};

// Present = at least one matching interface; Absent = none; Error = API failure.
// detailOut holds English diagnostics for Absent/Error (and may be empty for Present).
Mt4WinUsbPresence queryMt4WinUsbPresence(std::string& detailOut);
