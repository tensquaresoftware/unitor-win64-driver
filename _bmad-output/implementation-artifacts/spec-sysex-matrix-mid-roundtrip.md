---
title: 'MT4 Bridge — mid-size Matrix SysEx round-trip lab (palier 1)'
type: 'feature'
created: '2026-08-07'
status: 'done'
baseline_commit: 'f91f883c4bce304d294c8736b08daf68a99582b1'
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
- Windows MIDI Services; MidiView/ShowMIDI (retired; use MIDI-OX); AMT8 / Unitor8.
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
- [x] `_bmad-output/implementation-artifacts/spec-sysex-matrix-mid-roundtrip.md` -- keep Design Notes for lab stamps/rates -- evidence SSOT
- [x] `scripts/lab/sysex-matrix-mid-loop.py` -- new harness (push ×2, dump ×2, Bridge lifecycle, reassemble-to-F7, 100 % gate) -- automated palier-1 gate
- [x] `tests/lab-logs/sysex-matrix-mid/README.md` -- short EN how-to + log naming -- operator path
- [x] `docs/tests/checklists/smoke-mt4-sysex-matrix-mid.md` -- one-shot EN checklist + Pass criteria -- human retest
- [x] Lab run on Zadig MT4 + Matrix DIN Out1↔In1 -- ≥10 reps/scenario, ≥2 fresh Starts for dumps, document stamps -- prove 100 % or instrument Fail
- [x] Bridge C++ (only if lab KO) -- minimal fix + lint-touched -- close proven hole only

**Acceptance Criteria:**
- Given fixtures and Matrix on cable 1, when the harness runs host→device patch and master pushes (≥10 each), then every send completes and Bridge logs show no pump/WriteBulk fail for those windows.
- Given a fresh Bridge Start, when the harness sends patch dump request slot `00` (≥10×) and master dump request (≥10×), then every reply is exactly 275 / 351 bytes with the locked prefixes and trailing `F7` inside the timeout.
- Given at least two fresh Bridge Starts, when the dump pair is repeated on each Start, then both Starts meet 100 % (no first-shot-only dump loss papered over by warm-up).
- Given WinMM/rtmidi may fragment SysEx, when a dump reply arrives in pieces, then the harness only evaluates after `F7` and still meets the exact-size rules.
- Given palier-1 complete, when reviewing scope, then All-bank / long synthetic SysEx / Matrix-Control UI gate remain out of scope.

## Spec Change Log

- 2026-08-07 — Bridge: post-start IN calm + OUT silence after dump request; leading-F0 repair; wrong-size Matrix dump reject + one dump-request retry; bulk IN ring depth 16. Lab gate 100 % including Matrix power-cycle confirm (`20260807T190336Z`; prior `185403Z` / `185508Z` / `185953Z`).

## Design Notes

Defaults locked for implementers:
- Patch dump program byte `<n> = 00`.
- Dump reply timeout default 3.0 s (Matrix-Control ~2 s + margin).
- Target one-shot: `python scripts/lab/sysex-matrix-mid-loop.py --with-bridge --pass-percent 100`
- Reuse Inquiry deps file unless a sibling requirements file is strictly needed.
- Harness runs dumps before pushes on Start 1 (Matrix can stay busy after librarian writes).
- Bridge change under test: in-order bulk IN harvest via reorder buffer (`WinUsbBulkInAsyncRing.h`) so completed slots resubmit immediately while delivery stays submit-ordered.
- Post-start: quiet-CC prime, then wait until deliver queue idle + full pending depth before librarian OUT.
- After each short host SysEx (dump request): silence further WriteBulk until a complete device→host SysEx or timeout.
- Narrow guards (lab-proven): if expect-IN is armed and the first product byte is `0x10` without a hold, prepend `F0`; if a Matrix dump reply arrives with wrong length, do not `SendToHost` and re-send the dump request once.
- `kBulkInAsyncSlotCount = 16` (was 7) for deeper always-pending IN during Matrix bursts.

### Lab evidence (2026-08-07)

