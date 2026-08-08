// Matrix dump expect window, size-reject retry, and sticky-state expiry.

#include "Device/DeviceSession.h"
#include "Device/DeviceSessionSupport.h"

#include <chrono>
#include <iostream>
#include <string>

namespace
{
struct MatrixDumpRewriteGuard
{
    bool& flag;
    explicit MatrixDumpRewriteGuard(bool& inProgress) noexcept
        : flag(inProgress)
    {
        flag = true;
    }
    ~MatrixDumpRewriteGuard()
    {
        flag = false;
    }
    MatrixDumpRewriteGuard(const MatrixDumpRewriteGuard&) = delete;
    MatrixDumpRewriteGuard& operator=(const MatrixDumpRewriteGuard&) = delete;
};
} // namespace

void DeviceSession::clearExpectInBurstIfExpired() noexcept
{
    if (expectInBurstUntil_.time_since_epoch().count() == 0)
    {
        return;
    }
    if (std::chrono::steady_clock::now() < expectInBurstUntil_)
    {
        return;
    }
    clearExpectInBurst();
}

bool DeviceSession::rejectShortMatrixDumpAndRetry(std::size_t gotLength)
{
    std::cerr << "SysEx size reject: len=" << gotLength
              << " (Matrix dump; keeping expect window)\n"
              << std::flush;
    if (matrixDumpRewriteInProgress_)
    {
        // Drop the short frame; do not nest WriteEmagicHostMidi / flush during OUT.
        std::cerr << "SysEx size reject: short during active dump rewrite; "
                     "dropping frame (expect kept)\n"
                  << std::flush;
        return true;
    }
    if (dumpRequestRetryRemaining_ == 0 || lastDumpRequest_.empty())
    {
        // Fail this frame only — do not tear down the Bridge mid bank-burst.
        clearExpectInBurst();
        std::cerr << "SysEx size reject: no retry left (got_len=" << gotLength
                  << "); expect cleared, session continues\n"
                  << std::flush;
        return false;
    }
    --dumpRequestRetryRemaining_;
    const unsigned retriesLeftAfterRewrite = dumpRequestRetryRemaining_;
    if (!rewriteLastDumpRequestLocked(retriesLeftAfterRewrite))
    {
        clearExpectInBurst();
        recordPumpFailure(
            "Matrix dump reply rejected (short SysEx; dump-request retry failed) got_len="
            + std::to_string(gotLength));
        return false;
    }
    // Exact dump may already have cleared expect during rewrite's flush.
    if (lastDumpRequest_.empty())
    {
        return true;
    }
    expectInBurstUntil_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(3500);
    std::cerr << "SysEx size reject: re-sent dump request (" << lastDumpRequest_.size()
              << " B) retries_left=" << dumpRequestRetryRemaining_ << "\n"
              << std::flush;
    return true;
}

bool DeviceSession::rewriteLastDumpRequestLocked(unsigned retriesLeftAfterRewrite)
{
    MatrixDumpRewriteGuard rewriteGuard(matrixDumpRewriteInProgress_);
    uint8_t encodeBytes[64] = {};
    HostEncodeScratch scratch{encodeBytes, sizeof(encodeBytes), 0, 0};
    if (!encodeHostMidiLocked(
            lastDumpOutPort_, lastDumpRequest_.data(), lastDumpRequest_.size(), scratch))
    {
        return false;
    }
    betweenOutChunkDemuxFailed_ = false;
    // Keep deferredHostSends_ — dump body may already be framed for post-OUT flush.
    deferHostSendDuringOut_ = true;
    std::string error;
    const WinUsbTransport::EmagicBetweenChunks between{
        &DeviceSession::betweenOutChunksDrainIn, this, &betweenOutChunkDemuxFailed_};
    const bool wrote =
        transport_.WriteEmagicHostMidi(scratch.bytes, scratch.size, error, &between);
    if (wrote && !betweenOutChunkDemuxFailed_)
    {
        armExpectInBurstAfterHostSysex(
            lastDumpOutPort_, lastDumpRequest_.data(), lastDumpRequest_.size());
        // armExpect resets the budget; restore remaining before flush can see shorts.
        dumpRequestRetryRemaining_ = retriesLeftAfterRewrite;
    }
    flushDeferredHostSends();
    if (!wrote)
    {
        std::cerr << "SysEx size reject: dump-request retry WriteBulk failed: " << error
                  << "\n"
                  << std::flush;
        return false;
    }
    if (betweenOutChunkDemuxFailed_)
    {
        std::cerr << "SysEx size reject: dump-request retry IN demux failed\n" << std::flush;
        return false;
    }
    return true;
}
