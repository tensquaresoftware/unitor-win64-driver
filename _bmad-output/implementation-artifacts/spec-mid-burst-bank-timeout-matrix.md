---
title: 'Fix mid-burst Matrix bank TIMEOUT (truncated dump / size-reject exhaust)'
type: 'bugfix'
created: '2026-08-08'
status: 'done'
baseline_commit: '615882d'
review_loop_iteration: 0
context:
  - '{project-root}/docs/dev/prompt-fix-mid-burst-bank-timeout-matrix.md'
  - '{project-root}/_bmad-output/implementation-artifacts/spec-cold-start-premier-dump-matrix.md'
  - '{project-root}/_bmad-output/implementation-artifacts/deferred-work.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** After cold-start F0 repair, mid Matrix dumps pass, but the bank day gate still fails occasionally mid-burst: the lab sees `TIMEOUT last=none` because the Bridge rejects truncated dump replies twice and clears expect without ever delivering a full dump to the host.

**Approach:** Find and fix why mid-burst Matrix dump SysEx arrives short (lengths like 243/211 = 275 − N×32), so a dump under bank load either completes on the first reply or recovers to an exact-length delivery within the expect window — without relaxing the lab gate.

## Boundaries & Constraints

**Always:**
- Keep cold-start 1-byte leading-F0 repair (`615882d`) and short-dump race invariants from `835c992` (expect before flush, deferred retry, abandon→retry, no Write/SendToHost from VirtualMIDI callback, expect expiry, deferred cap).
- Score every dump including mid-burst; bank gate stays `--pass-percent 100`.
- Prefer the smallest Bridge change that makes bank `20×100` / 20 fresh Starts all 100%, with mid `5×10` non-regression.
- English diagnostics only; run `scripts/quality/lint-touched.py` on touched C++.

**Ask First:**
- Raising size-reject retry count as the *primary* fix without addressing truncation.
- Widening lab reply timeout or Bridge expect beyond a tiny secondary tweak.
- Touching framer/clock or USB ring sizing (Epic 2 Group A lots B/C) without new lab proof tying them to the 32-byte short lengths.

**Never:**
- Masking misses in the harness (ignore dump, lower pass-percent, discard mid-burst).
- Overnight 8 h or long SysEx palier-3 as this ticket’s goal.
- Reverting cold-start F0 repair or the rest of `835c992`.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Exact dump mid-burst | Expect armed; framed SysEx len 275 or 351 | `SendToHost`; clear expect | N/A |
| Truncated dump once | Expect armed; Matrix dump reply short (e.g. 243/211) | Do not forward short frame; recover so host still gets exact dump in window | Log size reject; keep session alive |
| Truncation storm | Second short after recovery attempt | Session continues; next dump requests still work | No pump tear-down; English diagnostic |
| Cold-start first dump | Fresh Start; first `dump_patch` | Still receives exact dump (non-regression) | Keep F0 repair path |
| Non-dump SysEx | Expect idle; other SysEx | Unchanged framing / host delivery | Existing reject paths |

</frozen-after-approval>

## Code Map

- `src/Device/DeviceSessionMatrixDump.cpp` -- size-reject + one rewrite; clears expect when retry exhausted (lab `last=none` fingerprint)
- `src/Device/DeviceSessionDeviceHost.cpp` -- `sendFramedToHost` gates short Matrix dumps; `noteSendToHostSuccess` clears on exact 275/351
- `src/Device/DeviceSessionBulkInDeliver.cpp` -- IN demux / hold abandon → size-reject path
- `src/Device/DeviceSessionHostOutbound.cpp` -- arm expect before deferred flush; `dumpRequestRetryRemaining_=1`
- `src/Device/DeviceSessionSupport.cpp` -- `isExactMatrixDumpLength` / F0 repair (keep 1-byte `0x10`)
- `tests/lab-logs/sysex-matrix-bank/post-epic2-cr-coldfix*` -- evidence: dual short reject → expect cleared on miss Starts only
- `scripts/lab/sysex-matrix-bank-loop.py` / `sysex-matrix-mid-loop.py` -- day gates

## Tasks & Acceptance

