---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 2 — MT4 (session longevity / ~4 h soak)
author: Guillaume DUPONT
created: 2026-08-05
updated: 2026-08-05
---

# Smoke guide — Epic 2.5 (MT4 session longevity for ~4 h studio use)

> **Operator lab (French):** use [`smoke-epic2-mt4.md`](../smoke-epic2-mt4.md) §7. This file is the per-story English checklist (agents / validation matrix).

Use this checklist to prove and document that a **continuous studio/editor session of about four hours**, including **SysEx Session activity**, does **not** require a **mandatory Bridge restart** for normal operation.

Short clock / MTC / SysEx / Matrix-Control proofs live in their own Epic 2 guides. This document owns the **longevity contract**, the **stability sample plan**, and the **Pass/Fail matrix** for SM-3 / NFR-R1 / CAP-17 / AD-18.

Mark each validation line **Pass** or **Fail**. Leave blank when not yet run — **blank ≠ Pass**. On Fail, write English notes with **Port N / cable / direction**, Bridge build identity, and whether a restart was required. Any soak **Fail** must also get a `deferred-work.md` entry (or an in-path fix) before a later **Pass** may count.

## Longevity contract (V1)

**~4 hours** ≈ one long writing/editing block (PRD NFR-R1 / SM-3 / §10), including SysEx Sessions, without a **mandatory** Bridge restart for normal operation.

| Term | Meaning for Pass/Fail |
|---|---|
| Mandatory restart | Operator must kill/relaunch Bridge to keep using Virtual Ports under normal studio/editor traffic |
| Normal operation | Notes/CC, clock/MTC when in use, Matrix-Control / librarian SysEx at stock pacing — not intentional abuse (e.g. continuous nested-F0 flood). Soft pacing floor: **≥10 ms** between SysEx frames when using Matrix-Control stock pacing; never continuous flood |
| Supervised restart | Allowed for **hot-plug** recovery (Epic 3 / AD-10) — **not** an excuse for steady-state longevity Fail |
| Defect | Crash, leak, pump fail, hang, or forced restart attributable to Bridge — logged in `deferred-work.md` (or fixed in-path), **not** accepted as “usermode limits” |

**Nuance:** Story **2.3** already shipped bounded buffering (AD-18 / NFR-R3). This story binds the **time horizon** and **sample evidence**. It does **not** invent Studio-Done latency/jitter numbers (Epic **5** / OQ-2).

**SSOT citations:** PRD NFR-R1 / SM-3 / §10; architecture AD-18; epics Story 2.5; `_bmad-output/specs/spec-unitor-win64-driver/validation-matrix.md` (“Stability sample: ~4h…”).

## Bounded resources already in tree (do not redesign blindly)

| Resource | Bound | Notes |
|---|---|---|
| `HostOutboundQueue` | **128** messages / **128×400** bytes | Overflow → English pump failure (visible Fail) |
| `MidiMessageFramer` SysEx hold | `kMaxSysexHoldBytes = 1024` | Oversize / nested-F0 → reject + pump fail |
| teVirtualMIDI max SysEx | **65535** | Frames above create-port max are discarded by the driver |
| WinUSB session bulk timeout | **3000 ms** | Nonzero `PIPE_TRANSFER_TIMEOUT` — keep; do not set 0 without evidence |
| Session shape | One reader thread + CLI poll (~**50 ms**) | `--start-session` / `--run-midi` |
| Console heartbeat | Every **~3 s** | At least ~4800 lines / 4 h if redirected (more if multi-line ticks) — prefer **not** redirecting stdout for the soak; void the sample if logging stops mid-run (disk full / pipe stall) |

Pipeline (unchanged):

```text
DAW / MIDI-OX / Matrix-Control ↔ Virtual Ports ↔ VirtualMidiBackend
  ↔ MidiMessageFramer (device→host) / HostOutboundQueue (host→device)
  ↔ EmagicCableMapper ↔ WinUsbTransport ↔ MT4
```

Long-run entrypoint (unchanged — **do not** invent a mandatory `--soak` CLI unless lab proves the 3 s heartbeat / cancel path unusable for 4 h):

```text
Bridge --start-session   # alias: --run-midi
  → MidiSessionCli::runMt4MidiSession
  → DeviceSession::Start → reader thread + 50 ms CLI poll + 3 s heartbeat
  → Ctrl+C / cancel → Stop (join reader, DestroyPortSet, Close)
```

## Known restart / hang modes (soak watch list)

Treat these as **observe during the sample**. Any hit under normal pacing is a **defect**, not architecture destiny.

