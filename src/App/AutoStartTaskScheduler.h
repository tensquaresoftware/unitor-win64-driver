// Task Scheduler backend for Bridge Auto-Start (logon, interactive, limited).

#pragma once

#include <string>

bool registerAutoStartTaskScheduler(
    const std::wstring& exeWide,
    std::string& messageOut,
    std::string& errorOut);

bool unregisterAutoStartTaskScheduler(std::string& messageOut, std::string& errorOut);
