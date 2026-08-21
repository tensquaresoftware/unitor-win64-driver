// Windows-only helpers for WmsMidiBackend (runtime bootstrap, name checks).

#pragma once

#include "Midi/MidiBackend.h"

#include <string>

#ifdef _WIN32

bool ensureWmsRuntimeReady(std::string& errorOut);
void releaseWmsRuntime() noexcept;

bool queryWmsTransportAvailable(std::string& errorOut);
bool validateWmsPortNameSet(const PortNameSet& names, std::string& errorOut);

std::wstring makeWmsProductInstanceId(
    const std::wstring& wideName,
    const std::wstring& instanceSuffix);

std::wstring utf8ToWideWms(const std::string& utf8, std::string& errorOut);

#endif // _WIN32