| Mode | Expected / risk |
|---|---|
| HostOutboundQueue overflow | Pump fail → session stops — Fail if it happens under normal editor pacing |
| Oversize / nested-F0 SysEx | Pump fail — Fail for normal dumps; intentional abuse is out of Pass bar |
| Incomplete SysEx hold with no `F7` | No idle timeout today. **Classify:** if Bridge stops pumping / Virtual Ports become unusable while a dump is held open → **Fail** (Bridge). If the host abandons an incomplete dump but Bridge and ports remain usable → note anomaly, **not** automatic Fail |
| `usbIoMutex_` held across decode | Possible stall under dense clock/SysEx load — observe; fix only with evidence |
| CTRL_CLOSE before `Stop` | May orphan Virtual Ports — **Ctrl+C only** for a valid sample. Console window close mid-sample → **void** the run (or Fail); clean orphan ports before re-run |
| After pump fail, ports until CLI Stop | ~50 ms poll then Stop — expected Fail→Stop, not silent death |
| Heartbeat every 3 s | Ops risk for redirected 4 h logs — avoid redirect; void if logging dies mid-soak |

**Forbidden excuses:** “usermode can’t do 4 h”, “VirtualMIDI limit”, “WinUSB always needs restart” — without measurement and a logged defect.

## Scope

**In scope (story 2.5)**

- Longevity design notes (this contract + bounds + Fail modes)
- Reproducible ~**4 hour** stability sample including **SysEx activity**
- Win10 x64 Pass/Fail matrix (mandatory); Win11 x64 when available
- Honest blank rows until lab runs; optional 30–60 min interim soak (does **not** close SM-3 alone)
- Defect logging for soak-discovered leak/restart/hang modes
- Lab-gated minimal Bridge fixes **only** if soak Fail points at Bridge

**Ownership / fences** (single table — do not duplicate elsewhere)

| Claim | Owner |
|---|---|
| Timing Clock / Start-Stop-Continue short smoke | **2.1** (`smoke-epic2-clock-mt4.md`) |
| MTC quarter-frame / full-frame short smoke | **2.2** (`smoke-epic2-mtc-mt4.md`) |
| Transparent SysEx pipe + short burst buffering | **2.3** (`smoke-epic2-sysex-mt4.md`) |
| Matrix-Control locked minimum vectors | **2.4** (`smoke-epic2-matrix-control-mt4.md`) |
| ~4 h longevity design / soak sample (this checklist) | **2.5** (this doc) |
| Auto-Start without daily Administrator | Epic **3.1** |
| Hot-plug recovery without reboot | Epic **3.2** ([`docs/tests/smoke-epic3-hotplug-mt4.md`](../smoke-epic3-hotplug-mt4.md)) |
| Multi-client DAW + MIDI-OX (SM-7) | Epic **3.3** ([`docs/tests/smoke-epic3-multiclient-mt4.md`](../smoke-epic3-multiclient-mt4.md)) |
| MIDI Path latency/jitter / Studio-Done numbers | Epic **5** / OQ-2 |
| Linking Matrix-Control into the Bridge | Forbidden — CAP-8 |

**Failure definition for this story:** Bridge-induced forced restart, crash, hang, leak, or unusable Virtual Ports before ~4 h under a normal studio/editor scenario that **includes SysEx activity**. Not Epic 5 p99 thresholds, not optional bank-stress hard fail, not hot-plug (→ 3.2), not “DAW host crashed so Bridge must restart” without Bridge fault evidence.

## Prerequisites

1. Epic 1 notes/CC smoke green on ≥1 IN and ≥1 OUT (`docs/tests/smoke-epic1-mt4.md`).
2. Story **2.3** pipe + short burst green (synthetic minimum: `Bridge --test-mapper` exit 0; prefer hardware burst green).
3. Prefer Story **2.4** hard gates green — or record honesty if Matrix-Control lab still open. Longevity may use **any** host that can send librarian-scale SysEx (MIDI-OX / SysEx tool / DAW / Matrix-Control); **record which host**.
4. Stories **2.1** / **2.2**: synthetic gates green; include clock and/or MTC in the soak when those lab rows are green — do not block the **design** on blank DAW rows.
5. **Windows 10 x64 is mandatory** to close SM-3 / NFR-R1. Win11 x64 is additional when available — a Win11-only lab **cannot** close SM-3.
6. WinUSB-bound MT4 + teVirtualMIDI present (same lab path as Epic 1).
7. Bridge build identity recorded (commit hash / build path).
8. **Host power policy:** disable sleep / hibernate and USB selective suspend for the soak machine; keep the session unlocked. If the host slept or USB suspended during the wall-clock window → **void** or **Fail** the sample (do not blame Bridge without evidence).

## Synthetic gate (no hardware)

Run before claiming readiness to soak:

```text
Bridge --test-mapper
```

Expect exit 0 (SysEx + realtime + MTC vectors). This does **not** close SM-3 / NFR-R1.

Also (when C++ changed in the same work item):

```text
ctest --test-dir builds/<config>   # or run BridgeTests
python scripts/quality/lint-touched.py
```

## Stability sample procedure (~4 h)

### Vehicle

Leave Bridge running for the wall-clock sample:

```text
Bridge.exe --start-session
```

