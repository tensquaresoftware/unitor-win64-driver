---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 2 — MT4 (MIDI clock / transport)
author: Guillaume DUPONT
created: 2026-08-05
updated: 2026-08-10
---

# Smoke guide — Epic 2.1 (MT4 MIDI clock and transport)

> **Operator lab (French):** use [`smoke-epic2-mt4.md`](../smoke-epic2-mt4.md) §3. This file is the per-story English checklist (agents / validation matrix).

Use this checklist to prove **Timing Clock** (`0xF8`) and **Start / Continue / Stop** (`0xFA` / `0xFB` / `0xFC`) through MT4 Virtual Ports under a short sequencing smoke.

Epic 1 notes/CC must already pass on at least one IN and one OUT. That baseline lives in `docs/tests/smoke-epic1-mt4.md`. This document owns clock/transport only. MTC quarter-frame / full-frame proof lives in `docs/tests/checklists/smoke-epic2-mtc-mt4.md` (story 2.2).

Mark each validation line **Pass** or **Fail**. On Fail, write English notes with **Port N / cable / direction**.

## Scope

**In scope (story 2.1)**

- Timing Clock (`0xF8`)
- Start (`0xFA`), Continue (`0xFB`), Stop (`0xFC`)
- Both directions: device → host (physical IN → DAW MIDI IN) and host → device (DAW MIDI OUT → physical OUT)
- Short sequencing session without Bridge restart
- Bridge-induced clock/transport dropouts = defect

**Out of scope**

- MTC quarter-frame / full-frame → story 2.2 (`smoke-epic2-mtc-mt4.md`)
- Transparent SysEx librarian / burst buffering → story 2.3 (`smoke-epic2-sysex-mt4.md`)
- Matrix-Control minimum SysEx vectors → story 2.4 (`smoke-epic2-matrix-control-mt4.md`)
- ~4 h longevity → story **2.5** (`smoke-epic2-longevity-mt4.md`)
- Latency / jitter Studio-Done numbers → Epic 5
- MIDI-OX alone as sole clock-slave proof when a Validation Matrix DAW is available

**Dropout definition:** Bridge-induced loss or stall of clock/transport under a normal short sequencing smoke (not a multi-hour soak, not Epic 5 p99 thresholds). If the DAW loses sync or Start/Stop/Continue is missing while notes still flow, that is a Fail.

## Prerequisites

1. Epic 1 notes/CC smoke green on ≥1 IN and ≥1 OUT.
2. Windows 10 x64 (mandatory matrix row) or Windows 11 x64 when that hardware is available.
3. WinUSB-bound MT4 + teVirtualMIDI present (same lab path as Epic 1).
4. Validation Matrix DAW: **Ableton Live 12** or **Reason Studios 12** (minimum: one of these).
5. Optional observer: MIDI-OX — may watch bytes; does not replace DAW slave/observe for this story.
6. Bridge build that includes story 2.1 framer/mapper smoke vectors (`Bridge --test-mapper` exit 0).

## Synthetic gate (no hardware)

Run before hardware:

```text
Bridge.exe --test-mapper
```

Expected: exit 0, including framer realtime vectors and mapper clock/transport encode/decode.

Synthetic gate: Pass / Fail  
Notes:

## Hardware session start

Same long-run path as Epic 1:

```text
Bridge.exe --start-session
Bridge.exe --start-session --dev-zadig
```

Leave the session running. Do not restart the Bridge during the short smoke below.

Session started, Virtual Ports visible: Pass / Fail  
Notes (Port N):

## Matrix A — Host → device (DAW OUT → MT4 physical OUT)

**Goal:** DAW sends clock/transport to a Virtual OUT; MT4 physical OUT carries them (DIN LED / slave device / loopback observation).

