---
title: 'Windows Bridge soak post mid-burst (measure-first)'
type: 'chore'
created: '2026-08-09'
status: 'in-review'
baseline_commit: 'd4ddbc72e9d1ba1101406efcb7bfdf08799192e6'
review_loop_iteration: 0
context:
  - '{project-root}/conventions.md'
  - '{project-root}/docs/dev/software-development-quality-principles.md'
  - '{project-root}/_bmad-output/implementation-artifacts/deferred-work.md'
  - '{project-root}/docs/tests/lab-evidence/overnight-macos-sysex-2026-08-08/'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** Day-gate mid/bank under Windows Bridge is green after cold-start and mid-burst fixes, and macOS overnight (~8 h, Apple driver, no Bridge) already proved the MT4+Matrix hardware. What is still unproven is long-duration hold of the **current** Windows Bridge — no post-`cd64193` overnight evidence yet.

**Approach:** Measure first on `main` with the existing Windows overnight harness (mid + bank). Ship a clear Pass/Fail soak report. Change Bridge code **only** if a real lab hole reappears (truncated: truncated Matrix dump / `TIMEOUT last=none`), then minimal fix + units + review + re-prove. If soak is green: no preventive code.

## Boundaries & Constraints

**Always:**
- Start from clean `main` (pull if needed); rebuild debug Bridge before soak.
- Gate stays `--pass-percent 100`. Incomplete/corrupt Matrix dumps are failures (AD-18).
- Keep won Bridge invariants: leading-F0 repair, `kBulkInAsyncSlotCount = 32`, `kMatrixDumpSizeRejectRetries = 2`, nested rewrite guard, expect-before-flush / deferred-retry family from `835c992`.
- One cause → one minimal fix if a hole appears; run `scripts/quality/lint-touched.py` on touched C++.
- Code review (Quick Dev step-04 or `bmad-code-review`) before declaring victory on any code change.
- No commit unless Guillaume explicitly asks.

**Ask First:**
- Hardware not ready (MT4 USB + Matrix on DIN Out1↔In, ports free) before starting soak.
- Cause fingerprint unclear after a FAIL — do not invent a second theory; present evidence and wait.
- Any change that would loosen harness gate, discard mid-burst, or alter cold-start F0 / ring / retries without a new lab fingerprint.

**Never:**
- Preventive Bridge redesign “just in case” when soak is green.
- Kernel driver work; macOS overnight redo; long-SysEx palier as this pass’s goal.
- Mask by lowering `pass-percent`, ignoring dumps, or magic delays without fingerprint.
- Reopen deferred entries already `resolved-by-615882d` / `resolved-by-cd64193` without new proof.
- Retest/revert cold-start or day-gate mid-burst without new evidence.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Palier A green | Overnight mid+bank Bridge ~2–4 h @ 100% | Journal + cycle logs: 0 FAIL mid, 0 FAIL bank | Report Pass; optional Palier B |
| Palier B green | Same harness ~8 h @ 100% | Same, longer duration | Report Pass; no code |
| Hole mid-soak | Lab `TIMEOUT … last=none` and/or Bridge `SysEx size reject` under load | Stop “measure-only”; investigate bulk-IN / MatrixDump path | One cause → minimal Bridge fix |
| False cold-start chase | First-dump-only miss with F0 fingerprint | Treat as regression of closed cold-start, not mid-burst | Ask First before touching F0 |
| Green soak | A (and B if run) Pass | Update deferred only if a real residual remains; no code | Mission complete |

</frozen-after-approval>

## Code Map

- `scripts/lab/overnight-matrix-stress.py` -- Windows Bridge soak wrapper (mid then bank cycles; logs under `tests/lab-logs/overnight-matrix/`)
- `scripts/lab/sysex-matrix-mid-loop.py` -- mid day-gate / re-proof (`--with-bridge`, `--fresh-starts`, `--pass-percent`)
- `scripts/lab/sysex-matrix-bank-loop.py` -- bank day-gate / re-proof
- `builds/debug/Debug/Bridge.exe` -- default Bridge under test
- `src/Usb/WinUsbTransport.h` -- bulk-IN async ring (`kBulkInAsyncSlotCount`) — touch only if hole points here
- `src/Device/DeviceSessionSupport.h` -- `kMatrixDumpSizeRejectRetries`
- `src/Device/DeviceSessionHostOutbound.cpp` -- size-reject arm / rewrite retries
- `src/Device/DeviceSessionMatrixDump.cpp` -- dump rewrite guard / short-frame drop
- `_bmad-output/implementation-artifacts/deferred-work.md` -- residual notes only; do not reopen resolved cold-start / day-gate mid-burst
- `docs/tests/lab-evidence/overnight-macos-sysex-2026-08-08/` -- hardware control (Apple, no Bridge) — do not re-run as goal

