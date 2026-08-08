# Review Findings — Epic 2 Group A (transport / short SysEx Matrix)

**Date:** 2026-08-08  
**Scope:** `4814f82^`..`HEAD` DeviceSession host/device/bulk-in/support/queue  
**Diff:** `_cr-epic2-group-a.diff`  
**Layers:** Blind Hunter, Edge Case Hunter, Acceptance Auditor  
**Symptom target:** overnight bank ~84%, typical `TIMEOUT last=none` (99/100)

## Decision-needed

- [x] [Review][Defer] Idle-finalize synthesizes trailing F7 for 274/350-byte holds — deferred (A3): Reporté pour ne pas bloquer les correctifs overnight 99/100 (expect/flush, retry, abandon).
- [x] [Review][Defer] Post-Start IN calm timeout still opens librarian OUT — deferred (B3): Reporté pour ne pas bloquer les correctifs overnight 99/100 (expect/flush, retry, abandon).

## Patch

- [x] [Review][Patch] Phantom expect: arm expect after flushDeferred so early dump reply clears nothing then opens 3.5s gate [DeviceSessionHostOutbound.cpp:writeHostOutboundItem] — fixed: arm before flush
- [x] [Review][Patch] Nested dump retry clears deferredHostSends_ mid-OUT [DeviceSessionHostOutbound.cpp:rewriteLastDumpRequestLocked] — fixed: no clear; moved to DeviceSessionMatrixDump.cpp
- [x] [Review][Patch] Abandon partial hold during expect without dump retry → TIMEOUT last=none [DeviceSessionBulkInDeliver.cpp:abandonIdlePartialSysexHoldUnlocked] — fixed: reject/retry
- [x] [Review][Patch] Second wrong-size Matrix dump calls recordPumpFailure and kills session [DeviceSessionHostOutbound.cpp:rejectShortMatrixDumpAndRetry] — fixed: clear expect, session continues
- [x] [Review][Patch] Opportunistic host-callback drain can WriteBulk/SendToHost off reader thread [DeviceSessionHostOutbound.cpp:handleHostMidi/drainHostOutbound] — fixed: signal only
- [x] [Review][Patch] Expect window expiry leaves sticky lastDumpRequest_ / until timestamp [DeviceSessionHostOutbound.cpp:hostOutboundWriteBlocked] — fixed: clearExpectInBurstIfExpired
- [x] [Review][Patch] Unbounded deferredHostSends_ during long OUT [DeviceSession.h / flushDeferredHostSends] — fixed: kMaxDeferredHostSends=64
- [x] [Review][Patch] Leading-F0 repair arms on any 0x10 head while expect active [DeviceSessionSupport.cpp:maybePrependLostLeadingF0] — fixed: require 10 06

## Defer

- [x] [Review][Defer] SysEx holds >400 bytes never abandoned — OUT gated forever — deferred (palier-3 / long SysEx)
- [x] [Review][Defer] Removed sync IN capacity / LastReadTimedOut soft-continue — deferred (async ring model)
- [x] [Review][Defer] AddHostOutOk on dump-request retry masks reject rate — deferred (counter hygiene; also removed AddHostOutOk on retry path)
