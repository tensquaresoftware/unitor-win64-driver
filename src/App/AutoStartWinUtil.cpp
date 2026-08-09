// Shared Win32 string/COM helpers for Auto-Start backends.

#include "App/AutoStartWinUtil.h"

#include <cstdio>
#include <string>

#ifdef _WIN32
#include <objbase.h>

std::string autoStartFormatHresult(const char* context, HRESULT hr)
{
    char buffer[96] = {};
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%s (hr=0x%08lX)",
        context,
        static_cast<unsigned long>(hr));
    return std::string(buffer);
}

std::string autoStartWideToUtf8(const std::wstring& wide)
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

std::wstring autoStartDirectoryOfExe(const std::wstring& exePath)
{
    const std::size_t slash = exePath.find_last_of(L"\\/");
    if (slash == std::wstring::npos)
    {
        return std::wstring();
    }
    return exePath.substr(0, slash);
}

AutoStartComScope::AutoStartComScope()
{
    const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
        ok = true;
        initializedHere = true;
        return;
    }
    ok = (hr == RPC_E_CHANGED_MODE);
}

AutoStartComScope::~AutoStartComScope()
{
    if (initializedHere)
    {
        CoUninitialize();
    }
}
#endif
