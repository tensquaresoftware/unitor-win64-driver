---
title: 'MT4 Bridge — mid-size Matrix SysEx round-trip lab (palier 1)'
type: 'feature'
created: '2026-08-07'
status: 'in-progress'
baseline_commit: 'c29583551d143a29455fd636f8e38f52a1ea1e1f'
review_loop_iteration: 0
context:
  - '{project-root}/conventions.md'
  - '{project-root}/_bmad/custom/clarity-bar-fr.md'
  - '{project-root}/docs/tests/checklists/smoke-epic2-matrix-control-mt4.md'
  - '{project-root}/scripts/lab/device-inquiry-loop.py'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiate">

## Intent

**Problem:** After Device Inquiry Identity is at 100 %, we still lack an automated lab gate that proves Matrix-Control–sized SysEx (~275 B patch / ~351 B master) survives the Bridge host↔device path in both directions with a strict 100 % bar.

**Approach:** Ship a Python lab harness modeled on the Device Inquiry loop, drive the four palier-1 scenarios against real fixtures and Matrix dump requests, document results in a Quick Dev spec + short checklist, and only touch Bridge C++ if the lab proves a hole.

## Boundaries & Constraints

**Always:**
- Single goal: palier-1 mid-size SysEx round-trip credibility (patch + master, both directions).
- Prefer script-only; Bridge patch only when lab evidence shows truncation, drop, timeout, or pump fail on the Bridge path.
- Reuse Inquiry Bridge lifecycle: `--with-bridge`, `builds/debug/Debug/Bridge.exe --start-session --dev-zadig`, fresh WinMM process after ports appear, clean stop.
- Default ports `MT4 Output 1` / `MT4 Input 1`; close MIDI-OX / Matrix-Control on those ports during the run.
- Fixtures: `tests/fixtures/sysex/Patch.syx` (275 B), `tests/fixtures/sysex/Master.syx` (351 B).
- Dump request patch: `F0 10 06 04 01 00 F7` (slot `00` documented); master: `F0 10 06 04 03 00 F7`.
- Device→host Pass = exact length + prefix/suffix match within timeout (~3 s default + small margin); reassemble host MIDI SysEx until `F7` — never Pass on a first fragment.
- Host→device Pass = send completes + no Bridge pump / WriteBulk fail in the captured Bridge log for that window (no audible Matrix proof required).
- Each critical scenario ≥10 reps; dump patch+master across ≥2 fresh Bridge Starts; `--pass-percent 100` (zero TIMEOUT / truncate / wrong size).
- Logs under `tests/lab-logs/sysex-matrix-mid/` + short EN README; checklist under `docs/tests/checklists/`; no opaque warm-up; no artificial slowdown to “pass”.
- If C++ changes: `python scripts/quality/lint-touched.py` clean.

**Ask First:**
- Treating Matrix silence as root cause when MT4 DIN LEDs show activity.
- Broad WinUSB / queue retunes without size/prefix/suffix/delay instrumentation first.
- Lowering the 100 % bar or skipping a fresh-Start dump pair.
- Expanding into All-bank dump, synthetic long SysEx, or Matrix-Control UI as the primary gate.

**Never:**
- Dump request All / ~100× bank stress; synthetic very-long SysEx generator; fake echo app.
- Matrix-Control UI as the gate (Python harness is the gate).
- Windows MIDI Services, MidiView, AMT8 / Unitor8.
- Changing Matrix-Control heartbeat.
- Claiming Pass from unit smokes alone without this lab.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Host→device patch push | Send `Patch.syx` (275 B) on Output 1 | Send OK; Bridge log free of pump/WriteBulk fail | Log size + first/last byte; Fail run |
| Host→device master push | Send `Master.syx` (351 B) on Output 1 | Same as patch push | Same |
| Device→host patch dump | Request `F0 10 06 04 01 00 F7` | Exactly 275 B starting `F0 10 06 01` ending `F7` within timeout | TIMEOUT / wrong size / bad ends → Fail; log len, head, tail, dt_ms |
| Device→host master dump | Request `F0 10 06 04 03 00 F7` | Exactly 351 B starting `F0 10 06 03` ending `F7` | Same |
| Fresh Start dumps | ≥2 full Bridge Starts; dump pair each | 100 % on both Starts | Do not warm-up Inquiry to hide first-shot loss |
| Fragmented host recv | WinMM/rtmidi may deliver SysEx in pieces | Harness buffers until `F7` then validates | Never Pass on incomplete buffer |

</frozen-after-approval>

## Code Map

- `scripts/lab/device-inquiry-loop.py` -- BridgeSession, WinMM refresh, port match, logging patterns to reuse
- `scripts/lab/requirements-device-inquiry.txt` -- mido / python-rtmidi (reuse or sibling)
- `tests/fixtures/sysex/Patch.syx` / `Master.syx` -- 275 / 351 B Matrix-Control fixtures
- `docs/tests/checklists/smoke-epic2-matrix-control-mt4.md` -- vectors #2 / #3 / #4 reference shapes
- `docs/tests/checklists/smoke-mt4-device-inquiry-bridge.md` -- prior 100 % Identity lab pattern
- `src/Protocol/MidiMessageFramer.*` -- device→host F0…F7 assembly (Bridge side)
- `src/Device/DeviceSessionHostOutbound.cpp` -- host→device encode + WriteBulk
- `src/Device/DeviceSessionDeviceHost.cpp` -- async IN → demux → SendToHost
- `src/Device/HostOutboundQueue.*` -- outbound queue caps for librarian-sized frames
- `builds/debug/Debug/Bridge.exe` -- lab binary under test (HEAD with Inquiry 100 %)

