// Shared Win32 string/COM helpers for Auto-Start backends.

#pragma once

#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

std::string autoStartFormatHresult(const char* context, HRESULT hr);
std::string autoStartWideToUtf8(const std::wstring& wide);
std::wstring autoStartDirectoryOfExe(const std::wstring& exePath);

struct AutoStartComScope
{
    bool ok = false;
    bool initializedHere = false;

    AutoStartComScope();
    ~AutoStartComScope();
};
#endif
