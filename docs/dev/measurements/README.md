---
organization: Ten Square Software
project: unitor-win64-driver
title: MIDI Path measurements (index)
author: Guillaume DUPONT
created: 2026-08-11
updated: 2026-08-11
---

# MIDI Path measurements

> **Studio-Done Gate — timing anchors confirmed** — Gate **2026-08-11** outcome **(a)** after Win10 hardware-loop + classical `jitter_us_p99`. Decision: [`studio-done-gate-decision.md`](studio-done-gate-decision.md). Healthy targets ≤4–5 ms p99 latency and ≤1–2 ms p99 jitter stand. Do **not** cite ASIO buffer size. Do **not** clear jitter with `latency_spread_us`.

Published method notes and latest baseline tables for the **MIDI Path** (Bridge Virtual Ports ↔ host WinMM, QPC inject/observe). This folder is the shared evidence surface for Studio-Done discussions (architecture AD-11).

| Doc | Role |
|---|---|
| [`method-midi-path.md`](method-midi-path.md) | Locked measurement method (plane, path types, CLI fields, classical jitter, anti-ASIO) |
| [`baseline-latest.md`](baseline-latest.md) | Latest tables (Win10 software-loop + hardware-loop with jitter) |
| [`studio-done-gate-decision.md`](studio-done-gate-decision.md) | **Studio-Done Gate** checklist + recorded outcome **(a)/(b)/(c)** |

## Honesty fence (one place)

- **ASIO / WASAPI buffer size is never MIDI Path proof** (NFR-P3 / SM-C2).
- **`software-loop` ≠ full MT4 / WinUSB / DIN latency** — soft-echo Virtual Port round-trip only.
- Blank / `N/A` hardware-loop rows mean **not run** — blank ≠ Pass.
- Excessive jitter is not a usermode alibi (SM-C4).
- Classical jitter is `jitter_us_p99` (`p99_abs_dev_from_median`), not `latency_spread_us`.
- Raw evidence capsules live under [`docs/tests/lab-evidence/`](../../tests/lab-evidence/); this folder publishes method + summarized latest tables, not multi-KB log bodies.

## Operator entry

Smoke / run recipe: [`docs/tests/smoke-epic5-midi-path-harness-mt4.md`](../../tests/smoke-epic5-midi-path-harness-mt4.md)
