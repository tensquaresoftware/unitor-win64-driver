---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 2 — MT4 (MTC quarter-frame / full-frame)
author: Guillaume DUPONT
created: 2026-08-05
updated: 2026-08-09
---

# Smoke guide — Epic 2.2 (MT4 MTC quarter-frame and full-frame)

> **Operator lab (French):** use [`smoke-epic2-mt4.md`](../smoke-epic2-mt4.md) §4. This file is the per-story English checklist (agents / validation matrix).

Use this checklist to prove **MTC quarter-frame** (`0xF1` + data) and **MTC full-frame** (Universal Real Time SysEx `F0 7F … 01 01 hr mn sc fr F7`) through MT4 Virtual Ports under a short sync smoke.

Clock / Start-Stop-Continue proof lives in `docs/tests/checklists/smoke-epic2-clock-mt4.md` (story 2.1). Epic 1 notes/CC baseline lives in `docs/tests/smoke-epic1-mt4.md`. This document owns **MTC sync only**.

Mark each validation line **Pass** or **Fail**. On Fail, write English notes with **Port N / cable / direction**.

## Scope

**In scope (story 2.2)**

- MTC quarter-frame (`0xF1` + one data byte; types 0–7 for a complete SMPTE time)
- MTC full-frame used for sync locate/cue (`F0 7F <id> 01 01 hr mn sc fr F7`, typically broadcast `id = 0x7F`)
- Both directions: device → host (physical IN → DAW MIDI IN) and host → device (DAW MIDI OUT → physical OUT)
- Short sync session without Bridge restart
- Bridge-induced MTC dropouts = defect
- MTC is required V1 transport coverage (not optional polish)

**Out of scope**

- Timing Clock / Start-Stop-Continue → story 2.1 (`smoke-epic2-clock-mt4.md`)
- Transparent SysEx librarian / burst buffering → story 2.3 (`smoke-epic2-sysex-mt4.md`)
- Matrix-Control minimum SysEx vectors → story 2.4 (`smoke-epic2-matrix-control-mt4.md`)
- ~4 h longevity → story **2.5** (`smoke-epic2-longevity-mt4.md`)
- Latency / jitter Studio-Done numbers → Epic 5
- SMPTE user-bits / MMC / MTC cueing variants beyond full-frame sync
- Building an MTC generator UI or SMPTE display — Bridge carries bytes only
- ShowMIDI alone as sole MTC-slave proof when a Validation Matrix DAW is available

**Important:** Full-frame MTC is SysEx-shaped, but **this story’s acceptance is MTC sync only**. Passing full-frame here does **not** claim Matrix-Control / librarian SysEx done (stories 2.3 / 2.4).

**Dropout definition:** Bridge-induced loss or stall of MTC quarter-frame / full-frame under a normal short sync smoke (not a multi-hour soak, not Epic 5 p99 thresholds). If the DAW loses timecode lock or full-frame cues never arrive while notes still flow, that is a Fail.

## Prerequisites

1. Epic 1 notes/CC smoke green on ≥1 IN and ≥1 OUT.
2. Prefer story 2.1 clock/transport Win10 row green on the same lab path when available (soft dependency — synthetic MTC work does not wait on it).
3. Windows 10 x64 (mandatory matrix row) or Windows 11 x64 when that hardware is available.
4. WinUSB-bound MT4 + teVirtualMIDI present (same lab path as Epic 1).
5. Validation Matrix DAW: **Ableton Live 12** or **Reason Studios 12** (minimum: one of these).
6. Optional observer: ShowMIDI — may watch bytes / loopback; does not replace DAW MTC send/slave/observe for this story when a matrix DAW is available.
7. Bridge build that includes story 2.2 framer/mapper MTC smoke vectors (`Bridge --test-mapper` exit 0).

**DAW tip:** Prefer a port dedicated to MTC when the DAW allows — MTC can be chatty; Bridge must still carry it on a shared Virtual Port without dropouts under short smoke.

## Synthetic gate (no hardware)

Run before hardware:

```text
Bridge.exe --test-mapper
```

Expected: exit 0, including framer MTC quarter-frame / full-frame vectors and mapper MTC encode/decode.

Synthetic gate: Pass  
Notes: `Bridge --test-mapper` / Catch2 MTC vectors (story implementation 2026-08-05)

## Hardware session start

Same long-run path as Epic 1 / 2.1:

```text
Bridge.exe --start-session
Bridge.exe --start-session --dev-zadig
```

Leave the session running. Do not restart the Bridge during the short smoke below.

