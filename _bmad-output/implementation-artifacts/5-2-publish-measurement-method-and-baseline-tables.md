---
baseline_commit: 18ee91dbb0673781e9c2d7341e4d8e2169ea42e2
---

# Story 5.2: Publish measurement method and baseline tables

Status: done

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a project maintainer,
I want published method notes and latest measurement tables under `docs/dev/measurements/`,
so that Studio-Done discussions share the same evidence.

## Acceptance Criteria

1. **Given** the harness from Story 5.1 can run a software-loop (and hardware-loop when available)  
   **When** a measurement run completes on at least Win10 x64  
   **Then** `docs/dev/measurements/` contains method documentation plus latest tables including host OS, Bridge build, virtualMIDI presence, and path type (`software-loop` vs `hardware-loop`) — AD-11

2. **And** reported metrics include bridge-relevant latency and jitter at p99 (or clearly labeled equivalent percentiles)

3. **And** results are explicitly labeled **provisional** until Studio-Done Gate revises anchors — NFR-P1 / NFR-P2

4. **And** ASIO buffer size is never cited as MIDI Path proof — NFR-P3 / SM-C2

**Traces:** CAP-16, NFR-P1, NFR-P2, NFR-P3, AD-11, SM-9 (method published — timing not “done” until 5.3)

## Tasks / Subtasks

- [x] Task 1: Create `docs/dev/measurements/` publish tree (AC: 1, 3, 4)
  - [x] Add folder `docs/dev/measurements/` (does **not** exist yet — Story 5.1 deliberately left it empty)
  - [x] Add English method doc (kebab-case), e.g. `docs/dev/measurements/method-midi-path.md`
  - [x] Add English latest-tables doc, e.g. `docs/dev/measurements/baseline-latest.md` (or `README.md` index + `baseline-latest.md` — keep one obvious “latest” entry point)
  - [x] Optional thin `docs/dev/measurements/README.md` that points to method + latest tables and states the provisional banner in one place
  - [x] Match EN contributor-doc style: optional YAML frontmatter (`organization`, `project`, `title`, `author`, `created`, `updated`) like `docs/dev/contributor-dual-machine-loop.md`
  - [x] Language: **English** for these durable measurement docs (chat stays French; docs EN)

- [x] Task 2: Lock the published method (AC: 1, 2, 4)
  - [x] Document the **locked timestamp plane** (same as 5.1): host WinMM client vs Bridge Virtual Ports; inject QPC immediately before `midiOutShortMsg`; observe QPC in midiIn callback on matching Note On; do **not** mix MidiBackend-internal or WinUSB URB stamps into the same series
  - [x] Document path types honestly:
    - **`software-loop`:** Bridge soft-echo ON (`Bridge.exe --start-session --soft-echo` or `UNITOR_MIDI_SOFT_ECHO=1`); harness `--path software-loop`; **skips USB/DIN** — Virtual Port round-trip only; must never be described as full MT4/WinUSB path latency
    - **`hardware-loop`:** soft-echo OFF (`--no-soft-echo` wins over sticky env); DIN Out→In present; harness `--path hardware-loop --confirm-soft-echo-off`; missing DIN → honest fail, not Pass
  - [x] Document required metadata columns for every published row: host OS (Win10 x64 minimum to close claim), Bridge build/version (and commit or artifact path when known), virtualMIDI presence (yes/no), `path_type`, sample count, date/UTC, pointer to raw evidence capsule under `docs/tests/lab-evidence/` when present
  - [x] Document harness CLI summary fields used as sources of truth: `latency_us_min`, `latency_us_mean`, `latency_us_p99`, `latency_us_max`, `path_type`, `plane` / `asio_buffer_proof=false`, `studio_done=false`
  - [x] **Jitter (mandatory honesty):** harness today does **not** emit `jitter_us_*`. Do **one** of:
    1. **Preferred (docs-only):** define and publish a **clearly labeled equivalent**, e.g. `latency_spread_us = latency_us_p99 − latency_us_min` (or p99−mean), with an explicit sentence that this is **not** classical peak-to-peak / MAD jitter until the harness computes it — satisfies AC2 “clearly labeled equivalent”
    2. **Optional tiny harness follow-up:** add `jitter_us_p99` (e.g. p99 of |sample − median|) to plain/JSON summary and re-run once — only if docs-only equivalent feels too weak; keep Protocol/Profile untouched
  - [x] Explicitly forbid citing ASIO / WASAPI buffer size as MIDI Path proof anywhere in the new docs
  - [x] Cite provisional planning anchors **as provisional only** (healthy ≤4–5 ms p99 bridge-added; jitter ≤1–2 ms p99; do-not-ship-worse ~8–10 ms p99) — do **not** mark them confirmed

