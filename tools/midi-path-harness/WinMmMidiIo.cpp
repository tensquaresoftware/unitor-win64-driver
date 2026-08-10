#include "WinMmMidiIo.h"

#include "PortResolve.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>

namespace
{
const wchar_t kMidiInWindowClass[] = L"UnitorMidiPathHarnessMidiIn";

std::int64_t readQpcTicks() noexcept
{
    LARGE_INTEGER counter = {};
    if (QueryPerformanceCounter(&counter) == 0)
    {
        return 0;
    }
    return counter.QuadPart;
}

LRESULT CALLBACK midiInWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == MM_MIM_DATA)
    {
        auto* self = reinterpret_cast<WinMmMidiIo*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (self != nullptr)
        {
            self->onMidiInShort(static_cast<std::uint32_t>(lParam));
        }
        return 0;
    }
    (void)wParam;
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool ensureMidiInWindowClass(std::string& errorOut)
{
    static bool registered = false;
    if (registered)
    {
        return true;
    }
    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = midiInWndProc;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = kMidiInWindowClass;
    if (RegisterClassW(&windowClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        errorOut = "RegisterClassW for MIDI IN window failed";
        return false;
    }
    registered = true;
    return true;
}

bool openOutDevice(UINT outIndex, HMIDIOUT* outHandle, std::string& errorOut)
{
    // Default open — not exclusive; peer with DAW/MIDI-OX (AD-8).
    if (midiOutOpen(outHandle, outIndex, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
    {
        errorOut = "midiOutOpen failed";
        return false;
    }
    return true;
}

bool openInDevice(UINT inIndex, HWND hwnd, HMIDIIN* inHandle, std::string& errorOut)
{
    if (midiInOpen(
            inHandle,
            inIndex,
            reinterpret_cast<DWORD_PTR>(hwnd),
            0,
            CALLBACK_WINDOW)
        != MMSYSERR_NOERROR)
    {
        errorOut = "midiInOpen failed";
        return false;
    }
    if (midiInStart(*inHandle) != MMSYSERR_NOERROR)
    {
        midiInClose(*inHandle);
        *inHandle = nullptr;
        errorOut = "midiInStart failed";
        return false;
    }
    return true;
}

struct OpenDeviceRequest
{
    const ResolvedPorts* resolved = nullptr;
    HWND hwnd = nullptr;
    HMIDIOUT* outHandle = nullptr;
    HMIDIIN* inHandle = nullptr;
};

bool openMatchedDevices(const OpenDeviceRequest& request, std::string& errorOut)
{
    if (!openOutDevice(request.resolved->outIndex, request.outHandle, errorOut))
    {
        return false;
    }
    if (!openInDevice(request.resolved->inIndex, request.hwnd, request.inHandle, errorOut))
    {
        midiOutClose(*request.outHandle);
        *request.outHandle = nullptr;
        return false;
    }
    return true;
}

HWND createMidiInMessageWindow(WinMmMidiIo* owner, std::string& errorOut)
{
    HWND hwnd = CreateWindowExW(
        0,
        kMidiInWindowClass,
        L"",
        0,
        0,
        0,
        0,
        0,
        HWND_MESSAGE,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr);
    if (hwnd == nullptr)
    {
        errorOut = "CreateWindowExW for MIDI IN failed";
        return nullptr;
    }
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(owner));
    return hwnd;
}
} // namespace

WinMmMidiIo::~WinMmMidiIo()
{
    closePorts();
}

void WinMmMidiIo::onMidiInShort(std::uint32_t packed) noexcept
{
    if (expectArmed_ == 0)
    {
        return;
    }
    const auto status = static_cast<std::uint8_t>(packed & 0xFFu);
    const auto data1 = static_cast<std::uint8_t>((packed >> 8) & 0xFFu);
    const auto data2 = static_cast<std::uint8_t>((packed >> 16) & 0xFFu);
    // Exact status 0x90 (Note On channel 1) — not channel-blind.
    if (status != 0x90u || data2 == 0)
    {
        return; // Note On with velocity 0 is Note Off.
    }
    if (data1 != expectNote_ || data2 != expectVel_)
    {
        return;
    }
    observeTicks_ = readQpcTicks();
    observeReady_ = 1;
    expectArmed_ = 0;
}

bool WinMmMidiIo::openPorts(
    const std::string& outNeedle,
    const std::string& inNeedle,
    std::string& errorOut)
{
    closePorts();

    ResolvedPorts resolved;
    if (!resolvePortIndices(outNeedle, inNeedle, resolved, errorOut))
    {
        return false;
    }
    if (!ensureMidiInWindowClass(errorOut))
    {
        return false;
    }

    HWND hwnd = createMidiInMessageWindow(this, errorOut);
    if (hwnd == nullptr)
    {
        return false;
    }

    HMIDIOUT outHandle = nullptr;
    HMIDIIN inHandle = nullptr;
    OpenDeviceRequest request;
    request.resolved = &resolved;
    request.hwnd = hwnd;
    request.outHandle = &outHandle;
    request.inHandle = &inHandle;
    if (!openMatchedDevices(request, errorOut))
    {
        DestroyWindow(hwnd);
        return false;
    }

    outHandle_ = outHandle;
    inHandle_ = inHandle;
    messageHwnd_ = hwnd;
    openedOutName_ = resolved.outName;
    openedInName_ = resolved.inName;
    errorOut.clear();
    return true;
}

void WinMmMidiIo::closePorts() noexcept
{
    if (inHandle_ != nullptr)
    {
        auto* handle = static_cast<HMIDIIN>(inHandle_);
        midiInStop(handle);
        midiInReset(handle);
        midiInClose(handle);
        inHandle_ = nullptr;
    }
    if (outHandle_ != nullptr)
    {
        auto* handle = static_cast<HMIDIOUT>(outHandle_);
        midiOutReset(handle);
        midiOutClose(handle);
        outHandle_ = nullptr;
    }
    if (messageHwnd_ != nullptr)
    {
        DestroyWindow(static_cast<HWND>(messageHwnd_));
        messageHwnd_ = nullptr;
    }
    openedOutName_.clear();
    openedInName_.clear();
}

bool WinMmMidiIo::armExpectedNote(std::uint8_t note, std::uint8_t velocity) noexcept
{
    drainPendingInput();
    observeReady_ = 0;
    expectNote_ = note;
    expectVel_ = velocity;
    expectArmed_ = 1;
    return true;
}

void WinMmMidiIo::drainPendingInput() noexcept
{
    expectArmed_ = 0;
    observeReady_ = 0;
    if (messageHwnd_ == nullptr)
    {
        return;
    }
    MSG msg = {};
    while (PeekMessageW(&msg, static_cast<HWND>(messageHwnd_), 0, 0, PM_REMOVE) != 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    observeReady_ = 0;
}

bool WinMmMidiIo::injectNoteOn(
    const QpcClock& clock,
    std::uint8_t note,
    std::uint8_t velocity)
{
    if (outHandle_ == nullptr)
    {
        return false;
    }
    const DWORD packed = static_cast<DWORD>(0x90u)
        | (static_cast<DWORD>(note) << 8)
        | (static_cast<DWORD>(velocity) << 16);
    lastInjectTicks_ = clock.nowTicks();
    if (lastInjectTicks_ == 0)
    {
        return false; // QPC failure must not become a huge fake latency.
    }
    const MMRESULT result =
        midiOutShortMsg(static_cast<HMIDIOUT>(outHandle_), packed);
    return result == MMSYSERR_NOERROR;
}

bool WinMmMidiIo::sendNoteOff(std::uint8_t note, std::uint8_t velocity) noexcept
{
    if (outHandle_ == nullptr)
    {
        return false;
    }
    const DWORD packed = static_cast<DWORD>(0x80u)
        | (static_cast<DWORD>(note) << 8)
        | (static_cast<DWORD>(velocity) << 16);
    const MMRESULT result =
        midiOutShortMsg(static_cast<HMIDIOUT>(outHandle_), packed);
    return result == MMSYSERR_NOERROR;
}

bool WinMmMidiIo::waitForObserve(
    std::int64_t& observeTicksOut,
    unsigned timeoutMs) noexcept
{
    const DWORD start = GetTickCount();
    while (observeReady_ == 0)
    {
        MSG msg = {};
        while (PeekMessageW(
                   &msg,
                   static_cast<HWND>(messageHwnd_),
                   0,
                   0,
                   PM_REMOVE)
            != 0)
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (observeReady_ != 0)
        {
            break;
        }
        const DWORD elapsed = GetTickCount() - start;
        if (elapsed >= timeoutMs)
        {
            expectArmed_ = 0;
            return false;
        }
        const DWORD remain = timeoutMs - elapsed;
        const DWORD slice = remain < 1u ? remain : 1u;
        // Wake as soon as a midiIn message is posted (avoids blind Sleep bias).
        MsgWaitForMultipleObjects(0, nullptr, FALSE, slice, QS_ALLINPUT);
    }
    observeTicksOut = observeTicks_;
    return true;
}
