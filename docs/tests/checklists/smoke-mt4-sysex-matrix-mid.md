---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke — MT4 mid-size Matrix SysEx round-trip (Bridge)
author: Guillaume DUPONT
created: 2026-08-07
updated: 2026-08-07
---

# Smoke — mid-size Matrix SysEx round-trip (Bridge + host MIDI)

Lab gate for Matrix-Control–sized SysEx (~275 B patch / ~351 B master) through MT4
Virtual Ports after Device Inquiry Identity is already at 100 %.

**Related:** Epic 2.4 vectors #2 / #3 / #4 in `smoke-epic2-matrix-control-mt4.md`.
This checklist is the automated Python harness gate (not Matrix-Control UI).

**Spec:** `_bmad-output/implementation-artifacts/spec-sysex-matrix-mid-roundtrip.md`

## Prerequisites

1. Win10/11 x64, Zadig-bound MT4, Bridge Debug under `builds/debug/Debug/Bridge.exe`.
2. Matrix-1000 powered; DIN Out 1 ↔ In 1.
3. Close MIDI-OX / Matrix-Control on `MT4 Output 1` / `MT4 Input 1`.
4. Deps: `python -m pip install -r scripts/lab/requirements-device-inquiry.txt`

## Automated procedure (preferred)

```text
python scripts/lab/sysex-matrix-mid-loop.py --with-bridge --pass-percent 100
```

Defaults:

- Ports: `MT4 Output 1` / `MT4 Input 1`
- Count: 10 per scenario
- Fresh Bridge Starts: 2 (start 1 = pushes + dumps; start 2 = dumps only)
- Dump reply timeout: 3.0 s
- Fixtures: `tests/fixtures/sysex/Patch.syx` (275 B), `Master.syx` (351 B)
- Patch dump request: `F0 10 06 04 01 00 F7` (slot `00`)
- Master dump request: `F0 10 06 04 03 00 F7`

Logs:

- `tests/lab-logs/sysex-matrix-mid/sysex-matrix-mid-<UTC>.log`
- `tests/lab-logs/sysex-matrix-mid/bridge-<UTC>-start<N>.log`

## Pass criteria (strict 100 %)

| Scenario | Pass |
|---|---|
| Host→device patch push | Send completes; Bridge log has no pump / WriteBulk fail |
| Host→device master push | Same |
| Device→host patch dump | Exactly **275** B starting `F0 10 06 01` ending `F7` within timeout |
| Device→host master dump | Exactly **351** B starting `F0 10 06 03` ending `F7` within timeout |
| Fresh Starts | Dump pair **100 %** on each of ≥2 fresh Starts |

Harness must reassemble host SysEx until `F7` (never Pass on a first fragment).
No opaque warm-up Inquiry; no artificial slowdown to pass.

**Overall Pass:** script exit 0 and lab log `overall_pass=true`.

## Fail diagnostics

On TIMEOUT / wrong size, the lab log records `len`, head/tail bytes, and `dt_ms`.
Do not blame the Matrix if MT4 DIN LEDs show activity on a Fail.

## Out of scope (later paliers)

- Dump request All / ~100× bank stress
- Synthetic very-long SysEx
- Matrix-Control UI as the primary gate
- Windows MIDI Services; **MidiView** / **ShowMIDI** (retired — use **MIDI-OX**); AMT8 / Unitor8

## Lab notes (2026-08-07)

- Harness + checklist shipped; Bridge async IN reorder harvest under test.
- **Not closed at 100 % yet:** intermittent device→host patch dump TIMEOUT / short frames (`demux_spans=274` without `send_ok`). Pushes and master dumps are usually solid.
- See spec Design Notes for stamps.
