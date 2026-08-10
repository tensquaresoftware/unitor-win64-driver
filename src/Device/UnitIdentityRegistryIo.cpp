#include "Device/UnitIdentityRegistry.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace
{
constexpr const char* kRegistryFileHeader = "unitor-unit-identity-v1";

const char* kindToken(UnitIdentityKind kind)
{
    return kind == UnitIdentityKind::Serial ? "serial" : "topology";
}

bool parseKindToken(const std::string& token, UnitIdentityKind& kindOut)
{
    if (token == "serial")
    {
        kindOut = UnitIdentityKind::Serial;
        return true;
    }
    if (token == "topology")
    {
        kindOut = UnitIdentityKind::Topology;
        return true;
    }
    return false;
}

bool isAllWhitespace(const std::string& text)
{
    return std::all_of(text.begin(), text.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
}

void trimTrailingCr(std::string& line)
{
    if (!line.empty() && line.back() == '\r')
    {
        line.pop_back();
    }
}

bool parseStrictOrdinal(const std::string& text, unsigned& ordinalOut, std::string& errorOut)
{
    if (text.empty() || !std::isdigit(static_cast<unsigned char>(text[0])))
    {
        errorOut = "UnitIdentityRegistry: invalid ordinal in persistence file";
        return false;
    }
    std::size_t consumed = 0;
    unsigned long value = 0;
    try
    {
        value = std::stoul(text, &consumed, 10);
    }
    catch (...)
    {
        errorOut = "UnitIdentityRegistry: invalid ordinal in persistence file";
        return false;
    }
    if (consumed != text.size() || value < 1UL
        || value > static_cast<unsigned long>(UINT32_MAX))
    {
        errorOut = "UnitIdentityRegistry: invalid ordinal in persistence file";
        return false;
    }
    ordinalOut = static_cast<unsigned>(value);
    return true;
}

bool parseBindingLine(const std::string& line, UnitIdentityBinding& bindingOut, std::string& errorOut)
{
    const auto bar1 = line.find('|');
    const auto bar2 = (bar1 == std::string::npos) ? std::string::npos : line.find('|', bar1 + 1);
    if (bar1 == std::string::npos || bar2 == std::string::npos
        || line.find('|', bar2 + 1) != std::string::npos)
    {
        errorOut = "UnitIdentityRegistry: malformed persistence line";
        return false;
    }
    if (!parseKindToken(line.substr(0, bar1), bindingOut.kind))
    {
        errorOut = "UnitIdentityRegistry: unknown identity kind in file";
        return false;
    }
    bindingOut.key = line.substr(bar1 + 1, bar2 - bar1 - 1);
    if (bindingOut.key.empty() || bindingOut.key.find('|') != std::string::npos)
    {
        errorOut = "UnitIdentityRegistry: malformed persistence line";
        return false;
    }
    return parseStrictOrdinal(line.substr(bar2 + 1), bindingOut.unitOrdinalK, errorOut);
}

bool writeRegistryToPath(
    const std::string& path,
    const std::vector<UnitIdentityBinding>& bindings,
    std::string& errorOut)
{
    std::ofstream output(path, std::ios::trunc);
    if (!output)
    {
        errorOut = "UnitIdentityRegistry: failed to open persistence file for write";
        return false;
    }
    output << kRegistryFileHeader << '\n';
    for (const UnitIdentityBinding& binding : bindings)
    {
        output << kindToken(binding.kind) << '|' << binding.key << '|'
               << binding.unitOrdinalK << '\n';
    }
    if (!output)
    {
        errorOut = "UnitIdentityRegistry: failed while writing persistence file";
        return false;
    }
    errorOut.clear();
    return true;
}

#ifdef _WIN32
bool replaceFileAtomically(
    const std::string& tempPath,
    const std::string& finalPath,
    std::string& errorOut)
{
    if (MoveFileExA(
            tempPath.c_str(),
            finalPath.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)
        != 0)
    {
        errorOut.clear();
        return true;
    }
    errorOut = "UnitIdentityRegistry: failed to replace persistence file";
    DeleteFileA(tempPath.c_str());
    return false;
}

// Exclusive lock file so two Bridge processes cannot tear the K map.
class ScopedRegistryFileLock
{
public:
    bool acquire(const std::string& registryPath, std::string& errorOut)
    {
        const std::string lockPath = registryPath + ".lock";
        handle_ = CreateFileA(
            lockPath.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (handle_ == INVALID_HANDLE_VALUE)
        {
            errorOut = "UnitIdentityRegistry: failed to acquire persistence lock";
            return false;
        }
        errorOut.clear();
        return true;
    }

    ~ScopedRegistryFileLock()
    {
        if (handle_ != INVALID_HANDLE_VALUE)
        {
            CloseHandle(handle_);
            handle_ = INVALID_HANDLE_VALUE;
        }
    }

    ScopedRegistryFileLock() = default;
    ScopedRegistryFileLock(const ScopedRegistryFileLock&) = delete;
    ScopedRegistryFileLock& operator=(const ScopedRegistryFileLock&) = delete;

private:
    HANDLE handle_ = INVALID_HANDLE_VALUE;
};
#endif

bool loadRegistryBody(const std::string& path, UnitIdentityRegistry& registry, std::string& errorOut)
{
    std::ifstream input(path);
    if (!input)
    {
        registry.clear();
        errorOut.clear();
        return true;
    }

    std::string line;
    if (!std::getline(input, line))
    {
        registry.clear();
        errorOut.clear();
        return true;
    }
    trimTrailingCr(line);
    if (line != kRegistryFileHeader)
    {
        errorOut = "UnitIdentityRegistry: unrecognized persistence header";
        return false;
    }

    std::vector<UnitIdentityBinding> bindings;
    while (std::getline(input, line))
    {
        trimTrailingCr(line);
        if (line.empty() || line[0] == '#' || isAllWhitespace(line))
        {
            continue;
        }
        UnitIdentityBinding binding;
        if (!parseBindingLine(line, binding, errorOut))
        {
            return false;
        }
        bindings.push_back(std::move(binding));
    }
    return registry.replaceAll(bindings, errorOut);
}
} // namespace

bool UnitIdentityRegistry::loadFromFile(const std::string& path, std::string& errorOut)
{
#ifdef _WIN32
    ScopedRegistryFileLock lock;
    if (!lock.acquire(path, errorOut))
    {
        return false;
    }
#endif
    return loadRegistryBody(path, *this, errorOut);
}

bool UnitIdentityRegistry::saveToFile(const std::string& path, std::string& errorOut) const
{
#ifdef _WIN32
    ScopedRegistryFileLock lock;
    if (!lock.acquire(path, errorOut))
    {
        return false;
    }
#endif
    const std::vector<UnitIdentityBinding> bindings = snapshot();
#ifdef _WIN32
    const std::string tempPath = path + ".tmp";
    if (!writeRegistryToPath(tempPath, bindings, errorOut))
    {
        return false;
    }
    return replaceFileAtomically(tempPath, path, errorOut);
#else
    return writeRegistryToPath(path, bindings, errorOut);
#endif
}

std::string UnitIdentityRegistry::defaultPersistencePath(std::string& errorOut)
{
#ifdef _WIN32
    char buffer[MAX_PATH] = {};
    const DWORD length = GetEnvironmentVariableA("LOCALAPPDATA", buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
    {
        errorOut = "UnitIdentityRegistry: LOCALAPPDATA is unavailable";
        return {};
    }
    const std::string dir = std::string(buffer) + "\\unitor-win64-driver";
    if (!CreateDirectoryA(dir.c_str(), nullptr))
    {
        const DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS)
        {
            errorOut = "UnitIdentityRegistry: failed to create persistence directory";
            return {};
        }
    }
    errorOut.clear();
    return dir + "\\unit-identity-registry.txt";
#else
    errorOut = "UnitIdentityRegistry persistence requires Windows";
    return {};
#endif
}