## Tasks & Acceptance

**Execution:**
- [ ] `main` working tree -- confirm clean + up to date with `origin/main`; `cmake --build --preset debug` if Bridge binary stale -- baseline for soak
- [ ] Hardware checklist -- MT4 USB + Matrix DIN Out1↔In (not Thru); no MIDI-OX / Matrix-Control / DAW on ports -- Ask First if blocked
- [ ] `scripts/lab/overnight-matrix-stress.py` -- run **Palier A** (`--hours 2` or `4`) with default Bridge; logs under `tests/lab-logs/overnight-matrix/` (optionally mirror day-gate dirs under `post-midburst-soak` if documenting separate mid/bank loops) -- measure-first proof
- [ ] Same script -- if A green, run **Palier B** (`--hours 8`) recommended -- long hold matching macOS control duration spirit
- [ ] `_bmad-output/implementation-artifacts/` report (spec Verification + deferred if needed) -- Pass/Fail table, exact commands, log paths -- deliverable when green
- [ ] **Only if FAIL:** Bridge logs around miss (`SysEx size reject`, `retries_left`, nested rewrite) + MatrixDump / WinUSB path -- single cause, Ask First if ambiguous
- [ ] **Only if FAIL:** minimal Bridge fix in mapped sources + unit coverage for the edge + `python scripts/quality/lint-touched.py` -- cause-shaped, no harness mask
- [ ] **Only if FAIL:** re-proof mid `5×10` + bank `20×100` @ 100% and re-run the failing soak duration -- close the loop
- [ ] Code review on any C++ diff before done -- step-04 / adversarial review

**Acceptance Criteria:**
- Given clean `main` with mid-burst (`cd64193` or newer) built, when Palier A (~2–4 h overnight mid+bank Bridge @ 100%) completes, then the journal shows **0 FAIL** mid and **0 FAIL** bank, and log paths are recorded.
- Given Palier A green and Palier B is run, when ~8 h completes at 100%, then the same Pass record is extended; **no** Bridge code change is introduced “for safety.”
- Given a soak FAIL with truncated dump / `TIMEOUT last=none`, when investigation finishes, then exactly one Bridge cause is named, a minimal fix (+ units + lint) lands, review passes, and mid `5×10` + bank `20×100` @ 100% plus the failed soak scenario re-pass.
- Given soak green, when closing, then deferred does **not** reopen cold-start / day-gate mid-burst resolved entries; only a real remaining residual may be noted.

## Spec Change Log

## Verification

**Commands:**
- `cmake --build --preset debug` -- Bridge.exe builds
- `python scripts/lab/overnight-matrix-stress.py --hours 2` (or `4`) -- Palier A: 0 mid FAIL, 0 bank FAIL in overnight journal
- `python scripts/lab/overnight-matrix-stress.py --hours 8` -- Palier B (if A green): same
- Day-gate re-proof (required after any code fix; optional smoke before soak):
  - `python scripts/lab/sysex-matrix-mid-loop.py --with-bridge --out-port "MT4 Out 1" --in-port "MT4 In 1" --pass-percent 100 --count 10 --fresh-starts 5 --bridge-exe builds\debug\Debug\Bridge.exe --log-dir tests\lab-logs\sysex-matrix-mid\post-midburst-soak`
  - `python scripts/lab/sysex-matrix-bank-loop.py --with-bridge --out-port "MT4 Out 1" --in-port "MT4 In 1" --pass-percent 100 --count 100 --fresh-starts 20 --bridge-exe builds\debug\Debug\Bridge.exe --log-dir tests\lab-logs\sysex-matrix-bank\post-midburst-soak`
- After C++ change: `python scripts/quality/lint-touched.py` -- clean on touched files
- Unit tests covering the fixed edge (if code changed) -- pass

**Manual checks (if no CLI):**
- MT4 + Matrix topo and free ports before soak
- If FAIL: Bridge log fingerprint near miss (size reject / retries / nested rewrite) before coding
)
