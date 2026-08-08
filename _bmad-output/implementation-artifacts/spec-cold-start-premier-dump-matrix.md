---
title: 'Fix Matrix cold-start first dump_patch after Bridge Start'
type: 'bugfix'
created: '2026-08-08'
status: 'in-progress'
baseline_commit: '12003de771f2ac943dbf3ba636f88f781d7b1f6d'
review_loop_iteration: 0
context:
  - '{project-root}/docs/dev/prompt-fix-cold-start-premier-dump-matrix.md'
  - '{project-root}/_bmad-output/implementation-artifacts/spec-post-epic2-cr-bank-mid-day-gate.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** After a fresh Bridge Start, the first Matrix `dump_patch` often times out with no frame (`TIMEOUT last=none`) while later dumps on the same Start succeed — so the bank+mid day gate stays red even though the warm path mostly works.

**Approach:** Restore a Bridge-side leading-F0 repair that works with Emagic’s 1-byte demux spans under the existing expect window, then re-run the mid and bank day gates. Do not mask the hole in the lab harness.

## Boundaries & Constraints

**Always:**
- Keep short-dump race fixes from `835c992` (expect before flush, deferred retry, abandon→retry, no Write/SendToHost from VirtualMIDI callback, expect expiry, deferred cap).
- Score every dump including the first after Start (no lab discard of dump #1).
- Prefer the smallest change that makes mid 5×10 and bank 20×100 pass at 100% per Start.
- English diagnostics only; run `scripts/quality/lint-touched.py` on touched C++.

**Ask First:**
- Widening Start settle / post-calm delay beyond a tiny secondary tweak if F0 repair alone is insufficient.
- Changing lab harness scoring to ignore the first dump.
- Touching framer/clock or USB ring sizing (Epic 2 Group A deferred lots B/C) without new lab proof.

**Never:**
- Overnight 8 h or long SysEx palier-3 as this ticket’s goal.
- Reverting the rest of `835c992` to “fix” cold-start.
- Treating mid-burst rares as the primary target before cold-start is fixed.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Cold first dump, lost leading F0 | Expect armed; first IN span is single-byte `0x10` (next spans continue `06…`) | Prepend `F0`, framer holds SysEx, host gets full dump reply | Log `SysEx leading-F0 repair` with span_len=1 |
| Cold first dump, intact F0 | Expect armed; first span starts `F0` | No repair; normal deliver | N/A |
| Multi-byte span `10 06…` under expect | Expect armed; span_len≥2 starts `10 06` | Same prepend-F0 repair | Log repair with span_len≥2 |
| Expect idle / already holding SysEx | `armRepair` false | No prepend; pass span unchanged | N/A |
| Mid-burst residual after cold-start green | Rare `TIMEOUT last=none` mid bank | Out of primary scope; note in deferred if gate still red | Do not expand Start settle without Ask First |

</frozen-after-approval>

## Code Map

- `src/Device/DeviceSessionSupport.cpp` -- `maybePrependLostLeadingF0` currently requires same-span `10 06` with `byteCount >= 2`; overnight evidence used `span_len=1 head_was=10`
- `src/Device/DeviceSessionSupport.h` -- repair API + Matrix dump helpers / comments
- `src/Device/DeviceSessionDeviceHost.cpp` -- `forwardDeviceMidi` arms repair when expect is active and framer not holding
- `src/Device/DeviceSession.cpp` -- post-start pipe prime / IN calm / librarian OUT gate (secondary only if repair insufficient)
- `src/Device/DeviceSessionHostOutbound.cpp` -- expect before flush / outbound write (preserve `835c992`)
- `src/Device/DeviceSessionMatrixDump.cpp` -- size-reject retry (preserve; cannot save zero-byte replies)
- `tests/lab-logs/sysex-matrix-mid/post-epic2-cr/sysex-matrix-mid-20260808T161402Z.log` -- 5/5 first-dump TIMEOUT fingerprint
- `tests/lab-logs/sysex-matrix-bank/post-epic2-cr/sysex-matrix-bank-20260808T160736Z.log` -- 8/20 first-dump + mid-burst `last=none`

## Tasks & Acceptance

**Execution:**
- [x] `src/Device/DeviceSessionSupport.cpp` (+ `.h` comment if needed) -- Restore leading-F0 repair for expect-gated single-byte `0x10` demux spans (and keep multi-byte `10 06` case) -- Emagic delivers 1-byte spans; `835c992` same-span `10 06` gate never fires → silent `last=none`
- [x] Unit or focused smoke if a cheap hook already exists near Support helpers; otherwise rely on lab gates -- Prove repair accepts span_len=1 `0x10` and still ignores when `armRepair` is false
- [x] Rebuild debug Bridge + Lab B mid (`5×10`, 100%) then Lab A bank (`20×100`, 100%) under `post-epic2-cr-coldfix` log dirs -- Day gate is the product proof *(mid Pass; bank Fail on 1 mid-burst residual — HALT Ask First)*
- [x] If mid green but bank still red only on mid-burst rares -- Document residual in deferred-work; Ask First before Start-settle widening

**Acceptance Criteria:**
- Given a fresh Bridge Start with Matrix on DIN Out1↔In (not Thru), when Lab B mid runs `--fresh-starts 5 --count 10 --pass-percent 100`, then every Start passes 100% including dump #1 (no `TIMEOUT last=none` on the first `dump_patch`).
- Given the same setup, when Lab A bank runs `--fresh-starts 20 --count 100 --pass-percent 100`, then every Start passes 100%.
- Given expect armed and a first IN span of one byte `0x10`, when `maybePrependLostLeadingF0` runs, then it prepends `F0` and diagnostics show a leading-F0 repair (not silence until lab timeout).
- Given expect not armed (or framer already holding SysEx), when a span starts with `0x10`, then no F0 is prepended.
- Given success, when reporting, then Pass/Fail table + log paths are delivered; no commit unless Guillaume asks.

## Spec Change Log

## Design Notes

Day-gate Bridge logs show failing first dumps as `first-burst IN: head=10 has_f0=no holding=no` with `send_ok` stuck at 0 while `bulk_in_bytes` rises. Passing Starts show `head=F0 has_f0=yes`. Overnight pre-`835c992` repaired `span_len=1 head_was=10`. Current code requires `byteCount >= 2` and `midiBytes[1] == 0x06` in the **same** span — incompatible with 1-byte demux — so repair never runs and expect expires as `last=none`.

Preferred fix shape (illustrative):

```cpp
// Under expect: lost F0 often arrives as a lone 0x10 span (then 0x06…).
if (!armRepair || midiBytes == nullptr || byteCount < 1 || midiBytes[0] != 0x10)
    return {midiBytes, byteCount};
if (byteCount >= 2 && midiBytes[1] != 0x06)
    return {midiBytes, byteCount};
// prepend F0 into repairStorage …
```

Pipe-prime / post-calm settle remains secondary: calm already reports ready on failing Starts; the missing piece is repair on the truncated first reply.

## Verification

**Commands:**
- `cmake --build --preset debug` -- Bridge builds
- `python scripts/quality/lint-touched.py` -- no new §3 violations on touched C++
- `python scripts/lab/sysex-matrix-mid-loop.py --with-bridge --out-port "MT4 Out 1" --in-port "MT4 In 1" --pass-percent 100 --count 10 --fresh-starts 5 --bridge-exe builds\debug\Debug\Bridge.exe --log-dir tests\lab-logs\sysex-matrix-mid\post-epic2-cr-coldfix` -- 5/5 Starts at 100%
- `python scripts/lab/sysex-matrix-bank-loop.py --with-bridge --out-port "MT4 Out 1" --in-port "MT4 In 1" --pass-percent 100 --count 100 --fresh-starts 20 --bridge-exe builds\debug\Debug\Bridge.exe --log-dir tests\lab-logs\sysex-matrix-bank\post-epic2-cr-coldfix` -- 20/20 Starts at 100%

**Manual checks (if no CLI):**
- Hardware: MT4 USB + Matrix Out1↔In (not Thru); ports free (no MIDI-OX / Matrix-Control).
- Optional stress only if A+B green: bank `--fresh-starts 50` (not overnight).
