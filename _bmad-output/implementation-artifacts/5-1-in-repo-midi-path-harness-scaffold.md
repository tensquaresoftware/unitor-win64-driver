---
baseline_commit: 7708950562b5a072edd8c27fca7d52e04151d069
---

# Story 5.1: In-repo MIDI Path harness scaffold

Status: done

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a contributor measuring studio timing,
I want a C++17 MIDI Path harness in-repo that builds to `builds/`,
so that latency/jitter claims can be measured reproducibly instead of “feels fine.”

## Acceptance Criteria

1. **Given** the Bridge project scaffold and virtualMIDI available on the Windows validation machine  
   **When** `tools/midi-path-harness/` is built  
   **Then** the harness compiles as C++17 with artifacts under `builds/` — AD-11 / CAP-16

2. **And** it can inject/observe timestamped MIDI using high-resolution clocks (`QueryPerformanceCounter` + `QueryPerformanceFrequency`, or equivalent documented as QPC-based)

3. **And** the harness measures the **MIDI Path** only — never ASIO buffer size — NFR-P3 / SM-C2  
   (README / CLI / smoke must not cite ASIO buffer size as proof)

4. **And** first iteration supports a Bridge-mediated Virtual Port **software-loop**; hardware DIN loopback is optional when a physical loop setup exists — AD-11 assumption

5. **And** Windows CI builds the harness when present (same pinned `windows-2022` runner policy as the Bridge) and confirms the harness `.exe` under `builds/` — AD-13

**Traces:** CAP-16, NFR-P3, AD-11, AD-13, SM-9 (harness exists — not Studio-Done timing)

## Tasks / Subtasks

- [x] Task 1: Create `tools/midi-path-harness/` C++17 executable (AC: 1, 2)
  - [x] Add `tools/midi-path-harness/CMakeLists.txt` defining executable target `MidiPathHarness` (name may be `midi-path-harness` on disk; CMake target PascalCase OK)
  - [x] Sources: PascalCase translation units under the tool tree (e.g. `Main.cpp`, `QpcClock.cpp`, `WinMmMidiIo.cpp`, `MidiPathRunner.cpp`) — English-only
  - [x] Wire from root `CMakeLists.txt` via `add_subdirectory(tools/midi-path-harness)` (or equivalent) so configure/build into `builds/<preset>` produces the harness beside Bridge
  - [x] Implement `QpcClock`: cache frequency once; convert tick deltas to nanoseconds/microseconds; no `Sleep` used as a timer
  - [x] CLI minimum: `--help`, path type flag (`software-loop` / `hardware-loop`), port selectors, sample count, JSON or plain-text summary to stdout (tables/docs publish is Story **5.2**)

- [x] Task 2: Lock measurement plane + software-loop behavior (AC: 2, 3, 4)
  - [x] **Locked timestamp plane (mandatory):** host WinMM client against Bridge Virtual Ports  
    - Inject: `QueryPerformanceCounter` immediately before `midiOutShortMsg` (or equivalent short message send) to Bridge port `MT4 Out N`  
    - Observe: `QueryPerformanceCounter` in the `midiIn` callback when the matching Note On arrives on `MT4 In M`  
    - Do **not** mix MidiBackend-internal stamps or WinUSB URB completion times into the same p99 series
  - [x] **Port names:** use live Bridge names `MT4 In N` / `MT4 Out N` (unit 1) or `MT4 #K In N` / `MT4 #K Out N` (unit K≥2) — **not** obsolete planning string `MT4 Port N`
  - [x] **software-loop (required):** Bridge-mediated Virtual Port round-trip **without** a physical DIN cable. Implement a **fail-closed, default-OFF** Bridge soft-echo (CLI/env gate) that copies host→OUT MIDI back onto the matching IN Virtual Port **without** claiming USB/device path. Label every run `path_type=software-loop`. Soft-echo must not alter default studio behavior when the gate is off.
  - [x] **hardware-loop (optional):** when DIN Out→In is present and Bridge soft-echo is off, full Host→Out→Bridge→WinUSB→MT4→DIN→…→In path; label `path_type=hardware-loop`. Missing DIN must not fake a Pass.
  - [x] Strip / normalize teVirtualMIDI trailing index suffixes when matching port names (same idea as `scripts/lab/*.py`)
  - [x] Never open ports exclusively; remain a normal multi-client peer (AD-8)
  - [x] Prefer Ctrl+C teardown guidance in tool help (CTRL_CLOSE can orphan Virtual Ports — known lab footgun)

