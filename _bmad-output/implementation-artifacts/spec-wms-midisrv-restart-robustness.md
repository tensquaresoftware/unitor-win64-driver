---
title: 'WMS midisrv restart robustness (lab + minimal teardown)'
type: 'bugfix'
created: '2026-08-21'
status: 'done'
baseline_commit: '099fd4d0179de2302d2ef3ff32d522ac3e47c6fc'
review_loop_iteration: 0
context:
  - '{project-root}/_bmad-output/implementation-artifacts/spec-wms-session-longevity-stress.md'
  - '{project-root}/_bmad-output/implementation-artifacts/spec-6-1-windows-midi-services-midibackend-win11.md'
  - '{project-root}/conventions.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** After a clean Bridge WMS stop (or immediate relaunch), MIDI port enumeration via `mido` often hangs or times out because midisrv / Windows MIDI Services is left unhealthy — while a live single session already holds short longevity preflight at 100%. Labs then fail silently or wait without a clear recovery path.

**Approach:** Reproduce the post-stop enum failure with journaled evidence; apply a minimal Bridge/WMS teardown harden only if the repro proves incomplete Destroy/Create or ghost ports on our side; otherwise document midisrv as the cause and ship lab detection + one-shot reset + single clean Bridge restart (no aggressive restart loops).

## Boundaries & Constraints

**Always:**
- Order A → B → C: journaled short repro first; Bridge/WMS code only if evidence shows our teardown/lifecycle fault; lab harness/procedure for timeout, “midisrv suspect”, documented reset, then one clean Bridge session.
- Keep topology Matrix In1/Out1 + DIN Out2→In2; Bridge `--start-session --dev-zadig --midi-backend=wms`.
- Preserve existing WMS fixes: `Midi1StreamAssembler`; SendToHost cadence + BufferFull retry.
- Abort and report on dead LEDs, missing ports, Bridge crash, or overflow; chat FR / code+docs EN; commit only on request; `lint-touched.py` green on any touched C++/Python before done.
- Leave the ≥1 h single-session longevity gate open as a follow-up once A–C are green (do not treat it as this Build’s exit).

**Ask First:**
- Any teardown change broader than the reproduced fault (e.g. restoring Session `Close()` if it risks hang).
- Changing Matrix/DIN topology or substituting soft-echo for hardware.
- Making midisrv reset automatic without admin consent / documented lab procedure.

**Never:**
- “Fix midisrv” as a Microsoft service rewrite or broad Epic 6 expansion (6.2 packaging, 6.3 full matrix).
- Preventive WMS architecture refactor; overnight-combined aggressive Bridge restart as success gate.
- Silent hangs on MIDI enum; aggressive multi-restart loops after one failed enum.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| A Repro | Clean Bridge WMS up → stop/teardown → immediate `fresh_midi_port_names` / mido enum | Hang or TimeoutExpired (~20s) journaled under `tests/lab-logs/…` with Bridge stop timing | Capture midisrv probe + Bridge log tail; do not spin relaunches |
| B Bridge fault | Repro shows incomplete Destroy/Create, race, or ghost ports attributable to Bridge | Minimal teardown harden only; re-run A to green or improved | No change if evidence is service-only |
| B Service cause | Repro shows healthy Bridge teardown but enum still sick until midisrv reset | Note “cause: midisrv/service”; skip Bridge code | Proceed to C |
| C Detect | Enum exceeds clear timeout | Fail fast with explicit “midisrv suspect” message | No silent wait beyond timeout |
| C Recover | Documented midisrv reset (admin if needed) then one clean Bridge start | Ports ready once; lab continues | No aggressive restart loop |

</frozen-after-approval>

## Code Map

- `src/Midi/WmsMidiBackendPorts.cpp` — teardown hot path: `clearEndpointSlot`, `destroyDirectionalEndpoints` (`detach_abi` without `Close()`), `CreatePortSet` / `DestroyPortSet`. **Untouched** this Build (provisional midisrv/service cause).
- `src/Device/DeviceSession.cpp` / `src/App/MidiSessionMultiHost.cpp` — normative Stop → DestroyPortSet → backend reset. Read-only.
- `scripts/lab/lab_midi_common.py` — `enumerate_midi_ports` / `fresh_midi_port_names` (midisrv-suspect SystemExit), `midisrv_status`, `apply_midisrv_reset_once`, `CleanWmsBridgeStart` / `start_one_clean_wms_bridge`.
- `scripts/lab/wms-midisrv-restart-repro.py` — Path A stop→enum journal; Path B provisional classify; Path C detect/docs + optional reset + one Bridge.
- `scripts/lab/test_lab_midi_midisrv_helpers.py` — mocked timeout/OSError checks for midisrv-suspect messaging.
- `scripts/lab/wms-session-longevity-stress.py` — preflight reuses `lab_midi.midisrv_status` + reset procedure text.
- Evidence: gitignored `tests/lab-logs/wms-midisrv-restart/`; durable capsule `docs/tests/lab-evidence/wms-midisrv-restart-2026-08-21/`.
- **Read-only:** `Midi1StreamAssembler.*`; SendToHost BufferFull retry; overnight `--with-bridge` restart harness as success gate.

