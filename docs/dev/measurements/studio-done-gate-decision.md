---
organization: Ten Square Software
project: unitor-win64-driver
title: Studio-Done Gate decision record (timing anchors)
author: Guillaume DUPONT
created: 2026-08-11
updated: 2026-08-11
---

# Studio-Done Gate decision record (timing anchors)

This page is the **Studio-Done Gate** decision record for MIDI Path latency / jitter anchors (NFR-P1 / NFR-P2 / OQ-2 / SM-9 / CAP-16). It records one mutually exclusive outcome from **published** evidence under [`docs/dev/measurements/`](./).

Index: [`README.md`](README.md). Method: [`method-midi-path.md`](method-midi-path.md). Latest tables: [`baseline-latest.md`](baseline-latest.md).

## Honesty fences (always true)

| Fence | Rule |
|---|---|
| **SM-C4** | Excessive jitter is **not** excused merely because the path is usermode. Usermode is not an alibi for poor timing. |
| **Anti-ASIO (NFR-P3 / SM-C2)** | ASIO / WASAPI buffer size is **never** MIDI Path proof. |
| **Software-loop honesty** | Soft-echo Virtual Port round-trip ≠ bridge-added WinUSB / MT4 / DIN latency. Software-loop alone **cannot** clear NFR-P1 “bridge-added.” |
| **Jitter honesty** | `latency_spread_us` must **not** clear NFR-P2 ≤1–2 ms p99. Classical harness `jitter_us_p99` (`jitter_def=p99_abs_dev_from_median`) is required to confirm jitter. |

## Evidence checklist (fill before choosing an outcome)

Use published SSOT only — do not invent lab numbers here.

| # | Evidence item | Status for this gate pass (2026-08-11, superseding) | Met? |
|---|---|---|---|
| 1 | Method published under `docs/dev/measurements/` | Yes — [`method-midi-path.md`](method-midi-path.md) | **Yes** |
| 2 | Win10 x64 run with required metadata | Yes — hardware-loop + software-loop capsules + [`baseline-latest.md`](baseline-latest.md) | **Yes** |
| 3 | `path_type=software-loop` soft-echo published | Yes — plumbing ~2.11 ms p99 (50 samples) | **Yes** — plumbing only |
| 4 | `path_type=hardware-loop` DIN, soft-echo OFF | Yes — Out 2→In 2; latest p99 latency ≈ **2.32 ms** (100 samples) | **Yes** |
| 5 | Classical / harness jitter (`jitter_us_*`) | Yes — `jitter_us_p99` ≈ **0.73 ms** (`p99_abs_dev_from_median`) | **Yes** |
| 6 | ASIO cited as proof | Forbidden — not cited | **False (good)** |
| 7 | Usermode jitter alibi | Forbidden (SM-C4) — not used | **False (good)** |

**Evidence bar for outcome (a) Confirm:** items 1–5 must be **Yes** with honest path typing; items 6–7 must remain false.

## Decision (exactly one outcome)

| Field | Value |
|---|---|
| **Selected outcome** | **(a) Confirm** |
| **Date (UTC)** | **2026-08-11** (same-day superseding pass after hardware-loop + classical jitter lab) |
| **Owner** | Guillaume DUPONT |
| **Prior outcome superseded** | **(c) Defer** (morning pass — incomplete evidence) |
| **Evidence pointers** | [`baseline-latest.md`](baseline-latest.md); [`method-midi-path.md`](method-midi-path.md); hardware capsule [`midi-path-harness-hardware-loop-2026-08-11/`](../../tests/lab-evidence/midi-path-harness-hardware-loop-2026-08-11/) (latest `harness-20260810T225520Z-with-jitter.log`); software capsule [`midi-path-harness-software-loop-2026-08-11/`](../../tests/lab-evidence/midi-path-harness-software-loop-2026-08-11/) |

### Outcome meanings (reference)

| Outcome | Meaning |
|---|---|
| **(a) Confirm** | Healthy ≤4–5 ms p99 latency **and** ≤1–2 ms p99 jitter stand as studio targets from measured MIDI Path evidence |
| **(b) Revise** | Different numeric bands with rationale tied to published measurements |
| **(c) Defer** | Refuse ship-timing / studio-done claim for now; stay under do-not-ship-worse ~8–10 ms p99 unless a **separate** explicit product exception is recorded |

### Rationale for **(a)**

Published Win10 **hardware-loop** MIDI Path evidence (DIN Out 2→In 2, soft-echo OFF, n=100) shows:

- Bridge-relevant latency p99 ≈ **2.32 ms** — within healthy ≤4–5 ms
- Classical jitter p99 ≈ **0.73 ms** (`jitter_us_p99`, abs-dev-from-median) — within healthy ≤1–2 ms

Software-loop remains plumbing-only and is **not** used alone to clear NFR-P1. ASIO was not cited. Usermode was not used as a jitter alibi (SM-C4). Therefore the provisional healthy anchors are **confirmed** as studio targets; do-not-ship-worse ~8–10 ms p99 remains the ceiling for shipping worse than healthy without a separate explicit product exception.

**Lab caveats for this confirm (honesty):** evidence is a **single** quiet-lab Win10 DIN path (Out 2→In 2, n=100). It is **not** a multi-cable / multi-OS matrix and **not** a DAW-session guarantee. Published numbers are host-WinMM QPC MIDI Path measurements under the locked method — not a promise that every loaded studio session will stay inside the band.

**Measurement plane for NFR-P1:** Gate confirm uses published **hardware-loop** (Virtual Ports ↔ Bridge ↔ MT4 ↔ physical DIN). That plane includes device + cable delay. It is **not** a separated “usermode-only beyond host USB” delta.

## How to reopen (supersede this pass)

A future pass may supersede **(a)** only with new published MIDI Path evidence that forces revised bands **(b)** or an honest deferral **(c)** (e.g. regressions above healthy bands without an accepted product exception). Keep method + capsules as the evidence trail.

## Planning / release-note path applied for this outcome

For **(a)** (this pass):

| Surface | Action |
|---|---|
| PRD OQ-2 | Keep row; status **confirmed / closed** with pointer to this decision |
| NFR-P1 / NFR-P2 | Mark **confirmed** healthy bands; keep do-not-ship-worse ceiling |
| Architecture AD-11 | Anchors no longer “provisional until Gate” — confirmed with pointer |
| Spec + validation-matrix | Mirror confirmed anchors |
| Measurement banners | Timing anchors **confirmed**; `studio_done` claim allowed for Gate timing |
| User guides | May state published studio timing anchors exist (with method honesty) |

There is **no** root `CHANGELOG` — this decision doc + PRD OQ-2 + Spec / validation-matrix + measurement banners + user honesty surfaces are the release-note path.

### Adopted classical jitter definition (product sign-off)

```text
jitter_us_*  := statistics of |latency_us_sample − latency_us_median|
jitter_us_p99 := p99 of that absolute-deviation series (same n*99/100 index rule)
jitter_def   := p99_abs_dev_from_median
```

`latency_spread_us = latency_us_p99 − latency_us_min` remains a labeled spread only — **not** scored against NFR-P2.
