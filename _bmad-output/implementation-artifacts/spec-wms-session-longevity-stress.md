---
title: 'WMS single-session longevity stress (Win11)'
type: 'chore'
created: '2026-08-21'
status: 'in-progress'
baseline_commit: 'ee69d4dcc772a2086be69690a6e953e4bbf0e679'
review_loop_iteration: 0
context:
  - '{project-root}/_bmad-output/implementation-artifacts/epic-6-context.md'
  - '{project-root}/_bmad-output/implementation-artifacts/spec-6-1-windows-midi-services-midibackend-win11.md'
  - '{project-root}/conventions.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** Short WMS lab runs already hit 100% on Matrix mid, bank burst, and long DIN SysEx, but there is no proof that one continuous Bridge session under Windows MIDI Services stays reliable for studio-length sessions the way virtualMIDI did on Windows 10.

**Approach:** Add a single-session stress harness that keeps one Bridge WMS process alive and loops the three already-validated loads for at least ~1 h at 100% pass; harden Bridge/WMS code only if that lab fails with a clear transport or Bridge fault.

## Boundaries & Constraints

**Always:**
- One Bridge process for the whole stress (`--start-session --dev-zadig --midi-backend=wms`); no kill/restart between mid, bank, and long phases.
- Reuse existing child labs and proven short-run parameters (Matrix Out1/In1; long Out2→In2 sizes 1024, 4096, fixture ~14708).
- Abort immediately on first child FAIL, Bridge crash, missing ports, queue overflow, or midisrv death during a stable session; journal clearly under `tests/lab-logs/…`.
- Keep existing WMS fixes: `Midi1StreamAssembler` before HostOutboundQueue; `sendUmpWords` BufferFull retry cadence.
- Chat FR / code+docs EN; commit only on explicit request; `lint-touched.py` green on any C++/Python diff before done.

**Ask First:**
- Extending target duration past 1 h toward 4 h after a green 1 h run (lab time permitting).
- Changing Matrix / DIN topology or substituting soft-echo for hardware.
- Any Bridge/WMS change that is not tied to a reproduced longevity failure.

**Never:**
- Treating the overnight-combined `--with-bridge` restart harness as the success gate for this Build.
- Preventive Destroy/Create or midisrv hardening without a failure observed in a stable single session.
- Expanding Epic 6 scope (6.2 packaging, 6.3 full matrix rewrite) or rewriting WMS architecture.
- Soft-echo as a substitute for Matrix + DIN hardware proof.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Happy path ≥1 h | External Bridge WMS live; mid→bank→long loop until deadline | All phases exit 0; journal shows zero fails; same Bridge PID throughout | N/A |
| Child FAIL mid-session | Mid/bank/long returns non-zero | Harness stops immediately; FAIL journaled with cycle/phase | No further cycles; leave Bridge up for diagnosis unless crashed |
| Bridge dies | Bridge process exits during stress | Harness detects and aborts as FAIL | Capture last Bridge log path |
| midisrv dies mid-run | Service/process gone while Bridge was stable | Abort and report as environment/session fault | Optional preflight reset only if midisrv already sick before start |
| BufferFull / overflow | Device→host SysEx or host queue pressure | Prefer existing retry/assembler; if still fails, minimal Bridge fix then re-run | Log HRESULT / overflow; no preventive refactor |

</frozen-after-approval>

## Code Map