- [x] Task 3: CI + quality gate (AC: 5)
  - [x] Update `.github/workflows/windows-build.yml`: after Bridge build, confirm harness `.exe` under `builds/ci` (mirror Bridge.exe confirm step)
  - [x] Extend `scripts/quality/lint-touched.py` `SCOPE_PREFIXES` to include `tools/` so harness C++ is linted (AD-15 / conventions §3)
  - [x] Run `python scripts/quality/lint-touched.py` on the touched C++ diff — must exit 0
  - [x] Hot-path rules: no jitter-inducing alloc/logging on the inject/observe critical path beyond what measurement requires (conventions §3.1 / §6.10)

- [x] Task 4: Operator smoke (honesty) (AC: 3, 4)
  - [x] Add `docs/tests/smoke-epic5-midi-path-harness-mt4.md` with rows: compile; software-loop run (Win10 + Bridge + virtualMIDI); hardware-loop optional; blank row ≠ Pass
  - [x] Explicit: ASIO buffer size is out of scope / never proof; numbers from this story are **plumbing proof**, not Studio-Done
  - [x] Do **not** create `docs/dev/measurements/` method + tables here (Story **5.2**). Optional empty stub only if needed for tree presence — prefer leaving 5.2 to own that folder

- [x] Task 5: Anti-scope fence
  - [x] Do not invent or “confirm” NFR-P1/P2 final anchors (Story **5.3** / OQ-2)
  - [x] Do not replace Python DIN labs under `scripts/lab/` — they remain product/SysEx labs, not AD-11
  - [x] Do not vendor VirtualMIDI SDK; harness uses WinMM against ports Bridge already created (LoadLibrary path stays in Bridge)
  - [x] Do not pull VirtualMIDI/WinUSB headers into `src/Protocol/` or `src/Profile/`
  - [x] Do not implement Windows MIDI Services backend (Epic **6**)
  - [x] Do not treat Public Installer / Authenticode as blockers (Correct Course 2026-08-10)

### Review Findings

- [x] [Review][Patch] Hardware-loop requires operator confirm soft-echo OFF (decision A2) [`tools/midi-path-harness/Main.cpp`]
- [x] [Review][Patch] Add `--no-soft-echo` force-off over sticky env (decision B2) [`src/Midi/SoftEchoGate.cpp` / `src/App/Main.cpp`]
- [x] [Review][Patch] Soft-echo claims handled when OUT has no matching IN (silent drop) [`src/Midi/VirtualMidiBackend.cpp:227`]
- [x] [Review][Patch] Soft-echo ignores `sendData_` failure and still skips USB [`src/Midi/VirtualMidiBackend.cpp:237`]
- [x] [Review][Patch] Observe wait uses `Sleep(1)` between pumps (can dominate sub-ms software-loop samples) [`tools/midi-path-harness/WinMmMidiIo.cpp:389`]
- [x] [Review][Patch] No midiIn queue drain before arm (stale Note On can false-match) [`tools/midi-path-harness/WinMmMidiIo.cpp:340`]
- [x] [Review][Patch] `--timeout-ms 0` accepted and fails every sample immediately [`tools/midi-path-harness/Main.cpp:51`]
- [x] [Review][Patch] Header comment says CALLBACK_FUNCTION but code uses CALLBACK_WINDOW [`tools/midi-path-harness/WinMmMidiIo.h:35`]
- [x] [Review][Patch] JSON summary does not escape port name strings [`tools/midi-path-harness/MidiPathRunner.cpp:134`]

## Dev Notes

### Epic context

Epic 5 delivers the Studio-Done **method** path: harness (5.1) → published measurements (5.2) → decision record for anchors (5.3). Correct Course 2026-08-10 unblocked Epic 5: run on interim **virtualMIDI + Win10 lab**; not blocked by certificate, Tobias redistrib, or WMS.

Provisional planning anchors (cite only as provisional, never as proven in 5.1): healthy ≤4–5 ms p99 bridge-added; jitter ≤1–2 ms p99; do-not-ship-worse ~8–10 ms p99 — NFR-P1/P2 / AD-11.

### Scope fence

| In 5.1 | Out (later) |
|---|---|
| `tools/midi-path-harness/` builds to `builds/` | `docs/dev/measurements/` method + tables (**5.2**) |
| QPC inject/observe + software-loop runnable | Studio-Done confirm/revise (**5.3** / OQ-2) |
| CI compiles harness when present | WMS MidiBackend (**Epic 6**) |
| Optional hardware-loop if DIN available | Claiming studio-done timing / SM-9 timing-done |

### Architecture compliance

