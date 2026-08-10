// Strip teVirtualMIDI trailing client index suffixes when matching port names.

#pragma once

#include <string>

// "MT4 Out 2 1" -> "MT4 Out 2" when last token is digits.
std::string normalizePortLabel(const std::string& name);

// Ranked match vs needle (0 exact, 1 normalized, 2 trailing index). -1 = no match.
int portMatchRank(const std::string& name, const std::string& needle);
