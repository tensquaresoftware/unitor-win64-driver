#include "Midi/WmsMidiWinSupport.h"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <winrt/base.h>

#include <winmidi/init/Microsoft.Windows.Devices.Midi2.Initialization.hpp>

#include <winrt/Microsoft.Windows.Devices.Midi2.Endpoints.Virtual.h>

#include <mutex>
#include <sstream>

namespace
{
using Microsoft::Windows::Devices::Midi2::Initialization::MidiDesktopAppSdkInitializer;
namespace wms_virtual =
    winrt::Microsoft::Windows::Devices::Midi2::Endpoints::Virtual;

std::mutex g_runtimeMutex;
MidiDesktopAppSdkInitializer* g_initializer = nullptr;
int g_runtimeRefCount = 0;
bool g_apartmentReady = false;

bool isBlankPortName(const std::string& name)
{
    return name.find_first_not_of(" \t\r\n") == std::string::npos;
}

bool namesCollideWithinDirection(const std::string* names, std::size_t count)
{
    for (std::size_t left = 0; left < count; ++left)
    {
        for (std::size_t right = left + 1; right < count; ++right)
        {
            if (names[left] == names[right])
            {
                return true;
            }
        }
    }
    return false;
}

bool namesCollideAcrossDirections(const PortNameSet& names)
{
    for (std::size_t inIndex = 0; inIndex < names.inCount; ++inIndex)
    {
        for (std::size_t outIndex = 0; outIndex < names.outCount; ++outIndex)
        {
            if (names.inNames[inIndex] == names.outNames[outIndex])
            {
                return true;
            }
        }
    }
    return false;
}

bool rejectBlankPortNames(
    const std::string* names,
    std::size_t count,
    const char* blankError,
    std::string& errorOut)
{
    for (std::size_t index = 0; index < count; ++index)
    {
        if (isBlankPortName(names[index]))
        {
            errorOut = blankError;
            return false;
        }
    }
    return true;
}
} // namespace

namespace
{
bool createSdkInitializer(std::string& errorOut)
{
    auto* initializer = new MidiDesktopAppSdkInitializer();
    if (!initializer->InitializeSdkRuntime())
    {
        delete initializer;
        errorOut =
            "Windows MIDI Services App SDK runtime is not installed or "
            "failed to initialize. Install the runtime from "
            "https://aka.ms/MidiServicesLatestSdkRuntimeInstaller "
            "(Win11 WMS community path), or use --midi-backend=virtualmidi "
            "for lab virtualMIDI.";
        return false;
    }
    if (!initializer->EnsureServiceAvailable())
    {
        initializer->ShutdownSdkRuntime();
        delete initializer;
        errorOut =
            "Windows MIDI Services service could not be started "
            "(WmsMidiBackend prerequisite). Confirm WMS is installed on "
            "Windows 11, or use --midi-backend=virtualmidi for lab.";
        return false;
    }
    g_initializer = initializer;
    return true;
}

bool ensureWinrtApartment(std::string& errorOut)
{
    if (g_apartmentReady)
    {
        return true;
    }
    try
    {
        // MTA: MidiSession::Create can stall indefinitely under STA without a message pump.
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        g_apartmentReady = true;
        return true;
    }
    catch (const winrt::hresult_error& ex)
    {
        std::ostringstream stream;
        stream << "Windows MIDI Services WinRT apartment init failed: "
               << winrt::to_string(ex.message());
        errorOut = stream.str();
        return false;
    }
}
} // namespace

bool ensureWmsRuntimeReady(std::string& errorOut)
{
    std::lock_guard<std::mutex> lock(g_runtimeMutex);
    if (!ensureWinrtApartment(errorOut))
    {
        return false;
    }
    if (g_initializer == nullptr && !createSdkInitializer(errorOut))
    {
        return false;
    }
    ++g_runtimeRefCount;
    errorOut.clear();
    return true;
}