or:

```text
Bridge.exe --run-midi
```

**Ctrl+C only** for a valid stop (clean `Stop`). Console window close (CTRL_CLOSE) mid-sample → **void** the run; if orphan Virtual Ports remain, destroy/restart cleanly before the next attempt. Prefer **not** redirecting Bridge stdout for the full soak; if redirected and logging stops mid-run (disk full / broken pipe) → **void** the sample.

### Activity mix (must include SysEx)

Interleave over ~4 hours (stock pacing, not abuse — soft floor **≥10 ms** between SysEx frames; never continuous flood):

1. Ordinary notes/CC on ≥1 Virtual IN and ≥1 Virtual OUT — **at least once per hour** (spot-check).
2. **SysEx activity** (required): Inquiry / patch-shaped (~275 B) / short Matrix-Control or MIDI-OX SysEx exchanges — **at least once per hour**, both directions when feasible.
3. **Mid-soak health** (~every hour): confirm Virtual Ports still answer a short notes/CC exchange without restarting Bridge.
4. Clock and/or MTC welcome when those lab rows are green (do not invent Fail if clock lab still blank — note Skip).
5. At end: Virtual Ports still usable for a short notes/CC (and preferably one SysEx) exchange **without** having restarted Bridge.

Optional interim **confidence soak** (30–60 min) may be recorded below — it does **not** replace the ~4 h sample for AC closure.

### Pass bar

- Session reaches **≥ 3 h 45 min** wall clock (target ~4 h) without **mandatory** Bridge restart for normal operation. Shorter elapsed time → **Fail** (or blank), not Pass.
- Virtual Ports remain usable at end.
- No Bridge-induced silent MIDI death that forces a restart to recover.
- SysEx activity occurred during the sample on the required cadence (not notes-only / not a single one-shot at start).
- **Record** table below is filled (Bridge commit, OS, start/end wall times minimum) **before** any Matrix **Pass**.

### Fail bar

- Bridge process crash.
- Pump failure that stops the session under normal pacing.
- Forced restart to restore ports.
- Unbounded growth / resource exhaustion attributable to the Bridge — observe Task Manager (or equivalent) at start, mid (~2 h), and end: progressive Working Set / handle growth that forces a restart is Fail; note values in Anomalies.
- SysEx dumps that require restart mid-session under normal editor pacing.
- Host sleep / USB suspend during the window (void or Fail — not a Bridge longevity Pass).

### Record (English)

| Field | Value |
|---|---|
| Bridge build / commit | |
| OS | |
| Start wall time | |
| End wall time | |
| Elapsed (must be ≥ 3 h 45 for ~4 h Pass) | |
| Hosts used (DAW / MIDI-OX / Matrix-Control / …) | |
| Ports exercised (Port N / cable) | |
| SysEx activity summary (cadence met?) | |
| Resource spot-checks (start / mid / end) | |
| Anomalies | |

**Gate:** Matrix Result may be **Pass** only when Bridge commit + start/end wall times (and elapsed ≥ 3 h 45 for the ~4 h row) are filled above.

## Matrix

| OS | Scenario | Duration | SysEx included? | Host(s) | Result (Pass/Fail) | Notes |
|---|---|---|---|---|---|---|
| Win10 x64 | Continuous studio/editor soak (notes/CC + SysEx) | ~4 h (≥ 3 h 45) | **Yes (required)** | | | |
| Win10 x64 | Optional interim confidence soak | 30–60 min | Yes | | | Interim only — does not close SM-3 alone |
| Win11 x64 | Continuous studio/editor soak | ~4 h (≥ 3 h 45) | Yes | | | Skip/N/A when hardware absent; **cannot** close SM-3 alone |

**Honesty bar:** Do **not** claim SM-3 / NFR-R1 closed from synthetic-only work, a short smoke, or a **Win11-only** lab. Win10 x64 ~4 h **Pass** is mandatory to close SM-3 / NFR-R1. Win11 absence must **not** block a Win10 Pass. If a soak **Fails** and a later run **Passes**, the Fail must already be logged in `deferred-work.md` (or fixed in-path).

## Bilan

| Item | Status |
|---|---|
| Longevity contract documented (restart policy, bounds, Fail modes) | Pass (design landed 2026-08-05) |
| Win10 x64 ~4 h sample plan with SysEx (this matrix) | Pass (plan landed 2026-08-05) |
| Win10 x64 ~4 h sample executed | |
| Optional 30–60 min interim soak | |
| Win11 x64 (when available) | |
| Soak defects logged (no “usermode alibi”) | |
| SM-3 / NFR-R1 closed (requires Win10 ~4 h sample Pass) | Open — sample not yet run |
| Synthetic `Bridge --test-mapper` still green | Pass (2026-08-05) |
| No Matrix-Control linked into Bridge (CAP-8) | Pass |
| No Studio-Done latency/jitter numbers invented | Pass |
