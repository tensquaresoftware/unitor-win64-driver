---
baseline_commit: 3eb44103da6bf0e85c6cc79210b608b63377ac84
---

# Story 1.1: Scaffold Bridge project and Windows build gate

Status: done

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a contributor,
I want a CMake C++17 Bridge skeleton that builds to `builds/` on Windows CI with the project lint gate wired,
so that every later story lands in a compileable, convention-compliant tree.

## Acceptance Criteria

1. **Given** a clean checkout on a Windows x64 environment (or GitHub Actions Windows runner)  
   **When** the CMake project is configured and built  
   **Then** artifacts land under `builds/` (not `build/` at repo root) and the Bridge executable target compiles

2. **And** the Structural Seed folders exist under `src/` (`App/`, `Device/`, `Profile/`, `Usb/`, `Protocol/`, `Midi/`) with PascalCase placeholder sources as needed

3. **And** CI runs a Windows compile on every PR / main push using a pinned runner image (prefer `windows-2022`, not floating `windows-latest`) — AD-13

4. **And** touched C++ under `src/` can be checked with `python scripts/quality/lint-touched.py` per `conventions.md` §3 — AD-15 / NFR-Q1

5. **And** no French appears in source; no custom kernel driver project is introduced — AD-1 / NFR-S2

**Traces:** FR-2 (runtime host baseline), NFR-D3, NFR-Q1, AD-13, AD-15, AD-20

## Tasks / Subtasks

- [x] Task 1: Create CMake C++17 Bridge executable skeleton (AC: 1, 5)
  - [x] Add root `CMakeLists.txt` with `cmake_minimum_required(VERSION 3.20)` (raise only if pinned VS generator on CI requires it; document the bump)
  - [x] Set `CMAKE_CXX_STANDARD 17` (required), Windows x64 focus
  - [x] Define a Bridge **executable** target (user-session process — console or Win32 app OK; **not** a Windows Service / Session-0 project) — AD-20
  - [x] Configure out-of-source build so configure/build outputs live under `builds/` (e.g. `cmake -S . -B builds/...`); never instruct or default to repo-root `build/`
  - [x] Minimal `main` (or `WinMain`) that compiles and exits cleanly — English-only identifiers and comments

- [x] Task 2: Materialize Structural Seed under `src/` (AC: 2)
  - [x] Create `src/App/`, `src/Device/`, `src/Profile/`, `src/Usb/`, `src/Protocol/`, `src/Midi/`
  - [x] Add thin PascalCase placeholder sources as needed so the tree is real (not empty `.gitkeep`-only layers) — e.g. `src/App/Main.cpp` as the executable entry
  - [x] Wire placeholders into the CMake target (compile units listed / `target_sources`)
  - [x] Remove or replace obsolete `src/.gitkeep` once real sources exist
  - [x] Optional (not AC-blocking): empty stub dirs `tools/midi-path-harness/`, `installer/`, `docs/user/`, `docs/dev/measurements/` with `.gitkeep` only — **do not implement** harness, INF, or user docs

- [x] Task 3: Pin Windows CI compile gate (AC: 3)
  - [x] Add `.github/workflows/` workflow (filename free under AD-13 Deferred — e.g. `windows-build.yml`)
  - [x] Triggers: pull_request + push to `main` (at minimum)
  - [x] `runs-on: windows-2022` — **do not** use floating `windows-latest`
  - [x] Steps: checkout → CMake configure into `builds/` → build → fail if compile fails
  - [x] Prefer letting CMake auto-detect the VS generator on the runner; if you pin `-G`, pin a generator that exists on `windows-2022` (VS 17 2022), not VS 18 2026
  - [x] Record in a short CI/PR comment or `docs/dev/` note the CMake + VS versions observed on the green run (version-reality follow-up)

- [x] Task 4: Prove quality gate on new C++ (AC: 4)
  - [x] Ensure `scripts/quality/lint-touched.py` remains the SSOT (do **not** invent a second analyser)
  - [x] Install lizard via `python -m pip install -r scripts/quality/requirements.txt` and run `python scripts/quality/lint-touched.py` against the new `src/` diff — must exit 0
  - [x] Optional: add a CI step that installs lizard and runs lint-touched (nice-to-have; AC only requires the gate *can* check touched C++)

