// Auto-Start registration orchestrator: Task Scheduler first, HKCU Run fallback.

#include "App/AutoStartRegistration.h"

#include "App/AutoStartRunKey.h"
#include "App/AutoStartTaskScheduler.h"

#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

std::string buildAutoStartActionArguments()
{
    return std::string(kAutoSessionFlag);
}

#ifdef _WIN32
namespace
{
bool queryModulePathWide(std::wstring& pathOut, std::string& errorOut)
{
    std::wstring buffer(MAX_PATH, L'\0');
    for (;;)
    {
        const DWORD capacity = static_cast<DWORD>(buffer.size());
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), capacity);
        if (length == 0)
        {
            errorOut = "GetModuleFileNameW failed for Bridge.exe path";
            return false;
        }
        if (length < capacity)
        {
            buffer.resize(length);
            pathOut = std::move(buffer);
            return true;
        }
        // Truncated — grow and retry (long paths / deep trees).
        buffer.assign(buffer.size() * 2, L'\0');
        if (buffer.size() > 32768)
        {
            errorOut = "GetModuleFileNameW path exceeds supported length";
            return false;
        }
    }
}

std::string wideToUtf8(const std::wstring& wide)
{
    if (wide.empty())
    {
        return std::string();
    }
    const int needed = WideCharToMultiByte(
        CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (needed <= 0)
    {
        return std::string();
    }
    std::string utf8(static_cast<std::size_t>(needed), '\0');
    const int written = WideCharToMultiByte(
        CP_UTF8, 0, wide.c_str(), -1, utf8.data(), needed, nullptr, nullptr);
    if (written <= 0)
    {
        return std::string();
    }
    if (!utf8.empty() && utf8.back() == '\0')
    {
        utf8.pop_back();
    }
    return utf8;
}

bool utf8ToWide(const std::string& utf8, std::wstring& wideOut, std::string& errorOut)
{
    if (utf8.empty())
    {
        wideOut.clear();
        return true;
    }
    const int needed = MultiByteToWideChar(
        CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (needed <= 0)
    {
        errorOut = "Failed to convert Auto-Start arguments to wide string";
        return false;
    }
    wideOut.assign(static_cast<std::size_t>(needed), L'\0');
    const int written = MultiByteToWideChar(
        CP_UTF8, 0, utf8.c_str(), -1, wideOut.data(), needed);
    if (written <= 0)
    {
        errorOut = "Failed to convert Auto-Start arguments to wide string";
        return false;
    }
    if (!wideOut.empty() && wideOut.back() == L'\0')
    {
        wideOut.pop_back();
    }
    return true;
}

bool tryTaskSchedulerThenRunKey(
    const std::wstring& exeWide,
    std::string& messageOut,
    std::string& errorOut)
{
    std::string schedulerError;
    if (registerAutoStartTaskScheduler(exeWide, messageOut, schedulerError))
    {
        // Clear any prior Run fallback so logon cannot start two Bridges.
        std::string runMessage;
        std::string runError;
        if (!unregisterAutoStartRunKey(runMessage, runError))
        {
            errorOut = "Task Scheduler registered but clearing HKCU Run failed ("
                + runError + ")";
            return false;
        }
        return true;
    }

    const std::string argsUtf8 = buildAutoStartActionArguments();
    std::wstring argsWide;
    if (!utf8ToWide(argsUtf8, argsWide, errorOut))
    {
        return false;
    }

    std::string runMessage;
    std::string runError;
    const AutoStartRunKeyCommand command{&exeWide, &argsWide};
    if (!registerAutoStartRunKey(command, runMessage, runError))
    {
        errorOut = "Task Scheduler failed (" + schedulerError + "); HKCU Run failed ("
            + runError + ")";
        return false;
    }

    messageOut = runMessage + " [fallback after Task Scheduler: " + schedulerError + "]";
    return true;
}
} // namespace
#endif

bool resolveBridgeExecutablePath(std::string& pathOut, std::string& errorOut)
{
#ifdef _WIN32
    std::wstring wide;
    if (!queryModulePathWide(wide, errorOut))
    {
        return false;
    }
    pathOut = wideToUtf8(wide);
    if (pathOut.empty())
    {
        errorOut = "Failed to convert Bridge.exe path to UTF-8";
        return false;
    }
    return true;
#else
    (void)pathOut;
    errorOut = "Auto-Start path resolution requires Windows";
    return false;
#endif
}

bool registerAutoStart(std::string& messageOut, std::string& errorOut)
{
#ifdef _WIN32
    std::wstring exeWide;
    if (!queryModulePathWide(exeWide, errorOut))
    {
        return false;
    }
    return tryTaskSchedulerThenRunKey(exeWide, messageOut, errorOut);
#else
    (void)messageOut;
    errorOut = "Auto-Start registration requires Windows";
    return false;
#endif
}

bool unregisterAutoStart(std::string& messageOut, std::string& errorOut)
{
#ifdef _WIN32
    std::string schedulerMessage;
    std::string schedulerError;
    const bool schedulerOk =
        unregisterAutoStartTaskScheduler(schedulerMessage, schedulerError);

    std::string runMessage;
    std::string runError;
    const bool runOk = unregisterAutoStartRunKey(runMessage, runError);

    if (!schedulerOk || !runOk)
    {
        errorOut = "Unregister incomplete: Task Scheduler ("
            + (schedulerOk ? std::string("ok") : schedulerError) + "); HKCU Run ("
            + (runOk ? std::string("ok") : runError) + ")";
        return false;
    }

    messageOut = schedulerMessage + " | " + runMessage;
    return true;
#else
    (void)messageOut;
    errorOut = "Auto-Start unregistration requires Windows";
    return false;
#endif
}
