---
organization: Ten Square Software
project: unitor-win64-driver
title: MIDI Path measurement method
author: Guillaume DUPONT
created: 2026-08-11
updated: 2026-08-11
---

# MIDI Path measurement method

> **Studio-Done Gate — timing anchors confirmed** — Method remains the locked measurement contract. Gate **2026-08-11** outcome **(a)** — [`studio-done-gate-decision.md`](studio-done-gate-decision.md). Healthy ≤4–5 ms p99 latency and ≤1–2 ms p99 classical jitter stand from published hardware-loop evidence. Do **not** mark ASIO as proof.

This document locks **how** we measure bridge-relevant MIDI Path latency for Epic 5. Latest numbers live in [`baseline-latest.md`](baseline-latest.md). Index: [`README.md`](README.md).

## Locked timestamp plane

One series only — do **not** mix stamp sources:

| Locked | Forbidden in the same series |
|---|---|
| Host **WinMM** client against Bridge **Virtual Ports** | MidiBackend-internal timestamps |
| Inject **QPC** (`QueryPerformanceCounter`) immediately before `midiOutShortMsg` | WinUSB URB completion stamps |
| Observe QPC in the midiIn callback on the matching Note On | Sleep-as-timer, wall-clock-only intervals |

Plane label in harness output: `plane=host-winmm-qpc` (plain) / `"plane":"host-winmm-qpc"` (JSON). JSON also carries `asio_buffer_proof: false` and `studio_done: false` — markdown tables must mirror that honesty.

Reference: [Acquiring high-resolution time stamps](https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps).

## Path types

### `software-loop`

| Item | Contract |
|---|---|
| Bridge | Soft-echo **ON**: `Bridge.exe --start-session --soft-echo` or `UNITOR_MIDI_SOFT_ECHO=1` |
| Harness | `--path software-loop` |
| What it measures | Virtual Port round-trip only (host WinMM → Bridge soft-echo → host WinMM) |
| What it does **not** measure | USB / WinUSB / DIN / full MT4 cable path |

Soft-echo **skips USB/DIN**. Never describe software-loop µs as full MT4 / WinUSB path latency.

### `hardware-loop`

| Item | Contract |
|---|---|
| Bridge | Soft-echo **OFF** — `--no-soft-echo` wins over a sticky `UNITOR_MIDI_SOFT_ECHO` |
| Physical | DIN Out → In present on the measured cable |
| Harness | `--path hardware-loop --confirm-soft-echo-off` (confirm flag required) |
| Missing DIN | Honest **fail** — never invent Pass |
| Blank row | Not run ≠ Pass |

Do **not** mix software-loop and hardware-loop samples into one unlabeled p99 series.

## Harness CLI (source of truth for metrics)

Typical binary: `builds/ci/tools/midi-path-harness/Release/MidiPathHarness.exe` (or `builds/debug/...`).

| Flag | Role |
|---|---|
| `--path software-loop\|hardware-loop` | Path type |
| `--out` / `--in` | Virtual port names (e.g. `MT4 Out 1` / `MT4 In 1`) |
| `--samples` | Sample count (default **100**; early capsule used 50) |
| `--timeout-ms` | Per-sample wait (default **2000**; min **1**) |
| `--json` | Machine-readable summary |
| `--confirm-soft-echo-off` | **Required** for hardware-loop |

Preferred teardown for Bridge: **Ctrl+C** (`CTRL_CLOSE` can orphan Virtual Ports).

### Summary fields used as sources of truth

| Field | Meaning |
|---|---|
| `latency_us_min` | Minimum sample (µs) |
| `latency_us_mean` | Arithmetic mean (µs) |
| `latency_us_p99` | 99th percentile (µs) — harness index `n*99/100` on the sorted series (see honesty note below) |
| `latency_us_max` | Maximum sample (µs) |
| `latency_us_median` | Median sample (µs) |
| `latency_spread_us` | `latency_us_p99 − latency_us_min` (labeled spread — **not** classical jitter) |
| `jitter_us_mean` / `jitter_us_p99` / `jitter_us_max` | Classical jitter: abs-dev-from-median series |
| `jitter_def` | Must be `p99_abs_dev_from_median` for published classical jitter claims |
| `path_type` | `software-loop` or `hardware-loop` |
| `plane` | Must remain `host-winmm-qpc` |
| `asio_buffer_proof` | Always `false` in published claims |
| `studio_done` | Harness JSON stays `false` for a single run (run ≠ Gate decision). Gate timing claim lives in [`studio-done-gate-decision.md`](studio-done-gate-decision.md). Plain/help must not deny a published Gate **(a)** claim. |

The harness emits classical `jitter_us_*` fields. See definition below.

**p99 honesty:** the harness sorts samples ascending and takes index `n*99/100` (integer division). At small `n` (e.g. **50**), that index can land on the last sample, so published `latency_us_p99` / `jitter_us_p99` may **equal** the series max. Prefer the harness default `--samples 100` for stronger percentile evidence.