- **AD-11** — harness location, QPC method, MIDI Path only, software-loop first, results path (publish deferred to 5.2)
- **AD-13** — Windows CI builds harness when present; artifacts under `builds/`; pin `windows-2022`
- **AD-7** — virtualMIDI remains interim lab backend through Epic 5
- **AD-8** — multi-client; harness is another client
- **AD-2** — keep Protocol/Profile free of VirtualMIDI/WinUSB
- **AD-15** — lint-touched on touched C++; extend scope to `tools/`
- **AD-1** — usermode only; no kernel VirtualMIDI Plan B

[Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-11, AD-13, Structural Seed]

### Files to CREATE (NEW)

| Path | Role |
|---|---|
| `tools/midi-path-harness/CMakeLists.txt` | `MidiPathHarness` target, C++17 |
| `tools/midi-path-harness/*.cpp` / `*.h` | QPC clock, WinMM I/O, runner, `Main` |
| `docs/tests/smoke-epic5-midi-path-harness-mt4.md` | Operator honesty smoke |

### Files to UPDATE

| Path | Change |
|---|---|
| `CMakeLists.txt` | `add_subdirectory(tools/midi-path-harness)` (or wire target) |
| `.github/workflows/windows-build.yml` | Confirm harness exe under `builds/ci` |
| `scripts/quality/lint-touched.py` | Add `tools/` to `SCOPE_PREFIXES` |
| Bridge sources (minimal) | Fail-closed soft-echo gate for software-loop (default OFF) — prefer tiny surface near VirtualMIDI OUT→IN delivery; do not grow Protocol |

### Current Bridge / build reality (preserve)

- Root CMake: `Bridge` + `BridgeTests` (Catch2 Profile/Protocol only — no WinUSB/VirtualMIDI in unit tests). Harness is a **separate executable**, not BridgeTests.
- CI already: quality-gate (lint) → `windows-2022` configure `builds/ci` → build Release → confirm `Bridge.exe` → `ctest` BridgeTests. Add harness confirm; do not remove BridgeTests.
- Port naming owned by `DeviceSessionManager`: `MT4 In N` / `MT4 Out N` (see `src/Device/DeviceSessionManager.h`).
- Bridge loads `teVirtualMIDI.dll` at runtime (`VirtualMidiBackend*`); harness should **not** duplicate SDK vendoring — WinMM client against named ports is enough.
- No `QueryPerformanceCounter` usage in `src/` today — introduce it in the harness (and only elsewhere if soft-echo needs shared helpers; prefer keep QPC in `tools/`).

### Known confounders (document in smoke / help — do not “fix” in 5.1)

- `processBulkRead` may hold `usbIoMutex_` across decode → host→device stall under load (affects **hardware-loop** credibility under stress; first baselines: quiet lab, no concurrent stress apps).
- OUT-hinted IN sticky can mis-attribute unlabeled DIN traffic — prefer labeled Note On match + known Out/In pair.
- CTRL_CLOSE orphan Virtual Ports — document Ctrl+C.

[Source: `_bmad-output/implementation-artifacts/deferred-work.md`]

### Testing requirements

- **CI (required):** harness compiles on `windows-2022`; exe found under `builds/ci`.
- **Lab (required for AC4 software-loop Pass):** Win10 x64, Bridge running with live session + virtualMIDI, soft-echo gate ON, harness `--path software-loop` completes inject/observe and prints latency samples (provisional / plumbing — not Studio-Done).
- **Lab (optional):** hardware-loop with DIN Out→In, soft-echo OFF.
- **Offline:** lint-touched on touched C++ under `tools/` (+ Bridge soft-echo files if any).
- Do **not** fake hardware Pass in Catch2 unit tests.

### Latest tech notes (QPC)

- Use `QueryPerformanceCounter` + `QueryPerformanceFrequency` for same-machine interval measurement ([Microsoft: Acquiring high-resolution time stamps](https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps)).
- Prefer keeping inject/observe timing on a dedicated thread; optional `SetThreadAffinityMask` to one CPU if cross-core QPC anomalies appear on older hosts.
- Do not use `Sleep` duration as latency; do not use ASIO/WASAPI buffer size as MIDI Path proof.

### Previous story intelligence

- Epic 4 fenced MIDI Path → Epic 5; keep the same honesty bar (blank smoke ≠ Pass; partial claims labeled).
- Epic 1.5/1.6 established VirtualMIDI runtime load + directional port names — harness measures those ports.
- Epic 2.5 explicitly forbade inventing Studio-Done anchors before harness — still true.
- Python labs under `scripts/lab/` show port normalize, Bridge ready markers, Ctrl+C teardown — **reuse patterns**, do not make Python the AD-11 deliverable.
- Story 1.1 reserved `tools/midi-path-harness/` as optional stub only — tree is still absent; this story creates the real tool.

