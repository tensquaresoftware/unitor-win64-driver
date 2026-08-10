# Evidence — MIDI Path harness software-loop (2026-08-11)

**Machine :** Windows 10 x64 lab PC  
**Stack :** Bridge **Release** + WinUSB + virtualMIDI + soft-echo lab gate  
**Ports :** `MT4 Out 1` / `MT4 In 1` (unit K=1)  
**Story :** 5.1 — harness scaffold (plumbing proof, **not** Studio-Done)  
**Smoke guide :** [`docs/tests/smoke-epic5-midi-path-harness-mt4.md`](../../smoke-epic5-midi-path-harness-mt4.md)  
**Published baseline (Story 5.2) :** [`docs/dev/measurements/baseline-latest.md`](../../../dev/measurements/baseline-latest.md) (method: [`method-midi-path.md`](../../../dev/measurements/method-midi-path.md))

This capsule is the **durable memory** of the first agent-driven software-loop Pass after Story 5.1 implementation. It proves inject/observe QPC plumbing through Bridge Virtual Ports with soft-echo ON — **not** DIN/USB path latency and **not** NFR-P1/P2 anchors. Story **5.2** publishes summarized tables from these numbers; this folder stays the raw evidence.

---

## Headline results

| Item | Value |
|---|---|
| Verdict | **Pass** (software-loop plumbing) |
| Path | `path_type=software-loop` |
| Samples | 50 (exit 0) |
| latency_us_min | 1283.3 (~1.28 ms) |
| latency_us_mean | 1998.68 (~2.00 ms) |
| latency_us_p99 | 2110.8 (~2.11 ms) |
| latency_us_max | 2110.8 (~2.11 ms) |
| Plane | host WinMM + QPC (ASIO not used / not proof) |
| hardware-loop | **not run** (optional; blank ≠ Pass) |

---

## How it was run

1. Build Release: `builds/ci/Release/Bridge.exe` + `builds/ci/tools/midi-path-harness/Release/MidiPathHarness.exe`
2. Bridge: `--start-session --soft-echo` (stderr: `Bridge soft-echo ON …`)
3. Harness: `--path software-loop --out "MT4 Out 1" --in "MT4 In 1" --samples 50`
4. Bridge stopped after the run

---

## Artifacts in this capsule

| File | Role |
|---|---|
| [`harness-20260811T001700Z.log`](harness-20260811T001700Z.log) | Exact harness stdout summary |
| [`bridge-start-excerpt-20260811T001600Z.log`](bridge-start-excerpt-20260811T001600Z.log) | Bridge session start + soft-echo ON + port list |

---

## Honesty fence

- Soft-echo **skips USB/DIN** — these µs numbers are Virtual Port round-trip only.
- Do **not** cite as Studio-Done / NFR-P1/P2 (Story **5.3** / OQ-2).
- Do **not** treat ASIO buffer size as MIDI Path proof.
- Auto-Start / daily studio path must keep soft-echo **OFF**.

---

## Related

- Story 5.1: `_bmad-output/implementation-artifacts/5-1-in-repo-midi-path-harness-scaffold.md`
- Story 5.2 published tables: [`docs/dev/measurements/`](../../../dev/measurements/)
- Architecture AD-11 / AD-13 / CAP-16 / NFR-P3
