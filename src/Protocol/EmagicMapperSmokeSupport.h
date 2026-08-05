// Shared synthetic EmagicCableMapper encode/decode checks (CLI smoke + Catch2).

#pragma once

#include "Profile/DeviceProfile.h"

#include <ostream>

// Returns false and writes an English diagnostic to err on the first failure.
bool runEmagicMapperSmokeEncodeOutCables(const DeviceProfile& profile, std::ostream& err);
bool runEmagicMapperSmokeEncodeControlChange(const DeviceProfile& profile, std::ostream& err);
bool runEmagicMapperSmokeDecodeSynthetic(const DeviceProfile& profile, std::ostream& err);
bool runEmagicMapperSmokeDecodeControlChange(const DeviceProfile& profile, std::ostream& err);
bool runEmagicMapperSmokeDecodeSplitF5(const DeviceProfile& profile, std::ostream& err);
bool runEmagicMapperSmokeEncodeClockTransport(const DeviceProfile& profile, std::ostream& err);
bool runEmagicMapperSmokeDecodeClockTransport(const DeviceProfile& profile, std::ostream& err);
bool runEmagicMapperSmokeEncodeMtc(const DeviceProfile& profile, std::ostream& err);
bool runEmagicMapperSmokeDecodeMtc(const DeviceProfile& profile, std::ostream& err);
bool runEmagicMapperSmokeEncodeSysex(const DeviceProfile& profile, std::ostream& err);
bool runEmagicMapperSmokeDecodeSysex(const DeviceProfile& profile, std::ostream& err);

// Runs the synthetic checks above; prints a success line to out on pass.
bool runAllEmagicMapperSmokeTests(std::ostream& out, std::ostream& err);
