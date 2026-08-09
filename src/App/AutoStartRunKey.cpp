// HKCU\...\Run registration — user-session, no elevation (AD-20 alternate).

#include "App/AutoStartRunKey.h"

#include "App/AutoStartRegistration.h"
#include "App/AutoStartWinUtil.h"

#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#ifdef _WIN32
namespace
{
bool buildQuotedCommand(
    const std::wstring& exeWide,
    const std::wstring& argsWide,
    std::wstring& commandOut,
    std::string& errorOut)
{
    if (exeWide.find(L'"') != std::wstring::npos)
    {
        errorOut = "Bridge.exe path contains a double-quote; cannot write HKCU Run";
        return false;
    }
    commandOut = L"\"";
    commandOut += exeWide;
    commandOut += L"\" ";
    commandOut += argsWide;
    return true;
}

bool openRunKey(HKEY* keyOut, std::string& errorOut)
{
    const LONG status = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0,
        KEY_SET_VALUE | KEY_QUERY_VALUE,
        keyOut);
    if (status != ERROR_SUCCESS)
    {
        errorOut = "Failed to open HKCU Run key for Auto-Start";
        return false;
    }
    return true;
}

bool writeRunValue(HKEY key, const std::wstring& line, std::string& errorOut)
{
    const LONG status = RegSetValueExW(
        key,
        L"UnitorMt4BridgeAutoStart",
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(line.c_str()),
        static_cast<DWORD>((line.size() + 1) * sizeof(wchar_t)));
    if (status != ERROR_SUCCESS)
    {
        errorOut = "Failed to write HKCU Run value for Auto-Start";
        return false;
    }
    return true;
}
} // namespace
#endif

bool registerAutoStartRunKey(
    const AutoStartRunKeyCommand& command,
    std::string& messageOut,
    std::string& errorOut)
{
#ifdef _WIN32
    if (command.exeWide == nullptr || command.argsWide == nullptr)
    {
        errorOut = "Internal error: Auto-Start Run-key command missing path";
        return false;
    }

    std::wstring line;
    if (!buildQuotedCommand(*command.exeWide, *command.argsWide, line, errorOut))
    {
        return false;
    }

    HKEY key = nullptr;
    if (!openRunKey(&key, errorOut))
    {
        return false;
    }
    const bool written = writeRunValue(key, line, errorOut);
    RegCloseKey(key);
    if (!written)
    {
        return false;
    }

    messageOut = std::string("Auto-Start registered via HKCU Run. value=")
        + kAutoStartRunValueName + " command=" + autoStartWideToUtf8(line);
    return true;
#else
    (void)command;
    (void)messageOut;
    errorOut = "Auto-Start Run-key registration requires Windows";
    return false;
#endif
}

bool unregisterAutoStartRunKey(std::string& messageOut, std::string& errorOut)
{
#ifdef _WIN32
    HKEY key = nullptr;
    if (!openRunKey(&key, errorOut))
    {
        return false;
    }

    const LONG status = RegDeleteValueW(key, L"UnitorMt4BridgeAutoStart");
    RegCloseKey(key);
    if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND)
    {
        errorOut = "Failed to delete HKCU Run value for Auto-Start";
        return false;
    }

    messageOut = std::string("Auto-Start unregistered (HKCU Run). value=")
        + kAutoStartRunValueName;
    return true;
#else
    (void)messageOut;
    errorOut = "Auto-Start Run-key unregistration requires Windows";
    return false;
#endif
}