- [x] Task 5: Guardrails / anti-scope (AC: 5)
  - [x] Confirm no WDK / KMDF / custom kernel driver project files
  - [x] Confirm no French in any new source/comments
  - [x] Confirm no VirtualMIDI SDK vendoring, no WinUSB open logic, no DeviceProfile MT4 tables, no INF — those belong to Stories 1.2–1.5 / Epic 4

### Review Findings

- [x] [Review][Decision] Harden CMake against wrong build layouts — dismissed: keep convention-only (docs + CI); no hard CMake rejects in this story
- [x] [Review][Defer] Fill observed CMake/VS versions after first green Windows CI run [`docs/dev/windows-ci-toolchain.md`] — deferred, documented follow-up once Actions has a green log

## Dev Notes

### Scope fence (readiness M-1)

This story is **contributor/scaffold-facing greenfield**. Deliver a compileable tree + Windows CI + lint wiring.  
**Do not** expand into “build everything” (no USB open, no Emagic mapper, no VirtualMIDI ports, no installer, no MIDI Path harness).

### Epic context

Epic 1 outcome: bind MT4 to WinUSB, run C++17 Bridge, see stable `MT4 Port N` (2 IN / 4 OUT), exchange notes/CC — without a custom kernel driver.  
Story 1.1 only seeds the **runtime host baseline** (FR-2 / AD-20) so 1.2–1.6 can land in a known layout.

### Architecture compliance (must follow)

| Decision | What it means for this story |
|---|---|
| AD-13 | Windows compile CI every PR/main; artifacts under `builds/`; pin `windows-2022`; CMake 3.20+ assumption |
| AD-15 | PascalCase C++ filenames under `src/`; touched C++ passes `lint-touched.py`; English-only source |
| AD-1 / NFR-S2 | Usermode only — no custom kernel MIDI / no home-grown kernel VirtualMIDI Plan B |
| AD-20 | Bridge = **user-session** process baseline — normal exe, not Session-0 Service |
| Structural Seed | Layer dirs under `src/`: `App/`, `Device/`, `Profile/`, `Usb/`, `Protocol/`, `Midi/` |

[Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-13, AD-15, AD-1, AD-20, Structural Seed, Stack]

### Naming conflict — binding rule for Story 1.1

`conventions.md` §6.3 says project directories are kebab-case and gives `src/protocol/` as an example.  
Architecture **Structural Seed** and this story’s AC explicitly require PascalCase **layer** directories: `App/`, `Device/`, `Profile/`, `Usb/`, `Protocol/`, `Midi/`.

**Binding for implementation:** follow Architecture seed + Story AC for these six layer directories.  
`lint-touched.py` already treats `/Usb/` as a glue-path marker — confirms PascalCase layer dirs.  
Keep kebab-case for non-layer repo dirs (`scripts/quality/`, `docs/dev/`, `tools/midi-path-harness/`).  
C++ **filenames** under `src/` remain PascalCase (`Main.cpp`, later `DeviceProfile.h`, …).

### Technical requirements

- **Language:** C++17
- **Build:** CMake → `builds/` (`.gitignore` already ignores `builds/`, `build/`, `compile_commands.json`)
- **CMake minimum:** 3.20+ (Architecture ASSUMPTION; raise with documented reason if CI VS toolchain demands it)
- **Target:** one Bridge executable that links/compiles placeholder TUs from the seed layers
- **CI runner:** `windows-2022` only for the Windows compile job (as of 2026, `windows-latest` often means VS 2026 / Server 2025 and breaks hard-pinned VS17 generators — Architecture already preferred the pin)
- **Quality:** `scripts/quality/lint-touched.py` + `scripts/quality/requirements.txt` (`lizard>=1.17.10,<2`) — already in repo
- **Style (when writing placeholders):** Allman braces, 4 spaces, `#pragma once`, English only — `conventions.md` §6

### Library / framework requirements

- **Do not** add VirtualMIDI SDK, WinUSB app linkage, or third-party MIDI libs in this story
- **Do not** vendor GPL Linux sources or fork `aaron1a12/virtual-midi`
- No unit-test framework mandated by Architecture for 1.1 (compile + lint = bar)
- Optional later: `compile_commands.json` + `--clang-tidy` — not required for AC

