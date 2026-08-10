---
organization: Ten Square Software
project: unitor-win64-driver
title: MIDI Path baseline tables (latest)
author: Guillaume DUPONT
created: 2026-08-11
updated: 2026-08-11
---

# MIDI Path baseline tables (latest)

> **PROVISIONAL** — **Not** Studio-Done. Numbers below seed discussion only. Story **5.3** / OQ-2 confirms or revises timing anchors (NFR-P1 / NFR-P2). Do **not** treat this page as closed timing proof.

Method: [`method-midi-path.md`](method-midi-path.md). Index: [`README.md`](README.md).

## How to read these tables

- Metrics come from harness summary fields (`latency_us_*`, `path_type`, `plane`).
- **Labeled jitter equivalent:** `latency_spread_us = latency_us_p99 − latency_us_min` — **not** classical peak-to-peak / MAD jitter (harness does not emit `jitter_us_*` yet). See method doc.
- **Do not** score `latency_spread_us` against the provisional Jitter ≤ ~1–2 ms p99 planning band (different semantics).
- At `samples=50`, harness p99 index `n*99/100` can equal `latency_us_max` — treat that p99 as weak / provisional plumbing evidence; prefer `--samples 100` for later rows.
- **ASIO / WASAPI buffer size is never cited as MIDI Path proof.**
- `software-loop` = Virtual Port round-trip with Bridge soft-echo ON — **not** full MT4 / WinUSB / DIN path latency.
- Blank / `N/A` = not run (**≠** Pass).

## Latest rows

### Software-loop (provisional plumbing)

| Column | Value |
|---|---|
| Status banner | **PROVISIONAL** — plumbing path; soft-echo ON; not Studio-Done |
| Host OS | Windows 10 x64 |
| Bridge build | `0.1.0` Release — artifact `builds/ci` (`builds/ci/Release/Bridge.exe`) |
| Harness build | `builds/ci/tools/midi-path-harness/Release/MidiPathHarness.exe` |
| Commit / evidence date | Lab run **2026-08-11**; capsule below |
| virtualMIDI presence | yes |
| `path_type` | `software-loop` |
| Plane | `host-winmm-qpc` (`asio_buffer_proof=false`, `studio_done=false`) |
| Ports | `MT4 Out 1` / `MT4 In 1` |
| Samples | 50 (p99 may equal max at this n — see method) |
| Date / UTC | **2026-08-11T00:17:00Z** (from capsule log `harness-20260811T001700Z.log`; local lab ~00:17) |
| Raw evidence | [`harness-20260811T001700Z.log`](../../tests/lab-evidence/midi-path-harness-software-loop-2026-08-11/harness-20260811T001700Z.log) in [`midi-path-harness-software-loop-2026-08-11/`](../../tests/lab-evidence/midi-path-harness-software-loop-2026-08-11/) |

| Metric | µs | ≈ ms |
|---|---:|---:|
| `latency_us_min` | 1283.3 | 1.28 |
| `latency_us_mean` | 1998.68 | 2.00 |
| `latency_us_p99` | 2110.8 | 2.11 |
| `latency_us_max` | 2110.8 | 2.11 |
| `latency_spread_us` (= p99 − min) | **827.5** | **0.83** |

**Operator recipe (capsule):** Bridge `--start-session --soft-echo`; harness `--path software-loop --out "MT4 Out 1" --in "MT4 In 1" --samples 50`.

### Hardware-loop

| Column | Value |
|---|---|
| Status banner | **PROVISIONAL** / **not run** |
| Host OS | — |
| Bridge build | — |
| virtualMIDI presence | — |
| `path_type` | `hardware-loop` |
| Samples | **N/A** |
| Date / UTC | — |
| Latency / spread | **N/A** — not run |
| Raw evidence | — |
| Note | Blank ≠ Pass. Requires DIN Out→In, soft-echo OFF, harness `--path hardware-loop --confirm-soft-echo-off`. Do not invent Pass. |

## Provisional anchors (planning only)

Cite only as **provisional** (not confirmed):

| Band | Provisional planning value |
|---|---|
| Healthy bridge-added latency | ≤ ~4–5 ms p99 |
| Jitter | ≤ ~1–2 ms p99 (classical jitter not yet harness-emitted; **do not** clear with `latency_spread_us`) |
| Do-not-ship-worse | ~8–10 ms p99 |

Story **5.3** / OQ-2 decides confirmation or revision. Soft-echo software-loop ~2 ms mean is **plumbing evidence**, not a claim that bridge-added WinUSB/MT4 latency is “done.” Do **not** score software-loop µs against the Healthy bridge-added band.

## Cross-links

| Artifact | Path |
|---|---|
| Method | [`method-midi-path.md`](method-midi-path.md) |
| Smoke guide | [`docs/tests/smoke-epic5-midi-path-harness-mt4.md`](../../tests/smoke-epic5-midi-path-harness-mt4.md) |
| Seed capsule | [`midi-path-harness-software-loop-2026-08-11`](../../tests/lab-evidence/midi-path-harness-software-loop-2026-08-11/) |
| Lab evidence index | [`docs/tests/lab-evidence/README.md`](../../tests/lab-evidence/README.md) |
