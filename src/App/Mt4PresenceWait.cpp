// Bounded wait/rescan for project WinUSB GUID (Auto-Start + hot-plug).

#include "App/Mt4PresenceWait.h"

#include "App/Mt4WinUsbPresence.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace
{
void printPresenceWaitBanner(
    const Mt4PresenceWaitConfig& config,
    const std::string& presenceError)
{
    std::cout << "MT4 not present yet (" << presenceError << "); "
              << config.contextLabel << " waiting up to " << config.timeoutSeconds
              << "s (poll every " << (config.pollIntervalMs / 1000)
              << "s). Plug the MT4 or press Ctrl+C to abort.\n";
}

void printPresenceWaitProgress(
    const Mt4PresenceWaitConfig& config,
    std::chrono::steady_clock::time_point deadline,
    std::chrono::steady_clock::time_point now)
{
    const auto remaining =
        std::chrono::duration_cast<std::chrono::seconds>(deadline - now).count();
    std::cout << config.contextLabel << " still waiting for MT4 (" << remaining
              << "s remaining)...\n";
}

struct PresencePollEval
{
    Mt4WinUsbPresence presence = Mt4WinUsbPresence::Absent;
    const std::string* detail = nullptr;
    std::chrono::steady_clock::time_point deadline{};
    std::chrono::steady_clock::time_point now{};
};

bool handlePresencePollResult(
    const Mt4PresenceWaitConfig& config,
    const PresencePollEval& eval,
    bool& doneOut)
{
    doneOut = true;
    if (eval.presence == Mt4WinUsbPresence::Present)
    {
        std::cout << "MT4 WinUSB interface appeared; starting session\n";
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

bool pollUntilMt4Present(
    const Mt4PresenceWaitConfig& config,
    const std::atomic<bool>& cancelRequested,
    std::chrono::steady_clock::time_point deadline)
{
    std::string presenceDetail;
    auto lastProgress = std::chrono::steady_clock::now();
    while (!cancelRequested.load())
    {
        PresencePollEval eval;
        eval.presence = queryMt4WinUsbPresence(presenceDetail);
        eval.detail = &presenceDetail;
        eval.deadline = deadline;
        eval.now = std::chrono::steady_clock::now();
        bool done = false;
        const bool ok = handlePresencePollResult(config, eval, done);
        if (done)
        {
            return ok;
        }
        if (eval.now - lastProgress
            >= std::chrono::seconds(config.progressIntervalSeconds))
        {
            printPresenceWaitProgress(config, deadline, eval.now);
            lastProgress = eval.now;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(config.pollIntervalMs));
    }
    std::cerr << config.contextLabel << " wait cancelled before MT4 appeared\n";
    return false;
}
} // namespace

bool waitForMt4WinUsbOrTimeout(
    const Mt4PresenceWaitConfig& config,
    const std::atomic<bool>& cancelRequested)
{
    std::string presenceDetail;
    const Mt4WinUsbPresence presence = queryMt4WinUsbPresence(presenceDetail);
    if (presence == Mt4WinUsbPresence::Present)
    {
        std::cout << "MT4 WinUSB interface present; starting session\n";
        return true;
    }
    if (presence == Mt4WinUsbPresence::Error)
    {
        std::cerr << config.contextLabel << " presence check failed: " << presenceDetail
                  << '\n';
        return false;
    }
    printPresenceWaitBanner(config, presenceDetail);
    const auto deadline = std::chrono::steady_clock::now()
        + std::chrono::seconds(config.timeoutSeconds);
    return pollUntilMt4Present(config, cancelRequested, deadline);
}