- `scripts/lab/overnight-combined-stress.py` — clone pattern for mid→bank→long + `--hours` + awake hold; today hardcodes `--with-bridge` in `_matrix_child_args` / `_long_child_args` (~L133–172); success gate soft-counts fails (must harden for this Build: abort on first FAIL).
- **Create or extend** single-session orchestrator under `scripts/lab/` (prefer new thin wrapper e.g. `wms-session-longevity-stress.py`, or optional `--external-bridge` / no-`--with-bridge` mode on combined) — starts or assumes one Bridge with `--dev-zadig --midi-backend=wms`; children omit `--with-bridge`; logs under `tests/lab-logs/wms-session-longevity/`.
- `scripts/lab/lab_midi_common.py` — `BridgeSession` (~L165+): `extra_args` already supports `--dev-zadig` / `--midi-backend=wms`.
- `scripts/lab/sysex-matrix-mid-loop.py` + `sysex_matrix_mid_loop_lib.py` — Matrix mid Out1/In1; omit `--with-bridge` when ports exist.
- `scripts/lab/sysex-matrix-bank-loop.py` + `sysex_matrix_bank_loop_lib.py` — bank burst same ports.
- `scripts/lab/sysex-long-loopback.py` + `sysex_long_loopback_lib.py` — long DIN Out2/In2; sizes + fixture as overnight combined.
- `src/Midi/WmsMidiBackend.cpp` — touch only on lab fail: `sendUmpChunkWithRetry` / `sendUmpWords` (~L172–248), `SendToHost` (~L371–398), `acceptHostMidiBytes` + assemblers (~L339–368).
- `src/Midi/Midi1StreamAssembler.cpp` — keep SysEx reassembly before HostOutboundQueue; touch only if overflow returns.
- **Read-only unless forced:** `src/App/Main.cpp` backend flag; Epic 6 architecture; overnight restart-heavy path as non-gate.
- Evidence pattern: `docs/tests/lab-evidence/` capsules; short-run SSOT checklists under `docs/tests/checklists/smoke-mt4-sysex-*.md`.

## Tasks & Acceptance

**Execution:**
- [x] `scripts/lab/wms-session-longevity-stress.py` (or minimal `--external-bridge` extension of overnight-combined) -- orchestrate one Bridge WMS + mid→bank→long loop for `--hours` with abort-on-FAIL and clear journal -- enables the longevity gate without restart pollution.
- [x] Lab preflight -- if midisrv already unhealthy before start, reset once then start a clean Bridge; confirm Matrix Out1/In1 + DIN Out2→In2 ports -- avoids false fails from a sick host stack.
- [ ] Run stress ≥1 h under single Bridge WMS -- prove 100% across all phases; on FAIL, isolate cause, apply minimal Bridge/WMS fix, re-run to green -- product proof first, code only when evidence demands.
- [ ] Evidence note under `tests/lab-logs/wms-session-longevity/` (+ optional short FR summary in chat / lab-evidence capsule) -- records journal path, duration, pass counts, Bridge PID continuity; no commit unless asked.
- [x] If C++/Python changed: `python scripts/quality/lint-touched.py` green -- quality gate before claiming done.

**Acceptance Criteria:**
- Given Win11 lab with MT4 WinUSB and Matrix+DIN topology, when the single-session WMS stress runs for at least ~1 h, then every mid/bank/long phase passes at 100% with one continuous Bridge process and a journal proving zero phase failures.
- Given a phase failure or Bridge/midisrv collapse mid-run, when the harness detects it, then the stress stops immediately with actionable logs (no silent continuation).
- Given a reproduced Bridge/WMS fault (overflow, BufferFull exhaustion, crash), when a minimal fix lands, then the same single-session stress returns to 100% without preventive refactors outside the failure.

## Spec Change Log

## Verification

**Commands:**
- `.venv-lab\Scripts\python.exe scripts/lab/wms-session-longevity-stress.py --hours 0.083` -- expected: preflight green (short loop) under one Bridge WMS, abort-on-FAIL semantics verified.
- `.venv-lab\Scripts\python.exe scripts/lab/wms-session-longevity-stress.py --hours 1` -- expected: journal 100% mid/bank/long; same Bridge PID; no midisrv death mid-run.
- `python scripts/quality/lint-touched.py` -- expected: exit 0 on any touched C++/Python (when code changed).

**Manual checks (if no CLI):**
- Interrupt immediately if LEDs die, ports vanish, Bridge crashes, or queue overflow appears in Bridge log; capture paths before teardown.
