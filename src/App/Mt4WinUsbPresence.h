// Present-check and multi-unit enumeration for the project WinUSB DeviceInterfaceGUID.

#pragma once

#include "Device/UnitIdentityRegistry.h"

#include <cstddef>
#include <string>
#include <vector>

enum class Mt4WinUsbPresence
{
    Present,
    Absent,
    Error
};

// One live MT4 WinUSB interface (no open claim).
struct Mt4WinUsbInterfaceInfo
{
    std::string devicePathUtf8;
    std::string identityKey;
    UnitIdentityKind identityKind = UnitIdentityKind::Topology;
    // Topology/instance key for AD-6 serial↔topology migration (may equal identityKey).
    std::string topologyKey;
};

// Present = at least one matching interface; Absent = none; Error = API failure.
// detailOut holds English diagnostics for Absent/Error (and may be empty for Present).
Mt4WinUsbPresence queryMt4WinUsbPresence(std::string& detailOut);

// List all present MT4 WinUSB interfaces (paths + identity keys). Empty list = Absent.
// Returns false on API failure (errorOut set); true with empty list means Absent.
bool listMt4WinUsbInterfaces(
    std::vector<Mt4WinUsbInterfaceInfo>& interfacesOut,
    std::string& errorOut);