| Check | Win10 x64 | Win11 x64 (when available) | Notes (Port N / cable / direction) |
|---|---|---|---|
| ≥1 Virtual OUT selected in DAW | Pass (harness) | | `MT4 Out 2` via `midi-clock-loopback-lab.py` (no DAW) |
| Timing Clock (`0xF8`) observed on physical OUT path | Pass | | Out2→In2 DIN loopback; 108/108 clocks |
| Start (`0xFA`) observed | Pass | | |
| Stop (`0xFC`) observed | Pass | | two Stops in sequence scored |
| Continue (`0xFB`) observed (required; if DAW cannot emit Continue, mark N/A + English reason) | Pass | | |
| No Bridge-induced gaps under short sequencing smoke | Pass | | `bridge_fail_needles: none` |
| Notes still flow on same OUT (regression spot-check) | Pass (pre-score) | | harness note-on sanity before clock scoring |

**Host→device framer note:** Default assumption is teVirtualMIDI `PARSE_RX` delivers Start/Stop/Continue/Clock as complete single-byte commands, so existing Encode + WriteBulk is enough. Add a host→device framer **only if** this lab row shows interleaved realtime inside multi-byte spans or incomplete host→device commands.

Host→device path Pass / Fail: **Pass** (PC-only harness 2026-08-10)  
Lab evidence (LED / slave / loopback): DIN Out2→In2 via `midi-clock-loopback-lab.py` (Scarlett/DAW UAT deferred)

## Matrix B — Device → host (MT4 physical IN → DAW MIDI IN)

**Goal:** Clock/transport arriving on a physical IN appear on the matching Virtual IN without Bridge-induced dropouts.

| Check | Win10 x64 | Win11 x64 (when available) | Notes (Port N / cable / direction) |
|---|---|---|---|
| ≥1 Virtual IN armed / observed in DAW | Pass (harness) | | `MT4 In 2` via harness (no DAW) |
| Timing Clock (`0xF8`) observed or DAW slaves to it | Pass | | echo on In2; DAW slave UAT deferred |
| Start / Stop / Continue (`0xFA` / `0xFC` / `0xFB`) observed (Continue: N/A + reason only if DAW cannot emit) | Pass | | |
| No Bridge-induced gaps under short sequencing smoke | Pass | | |
| Notes still flow on same IN (regression spot-check) | Pass (pre-score) | | note-on sanity before clock scoring |

Device→host path Pass / Fail: **Pass** (same DIN loopback closes host→device→DIN→device→host)  
DAW used (Ableton Live 12 / Reason Studios 12): N/A — Python harness closeout; real DAW UAT deferred

## Short session stability

| Check | Result | Notes |
|---|---|---|
| Short sequencing smoke without Bridge restart | Pass | harness single Bridge session |
| No Bridge restart required to restore clock/transport | Pass | |
| Console shows no WriteBulk / SendToHost failure storm | Pass | `bridge_fail_needles: none` |

## OS matrix summary

| OS | Status | Date | Notes |
|---|---|---|---|
| Windows 10 x64 | Pass (PC-only harness) | 2026-08-10 | `midi-clock-loopback-lab.py` Out2→In2 stamp `20260809T221926Z`; Scarlett/DAW UAT deferred |
| Windows 11 x64 | when hardware available | | |

## Failure notes template (English)

```text
FAIL — clock/transport
Port N:
Cable index:
Direction: host→device | device→host
Symptom: missing Start | missing Stop | clock gaps | sync lost | other
Observer: Ableton | Reason | MIDI-OX | DIN LED | slave device
Bridge version / commit:
```

## Pass criteria (story 2.1)

All of the following:

1. Synthetic `--test-mapper` Pass.
2. At least one IN and one OUT carry `0xF8` + Start/Stop/Continue under short smoke without Bridge-induced dropouts.
3. Win10 x64 row documented (Pass or Fail with English notes). Win11 documented when hardware is available.
4. Notes/CC spot-check still Pass on the same ports.

**Overall story 2.1 hardware smoke:** Pass (PC-only DIN loopback harness; Scarlett/DAW UAT deferred)  
**Signed off by:** Guillaume (via Quick Dev harness closeout)  
**Date:** 2026-08-10  

**Harness evidence:** `python scripts/lab/midi-clock-loopback-lab.py --with-bridge` → exit 0; journal `tests/lab-logs/midi-clock-loopback/midi-clock-loopback-20260809T221926Z.log` (`clock=108/108`, Start/Continue/Stop with stop≥2, `bridge_fail_needles: none`). Same honesty barème as story 2.2 MTC loopback.