Session started, Virtual Ports visible: Pass  
Notes (Port N): 2026-08-09 `mtc-loopback-lab.py --with-bridge` — Virtual Ports up; DIN red loopback Out2→In2

## Host→device path assumption (lab-gated)

Default assumption: teVirtualMIDI `PARSE_RX` delivers complete System Common (`0xF1` + data) and complete SysEx full-frame units, so existing Encode + WriteBulk is enough.

**Only if** this lab shows incomplete host→device spans (split `0xF1` without data, truncated SysEx, or garbled full-frame): add a symmetric host→device framer. Do **not** add it preemptively.

If dense quarter-frame rate (~4× SMPTE frame rate) shows Bridge-induced dropouts under short smoke, investigate known load edge (`processBulkRead` holding `usbIoMutex_` across decode) — fix only if required for AC pass; full latency harness remains Epic 5.

## Matrix A — Host → device (DAW OUT → MT4 physical OUT)

**Goal:** DAW sends MTC to a Virtual OUT; MT4 physical OUT carries quarter-frame and at least one full-frame cue (DIN LED / slave device / ShowMIDI-on-loopback / DAW MTC observe).

| Check | Win10 x64 | Win11 x64 (when available) | Notes (Port N / cable / direction) |
|---|---|---|---|
| ≥1 Virtual OUT selected in DAW | Pass (harness) | | Python `mido` on `MT4 Out 2` (DAW UAT deferred) |
| MTC quarter-frame (`0xF1`) observed on physical OUT path | Pass | | DIN loopback Out2→In2; 72/72 QF |
| At least one MTC full-frame cue observed | Pass | | `F0 7F 7F 01 01 20 15 30 10 F7` |
| No Bridge-induced gaps under short sync smoke | Pass | | |
| Notes still flow on same OUT (regression spot-check) | Pass | | sanity `note_on` before MTC |

Host→device path Pass  
Lab evidence (LED / slave / loopback / DAW observe): `tests/lab-logs/mtc-loopback/mtc-loopback-20260809T213749Z.log`

## Matrix B — Device → host (MT4 physical IN → DAW MIDI IN)

**Goal:** MTC arriving on a physical IN appears on the matching Virtual IN without Bridge-induced dropouts.

| Check | Win10 x64 | Win11 x64 (when available) | Notes (Port N / cable / direction) |
|---|---|---|---|
| ≥1 Virtual IN armed / observed in DAW | Pass (harness) | | Python on `MT4 In 2` |
| MTC quarter-frame observed or DAW slaves to it | Pass | | same Out2→In2 loopback (after IN demux OUT-hint) |
| At least one MTC full-frame cue observed | Pass | | |
| No Bridge-induced gaps under short sync smoke | Pass | | |
| Notes still flow on same IN (regression spot-check) | Pass | | |

Device→host path Pass  
DAW used (Ableton Live 12 / Reason Studios 12): Python harness (Ableton/Scarlett UAT deferred to future dedicated guide)

## Short session stability

| Check | Result | Notes |
|---|---|---|
| Short sync smoke without Bridge restart | Pass | single Bridge Start |
| No Bridge restart required to restore MTC | Pass | |
| Console shows no WriteBulk / SendToHost failure storm | Pass | `bridge_fail_needles: none` |

## OS matrix summary

| OS | Status | Date | Notes |
|---|---|---|---|
| Windows 10 x64 | Pass | 2026-08-09 | Boot Camp lab; loopback harness |
| Windows 11 x64 | when hardware available | | |

## Failure notes template (English)

```text
FAIL — MTC quarter-frame / full-frame
Port N:
Cable index:
Direction: host→device | device→host
Symptom: missing quarter-frame | missing full-frame | timecode lock lost | garbled full-frame | other
Observer: Ableton | Reason | ShowMIDI | DIN LED | slave device
Bridge version / commit:
```

## Pass criteria (story 2.2)

All of the following:

1. Synthetic `--test-mapper` Pass (framer + mapper MTC vectors).
2. At least one IN and one OUT carry quarter-frame + at least one full-frame cue under short smoke without Bridge-induced dropouts.
3. Win10 x64 row documented (Pass or Fail with English notes). Win11 documented when hardware is available.
4. Notes/CC spot-check still Pass on the same ports.
5. Failures identify Virtual Port / cable in English diagnostics.

**Overall story 2.2 hardware smoke:** Pass  
**Signed off by:** Guillaume (Python harness accepted 2026-08-09; real-DAW UAT later)  
**Date:** 2026-08-09
