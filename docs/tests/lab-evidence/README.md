# Lab evidence capsules

Curated proof packs for lab failures and overnight runs that we want to remember in git.

**Raw captures** stay local under `tests/lab-logs/` (mostly gitignored).  
**Capsules** live here: short note + summary log + a few representative failure samples.

| Capsule | What it remembers |
|---|---|
| [`overnight-matrix-windows-2026-08-07/`](overnight-matrix-windows-2026-08-07/) | Windows overnight Matrix mid+bank (~8 h): rare TIMEOUT « aucune trame », Bridge still up |
| [`overnight-macos-sysex-2026-08-08/`](overnight-macos-sysex-2026-08-08/) | macOS overnight mid+bank+long (~8 h, Apple driver): **100 %** — hardware control vs Windows |
| [`midi-path-harness-software-loop-2026-08-11/`](midi-path-harness-software-loop-2026-08-11/) | Story 5.1 software-loop Pass (~2 ms mean QPC WinMM); soft-echo plumbing — not Studio-Done; seeds published baseline under [`docs/dev/measurements/`](../../dev/measurements/) |
| [`midi-path-harness-hardware-loop-2026-08-11/`](midi-path-harness-hardware-loop-2026-08-11/) | Hardware-loop Pass Out 2→In 2 with classical `jitter_us_p99` (~2.32 ms / ~0.73 ms p99, 100 samples); soft-echo OFF; Gate **(a)** confirm evidence |

**Published method + latest tables (Story 5.2+):** [`docs/dev/measurements/`](../../dev/measurements/) — capsules here remain **raw evidence**; the measurements folder is the human-facing latest summary.

**Studio-Done Gate (Story 5.3):** [`studio-done-gate-decision.md`](../../dev/measurements/studio-done-gate-decision.md) — outcome **(a) confirm** (2026-08-11; supersedes same-day **(c)**).

When adding a new capsule: keep it small (summary + ≤5 sample cycles), write a dated `README.md`, and leave bulk cycle trees out of git.
