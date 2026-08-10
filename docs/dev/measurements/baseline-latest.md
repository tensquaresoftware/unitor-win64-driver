---
organization: Ten Square Software
project: unitor-win64-driver
title: MIDI Path baseline tables (latest)
author: Guillaume DUPONT
created: 2026-08-11
updated: 2026-08-11
---

# MIDI Path baseline tables (latest)

> **Studio-Done Gate — timing anchors confirmed** — Gate **2026-08-11** outcome **(a)** — [`studio-done-gate-decision.md`](studio-done-gate-decision.md). Healthy ≤4–5 ms p99 latency and ≤1–2 ms p99 classical jitter stand from Win10 hardware-loop evidence. Do **not** cite ASIO. Do **not** score `latency_spread_us` as NFR-P2.

Method: [`method-midi-path.md`](method-midi-path.md). Index: [`README.md`](README.md).

## How to read these tables

- Metrics come from harness summary fields (`latency_us_*`, `jitter_us_*`, `path_type`, `plane`).
- **Classical jitter:** `jitter_us_p99` = p99 of \|sample − median\| (`jitter_def=p99_abs_dev_from_median`).
- **Labeled spread (not classical jitter):** `latency_spread_us = latency_us_p99 − latency_us_min` — do **not** score against ≤1–2 ms jitter band.
- At small `n`, harness p99 index `n*99/100` can equal max — prefer `--samples 100`.
- **ASIO / WASAPI buffer size is never cited as MIDI Path proof.**
- `software-loop` = Virtual Port round-trip with Bridge soft-echo ON — **not** full MT4 / WinUSB / DIN path latency.
- Blank / `N/A` = not run (**≠** Pass).

## Latest rows

### Software-loop (plumbing only)

| Column | Value |
|---|---|
| Status banner | Plumbing path; soft-echo ON — **not** the Gate confirm row |
| Host OS | Windows 10 x64 |
| Bridge build | `0.1.0` Release — artifact `builds/ci` (`builds/ci/Release/Bridge.exe`) |
| Harness build | `builds/ci/tools/midi-path-harness/Release/MidiPathHarness.exe` |
| Commit / evidence date | Lab run **2026-08-11**; capsule below |
| virtualMIDI presence | yes |
| `path_type` | `software-loop` |
| Plane | `host-winmm-qpc` (`asio_buffer_proof=false`) |
| Ports | `MT4 Out 1` / `MT4 In 1` |
| Samples | 50 (p99 may equal max at this n — see method) |
| Date / UTC | **2026-08-11T00:17:00Z** (from capsule log `harness-20260811T001700Z.log`) |
| Raw evidence | [`harness-20260811T001700Z.log`](../../tests/lab-evidence/midi-path-harness-software-loop-2026-08-11/harness-20260811T001700Z.log) in [`midi-path-harness-software-loop-2026-08-11/`](../../tests/lab-evidence/midi-path-harness-software-loop-2026-08-11/) |

| Metric | µs | ≈ ms |
|---|---:|---:|
| `latency_us_min` | 1283.3 | 1.28 |
| `latency_us_mean` | 1998.68 | 2.00 |
| `latency_us_p99` | 2110.8 | 2.11 |
| `latency_us_max` | 2110.8 | 2.11 |
| `latency_spread_us` (= p99 − min) | **827.5** | **0.83** |

**Operator recipe (capsule):** Bridge `--start-session --soft-echo`; harness `--path software-loop --out "MT4 Out 1" --in "MT4 In 1" --samples 50`.

### Hardware-loop (Gate confirm row)