## Classical jitter (harness)

**Definition (product-adopted for NFR-P2):**

```text
abs_dev_us     = |latency_us_sample − latency_us_median|
jitter_us_p99  = p99 of abs_dev_us   (same n*99/100 index rule)
jitter_def     = p99_abs_dev_from_median
```

Also published: `jitter_us_mean`, `jitter_us_max`.

### Labeled spread (not classical jitter)

```text
latency_spread_us = latency_us_p99 − latency_us_min
```

| Honesty | Statement |
|---|---|
| What spread is | A clearly labeled **spread** of the latency series |
| What it is **not** | Classical jitter — do **not** score `latency_spread_us` against ≤1–2 ms p99 |
| What clears NFR-P2 | `jitter_us_p99` (or a future equivalent with explicit product sign-off) |

## Required metadata columns (every published row)

Blank / not-run stubs may leave metric cells as `N/A` / `—`. **Complete** published rows (a real completed run) must include every column below.

| Column | Requirement |
|---|---|
| Host OS | Win10 x64 **minimum** to close a measurement claim (AD-13). Non-Win10 rows may be recorded for discussion but are **non-closing**. |
| Bridge build / version | Version string and commit or artifact path when known |
| Harness build / path | Binary path or version used for the numbers (when known) |
| virtualMIDI presence | yes / no. **`no` is non-closing** for Epic 5 interim-backend measurement claims — do not treat as a closeable MIDI Path row. |
| `path_type` | `software-loop` or `hardware-loop` |
| Sample count | Integer from the run |
| Date / UTC | Run timestamp in **UTC** (ISO-8601 / capsule `…Z` stamp preferred; local-only is not enough for a closeable row) |
| Plane / honesty flags | `plane=host-winmm-qpc`, `asio_buffer_proof=false`, `studio_done=false` |
| Raw evidence | Pointer under `docs/tests/lab-evidence/` when a capsule exists (prefer the exact summary log file) |

Optional human-friendly ms beside µs is fine; keep harness field names as source IDs.

## Anti-ASIO / anti-audio-buffer proof

**Forbidden:** citing ASIO buffer size, WASAPI buffer size, or any audio host buffer as MIDI Path proof.

MIDI Path proof is **only** the locked QPC WinMM ↔ Virtual Port series described above (NFR-P3 / SM-C2).

## Confirmed timing anchors (Studio-Done Gate)

Studio-Done Gate **2026-08-11** outcome **(a)** — anchors confirmed from published hardware-loop + classical jitter:

| Anchor | Confirmed band | Status |
|---|---|---|
| Healthy bridge-added latency | ≤ ~4–5 ms p99 | **Confirmed** |
| Classical jitter | ≤ ~1–2 ms p99 (`jitter_us_p99`) | **Confirmed** |
| Do-not-ship-worse | ~8–10 ms p99 | Ceiling unchanged — shipping above requires explicit product decision |

**Measurement plane:** Gate confirm evidence is published **hardware-loop** (Virtual Ports ↔ Bridge ↔ MT4 ↔ physical DIN). That includes device + cable delay — **not** a separated usermode-only “beyond host USB” delta. **Lab caveats:** single quiet-lab Out2→In2 path (n=100); not a DAW-session guarantee.

Excessive jitter is not a usermode alibi (SM-C4). Decision record: [`studio-done-gate-decision.md`](studio-done-gate-decision.md).

## Known confounders (document — do not “fix” here)

- Soft-echo skips USB/DIN — software-loop is plumbing / Virtual Port RT only
- Under load, bulk IN / USB I/O contention may affect **hardware-loop** credibility — first baselines: quiet lab
- CTRL_CLOSE can orphan Virtual Ports — prefer Ctrl+C teardown
- Mixing path types in one unlabeled series invalidates the claim

## Cross-links

| Artifact | Path |
|---|---|
| Latest tables | [`baseline-latest.md`](baseline-latest.md) |
| Studio-Done Gate decision | [`studio-done-gate-decision.md`](studio-done-gate-decision.md) |
| Smoke guide | [`docs/tests/smoke-epic5-midi-path-harness-mt4.md`](../../tests/smoke-epic5-midi-path-harness-mt4.md) |
| Seed capsule (software-loop) | [`docs/tests/lab-evidence/midi-path-harness-software-loop-2026-08-11/`](../../tests/lab-evidence/midi-path-harness-software-loop-2026-08-11/) |
| Gate-confirm capsule (hardware-loop + classical jitter) | [`docs/tests/lab-evidence/midi-path-harness-hardware-loop-2026-08-11/`](../../tests/lab-evidence/midi-path-harness-hardware-loop-2026-08-11/) |
| Lab evidence index | [`docs/tests/lab-evidence/README.md`](../../tests/lab-evidence/README.md) |
| Harness sources | `tools/midi-path-harness/` |