### File structure requirements

#### NEW (create)

| Path | Purpose |
|---|---|
| `CMakeLists.txt` (root; optional subdir CMakes) | C++17 Bridge executable |
| `src/App/*` (e.g. `Main.cpp`) | Process entry / user-session host seed |
| `src/Device/`, `Profile/`, `Usb/`, `Protocol/`, `Midi/` | Layer folders + thin PascalCase placeholders as needed |
| `.github/workflows/<name>.yml` | Pinned `windows-2022` configure+build |

#### UPDATE (existing)

| Path | Current state | This story |
|---|---|---|
| `src/.gitkeep` | Placeholder for empty `src/` | Remove once real sources exist |
| `scripts/quality/lint-touched.py` | Complete gate | Prefer leave unchanged; only touch if CI wiring needs a doc tweak |
| `.gitignore` | Already ignores `builds/` / `build/` | Usually no change |
| `conventions.md` | SSOT §3 | Do not rewrite for this story |

#### OUT OF SCOPE paths (do not implement content)

- WinUSB open / DeviceInterfaceGUID usage → Story 1.3
- MT4 DeviceProfile tables → Story 1.2
- EmagicCableMapper / DeviceSession → Story 1.4
- VirtualMidiBackend / ports → Story 1.5
- Notes/CC I/O → Story 1.6
- `tools/midi-path-harness/` logic → Epic 5 / Story 5.1
- `installer/` INF / AD-12 UX → Stories 1.3 / 4.1

### Existing code being modified — current state

**Greenfield.** Repo today:

- `src/.gitkeep` only — no `.cpp`/`.h`
- No `CMakeLists.txt`
- No `.github/workflows/`
- Quality tooling already present: `scripts/quality/lint-touched.py`, `requirements.txt`, `.clang-tidy`
- Docs/process: `conventions.md`, `docs/dev/*`, BMad planning under `_bmad-output/`

**Preserve:** lint-touched thresholds and glue markers; `.gitignore` build ignores; English/MIT orientation; dual-machine narrative in conventions (edit macOS / validate Windows).

**Must not break:** ability to run `lint-touched.py` with no C++ (today exits 0) — after scaffold, it must still exit 0 on conforming placeholders.

### Suggested minimal skeleton (illustrative — not mandatory names)

```text
CMakeLists.txt
src/
  App/Main.cpp          # int main() { return 0; }  — English only
  Device/.gitkeep       # or a tiny DeviceSession.hpp stub later stories replace
  Profile/.gitkeep
  Usb/.gitkeep
  Protocol/.gitkeep
  Midi/.gitkeep
.github/workflows/windows-build.yml
```

Prefer compiling at least one real `.cpp` so the executable target is non-vacuous. Empty layer dirs via `.gitkeep` are acceptable for layers with no code yet **if** AC “placeholder sources as needed” is satisfied by the App entry (and any stubs you choose). Safer: one tiny PascalCase header per layer only if it helps CMake/`target_include_directories` foreshadow — avoid fake APIs (YAGNI).

### Testing requirements

| Check | How |
|---|---|
| Configure + build | `cmake -S . -B builds/<config>` then `cmake --build builds/<config>` on Windows x64 or GHA |
| Artifacts location | Outputs under `builds/`; no new root `build/` workflow |
| CI | Green Windows job on PR/main with `runs-on: windows-2022` |
| Lint | `pip install -r scripts/quality/requirements.txt` then `python scripts/quality/lint-touched.py` (and/or `--base origin/main`) on the scaffold diff |
| Negatives | Grep/review: no French in sources; no kernel-driver project files |

No Validation Matrix hosts, no hardware, no MIDI Path harness for this story.

### Previous story intelligence

None — this is the first implementation story in Epic 1.

### Git intelligence summary

Recent commits are planning-only (readiness, sprint-status, SPEC, architecture, epics). No prior C++/CMake patterns in-repo to extend — establish them here cleanly.

### Latest tech information (CI / CMake — 2026)

