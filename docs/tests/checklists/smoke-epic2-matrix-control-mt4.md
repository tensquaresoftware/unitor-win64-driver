---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 2 — MT4 (Matrix-Control minimum SysEx pass vectors)
author: Guillaume DUPONT
created: 2026-08-05
updated: 2026-08-05
---

# Smoke guide — Epic 2.4 (Matrix-Control minimum SysEx pass vectors)

> **Operator lab (French):** use [`smoke-epic2-mt4.md`](../smoke-epic2-mt4.md) §6. This file is the per-story English checklist (agents / validation matrix).

Use this checklist to prove the **locked Matrix-Control minimum SysEx pass vectors** through MT4 Virtual Ports on Windows, without Bridge restart for normal librarian completion.

Transparent SysEx pipe + short burst buffering lives in `docs/tests/checklists/smoke-epic2-sysex-mt4.md` (story 2.3). This document owns **SM-2 / FR-8 Matrix-Control vectors only**.

**SSOT sources**

- `_bmad-output/specs/spec-unitor-win64-driver/validation-matrix.md` — section **Minimum SysEx pass vectors**
- `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/matrix-control-sysex-extract.md` — §5 (note: PRD / Story 2.4 / AD-16 elevated **Master dump** to a locked minimum vector; prefer that over the extract “second-tier” footnote)

**Numbering:** Always use **validation-matrix IDs** in Result notes and Fail diagnostics (`#1`…`#7`). Story AC lists the six hard gates as enumerated items 1–6; AC item **6** = validation-matrix **#7** (mixed-wire). Optional bank stress is always validation-matrix **#6** (not a hard gate).

**Result rules**

- Win10 hard gates (`#1`–`#5`, `#7`): **Pass** or **Fail** only — do not Skip.
- Optional bank (`#6`): **Pass**, **Skip**, or **Fail** (Skip is OK).
- Win11 rows: fill when that lab machine exists; otherwise mark **Skip** / **N/A** — do not block Win10 Pass on Win11 absence.

On Fail, write English notes with **Port N / cable / direction / validation-matrix vector # / Bridge build identity (path or commit)**.

## Scope

**In scope (story 2.4)**

- Hard gates (validation-matrix numbering):
  1. Device Inquiry round-trip
  2. Single patch dump (~275 B)
  3. Master dump (~351 B)
  4. Edit-buffer / patch push (outbound ~275 B; slot `01` and/or edit-buffer `0D`)
  5. Live editor stream (7 B / 9 B remote edits at stock Matrix-Control spacing)
  7. Mixed-wire tolerance (non-patch SysEx during a dump must not permanently block a later valid patch frame)