### Git intelligence

Recent work is Correct Course / Epic 4 docs honesty, not MIDI backend changes. Last Bridge/Midi-touching commits: Epic 3 multi-client / dual-MT4 / review harden. Expect CMake + CI + new `tools/` tree as the main diff shape (similar to how installer stories grew `installer/` without touching Protocol).

### Project structure notes

- Dirs: kebab-case (`tools/midi-path-harness/`)
- C++ files: PascalCase TUs
- Output: `builds/` only (never repo-root `build/`)
- Vendor spelling in user-facing strings: **virtualMIDI**; code symbols stay `VirtualMidi*`

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Epic 5 / Story 5.1]
- [Source: `_bmad-output/planning-artifacts/architecture/.../ARCHITECTURE-SPINE.md` — AD-11, AD-13]
- [Source: `_bmad-output/specs/spec-unitor-win64-driver/SPEC.md` — CAP-16]
- [Source: `_bmad-output/planning-artifacts/sprint-change-proposal-2026-08-10.md`]
- [Source: `conventions.md` §3, §5, §6, §8]
- [Source: `.github/workflows/windows-build.yml`]
- [Source: `scripts/quality/lint-touched.py` — `SCOPE_PREFIXES`]

## Dev Agent Record

### Agent Model Used

Composer (Cursor Agent)

### Debug Log References

- Local `builds/ci` Release: Bridge + MidiPathHarness + BridgeTests green; `lint-touched.py --base HEAD` exit 0.
- Harness without live Bridge ports fails closed (`No MIDI OUT port matched`) — expected offline.

### Completion Notes List

- Added `tools/midi-path-harness/` (`MidiPathHarness`) with QPC inject/observe WinMM plane; CLI `--path software-loop|hardware-loop`, ports, samples, `--json`.
- Bridge soft-echo fail-closed default OFF: `--soft-echo` or `UNITOR_MIDI_SOFT_ECHO=1`; copies host→OUT to matching IN and skips USB sink.
- CI confirms `MidiPathHarness.exe` under `builds/ci`; lint scope includes `tools/`.
- Smoke guide documents plumbing-only honesty; no `docs/dev/measurements/` (deferred to 5.2); no NFR-P1/P2 anchors.
- Lab evidence capsule: Win10 software-loop Pass (50 samples, ~2 ms mean QPC) — not Studio-Done; hardware-loop not run.

### File List

- `tools/midi-path-harness/CMakeLists.txt`
- `tools/midi-path-harness/Main.cpp`
- `tools/midi-path-harness/QpcClock.h`
- `tools/midi-path-harness/QpcClock.cpp`
- `tools/midi-path-harness/WinMmMidiIo.h`
- `tools/midi-path-harness/WinMmMidiIo.cpp`
- `tools/midi-path-harness/MidiPathRunner.h`
- `tools/midi-path-harness/MidiPathRunner.cpp`
- `tools/midi-path-harness/PortNameNormalize.h`
- `tools/midi-path-harness/PortNameNormalize.cpp`
- `src/Midi/SoftEchoGate.h`
- `src/Midi/SoftEchoGate.cpp`
- `src/Midi/VirtualMidiBackend.h`
- `src/Midi/VirtualMidiBackend.cpp`
- `src/App/Main.cpp`
- `CMakeLists.txt`
- `.github/workflows/windows-build.yml`
- `scripts/quality/lint-touched.py`
- `docs/tests/smoke-epic5-midi-path-harness-mt4.md`
- `docs/tests/lab-evidence/README.md`
- `docs/tests/lab-evidence/midi-path-harness-software-loop-2026-08-11/README.md`
- `docs/tests/lab-evidence/midi-path-harness-software-loop-2026-08-11/harness-20260811T001700Z.log`
- `docs/tests/lab-evidence/midi-path-harness-software-loop-2026-08-11/bridge-start-excerpt-20260811T001600Z.log`
- `_bmad-output/implementation-artifacts/sprint-status.yaml`
- `_bmad-output/implementation-artifacts/5-1-in-repo-midi-path-harness-scaffold.md`

## Change Log

- 2026-08-10: Story context created (create-story) — ready-for-dev
- 2026-08-10: Implemented MIDI Path harness + Bridge soft-echo + CI/lint/smoke — review
- 2026-08-11: Lab evidence capsule — Win10 software-loop Pass (plumbing; not Studio-Done)
- 2026-08-11: Code review patches — hardware confirm, `--no-soft-echo`, soft-echo honesty, harness wait/drain/JSON — done