- [x] Task 3: Publish latest baseline tables from existing Win10 evidence (AC: 1, 2, 3)
  - [x] Seed tables from the durable 5.1 capsule (do **not** invent numbers): `docs/tests/lab-evidence/midi-path-harness-software-loop-2026-08-11/`
  - [x] Software-loop row (provisional / plumbing path, soft-echo): samples=50; min≈1283.3 µs; mean≈1998.68 µs; p99≈2110.8 µs; max≈2110.8 µs; Bridge `0.1.0` Release `builds/ci`; ports `MT4 Out 1` / `MT4 In 1`; host Win10 x64; virtualMIDI present; plane host-winmm-qpc
  - [x] Compute and publish the labeled jitter equivalent from those summary fields (e.g. spread p99−min ≈ 827.5 µs) with the method definition from Task 2
  - [x] Hardware-loop row: **not run** — leave blank / `N/A` with note blank ≠ Pass (optional when DIN available; do not fake)
  - [x] Banner on every table page: **PROVISIONAL** — not Studio-Done; Story **5.3** / OQ-2 confirms or revises anchors
  - [x] Cross-link: method ↔ baseline ↔ lab-evidence capsule ↔ `docs/tests/smoke-epic5-midi-path-harness-mt4.md`
  - [x] If a fresh lab re-run is convenient, prefer `--samples 100` (harness default) and `--json`, but **re-using the 2026-08-11 capsule is enough** to satisfy “run completed on at least Win10 x64”

- [x] Task 4: Cross-doc pointers + anti-scope fence (AC: 3, 4)
  - [x] Update smoke guide and/or lab-evidence index with a short pointer that method + tables now live under `docs/dev/measurements/` (keep smoke honesty bar; do not rewrite Epic 5.1 as Studio-Done)
  - [x] Do **not** create Story 5.3 decision-record checklist here
  - [x] Do **not** confirm/revise NFR-P1/P2 anchors
  - [x] Do **not** require certificate, Tobias redistrib, or Windows MIDI Services (Correct Course 2026-08-10)
  - [x] Do **not** replace Python DIN labs under `scripts/lab/`
  - [x] Do **not** vendor VirtualMIDI SDK; do not touch `src/Protocol/` / `src/Profile/`
  - [x] C++ changes are **out of scope unless** choosing optional jitter field (Task 2 option 2); if C++ touched → `python scripts/quality/lint-touched.py` must exit 0

### Review Findings

- [x] [Review][Patch] Document harness p99 index rule and that at `samples=50` p99 can equal max [`docs/dev/measurements/method-midi-path.md`] [`docs/dev/measurements/baseline-latest.md`]
- [x] [Review][Patch] Record true UTC for the seeded row (capsule log `20260811T001700Z`), not local-only time [`docs/dev/measurements/baseline-latest.md`:39]
- [x] [Review][Patch] Explicitly forbid scoring `latency_spread_us` against the provisional Jitter ≤1–2 ms band [`docs/dev/measurements/method-midi-path.md`] [`docs/dev/measurements/baseline-latest.md`]
- [x] [Review][Patch] Document `--timeout-ms` default 2000 / min 1 beside `--samples` [`docs/dev/measurements/method-midi-path.md`:64]
- [x] [Review][Patch] Add harness binary path to the software-loop metadata row (from capsule) [`docs/dev/measurements/baseline-latest.md`]
- [x] [Review][Patch] Point Raw evidence at the exact harness summary log, not only the capsule folder [`docs/dev/measurements/baseline-latest.md`:40]
- [x] [Review][Patch] Require `plane` / `asio_buffer_proof` / `studio_done` on every complete published row [`docs/dev/measurements/method-midi-path.md`:100]
- [x] [Review][Patch] State that `virtualMIDI=no` is non-closing for measurement claims [`docs/dev/measurements/method-midi-path.md`:100]
- [x] [Review][Defer] Define refresh/archive protocol for `baseline-latest.md` when a newer run supersedes “latest” — deferred, pre-existing ops gap (file invented this story; process not specified)
- [x] [Review][Defer] Symmetric soft-echo-ON confirm flag for software-loop (mirror hardware `--confirm-soft-echo-off`) — deferred, pre-existing harness design from 5.1; out of 5.2 docs-only scope

## Dev Notes

### Epic context

Epic 5 sequence: harness (**5.1** done) → **publish method + tables (this story)** → Studio-Done Gate decision record (**5.3**). Correct Course 2026-08-10: Epic 5 runs on interim **virtualMIDI + Win10 lab**; not blocked by certificate, Tobias redistrib, or WMS.

SM-9 split: 5.1 = harness exists; **5.2 = method published**; 5.3 = anchors confirmed or explicitly revised before calling timing “done.”

