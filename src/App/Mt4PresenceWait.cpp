// Bounded wait/rescan for project WinUSB GUID (Auto-Start + hot-plug).

#include "App/Mt4PresenceWait.h"

#include "App/AutoStartRegistration.h"
#include "App/Mt4WinUsbPresence.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace
{
using SteadyClock = std::chrono::steady_clock;

void printPresenceWaitBanner(
    const Mt4PresenceWaitConfig& config,
    const std::string& presenceError)
{
    std::cout << "MT4 not present yet (" << presenceError << "); "
              << config.contextLabel << " waiting up to " << config.timeoutSeconds
              << "s (poll every " << (config.pollIntervalMs / 1000)
              << "s). Plug the MT4 or press Ctrl+C to abort.\n";
}

void printSecondsRemainingProgress(
    const Mt4PresenceWaitConfig& config,
    SteadyClock::time_point deadline,
    SteadyClock::time_point now,
    const char* waitingFor)
{
    const auto remaining =
        std::chrono::duration_cast<std::chrono::seconds>(deadline - now).count();
    std::cout << config.contextLabel << " still waiting for " << waitingFor << " ("
              << remaining << "s remaining)...\n";
}

void printPresentReady(const Mt4PresenceWaitConfig& config, bool wasImmediate)
{
    if (wasImmediate)
    {
        std::cout << config.contextLabel
                  << ": MT4 WinUSB interface present; starting session\n";
        return;
    }
    std::cout << config.contextLabel
              << ": MT4 WinUSB interface appeared; starting session\n";
}

struct PresencePollEval
{
    Mt4WinUsbPresence presence = Mt4WinUsbPresence::Absent;
    const std::string* detail = nullptr;
    SteadyClock::time_point deadline{};
    SteadyClock::time_point now{};
};

bool handlePresentPollResult(
    const Mt4PresenceWaitConfig& config,
    const PresencePollEval& eval,
    bool& doneOut)
{
    doneOut = true;
    if (eval.presence == Mt4WinUsbPresence::Present)
    {
        printPresentReady(config, false);
        return true;
    }
    if (eval.presence == Mt4WinUsbPresence::Error)
    {
        std::cerr << config.contextLabel
                  << " presence check failed: " << *eval.detail << '\n';
        return false;
    }
    if (eval.now >= eval.deadline)
    {
        std::cerr << config.contextLabel << " timed out after " << config.timeoutSeconds
                  << "s waiting for WinUSB GUID "
                  << "{aa209017-cf8a-49ad-a0e7-701187ff7e05}. "
                  << "Last check: " << *eval.detail << '\n';
        return false;
    }
    doneOut = false;
    return false;
}

bool maybePrintProgress(
    const Mt4PresenceWaitConfig& config,
    SteadyClock::time_point deadline,
    SteadyClock::time_point& lastProgress,
    const char* waitingFor)
{
    const auto now = SteadyClock::now();
    if (now - lastProgress < std::chrono::seconds(config.progressIntervalSeconds))
    {
        return false;
    }
    printSecondsRemainingProgress(config, deadline, now, waitingFor);
    lastProgress = now;
    return true;
}

bool pollUntilMt4Present(
    const Mt4PresenceWaitConfig& config,
    const std::atomic<bool>& cancelRequested,
    SteadyClock::time_point deadline)
{
    std::string presenceDetail;
    auto lastProgress = SteadyClock::now();
    while (!cancelRequested.load())
    {
        PresencePollEval eval;
        eval.presence = queryMt4WinUsbPresence(presenceDetail);
        eval.detail = &presenceDetail;
        eval.deadline = deadline;
        eval.now = SteadyClock::now();
        bool done = false;
        const bool ok = handlePresentPollResult(config, eval, done);
        if (done)
        {
            return ok;
        }
        maybePrintProgress(config, deadline, lastProgress, "MT4");
        std::this_thread::sleep_for(std::chrono::milliseconds(config.pollIntervalMs));
    }
    std::cerr << config.contextLabel << " wait cancelled before MT4 appeared\n";
    return false;
}

bool waitUntilMt4Absent(
    const Mt4PresenceWaitConfig& config,
    const std::atomic<bool>& cancelRequested,
    SteadyClock::time_point deadline)
{
    std::string presenceDetail;
    auto lastProgress = SteadyClock::now();
    while (!cancelRequested.load())
    {
        const auto now = SteadyClock::now();
        const Mt4WinUsbPresence presence = queryMt4WinUsbPresence(presenceDetail);
        if (presence == Mt4WinUsbPresence::Absent)
        {
            std::cout << config.contextLabel
                      << ": WinUSB GUID absent; waiting for replug...\n";
            return true;
        }
        if (presence == Mt4WinUsbPresence::Error)
        {
            std::cerr << config.contextLabel
                      << " presence check failed: " << presenceDetail << '\n';
            return false;
        }
        if (now >= deadline)
        {
            std::cerr << config.contextLabel << " timed out after "
                      << config.timeoutSeconds
                      << "s waiting for WinUSB GUID to leave after disconnect. "
                      << "Last check: " << presenceDetail << '\n';
            return false;
        }
        maybePrintProgress(config, deadline, lastProgress, "GUID absence");
        std::this_thread::sleep_for(std::chrono::milliseconds(config.pollIntervalMs));
    }
    std::cerr << config.contextLabel << " wait cancelled before GUID left\n";
    return false;
}

SteadyClock::time_point makeDeadline(const Mt4PresenceWaitConfig& config)
{
    return SteadyClock::now() + std::chrono::seconds(config.timeoutSeconds);
}

Mt4PresenceWaitConfig makePresenceWaitConfig(
    const char* label,
    int timeoutSeconds,
    int pollIntervalMs,
    int progressIntervalSeconds)
{
    Mt4PresenceWaitConfig config;
    config.contextLabel = label;
    config.timeoutSeconds = timeoutSeconds;
    config.pollIntervalMs = pollIntervalMs;
    config.progressIntervalSeconds = progressIntervalSeconds;
    return config;
}
} // namespace