void releaseWmsRuntime() noexcept
{
    std::lock_guard<std::mutex> lock(g_runtimeMutex);
    if (g_runtimeRefCount <= 0)
    {
        return;
    }
    --g_runtimeRefCount;
    if (g_runtimeRefCount == 0 && g_initializer != nullptr)
    {
        g_initializer->ShutdownSdkRuntime();
        delete g_initializer;
        g_initializer = nullptr;
    }
}

bool queryWmsTransportAvailable(std::string& errorOut)
{
    // Assumes ensureWmsRuntimeReady already succeeded for this thread/process.
    try
    {
        const bool available = wms_virtual::MidiVirtualDeviceManager::IsTransportAvailable();
        if (!available)
        {
            errorOut =
                "MidiVirtualDeviceManager::IsTransportAvailable returned false";
        }
        else
        {
            errorOut.clear();
        }
        return available;
    }
    catch (const winrt::hresult_error& ex)
    {
        std::ostringstream stream;
        stream << "Windows MIDI Services IsTransportAvailable failed: "
               << winrt::to_string(ex.message());
        errorOut = stream.str();
        return false;
    }
}

bool validateWmsPortNameSet(const PortNameSet& names, std::string& errorOut)
{
    if (names.inCount > kMaxMidiBackendInPorts || names.outCount > kMaxMidiBackendOutPorts)
    {
        errorOut = "WMS CreatePortSet rejected PortNameSet counts above backend caps";
        return false;
    }
    if (names.inCount == 0 && names.outCount == 0)
    {
        errorOut = "WMS CreatePortSet rejected empty PortNameSet (fail closed)";
        return false;
    }
    if (!rejectBlankPortNames(
            names.inNames,
            names.inCount,
            "WMS CreatePortSet rejected blank IN display name",
            errorOut)
        || !rejectBlankPortNames(
            names.outNames,
            names.outCount,
            "WMS CreatePortSet rejected blank OUT display name",
            errorOut))
    {
        return false;
    }
    if (namesCollideWithinDirection(names.inNames, names.inCount)
        || namesCollideWithinDirection(names.outNames, names.outCount)
        || namesCollideAcrossDirections(names))
    {
        errorOut =
            "WMS CreatePortSet rejected non-unique Input/Output display names";
        return false;
    }
    errorOut.clear();
    return true;
}

std::wstring utf8ToWideWms(const std::string& utf8, std::string& errorOut)
{
    if (utf8.empty())
    {
        errorOut = "WMS port name conversion rejected empty UTF-8 name";
        return {};
    }
    const int wideCount = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (wideCount <= 0)
    {
        errorOut = "WMS port name UTF-8 to UTF-16 conversion failed";
        return {};
    }
    std::wstring wide(static_cast<std::size_t>(wideCount), L'\0');
    if (MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            utf8.data(),
            static_cast<int>(utf8.size()),
            wide.data(),
            wideCount)
        <= 0)
    {
        errorOut = "WMS port name UTF-8 to UTF-16 conversion failed";
        return {};
    }
    errorOut.clear();
    return wide;
}

std::wstring makeWmsProductInstanceId(
    const std::wstring& wideName,
    const std::wstring& instanceSuffix)
{
    // ProductInstanceId max length is 32. Always keep '-' + suffix intact so
    // distinct CreatePortSet runs do not collide after name filtering.
    constexpr std::size_t kMaxIdChars = 32;
    constexpr std::size_t kPrefixAndSep = 2; // leading 'U' + '-'
    std::wstring body;
    for (wchar_t ch : wideName)
    {
        if ((ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z')
            || (ch >= L'0' && ch <= L'9'))
        {
            body.push_back(ch);
        }
    }

    std::wstring suffix = instanceSuffix;
    if (kPrefixAndSep + suffix.size() > kMaxIdChars)
    {
        suffix.resize(kMaxIdChars - kPrefixAndSep);
    }
    const std::size_t bodyMax = kMaxIdChars - kPrefixAndSep - suffix.size();
    if (body.size() > bodyMax)
    {
        body.resize(bodyMax);
    }

    std::wstring id = L"U";
    id.append(body);
    id.push_back(L'-');
    id.append(suffix);
    return id;
}

#endif // _WIN32