### Scope fence

| In 5.2 | Out (later / never here) |
|---|---|
| `docs/dev/measurements/` method + latest tables | Studio-Done confirm/revise (**5.3** / OQ-2) |
| Provisional labels + Win10 software-loop baseline from evidence | Claiming studio-done timing / closing SM-9 timing-done |
| Labeled jitter equivalent (or tiny optional harness field) | WMS MidiBackend (**Epic 6**) |
| Honesty that software-loop ≠ USB/DIN path | Treating soft-echo µs as bridge-added WinUSB/MT4 latency |
| Pointers from smoke / lab-evidence | Inventing hardware-loop Pass without DIN |

### Architecture compliance

- **AD-11** — results publish under `docs/dev/measurements/` (method + latest tables); required metadata: host OS, Bridge build, VirtualMIDI presence, path type; MIDI Path only; QPC method; provisional anchors remain until Studio-Done Gate
- **AD-13** — Win10 mandatory in matrix for closing measurement claims; edit on macOS OK for markdown; validate numbers on Windows lab evidence
- **AD-7** — virtualMIDI remains interim lab backend through Epic 5
- **NFR-P1 / NFR-P2** — cite provisional anchors only; do not treat as proven
- **NFR-P3 / SM-C2** — never ASIO buffer as MIDI proof
- **SM-C4** — excessive jitter is not a usermode alibi (document; do not excuse)
- **CAP-16** — harness exists + results publish path

[Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-11, AD-13]

### Files to CREATE (NEW)

| Path | Role |
|---|---|
| `docs/dev/measurements/README.md` | Index + provisional banner (optional but recommended) |
| `docs/dev/measurements/method-midi-path.md` | Published method (plane, path types, CLI, jitter definition, anti-ASIO) |
| `docs/dev/measurements/baseline-latest.md` | Latest tables with required metadata columns + provisional banner |

Exact filenames may vary slightly if a single combined file is clearer — **folder `docs/dev/measurements/` and both method + tables content are mandatory.**

### Files to UPDATE (light touch)

| Path | Change |
|---|---|
| `docs/tests/smoke-epic5-midi-path-harness-mt4.md` | Point “méthode/tables → 5.2” at the new folder (and optionally bump `updated`) |
| `docs/tests/lab-evidence/README.md` and/or capsule README | Note that published baseline lives under `docs/dev/measurements/` (capsule remains raw evidence) |
| `tools/midi-path-harness/*` | **Only if** choosing optional `jitter_us_p99` emission |

### Current harness / evidence reality (preserve)

- Harness CLI: `--path software-loop|hardware-loop`, `--out`, `--in`, `--samples` (default 100), `--timeout-ms`, `--json`, hardware requires `--confirm-soft-echo-off`
- Summary emits latency min/mean/p99/max only — **no jitter field today**
- Soft-echo: default OFF; ON via `--soft-echo` or `UNITOR_MIDI_SOFT_ECHO`; `--no-soft-echo` force-off
- Lab evidence capsule already Pass for software-loop plumbing on Win10 (2026-08-11); hardware-loop not run
- Port names: `MT4 In N` / `MT4 Out N` (not obsolete `MT4 Port N`)
- JSON already carries `asio_buffer_proof: false` and `studio_done: false` — mirror that honesty in markdown

### Known confounders (document in method — do not “fix” in 5.2)

- Soft-echo skips USB/DIN — software-loop numbers are Virtual Port round-trip only
- Under load, bulk IN / `usbIoMutex_` may affect **hardware-loop** credibility; first baselines: quiet lab
- CTRL_CLOSE can orphan Virtual Ports — prefer Ctrl+C teardown
- Do not mix software-loop and hardware-loop into one unlabeled p99 series

[Source: `_bmad-output/implementation-artifacts/deferred-work.md` / Story 5.1 Dev Notes]

### Testing requirements

- **Docs (required):** `docs/dev/measurements/` exists with method + tables; required metadata columns present; provisional banner present; ASIO never cited as proof; software-loop baseline populated from real evidence
- **Lab (already satisfied if capsule reused):** Win10 x64 software-loop run with published numbers — no mandatory re-run
- **Lab (optional):** hardware-loop when DIN available; otherwise leave blank ≠ Pass
- **CI:** no new CI job required for markdown-only; if harness C++ changed → existing windows-build + lint-touched
- **Do not** fake hardware Pass in Catch2 / unit tests

### Latest tech notes (QPC / publish)