Mt4PresenceWaitConfig makeAutoSessionPresenceWaitConfig()
{
    return makePresenceWaitConfig(
        "Auto-session",
        kAutoSessionWaitTimeoutSeconds,
        kAutoSessionPollIntervalMs,
        kAutoSessionProgressIntervalSeconds);
}

Mt4PresenceWaitConfig makeHotPlugReplugPresenceWaitConfig()
{
    return makePresenceWaitConfig(
        "Hot-plug recovery",
        kHotPlugReplugWaitTimeoutSeconds,
        kHotPlugReplugPollIntervalMs,
        kHotPlugReplugProgressIntervalSeconds);
}

bool waitForMt4WinUsbOrTimeout(
    const Mt4PresenceWaitConfig& config,
    const std::atomic<bool>& cancelRequested)
{
    std::string presenceDetail;
    const Mt4WinUsbPresence presence = queryMt4WinUsbPresence(presenceDetail);
    if (presence == Mt4WinUsbPresence::Present)
    {
        printPresentReady(config, true);
        return true;
    }
    if (presence == Mt4WinUsbPresence::Error)
    {
        std::cerr << config.contextLabel << " presence check failed: " << presenceDetail
                  << '\n';
        return false;
    }
    printPresenceWaitBanner(config, presenceDetail);
    return pollUntilMt4Present(config, cancelRequested, makeDeadline(config));
}

bool waitForMt4WinUsbReplugOrTimeout(
    const Mt4PresenceWaitConfig& config,
    const std::atomic<bool>& cancelRequested)
{
    const auto deadline = makeDeadline(config);
    std::string presenceDetail;
    const Mt4WinUsbPresence initial = queryMt4WinUsbPresence(presenceDetail);
    if (initial == Mt4WinUsbPresence::Error)
    {
        std::cerr << config.contextLabel << " presence check failed: " << presenceDetail
                  << '\n';
        return false;
    }
    if (initial == Mt4WinUsbPresence::Present)
    {
        std::cout << config.contextLabel
                  << ": WinUSB GUID still listed after disconnect; "
                     "waiting for absence before replug...\n";
        if (!waitUntilMt4Absent(config, cancelRequested, deadline))
        {
            return false;
        }
    }
    else
    {
        std::cout << config.contextLabel
                  << ": WinUSB GUID already absent; waiting for replug...\n";
    }
    if (cancelRequested.load())
    {
        std::cerr << config.contextLabel << " wait cancelled before MT4 appeared\n";
        return false;
    }
    printPresenceWaitBanner(config, "awaiting replug");
    return pollUntilMt4Present(config, cancelRequested, deadline);
}