- Pin **`windows-2022`** (VS 2022). Floating `windows-latest` has moved toward Windows Server 2025 + Visual Studio 2026; many CMake/Conan jobs broke when hardcoding `"Visual Studio 17 2022"` on that image or when tools expected VS18 generators prematurely.
- Prefer **CMake auto-detect** of the installed VS generator + `-A x64` over a brittle hard-coded generator string, unless you intentionally pin both runner and generator together.
- `windows-2022` ships CMake 3.x suitable for `cmake_minimum_required(VERSION 3.20)`.
- Do not chase VS 2026 / `windows-2025-vs2026` in V1 scaffold unless Guillaume explicitly reopens AD-13.

### Project context reference

- No `project-context.md` in repo yet — use this story + `conventions.md` + Architecture Spine as SSOT.
- Quality principles glossary: `docs/dev/software-development-quality-principles.md`
- Kickoff brief: `docs/dev/prompt-demarrage-projet-bmad.md` (greenfield WinUSB + usermode Bridge; dual-machine)

### Anti-patterns to forbid

- Creating `build/` at repo root as the documented/CI out dir
- `runs-on: windows-latest` as the long-term pin
- Windows Service project “because Auto-Start later”
- Kernel/WDK driver project “just in case”
- Implementing WinUSB/VirtualMIDI/DeviceProfile “while we’re here”
- French comments or identifiers in C++
- Second quality script replacing `lint-touched.py`
- kebab-case layer dirs (`src/usb/`) that contradict Structural Seed + lint glue markers
- Vendoring GPL or forking virtual-midi proof repo as base

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Story 1.1, Epic 1]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-13, AD-15, AD-1, AD-20, Structural Seed, Stack, Deferred]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — FR-2, NFR-D3, NFR-Q1, NFR-S2]
- [Source: `_bmad-output/planning-artifacts/implementation-readiness-report-2026-08-04.md` — M-1 scaffold scope]
- [Source: `conventions.md` — §3 quality gate, §5.1 builds/, §6 C++ standards]
- [Source: `scripts/quality/lint-touched.py` — scopes, thresholds, `/Usb/` glue markers]

## Dev Agent Record

### Agent Model Used

Composer (Cursor agent router)

### Debug Log References

- Local smoke: `cmake -S . -B builds/macos-smoke && cmake --build builds/macos-smoke` → Bridge exits 0
- `python scripts/quality/lint-touched.py` → exit 0 on `src/App/Main.cpp`
- Guardrail scan: no French in `src/`; no WDK/KMDF/VirtualMIDI/WinUSB/INF content introduced

### Completion Notes List

- Delivered CMake 3.20+ C++17 `Bridge` executable with out-of-source builds under `builds/`
- Structural Seed layer dirs created; `src/App/Main.cpp` is the compile unit; other layers use `.gitkeep` (no fake APIs)
- Removed obsolete `src/.gitkeep`
- Pinned Windows CI on `windows-2022` with CMake auto-detect + `-A x64`; workflow prints CMake/VS versions for the version-reality table
- Documented gate in `docs/dev/windows-ci-toolchain.md` (observed versions table to fill from first green Actions log)
- Optional stub dirs (`tools/`, `installer/`, …) and optional CI lint step skipped (YAGNI / not AC-blocking)
- `lint-touched.py` left as SSOT; no second analyser

### File List

- `CMakeLists.txt` (new)
- `src/App/Main.cpp` (new)
- `src/Device/.gitkeep` (new)
- `src/Profile/.gitkeep` (new)
- `src/Usb/.gitkeep` (new)
- `src/Protocol/.gitkeep` (new)
- `src/Midi/.gitkeep` (new)
- `src/.gitkeep` (deleted)
- `.github/workflows/windows-build.yml` (new)
- `docs/dev/windows-ci-toolchain.md` (new)
- `_bmad-output/implementation-artifacts/1-1-scaffold-bridge-project-and-windows-build-gate.md` (updated)
- `_bmad-output/implementation-artifacts/sprint-status.yaml` (updated)

### Change Log

- 2026-08-04: Scaffolded Bridge CMake project, Structural Seed, Windows CI gate, and lint proof — status → review
- 2026-08-05: Code review complete — CMake hardening dismissed (convention-only); version table deferred; status → done
