# Lab evidence capsules

Curated proof packs for lab failures and overnight runs that we want to remember in git.

**Raw captures** stay local under `tests/lab-logs/` (mostly gitignored).  
**Capsules** live here: short note + summary log + a few representative failure samples.

| Capsule | What it remembers |
|---|---|
| [`overnight-matrix-windows-2026-08-07/`](overnight-matrix-windows-2026-08-07/) | Windows overnight Matrix mid+bank (~8 h): rare TIMEOUT « aucune trame », Bridge still up |
| [`overnight-macos-sysex-2026-08-08/`](overnight-macos-sysex-2026-08-08/) | macOS overnight mid+bank+long (~8 h, Apple driver): **100 %** — hardware control vs Windows |

When adding a new capsule: keep it small (summary + ≤5 sample cycles), write a dated `README.md`, and leave bulk cycle trees out of git.