| Column | Value |
|---|---|
| Status banner | **Gate (a) confirm evidence** — DIN Out→In; soft-echo OFF; classical `jitter_us_*` present |
| Host OS | Windows 10 x64 |
| Bridge build | `0.1.0` Release — artifact `builds/ci` (`builds/ci/Release/Bridge.exe`) |
| Harness build | `builds/ci/tools/midi-path-harness/Release/MidiPathHarness.exe` (with `MidiPathStats` / `jitter_us_*`) |
| Commit / evidence date | Lab run **2026-08-11** (local); UTC stamp below |
| virtualMIDI presence | yes |
| `path_type` | `hardware-loop` |
| Plane | `host-winmm-qpc` (`asio_buffer_proof=false`; Gate timing claim = **confirmed** — see decision) |
| Ports | `MT4 Out 2` / `MT4 In 2` (DIN Out 2 → In 2) |
| Samples | 100 |
| Date / UTC | **2026-08-10T22:55:20Z** (from `harness-20260810T225520Z-with-jitter.log`) |
| Raw evidence | [`harness-20260810T225520Z-with-jitter.log`](../../tests/lab-evidence/midi-path-harness-hardware-loop-2026-08-11/harness-20260810T225520Z-with-jitter.log) in [`midi-path-harness-hardware-loop-2026-08-11/`](../../tests/lab-evidence/midi-path-harness-hardware-loop-2026-08-11/) |

| Metric | µs | ≈ ms |
|---|---:|---:|
| `latency_us_min` | 1443.9 | 1.44 |
| `latency_us_mean` | 1604.17 | 1.60 |
| `latency_us_median` | 1591.4 | 1.59 |
| `latency_us_p99` | 2321.2 | 2.32 |
| `latency_us_max` | 2321.2 | 2.32 |
| `latency_spread_us` (= p99 − min) | **877.3** | **0.88** |
| `jitter_us_mean` | 88.647 | 0.089 |
| `jitter_us_p99` | **729.8** | **0.73** |
| `jitter_us_max` | 729.8 | 0.73 |

**Operator recipe (capsule):** Bridge `--start-session --no-soft-echo`; harness `--path hardware-loop --confirm-soft-echo-off --out "MT4 Out 2" --in "MT4 In 2" --samples 100 --json`. Soft-echo ON banner must be absent.

Earlier same-day hardware-loop without classical jitter (`harness-20260810T225020Z.log`) is superseded by the jitter-capable run above.

## Confirmed timing anchors (Studio-Done Gate)

Gate outcome **(a)** confirms these healthy studio targets (do-not-ship-worse ceiling unchanged):

| Band | Confirmed value | Latest hardware-loop evidence |
|---|---|---|
| Healthy bridge-added latency | ≤ **4–5 ms** p99 | ~**2.32 ms** p99 |
| Classical jitter | ≤ **1–2 ms** p99 (`jitter_us_p99`) | ~**0.73 ms** p99 |
| Do-not-ship-worse | ~**8–10 ms** p99 | Still the ceiling for shipping worse than healthy without a separate product exception |

Software-loop remains plumbing evidence only. Decision: [`studio-done-gate-decision.md`](studio-done-gate-decision.md).

## How to refresh “latest”

When a newer complete run supersedes the tables on this page:

1. Prefer keeping **one** human-facing latest summary here (overwrite metrics + metadata for the superseded path type).
2. Keep the prior raw capsule under `docs/tests/lab-evidence/` (do not delete historical evidence).
3. Optionally add a one-line “superseded by …” note pointing at the new capsule date/UTC.
4. Changing Gate outcome requires a new decision pass in [`studio-done-gate-decision.md`](studio-done-gate-decision.md).

## Cross-links

| Artifact | Path |
|---|---|
| Method | [`method-midi-path.md`](method-midi-path.md) |
| Studio-Done Gate decision | [`studio-done-gate-decision.md`](studio-done-gate-decision.md) |
| Smoke guide | [`docs/tests/smoke-epic5-midi-path-harness-mt4.md`](../../tests/smoke-epic5-midi-path-harness-mt4.md) |
| Seed capsule (software-loop) | [`midi-path-harness-software-loop-2026-08-11`](../../tests/lab-evidence/midi-path-harness-software-loop-2026-08-11/) |
| Hardware-loop capsule | [`midi-path-harness-hardware-loop-2026-08-11`](../../tests/lab-evidence/midi-path-harness-hardware-loop-2026-08-11/) |
| Lab evidence index | [`docs/tests/lab-evidence/README.md`](../../tests/lab-evidence/README.md) |