- Interval measurement: `QueryPerformanceCounter` + `QueryPerformanceFrequency` ([Microsoft: Acquiring high-resolution time stamps](https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps))
- Publish human-readable ms alongside µs in tables if helpful (capsule already shows both) — keep harness field names as source IDs
- Prefer linking raw logs in `docs/tests/lab-evidence/` rather than duplicating multi-KB log bodies into `docs/dev/measurements/`

### Previous story intelligence (5.1)

- Delivered `tools/midi-path-harness/` + Bridge soft-echo fail-closed + CI confirm + smoke + lab capsule
- Explicitly deferred `docs/dev/measurements/` to **this story**
- Review patches locked hardware confirm, `--no-soft-echo`, soft-echo honesty, wait/drain/JSON escape — treat those as operator constraints in the method doc
- Numbers ~2 ms mean software-loop are **plumbing**, not Studio-Done — preserve that fence in published tables
- Do not reopen soft-echo design; document operator recipe only

### Git intelligence

- Latest relevant commit: `18ee91d` — Add MIDI Path harness and Bridge soft-echo for Story 5.1
- Expect this story’s primary diff to be **markdown under `docs/dev/measurements/`** (+ light smoke/lab-evidence pointers), similar to how Epic 4 honesty docs grew without Protocol churn
- Avoid committing `builds/` artifacts

### Project structure notes

- Dirs: kebab-case (`docs/dev/measurements/`)
- Docs files: kebab-case
- Vendor spelling in user-facing strings: **virtualMIDI**; code symbols stay `VirtualMidi*`
- Builds remain under `builds/` only

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Epic 5 / Story 5.2]
- [Source: `_bmad-output/planning-artifacts/architecture/.../ARCHITECTURE-SPINE.md` — AD-11, AD-13]
- [Source: `_bmad-output/planning-artifacts/prds/.../prd.md` — NFR-P1..P3, SM-9, SM-C2, SM-C4, OQ-2]
- [Source: `_bmad-output/specs/spec-unitor-win64-driver/SPEC.md` — CAP-16]
- [Source: `_bmad-output/planning-artifacts/sprint-change-proposal-2026-08-10.md`]
- [Source: `_bmad-output/implementation-artifacts/5-1-in-repo-midi-path-harness-scaffold.md`]
- [Source: `docs/tests/lab-evidence/midi-path-harness-software-loop-2026-08-11/`]
- [Source: `docs/tests/smoke-epic5-midi-path-harness-mt4.md`]
- [Source: `tools/midi-path-harness/MidiPathRunner.cpp` — summary fields]
- [Source: `docs/dev/contributor-dual-machine-loop.md` — EN doc style]

## Dev Agent Record

### Agent Model Used

Composer (Cursor agent router)

### Debug Log References

- Seeded numbers verified against `docs/tests/lab-evidence/midi-path-harness-software-loop-2026-08-11/harness-20260811T001700Z.log` and capsule README
- Chose Task 2 option 1 (docs-only `latency_spread_us`) — no C++ / no `lint-touched.py` required

### Completion Notes List

- Created `docs/dev/measurements/` with README index, locked method doc, and provisional latest baseline tables
- Documented host-winmm-qpc plane, software-loop vs hardware-loop honesty, required metadata columns, harness summary fields, anti-ASIO fence, and provisional NFR-P1/P2 anchors (not confirmed)
- Published labeled jitter equivalent `latency_spread_us = latency_us_p99 − latency_us_min` = 827.5 µs from the 2026-08-11 software-loop capsule (50 samples)
- Hardware-loop left N/A / not run (blank ≠ Pass)
- Cross-linked smoke guide, lab-evidence index, and capsule README to the new measurements folder without claiming Studio-Done
- Code review patches: p99 index honesty at n=50, UTC from capsule log, spread-vs-jitter fence, timeout-ms defaults, harness path + exact log pointer, required plane/honesty flags, virtualMIDI=no non-closing

### File List

- `docs/dev/measurements/README.md` (new)
- `docs/dev/measurements/method-midi-path.md` (new)
- `docs/dev/measurements/baseline-latest.md` (new)
- `docs/tests/smoke-epic5-midi-path-harness-mt4.md` (modified)
- `docs/tests/lab-evidence/README.md` (modified)
- `docs/tests/lab-evidence/midi-path-harness-software-loop-2026-08-11/README.md` (modified)
- `_bmad-output/implementation-artifacts/sprint-status.yaml` (modified)
- `_bmad-output/implementation-artifacts/5-2-publish-measurement-method-and-baseline-tables.md` (modified)

## Change Log

- 2026-08-11: Story context created (create-story) — ready-for-dev
- 2026-08-11: Published method + provisional baseline tables under `docs/dev/measurements/`; cross-doc pointers; status → review
- 2026-08-11: Code review patches applied (p99 honesty, UTC, spread-vs-jitter fence, metadata); status → done