- Optional bank stress (validation-matrix #6): ≈100× sequential 275 B — record when feasible; **not** a hard gate
- Win10 x64 = mandatory matrix column; Win11 x64 = fill when hardware available
- Lab-gated Bridge fixes only if a vector Fails (smallest wrong layer)

**Out of scope**

- Transparent SysEx pipe + short burst buffering already shipped in story **2.3** (`smoke-epic2-sysex-mt4.md`)
- Timing Clock / Start-Stop-Continue → story 2.1 (`smoke-epic2-clock-mt4.md`)
- MTC quarter-frame / full-frame → story 2.2 (`smoke-epic2-mtc-mt4.md`)
- ~4 h longevity / soak sample → story **2.5** (`smoke-epic2-longevity-mt4.md`)
- Latency / jitter Studio-Done numbers → Epic **5**
- Linking Matrix-Control as a Bridge runtime dependency (forbidden — CAP-8)
- Oberheim parser / checksum validator / SysexEngine in the Bridge
- Making optional bank stress a hard gate
- Matrix-6/6R member-byte hardware proof; split-patch `02H` primary flows
- Emagic escaping for `0xF5` / `0xFF` (still deferred; Matrix frames use mfr `0x10`)

**Important:** Passing synthetic 275 / 351 B frames in story 2.3 proves the **pipe + buffers**. It does **not** claim this checklist / SM-2 done.

**Failure definition:** Matrix-Control cannot complete a hard-gate vector because of Bridge-induced truncation, merge, rewrite, silent drop, permanent mixed-wire block, or forced Bridge restart under normal librarian use. Not a multi-hour soak, not Epic 5 p99 thresholds, not bank-stress hard fail.

**Host timeout note:** If Matrix-Control’s ~2 s RPC times out but Bridge logs / a second capture show the frame arrived intact on the Virtual Port, record that as a **host-timeout note** — not an automatic Bridge Fail. Bridge Fail requires evidence of truncation, merge, drop, rewrite, mixed-wire permanent block, or forced restart.

## Prerequisites

1. Story **2.3** transport smoke green (or at least synthetic + short burst): `docs/tests/checklists/smoke-epic2-sysex-mt4.md` and `Bridge --test-mapper` exit 0.
2. Epic 1 notes/CC smoke green on ≥1 IN and ≥1 OUT (`docs/tests/smoke-epic1-mt4.md`).
3. **Matrix-Control** installed on the lab PC (external host only — not part of this repo).
4. **Matrix-1000** preferred on a physical MT4 cable. Other Oberheim Matrix gear (“equivalent”) may run vectors **#2–#5 / #7**, but Device Inquiry (**#1**) Pass requires Universal reply identity bytes for M-1000 (`mfr 10`, family `06 00`, member `02 00`) — otherwise document alternate identity bytes and mark **#1** Skip (do not Fail the Bridge for a different member ID).
5. WinUSB-bound MT4 + teVirtualMIDI present (same lab path as Epic 1).
6. Computer Mode wake still via channel CC kick — **SysEx alone does not wake Computer Mode**.
7. **Windows 10 x64** is required to close hard-gate Pass for SM-2. Windows 11 x64 rows are additional when that hardware is available — Win11 alone does not close the mandatory column.
8. Bridge build that includes story 2.3 SysEx / queue / framer vectors (record build path or git commit in the matrix Notes / Fail notes).

## Synthetic gate (no hardware)

Run before hardware (regression only — does not close SM-2):

```text
Bridge --test-mapper
```

Expect exit 0 (includes 2.3 SysEx + 2.1/2.2 realtime/MTC vectors).

Also:

```text
ctest --test-dir builds/<config>   # or run BridgeTests
```

Expect Pass, including Catch2 `[framer][sysex]`, `[mapper][sysex]`, `[queue][sysex]`, realtime, and MTC cases.

**CAP-8:** Matrix-Control must not be linked, submoduled, or `#include`d in the Bridge.

## Hardware procedure (Matrix-Control)

1. Start Bridge session (`--start-session` / `--run-midi` per Epic 1 lab habit). Record Bridge build path or commit.
2. Confirm Computer Mode is active (channel CC kick — SysEx alone does not wake).
3. Open Matrix-Control; select the matching Virtual Ports (record Port N / cable).
4. Execute each hard-gate vector below at **stock Matrix-Control pacing**:
   - Remote param (`06H`): ≥10 ms between SysEx for M-1000.
   - Matrix Mod (`0BH`): expect host coalescing around **~25 ms**; do not force a denser stream.
   - If you intentionally underspace (<10 ms), **invalidate** the run and retest at stock spacing — do not Fail the Bridge on an underspaced trial.
   - Default Matrix-Control RPC timeout ~2000 ms.
5. On any Fail, capture English Bridge diagnostics (**Port N / cable / direction / validation-matrix vector # / Bridge build**) and whether the frame was truncated, merged, dropped, mixed-wire blocked, or forced a restart.
6. Optional: bank stress ≈100× sequential 275 B when time/hardware allow — record Pass/Skip/Fail; Skip is OK.
7. Fill Win11 rows when that lab machine is available; otherwise Skip/N/A — do not block Win10 Pass on Win11 absence.

### Locked vector reference + Matrix-Control actions

| # | Vector | Shape (summary) | Approx size | Hard gate? | Matrix-Control action (lab) |
|---|---|---|---|---|---|
| 1 | Device Inquiry round-trip | Host `F0 7E 7F 06 01 F7` → Universal reply (mfr `10`, family `06 00`, member `02 00` for M-1000) | 6 → 15 B | Yes | Open / reconnect device detect so Matrix-Control sends Inquiry; Pass = UI detects device **and** reply identity matches M-1000 bytes (or documented alternate + Skip policy above) |
| 2 | Single patch dump | Request `F0 10 06 04 01 <n> F7` → exactly **275-byte** `F0 10 06 01 … F7` within ~2 s; no truncation/merge | **275 B** | Yes | After typical nav (Set Bank + Program Change when used), request / receive a single patch dump; confirm complete 275 B patch frame |
| 3 | Master dump | Request `F0 10 06 04 03 00 F7` → **351-byte** `F0 10 06 03 … F7`; no Bridge restart | **351 B** | Yes | Request Master parameters dump; confirm 351 B master frame completes without Bridge restart |
| 4 | Edit-buffer / patch push | Outbound **275 B** write for slot opcode `01` **and/or** edit-buffer `0D` (literal `00` after `0DH`); synth accepts | **275 B** | Yes | Write / store a patch to a slot **and/or** push edit-buffer (`0D` — prefer when convenient; known regression surface). Pass = either path with synth accept (sound / UI) |
| 5 | Live editor stream | Sustained remote param (`06H`, 7 B) and/or Matrix Mod (`0BH`, 9 B) at stock spacing; no restart / obvious drop-reorder | 7 / 9 B | Yes | Move knobs / Matrix Mod in the editor at stock spacing for a short sustained stream; Pass = either message class without Bridge restart or obvious drop/reorder |
| 6 | Bank stress (optional) | ≈100× sequential 275 B patch frames (~28 KB inbound series), ≥10 ms pacing | ~28 KB series | **No** | Bank export/import-scale sequence when time allows; Skip OK |
| 7 | Mixed-wire tolerance | During dump wait, non-patch SysEx must not permanently block a later valid patch frame | — | Yes | **Recipe:** start a single patch dump (#2) and, while Matrix-Control is still waiting for the patch frame, inject non-patch SysEx on the same Virtual Port (Device Inquiry `F0 7E 7F 06 01 F7` from a second sender, or trigger another Inquiry/detect). Then ensure a later valid patch dump still completes intact — Matrix-Control keeps listening; Bridge must deliver the later patch without permanent block or restart |

## Matrix — hard gates + optional bank

| OS | # | Vector | Result (Pass/Fail/Skip) | Notes (Port N / cable / direction / vector # / Bridge build) |
|---|---|---|---|---|
| Win10 x64 | 1 | Device Inquiry round-trip | | |
| Win10 x64 | 2 | Single patch dump (~275 B) | | |
| Win10 x64 | 3 | Master dump (~351 B) | | |
| Win10 x64 | 4 | Edit-buffer / patch push (~275 B; `01` and/or `0D`) | | |
| Win10 x64 | 5 | Live editor stream (7 B / 9 B) | | |
| Win10 x64 | 7 | Mixed-wire tolerance | | |
| Win10 x64 | 6 | Bank stress ≈100× 275 B (optional) | | |
| Win11 x64 | 1 | Device Inquiry round-trip | | |
| Win11 x64 | 2 | Single patch dump (~275 B) | | |
| Win11 x64 | 3 | Master dump (~351 B) | | |
| Win11 x64 | 4 | Edit-buffer / patch push (~275 B; `01` and/or `0D`) | | |
| Win11 x64 | 5 | Live editor stream (7 B / 9 B) | | |
| Win11 x64 | 7 | Mixed-wire tolerance | | |
| Win11 x64 | 6 | Bank stress ≈100× 275 B (optional) | | |

## Explicit fences

| Claim | Owner |
|---|---|
| Transparent SysEx pipe + short burst buffering | **2.3** (`smoke-epic2-sysex-mt4.md`) |
| Matrix-Control locked minimum vectors (this checklist) | **2.4** (this doc) |
| ~4 h longevity design / soak sample | **2.5** (`smoke-epic2-longevity-mt4.md`) |
| Timing Clock / Start-Stop-Continue | **2.1** (`smoke-epic2-clock-mt4.md`) |
| Full-frame / quarter-frame MTC sync | **2.2** (`smoke-epic2-mtc-mt4.md`) |
| MIDI Path latency/jitter harness | Epic **5** |

## Bilan

Task-1 provisional snapshot (does **not** close SM-2 / Task 5 Windows lab regression):

| Item | Status |
|---|---|
| Synthetic `Bridge --test-mapper` (SysEx + realtime + MTC) | Pass provisional (2026-08-05, macOS host, `builds/debug`) — re-confirm on Windows lab before Task 5 |
| Catch2 / `BridgeTests` `[framer][sysex]`, `[mapper][sysex]`, `[queue][sysex]` | Pass provisional (46 cases / 521 assertions, same host) — re-confirm on Windows lab before Task 5 |
| Win10 x64 hard gates 1–5 + mixed-wire (#7) | |
| Win10 x64 optional bank stress (#6) | |
| Win11 x64 (when available; else Skip/N/A) | |
| No Matrix-Control linked into Bridge build (CAP-8) | Pass provisional (no refs in `src/` / `CMakeLists.txt`, 2026-08-05) — re-confirm at Task 5 |
| No claim of ~4 h longevity (→ 2.5 / `smoke-epic2-longevity-mt4.md`) | Confirmed |
| No Studio-Done latency numbers (→ Epic 5) | Confirmed |