## Tasks & Acceptance

**Execution:**
- [x] Short journaled repro A — Bridge WMS clean session → stop → enum MIDI; capture TimeoutExpired / hang proof under `tests/lab-logs/wms-midisrv-restart/` — establishes post-stop failure independent of live-session longevity.
- [x] Classify cause — Bridge/WMS teardown vs midisrv service; if Bridge-owned, minimal teardown harden in `WmsMidiBackendPorts.cpp` (+ callers only if required) and re-check A — no preventive refactor.
- [x] Lab path C — clear enum timeout + “midisrv suspect” messaging; documented midisrv reset (admin if needed); exactly one clean Bridge relaunch helper/procedure; wire into longevity preflight or shared `lab_midi_common` helpers as needed — lab recovers without silent hang or restart loops.
- [x] Ignore ephemeral `tests/lab-logs/wms-midisrv-restart/` in `.gitignore` if new tree — match overnight log policy.
- [x] If C++/Python changed: `python scripts/quality/lint-touched.py` green — quality gate before done.
- [x] Note follow-up: ≥1 h single-session longevity gate still open after A–C green — out of this Build’s exit criteria.

**Acceptance Criteria:**
- Given a clean Bridge WMS session on the lab topology, when it is stopped and MIDI ports are enumerated immediately, then the failure (hang/timeout) is reproducibly journaled under `tests/lab-logs/wms-midisrv-restart/` (or equivalent path recorded in the evidence note).
- Given that journal, when the cause is Bridge/WMS teardown, then a minimal teardown fix lands and the same repro no longer shows that Bridge-owned fault; when the cause is midisrv/service, then the evidence note states so and no speculative Bridge refactor ships.
- Given a post-stop enum that would hang, when the lab harness/procedure runs, then it fails within a clear timeout with an explicit midisrv-suspect message, documents reset steps, and allows exactly one clean Bridge session afterward without aggressive relaunch loops.

## Spec Change Log

- 2026-08-21: Path A journaled TimeoutExpired after clean Bridge WMS stop; classified cause midisrv/service (no Bridge teardown change). Lab helpers: midisrv-suspect enum timeout, documented reset, one-shot Bridge start; longevity preflight reuses shared midisrv helpers.
- 2026-08-21 review patches: path_c_ok no longer true for docs-only; recovery-attempt failures exit 2; one-clean blocked while enum sick; reset fallback Stop-Process+Start-Service; Bridge leak guards; OSError enum result; unit test for midisrv-suspect SystemExit; durable evidence capsule; Code Map/Verification refresh.

## Verification

**Commands:**
- `.venv-lab\Scripts\python.exe scripts/lab/wms-midisrv-restart-repro.py` -- expected: journaled Path A TimeoutExpired (or recover flags when elevated)
- `.venv-lab\Scripts\python.exe scripts/lab/test_lab_midi_midisrv_helpers.py` -- expected: exit 0
- `.venv-lab\Scripts\python.exe scripts/quality/lint-touched.py` -- expected: exit 0 on touched C++/Python diff

**Manual checks (if no CLI):**
- After elevated midisrv reset + `--recover-only --apply-midisrv-reset --one-clean-bridge`: Matrix Out1/In1 and DIN Out2/In2 ports appear; LEDs alive; no overflow.
- Evidence note lists log paths, provisional cause classification (Bridge vs service), and that the 1 h longevity gate remains open.

## Suggested Review Order

**Detect (no silent hang)**

- Bounded fresh-process enum with TimeoutExpired → MidiEnumResult
  [`lab_midi_common.py:200`](../../scripts/lab/lab_midi_common.py#L200)

- Shared callers get midisrv-suspect SystemExit + reset procedure
  [`lab_midi_common.py:273`](../../scripts/lab/lab_midi_common.py#L273)

**Path A repro + classification**

- Clean Bridge WMS → stop → immediate enum journal
  [`wms-midisrv-restart-repro.py:162`](../../scripts/lab/wms-midisrv-restart-repro.py#L162)

- Provisional midisrv/service vs Bridge-suspect heuristic
  [`wms-midisrv-restart-repro.py:139`](../../scripts/lab/wms-midisrv-restart-repro.py#L139)

**Path C recovery (one shot)**

- Documented reset + optional Restart-Service/fallback; docs-only ≠ path_c_ok
  [`wms-midisrv-restart-repro.py:259`](../../scripts/lab/wms-midisrv-restart-repro.py#L259)

- Exactly one clean Bridge with stop-on-ready-failure
  [`lab_midi_common.py:492`](../../scripts/lab/lab_midi_common.py#L492)

**Peripherals**

- Longevity preflight reuses shared midisrv helpers + procedure text
  [`wms-session-longevity-stress.py:131`](../../scripts/lab/wms-session-longevity-stress.py#L131)

- Mocked midisrv-suspect / OSError unit checks
  [`test_lab_midi_midisrv_helpers.py:1`](../../scripts/lab/test_lab_midi_midisrv_helpers.py#L1)

- Durable evidence capsule (gitignored lab logs stay local)
  [`README.md:1`](../../docs/tests/lab-evidence/wms-midisrv-restart-2026-08-21/README.md#L1)