| Stamp | Result | Notes |
|---|---|---|
| `20260807T133833Z`…`161017Z` | Fail | Pre-fix era: first dump after Start often `TIMEOUT` / `demux≈274` + `send_ok=0` (lost leading F0) or mid-stream discard wrong length |
| `20260807T183415Z` | Fail | Post-start calm only: Start1 colder (4× bulk_in=0); instrumentation later proved F0 loss vs hold |
| `20260807T183815Z` | Fail ~90 % | Calm + OUT silence: first-burst `head=10` (F0 lost); classic hole confirmed |
| `20260807T184459Z` | Pass once | Calm/silence lucky natural F0; not stable on next runs (mid-stream discard) |
| `20260807T185403Z` | **Pass** | F0 repair fired Start1#1; size-reject+retry once (len=319); both Starts 100 % |
| `20260807T185508Z` | **Pass** | Confirm harness; F0 repair on Start2; `overall_pass=true` exit 0 |
| `20260807T185953Z` | **Pass** | Post-review (narrow arm/clear expect); F0 repair Start1; `overall_pass=true` exit 0 — Matrix power state at Start not operator-confirmed |
| `20260807T190336Z` | **Pass** | Clean confirm: Matrix power-cycled before launch; F0 repair + size-reject retries; `overall_pass=true` exit 0 |

**Closed for palier 1:** fresh-Start first dump + mid-size Matrix round-trip at 100 % across ≥2 Starts, including one operator-confirmed Matrix power-cycle before launch (`190336Z`). macOS Apple-driver control remains the hardware SSOT (`docs/tests/lab-reports/macos-sysex-paliers-2026-08-07.md`).

## Suggested Review Order

**Dump-request quiet window + retry**

- Arm expect only for Matrix dump requests; block WriteBulk while waiting for the reply.
  [`DeviceSessionHostOutbound.cpp:49`](../../src/Device/DeviceSessionHostOutbound.cpp#L49)

- Size-reject wrong-length Matrix dumps and re-send the dump request once.
  [`DeviceSessionHostOutbound.cpp:73`](../../src/Device/DeviceSessionHostOutbound.cpp#L73)

- Deliver exact 275/351 only after length check; clear expect only on exact Matrix dump.
  [`DeviceSessionDeviceHost.cpp:109`](../../src/Device/DeviceSessionDeviceHost.cpp#L109)

**Leading-F0 repair (first-shot hole)**

- Prepend `F0` when dump expect is armed and demux starts at `0x10` without a hold.
  [`DeviceSessionSupport.cpp:76`](../../src/Device/DeviceSessionSupport.cpp#L76)

- Wire repair into the reader demux→framer path (never from WinUSB completion).
  [`DeviceSessionDeviceHost.cpp:179`](../../src/Device/DeviceSessionDeviceHost.cpp#L179)

**Post-start calm before librarian OUT**

- Gate host→device until IN deliver queue idle and pending ring is full.
  [`DeviceSession.cpp:243`](../../src/Device/DeviceSession.cpp#L243)

**Always-pending IN depth**

- Raise async bulk IN ring to 16 slots; expose pending count for calm/diag.
  [`WinUsbTransport.h:24`](../../src/Usb/WinUsbTransport.h#L24)

- Count pending slots without draining the ring.
  [`WinUsbBulkInAsync.cpp:246`](../../src/Usb/WinUsbBulkInAsync.cpp#L246)

## Verification

**Commands:**
- `python -m pip install -r scripts/lab/requirements-device-inquiry.txt` -- expected: deps installed
- `python scripts/lab/sysex-matrix-mid-loop.py --with-bridge --pass-percent 100` -- expected: exit 0; 100 % all scenarios; logs under `tests/lab-logs/sysex-matrix-mid/`
- `python scripts/quality/lint-touched.py` -- expected: clean if any C++ diff (skip if script-only)

**Manual checks (if no CLI):**
- Matrix-1000 powered; DIN Out1↔In1; MIDI-OX / Matrix-Control closed on MT4 ports; Bridge Debug present at `builds/debug/Debug/Bridge.exe`.
