// HKCU Run-key backend for Bridge Auto-Start (allowed alternate under AD-20).

#pragma once

#include <string>

inline constexpr const char* kAutoStartRunValueName = "UnitorMt4BridgeAutoStart";

struct AutoStartRunKeyCommand
{
    const std::wstring* exeWide = nullptr;
    const std::wstring* argsWide = nullptr;
};

bool registerAutoStartRunKey(
    const AutoStartRunKeyCommand& command,
    std::string& messageOut,
    std::string& errorOut);

bool unregisterAutoStartRunKey(std::string& messageOut, std::string& errorOut);
