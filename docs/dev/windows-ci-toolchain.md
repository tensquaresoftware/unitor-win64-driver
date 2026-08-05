# Windows CI toolchain

Pinned merge gate for the Bridge usermode host (WinUSB + VirtualMIDI). Windows x64 only — no macOS/Linux CI runners.

## Workflow

| Item | Value |
|---|---|
| Workflow | `.github/workflows/windows-build.yml` |
| Triggers | `push` to `main`, all `pull_request` |
| Merge gate job | `ci-success` (require this check alone in branch protection) |
| Checkout action | `actions/checkout@v5` |

### Jobs

| Job | Runner | Role |
|---|---|---|
| `quality-gate` | `ubuntu-latest` | `scripts/quality/lint-touched.py` on the PR/push C++ diff (`conventions.md` §3) |
| `build` | `windows-2022` (not `windows-latest`) | Runs after `quality-gate`; CMake configure + Release build; assert `Bridge.exe` under `builds/ci` |
| `ci-success` | `ubuntu-latest` | Green only when both upstream jobs succeed (`always()` + explicit success checks) |

Also: `permissions: contents: read`, and `concurrency` cancels superseded runs on the same PR/branch.

### Build commands (CI)

```text
cmake -S . -B builds/ci -A x64
cmake --build builds/ci --config Release
```

CMake minimum (project): 3.20. Generator: left to CMake auto-detect on the runner (Visual Studio 2022 on `windows-2022`). Do not hard-pin VS 18 / 2026 generators for V1.

### Quality gate (CI)

- PR: lint base = `origin/<base_ref>` (fetch base ref first).
- Push to `main`: lint base = `github.event.before` when valid, else `HEAD~1`.
- Dependencies: `python3 -m pip install -r scripts/quality/requirements.txt`
- Do **not** use `--all` as a failing CI gate (anti-drift on touched hunks only).

## Local checks

### Windows x64 build

```text
cmake -S . -B builds/debug -A x64
cmake --build builds/debug --config Debug
```

Outputs must stay under `builds/`. Never use repo-root `build/` as the documented out directory.

### Quality lint (any OS with git + Python)

```text
python -m pip install -r scripts/quality/requirements.txt
python scripts/quality/lint-touched.py --base origin/main
```

For a PR whose base is not `main`, use the same base ref CI uses (`origin/<base_branch>`).

## Observed versions

The `build` job prints CMake and Visual Studio versions on every green run (`Record toolchain versions` step). Copy the values from a successful Actions log into the table below when first verified on `main` / a PR.

| Tool | Observed on green run | Date |
|---|---|---|
| CMake | _(fill from Actions log)_ | |
| Visual Studio | _(fill from Actions log)_ | |

## What CI does **not** cover (manual Windows PC)

Keep these on a Windows 10/11 x64 machine with hardware / DAW as needed:

- WinUSB bind (Zadig) and live MT4 open
- VirtualMIDI port visibility and DAW round-trip
- SysEx, clock, and long soak sessions
- Epic 1 hardware smoke checklist: `docs/tests/smoke-epic1-mt4.md`

## Deferred: C++ unit tests without hardware

No CMake `Tests` target yet. Pure-logic candidates for a follow-up ticket (not this pipeline):

- `src/Protocol/EmagicCableMapper.*` — Emagic F5 cable encode/decode
- `src/Profile/DeviceProfile.*` — declarative VID/PID / port profile
- `src/App/MapperSmoke.*` — existing synthetic mapper smoke (`--test-mapper`)

Tracked in `_bmad-output/implementation-artifacts/deferred-work.md`.

## Matrix-Control: reused vs omitted

| Reused (adapted) | Omitted on purpose |
|---|---|
| `quality-gate` + `lint-touched` on diff | Multi-OS build matrix (macOS/Linux) |
| `ci-success` as sole merge signal | Draft-PR fast tier / `ci-full` label |
| `actions/checkout@v5` | JUCE checkout / plugin presets |
| Push/PR base resolution for lint | `release.yml` / Authenticode packaging |