**Execution:**
- [x] Bridge logs at miss indexes -- correlate `SysEx size reject` lengths (275−N×32) with demux/hold/abandon around the fail slot -- confirm truncation root before coding
- [x] `src/Usb/WinUsbTransport.h` -- deepen always-pending bulk IN ring 16→32 (lab shorts = missing N×32 URBs; comments already pointed at 32) -- stop mid-SysEx URB loss under bank load
- [x] `src/Device/DeviceSessionMatrixDump.cpp` / `DeviceSessionHostOutbound.cpp` / `DeviceSessionSupport.h` -- after ring-only still left 1/20 dual-short miss: allow `kMatrixDumpSizeRejectRetries=2` and restore budget before flush -- secondary recovery so host still gets exact dump
- [x] `tests/unit/DeviceInquiryHelpersTests.cpp` -- exact vs short Matrix dump length edges + retry budget constant -- lock I/O matrix length classification
- [x] Lab mid then bank -- rebuild debug Bridge; mid `5×10` / 5 Starts @ 100%; bank `20×100` / 20 Starts @ 100% (`post-epic2-cr-midburst-retry2`) -- prove gate green without harness masking

**Acceptance Criteria:**
- Given a mid-burst bank Start after cold-start repair, when Matrix issues 100 `dump_patch` requests, then every dump is received at exact length (no `TIMEOUT last=none`).
- Given the previous coldfix bank miss fingerprint (two short rejects then expect cleared), when the fix is in place, then that dual-short→silence path no longer occurs on the day gate (or occurs 0 times across 20 Starts).
- Given mid `5×10` after the same build, when run with `--pass-percent 100 --fresh-starts 5`, then all Starts stay 100% (cold-start non-regression).
- Given a short Matrix dump still appears, when recovery runs, then the Bridge either delivers an exact dump to the host within the expect window or fails that frame only without killing the session.

## Spec Change Log

## Design Notes

Lab coldfix + coldfix-rerun misses share one fingerprint: neighbors OK at ~125 ms; Bridge logs `size reject` at 243 and/or 211 (USB max-packet multiples under 275); one auto re-request; second short → `no retry left; expect cleared` → lab `TIMEOUT last=none`. F0 repair and `send_fail` are not proximate. Prefer fixing assembly/delivery of the full dump; treat extra retries as Ask First if used as the main lever.

Ship combo: deepen bulk-IN ring 16→32 (fewer shorts) + `kMatrixDumpSizeRejectRetries=2` with nested-rewrite guard (recover rare dual shorts without nested Write mid-OUT). Residual URB loss deferred.

## Verification

**Commands:**
- `cmake --build --preset debug` -- Bridge + BridgeTests build
- `.\builds\debug\Debug\BridgeTests.exe "[matrix-dump]"` -- unit edges green
- `python scripts/quality/lint-touched.py` -- no new §3 violations on touched C++
- Mid lab `post-epic2-cr-midburst-final` -- 5/5 Starts @ 100%
- Bank lab `post-epic2-cr-midburst-final` -- 20/20 Starts @ 100% (`overall_pass=true`)

**Manual checks (if no CLI):**
- MT4 USB + Matrix on DIN Out1↔In (not Thru); ports free; no MIDI-OX / Matrix-Control during gate.

## Suggested Review Order

**Bulk IN depth (fewer mid-SysEx URB drops)**

- Always-pending IN ring 16→32; WFMO-safe static floor.
  [`WinUsbTransport.h:25`](../../../src/Usb/WinUsbTransport.h#L25)

**Size-reject recovery (rare dual short)**

- Rewrite budget constant (original + up to two re-requests).
  [`DeviceSessionSupport.h:82`](../../../src/Device/DeviceSessionSupport.h#L82)

- Arm expect with that budget after dump request OUT.
  [`DeviceSessionHostOutbound.cpp:65`](../../../src/Device/DeviceSessionHostOutbound.cpp#L65)

- Drop short frames during active rewrite — no nested Write/flush.
  [`DeviceSessionMatrixDump.cpp:47`](../../../src/Device/DeviceSessionMatrixDump.cpp#L47)

- Restore remaining retries after armExpect, before deferred flush.
  [`DeviceSessionMatrixDump.cpp:106`](../../../src/Device/DeviceSessionMatrixDump.cpp#L106)

**Tests**

- Exact vs short lengths (275/351 − N×32) and retry budget floor.
  [`DeviceInquiryHelpersTests.cpp:90`](../../../tests/unit/DeviceInquiryHelpersTests.cpp#L90)
