// Lab probe: list WinUSB interfaces/pipes and try Emagic init magic on each bulk OUT.
#pragma once

#include <string>

// Opens MT4 via Zadig-style HWID path, prints interfaces, tries init writes.
// Returns 0 if at least one bulk OUT accepted the init magic.
int runMt4UsbBulkProbe(std::string& errorOut);
