---
organization: Ten Square Software
project: unitor-win64-driver
title: MIDI Path measurements (index)
author: Guillaume DUPONT
created: 2026-08-11
updated: 2026-08-11
---

# MIDI Path measurements

> **PROVISIONAL** — These results are **not** Studio-Done. Timing anchors stay provisional until Story **5.3** / OQ-2 confirms or revises them. Do **not** treat published numbers as closed NFR-P1 / NFR-P2 proof.

Published method notes and latest baseline tables for the **MIDI Path** (Bridge Virtual Ports ↔ host WinMM, QPC inject/observe). This folder is the shared evidence surface for Studio-Done discussions (architecture AD-11).

| Doc | Role |
|---|---|
| [`method-midi-path.md`](method-midi-path.md) | Locked measurement method (plane, path types, CLI fields, labeled jitter equivalent, anti-ASIO) |
| [`baseline-latest.md`](baseline-latest.md) | Latest provisional tables (Win10 software-loop seeded from lab evidence) |

## Honesty fence (one place)

- **ASIO / WASAPI buffer size is never MIDI Path proof** (NFR-P3 / SM-C2).
- **`software-loop` ≠ full MT4 / WinUSB / DIN latency** — soft-echo Virtual Port round-trip only.
- Blank / `N/A` hardware-loop rows mean **not run** — blank ≠ Pass.
- Raw evidence capsules live under [`docs/tests/lab-evidence/`](../../tests/lab-evidence/); this folder publishes method + summarized latest tables, not multi-KB log bodies.

## Operator entry

Smoke / run recipe: [`docs/tests/smoke-epic5-midi-path-harness-mt4.md`](../../tests/smoke-epic5-midi-path-harness-mt4.md)
