# Evidence — MIDI Path harness hardware-loop (2026-08-11)

**Machine :** Windows 10 x64 lab PC  
**Stack :** Bridge **Release** + WinUSB + virtualMIDI + DIN Out→In (soft-echo **OFF**)  
**Ports :** `MT4 Out 2` / `MT4 In 2` (unit K=1; physical red MIDI cable Out 2 → In 2)  
**Latest run :** jitter-capable harness (`jitter_us_*`, `jitter_def=p99_abs_dev_from_median`)  
**Smoke guide :** [`docs/tests/smoke-epic5-midi-path-harness-mt4.md`](../../smoke-epic5-midi-path-harness-mt4.md)  
**Published baseline :** [`docs/dev/measurements/baseline-latest.md`](../../../dev/measurements/baseline-latest.md)  
**Gate decision :** [`studio-done-gate-decision.md`](../../../dev/measurements/studio-done-gate-decision.md) — superseded same-day to **(a) Confirm** after this jitter series

Same-day history: first hardware-loop Pass without classical jitter fields (`harness-20260810T225020Z.log`), then superseding Pass with harness `jitter_us_*` (`harness-20260810T225520Z-with-jitter.log`). **Latest = jitter-capable run.**

---

## Headline results (latest — with classical jitter)

| Item | Value |
|---|---|
| Verdict | **Pass** (hardware-loop + classical jitter; exit 0) |
| Path | `path_type=hardware-loop` |
| Samples | 100 (**p99≡max** under harness index `n*99/100`) |
| latency_us_min | 1443.9 (~1.44 ms) |
| latency_us_mean | 1604.17 (~1.60 ms) |
| latency_us_median | 1591.4 (~1.59 ms) |
| latency_us_p99 | 2321.2 (~2.32 ms) — equals max at n=100 |
| latency_us_max | 2321.2 (~2.32 ms) |
| latency_spread_us | 877.3 (~0.88 ms) — **not** classical jitter |
| jitter_us_mean | 88.647 (~0.089 ms) |
| jitter_us_p99 | 729.8 (~0.73 ms) — equals max at n=100 |
| jitter_us_max | 729.8 (~0.73 ms) |
| jitter_def | `p99_abs_dev_from_median` |
| Plane | `host-winmm-qpc` (`asio_buffer_proof=false`; `studio_done=false`) |
| Date / UTC | **2026-08-10T22:55:20Z** (= local **2026-08-11 ~00:55** UTC+2) |

---

## How it was run (latest)

1. Rebuild Release harness with `MidiPathStats` / `jitter_us_*`
2. Bridge: `--start-session --no-soft-echo`
3. Physical: DIN Out 2 → In 2
4. Harness: `--path hardware-loop --confirm-soft-echo-off --out "MT4 Out 2" --in "MT4 In 2" --samples 100 --json`
5. Bridge stopped after the run

---

## Artifacts in this capsule

| File | Role |
|---|---|
| [`harness-20260810T225520Z-with-jitter.log`](harness-20260810T225520Z-with-jitter.log) | **Latest** harness JSON (latency + classical jitter) |
| [`bridge-start-excerpt-20260810T225403Z-jitter-run.log`](bridge-start-excerpt-20260810T225403Z-jitter-run.log) | Bridge excerpt for jitter run |
| [`harness-20260810T225020Z.log`](harness-20260810T225020Z.log) | Earlier same-day hardware-loop (latency only; superseded) |
| [`bridge-start-excerpt-20260810T224920Z.log`](bridge-start-excerpt-20260810T224920Z.log) | Bridge excerpt for earlier run |

---

## Honesty fence

- Classical jitter is **`jitter_us_p99`** = p99 of \|sample − median\| — do **not** clear NFR-P2 with `latency_spread_us`.
- Do **not** cite ASIO / WASAPI buffer size as MIDI Path proof.
- Soft-echo was **OFF** (`--no-soft-echo`; no soft-echo ON banner).
- p99 may equal max at this harness index rule — documented; n=100 preferred over n=50.
- Excessive jitter is not a usermode alibi (SM-C4).

---

## Related

- Software-loop plumbing: [`../midi-path-harness-software-loop-2026-08-11/`](../midi-path-harness-software-loop-2026-08-11/)
- Measurements: [`docs/dev/measurements/`](../../../dev/measurements/)
- Architecture AD-11 / AD-13 / CAP-16 / NFR-P1–P3