## Tasks & Acceptance

**Execution:**
- [ ] `_bmad-output/implementation-artifacts/spec-sysex-matrix-mid-roundtrip.md` -- keep Design Notes for lab stamps/rates -- evidence SSOT
- [ ] `scripts/lab/sysex-matrix-mid-loop.py` -- new harness (push ×2, dump ×2, Bridge lifecycle, reassemble-to-F7, 100 % gate) -- automated palier-1 gate
- [ ] `tests/lab-logs/sysex-matrix-mid/README.md` -- short EN how-to + log naming -- operator path
- [ ] `docs/tests/checklists/smoke-mt4-sysex-matrix-mid.md` -- one-shot EN checklist + Pass criteria -- human retest
- [ ] Lab run on Zadig MT4 + Matrix DIN Out1↔In1 -- ≥10 reps/scenario, ≥2 fresh Starts for dumps, document stamps -- prove 100 % or instrument Fail
- [ ] Bridge C++ (only if lab KO) -- minimal fix + lint-touched -- close proven hole only

**Acceptance Criteria:**
- Given fixtures and Matrix on cable 1, when the harness runs host→device patch and master pushes (≥10 each), then every send completes and Bridge logs show no pump/WriteBulk fail for those windows.
- Given a fresh Bridge Start, when the harness sends patch dump request slot `00` (≥10×) and master dump request (≥10×), then every reply is exactly 275 / 351 bytes with the locked prefixes and trailing `F7` inside the timeout.
- Given at least two fresh Bridge Starts, when the dump pair is repeated on each Start, then both Starts meet 100 % (no first-shot-only dump loss papered over by warm-up).
- Given WinMM/rtmidi may fragment SysEx, when a dump reply arrives in pieces, then the harness only evaluates after `F7` and still meets the exact-size rules.
- Given palier-1 complete, when reviewing scope, then All-bank / long synthetic SysEx / Matrix-Control UI gate remain out of scope.

## Spec Change Log

## Design Notes

Defaults locked for implementers:
- Patch dump program byte `<n> = 00`.
- Dump reply timeout default 3.0 s (Matrix-Control ~2 s + margin).
- Target one-shot: `python scripts/lab/sysex-matrix-mid-loop.py --with-bridge --pass-percent 100`
- Reuse Inquiry deps file unless a sibling requirements file is strictly needed.
- Harness runs dumps before pushes on Start 1 (Matrix can stay busy after librarian writes).
- Bridge change under test: in-order bulk IN harvest via reorder buffer (`WinUsbBulkInAsyncRing.h`) so completed slots resubmit immediately while delivery stays submit-ordered.

### Lab evidence (2026-08-07, baseline `c295835` + async reorder)

| Stamp | Result | Notes |
|---|---|---|
| `20260807T133833Z` | Fail | Pre-fix: dump_patch corrupt/truncate (`F0 10 00 02` len=272; len=274) + Start2#1 TIMEOUT |
| `20260807T134744Z` | Fail | After in-order-only wait: Start2 dumps 100%; Start1 dump_patch#1 TIMEOUT after pushes |
| `20260807T134928Z` | Fail | Dumps-before-pushes: dump_patch 100%; dump_master#5 discard len=191 tail matches full master (middle drop) |
| `20260807T135214Z` / `135942Z` | Fail | Reorder buffer: still intermittent dump_patch TIMEOUT with Bridge `demux_spans=274` + `send_ok=0` (missing final F7) on fresh Start #1; later trials often 125 ms / 275 B |

**Stable so far:** host→device push patch/master 10/10; dump_master often 10/10; dump_patch usually ≥8/10 with ~125 ms when OK; no Bridge pump/WriteBulk fail lines.

**Open hole:** device→host patch dump still loses the closing byte/packet often enough to miss 100 % (framer holds 274 spans without SendToHost). Not blamed on Matrix when DIN LEDs / bulk_in activity are present.

## Verification

**Commands:**
- `python -m pip install -r scripts/lab/requirements-device-inquiry.txt` -- expected: deps installed
- `python scripts/lab/sysex-matrix-mid-loop.py --with-bridge --pass-percent 100` -- expected: exit 0; 100 % all scenarios; logs under `tests/lab-logs/sysex-matrix-mid/`
- `python scripts/quality/lint-touched.py` -- expected: clean if any C++ diff (skip if script-only)

**Manual checks (if no CLI):**
- Matrix-1000 powered; DIN Out1↔In1; MIDI-OX / Matrix-Control closed on MT4 ports; Bridge Debug present at `builds/debug/Debug/Bridge.exe`.
