// WinMM open/send/observe against Bridge Virtual Ports (multi-client peer).

#pragma once

#include "QpcClock.h"

#include <cstdint>
#include <string>

class WinMmMidiIo
{
public:
    WinMmMidiIo() = default;
    ~WinMmMidiIo();

    WinMmMidiIo(const WinMmMidiIo&) = delete;
    WinMmMidiIo& operator=(const WinMmMidiIo&) = delete;

    bool openPorts(
        const std::string& outNeedle,
        const std::string& inNeedle,
        std::string& errorOut);
    void closePorts() noexcept;

    // Arm expected Note On; then inject with QPC stamp immediately before send.
    bool armExpectedNote(std::uint8_t note, std::uint8_t velocity) noexcept;
    bool injectNoteOn(const QpcClock& clock, std::uint8_t note, std::uint8_t velocity);
    // Wait until matching observe or timeoutMs. Fills observeTicks on success.
    bool waitForObserve(std::int64_t& observeTicksOut, unsigned timeoutMs) noexcept;
    std::int64_t lastInjectTicks() const noexcept { return lastInjectTicks_; }

    const std::string& openedOutName() const noexcept { return openedOutName_; }
    const std::string& openedInName() const noexcept { return openedInName_; }

    // Invoked from WinMM midiIn CALLBACK_WINDOW handler (message-only HWND).
    void onMidiInShort(std::uint32_t packed) noexcept;

    // Drop pending MIM_DATA before arming the next expected Note On.
    void drainPendingInput() noexcept;

private:
    void* outHandle_ = nullptr; // HMIDIOUT
    void* inHandle_ = nullptr;  // HMIDIIN
    void* messageHwnd_ = nullptr; // HWND message-only window for CALLBACK_WINDOW
    std::string openedOutName_;
    std::string openedInName_;
    std::int64_t lastInjectTicks_ = 0;

    // Shared with midiIn callback (no heap on observe path).
    volatile std::uint8_t expectNote_ = 0;
    volatile std::uint8_t expectVel_ = 0;
    volatile std::int64_t observeTicks_ = 0;
    volatile int observeReady_ = 0;
    volatile int expectArmed_ = 0;
};
