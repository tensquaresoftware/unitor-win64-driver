---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 2 — MT4 (transparent SysEx / burst buffering)
author: Guillaume DUPONT
created: 2026-08-05
updated: 2026-08-05
---

# Smoke guide — Epic 2.3 (MT4 transparent SysEx transport with burst buffering)

> **Operator lab (French):** use [`smoke-epic2-mt4.md`](../smoke-epic2-mt4.md) §5. This file is the per-story English checklist (agents / validation matrix).

Use this checklist to prove **librarian-sized SysEx** (including Oberheim Matrix-shaped frames) through MT4 Virtual Ports in both directions, with enough burst buffering that a short bank-export-scale sequence completes **without Bridge restart**.

Clock / transport proof lives in `docs/tests/checklists/smoke-epic2-clock-mt4.md` (story 2.1). MTC proof lives in `docs/tests/checklists/smoke-epic2-mtc-mt4.md` (story 2.2). Matrix-Control locked minimum vectors live in `docs/tests/checklists/smoke-epic2-matrix-control-mt4.md` (story 2.4). This document owns **SysEx transport + short burst buffering only**.

Mark each validation line **Pass** or **Fail**. On Fail, write English notes with **Port N / cable / direction**.

## Scope

**In scope (story 2.3)**

- Transparent carry of SysEx between Virtual Ports and MT4 cables (no Emagic-side rewrite of Oberheim payloads)
- Librarian-sized frames both directions: Inquiry (6 B), Inquiry reply (~15 B), patch-shaped (~275 B), master-shaped (~351 B)
- Short burst (≥ several 275 B frames, or equivalent) without Bridge restart
- Incomplete / corrupt / oversize dumps treated as **failures** (English diagnostics), not silent success
- Win10 x64 = mandatory matrix row; Win11 x64 when hardware available

**Out of scope**

- Timing Clock / Start-Stop-Continue → story 2.1 (`smoke-epic2-clock-mt4.md`)
- MTC quarter-frame / full-frame sync → story 2.2 (`smoke-epic2-mtc-mt4.md`)
- Matrix-Control **locked minimum pass-vector table** (Inquiry + patch + master + push + live edits + mixed-wire) → story **2.4** (`smoke-epic2-matrix-control-mt4.md`)
- ~4 h longevity / soak sample → story **2.5** (`smoke-epic2-longevity-mt4.md`)
- Latency / jitter Studio-Done numbers → Epic 5
- Linking Matrix-Control as a Bridge runtime dependency (forbidden — CAP-8)
- Building a SysEx librarian UI or Oberheim parser in the Bridge

**Important:** Passing synthetic 275 / 351 B frames here proves the **pipe + buffers**. It does **not** claim Story 2.4 / SM-2 Matrix-Control minimum vectors done.

**Failure definition:** Bridge-induced truncation, merge, rewrite, silent drop, or forced Bridge restart under normal librarian-scale SysEx (single 275/351 B frames and short bursts on the order of bank-export pacing). Not a multi-hour soak, not Epic 5 p99 thresholds.

## Prerequisites

1. Epic 1 notes/CC smoke green on ≥1 IN and ≥1 OUT (`docs/tests/smoke-epic1-mt4.md`).
2. Prefer stories 2.1 / 2.2 synthetic gates green (`Bridge --test-mapper`); hardware clock/MTC rows may still be pending.
3. Windows 10 x64 (mandatory matrix row) or Windows 11 x64 when that hardware is available.
4. WinUSB-bound MT4 + teVirtualMIDI present (same lab path as Epic 1).
5. Host options for **this** story: ShowMIDI / SysEx file sender / DAW SysEx, **and/or** Matrix-Control when installed. Matrix-Control is welcome but **not** required to close synthetic work.
6. Bridge build that includes story 2.3 framer/mapper SysEx vectors (`Bridge --test-mapper` exit 0).

## Synthetic gate (no hardware)

Run before hardware:

```text
Bridge --test-mapper
```

Expect exit 0. This includes Device Inquiry, 275 B patch-shaped, 351 B master-shaped, split Push / interleave, oversize reject observability, and opaque Emagic encode/decode of those frames.

Also:

```text
ctest --test-dir builds/<config>   # or run BridgeTests
```

Expect Pass, including Catch2 `[framer][sysex]`, `[mapper][sysex]`, and `[queue][sysex]` cases.

## Hardware procedure

1. Start Bridge session (`--start-session` / `--run-midi` per Epic 1 lab habit).
2. Confirm Computer Mode is active (channel CC kick already in session start — **SysEx alone does not wake Computer Mode**).
3. Pick ≥1 Virtual IN and ≥1 Virtual OUT (record Port N / cable).
4. **Device → host:** send librarian-sized SysEx into MT4 physical IN; observe complete frames on the matching Virtual IN (ShowMIDI / SysEx tool / Matrix-Control).
5. **Host → device:** send librarian-sized SysEx to Virtual OUT; observe complete frames on MT4 physical OUT (LED / external MIDI monitor / Matrix-Control).
6. **Short burst:** send several sequential ~275 B frames (or equivalent bank-export pacing, ≥10 ms between frames if using Matrix-Control stock pacing) without restarting Bridge.
7. On any Fail, capture English Bridge diagnostics (Port N / cable / direction) and whether the dump was truncated, dropped, or forced a restart.

## Matrix

| OS | Direction | Size / scenario | Host tool | Result (Pass/Fail) | Notes (Port N / cable / direction) |
|---|---|---|---|---|---|
| Win10 x64 | Device → host | Inquiry (6 B) | | | |
| Win10 x64 | Device → host | Patch-shaped (~275 B) | | | |
| Win10 x64 | Device → host | Master-shaped (~351 B) | | | |
| Win10 x64 | Host → device | Inquiry (6 B) | | | |
| Win10 x64 | Host → device | Patch-shaped (~275 B) | | | |
| Win10 x64 | Host → device | Master-shaped (~351 B) | | | |
| Win10 x64 | Both / burst | ≥ several 275 B frames, no Bridge restart | | | |
| Win11 x64 | (fill when hardware available) | | | | |

## Explicit fences

| Claim | Owner |
|---|---|
| Transparent SysEx pipe + burst buffering (this checklist) | **2.3** (this doc) |
| Matrix-Control locked minimum vectors (Inquiry + patch + master + push + live edits + mixed-wire) | **2.4** (`smoke-epic2-matrix-control-mt4.md`) |
| ~4 h longevity design / soak sample | **2.5** (`smoke-epic2-longevity-mt4.md`) |
| Full-frame MTC sync | **2.2** (`smoke-epic2-mtc-mt4.md`) |
| MIDI Path latency/jitter harness | Epic **5** |

## Bilan

| Item | Status |
|---|---|
| Synthetic `Bridge --test-mapper` (SysEx vectors) | |
| Catch2 / `BridgeTests` SysEx + queue cases | |
| Win10 x64 hardware matrix (≥1 IN + ≥1 OUT + short burst) | |
| Win11 x64 (when available) | |
| No Matrix-Control linked into Bridge build (CAP-8) | |
