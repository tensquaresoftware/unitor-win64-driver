---
title: 'Scripts quality — sysex-matrix-mid-loop.py'
type: 'refactor'
created: '2026-08-12'
status: 'done'
review_loop_iteration: 0
baseline_commit: '804043c'
context:
  - '{project-root}/conventions.md'
  - '{project-root}/docs/dev/scripts-quality-cleanup-checklist.md'
  - '{project-root}/scripts/quality/lint-touched.py'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** `scripts/lab/sysex-matrix-mid-loop.py` fails the scripts quality gate (§3.4) with eight lizard findings (oversized `run_lab` / `build_parser`, high nesting/params on scenario runners, file ~944 useful lines). That blocks Vague A cleanup without a lab big-bang.

**Approach:** Structural refactor of this file only (reuse existing `lab_midi_common` where duplicated, options/dataclass for multi-arg scenarios, split glue) so all `[scripts:…]` findings for this path disappear under `--all`, while preserving public CLI flags, Pass/Fail exits, MIDI topology, and log shape. Add at most one thin Matrix-specific companion under `scripts/lab/` if still over useful-line after common reuse — do not migrate other callers.

## Boundaries & Constraints

**Always:**
- Touch primarily `scripts/lab/sysex-matrix-mid-loop.py`; optional companion extract only if indispensable for this file’s findings.
- Prefer importing existing `scripts/lab/lab_midi_common.py` for Bridge/port helpers already extracted (≥3× duplication).
- Preserve documented CLI flags, defaults, exit codes (`0` pass / list-ports, `2` fail), log headers/line patterns, and lab behavior.
- English only in code, logs, and argparse help.
- End of ticket: `python scripts/quality/lint-touched.py` exit 0 on the diff; zero `[scripts:…]` findings for this path under `--all`.
- Update checklist checkbox + note (`pending commit` until a hash is known).
- No commit unless Guillaume explicitly asks.

**Ask First:**
- Renaming or removing a public CLI flag (document justification first).
- Changing §3.4 thresholds or `lint-touched.py` behavior (HALT on suspected linter bug).
- Migrating more than this one primary caller onto shared modules beyond what this file needs.
- Expanding Bridge fail needles beyond the current local set if that would change Pass/Fail semantics in a surprising way.

**Never:**
- Second checklist file in this conversation.
- New SysEx scenario (Request All `04H` type 0), overnight suites, epic 6, or C++ Bridge changes.
- Mass reformat / cleanup of all `scripts/lab/*`.
- Hardware lab runs unless public CLI risk forces a `--help` / smoke check.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Help | `python scripts/lab/sysex-matrix-mid-loop.py --help` | Same documented flags/defaults as before refactor | N/A |
| List ports | `--list-ports` | Lists MIDI in/out names; exit 0 | Missing mido → clear error, non-zero |
| Gate touched | Diff touches only this ticket’s files | `lint-touched.py` exit 0 | Any new `[scripts:…]` on hunks → exit 1 |
| Gate all (target) | `--all` filtered to this path | Zero findings for `sysex-matrix-mid-loop.py` (+ any new companion) | Other dirty labs may still report |

</frozen-after-approval>

## Code Map

- `scripts/lab/sysex-matrix-mid-loop.py` — Matrix mid-loop lab entrypoint (orchestration + scenarios today)
- `scripts/lab/lab_midi_common.py` — existing BridgeSession + MIDI port helpers (reuse; do not re-extract)
- `scripts/lab/sysex_matrix_mid_loop_lib.py` — optional companion if useful-lines still >700 after common reuse + in-file splits
- `scripts/quality/lint-touched.py` — enforcer (§3.4); do not change unless bug HALT
- `conventions.md` §3.4 — scripts thresholds (lab glue ~90 nloc, CCN≤14, nesting≤5, params≤4, useful file ~700)
- `docs/dev/scripts-quality-cleanup-checklist.md` — Vague A item for this path
- `scripts/lab/sysex-long-loopback.py` — prior pattern: thin `run_lab` + lib split (orchestration model only)

### Baseline findings (`--all`, this path only, pre-refactor)

- `[scripts:complexity]` `run_lab` ccn=35 > 14 (L705)
- `[scripts:function-length]` `run_lab` nloc=179 > 90 (L705–904)
- `[scripts:function-length]` `build_parser` nloc=139 > 90 (L907–1045)
- `[scripts:parameters]` `_run_dump_scenario` params=11 > 4 (L518)
- `[scripts:parameters]` `_run_push_scenario` params=8 > 4 (L478)
- `[scripts:nesting]` `run_lab` nesting~=8 > 5 (L705)
- `[scripts:file-size]` useful lines 944 > 700
- `[scripts:parameters]` `_run_all_scenarios` params=6 > 4 (L571)
- Score ~1035, findings=8

## Tasks & Acceptance

**Execution:**
- [x] `scripts/lab/sysex-matrix-mid-loop.py` -- Import `lab_midi_common` for Bridge/ports; pack scenario args into dataclasses; split `run_lab` / `build_parser`; flatten nesting -- Clear length/CCN/params/nesting findings without changing behavior
- [x] `scripts/lab/sysex_matrix_mid_loop_lib.py` (only if needed) -- Move Matrix assembler/CLI builders/scenario helpers -- Drop useful-line count under ~700; Boy Scout: no other caller migration
- [x] `docs/dev/scripts-quality-cleanup-checklist.md` -- Check `sysex-matrix-mid-loop.py` + note (`pending commit`) -- Track Vague A progress
- [x] `_bmad-output/implementation-artifacts/spec-scripts-quality-sysex-matrix-mid-loop.md` -- Record after-findings in Design Notes when gate is green -- Close the before/after trail

**Acceptance Criteria:**
- Given the target path under `python scripts/quality/lint-touched.py --all`, when findings are filtered to `sysex-matrix-mid-loop.py` (and any new companion), then count is 0
- Given this ticket’s diff only, when `python scripts/quality/lint-touched.py` runs, then exit code is 0
- Given `--help` (and existing documented flags), when invoked after refactor, then flag names/defaults match pre-refactor unless a rename was approved and noted
- Given checklist Vague A, when the ticket finishes, then the `sysex-matrix-mid-loop.py` box is checked with a note line

## Spec Change Log

## Design Notes

**Strategy:** Prefer deleting the local Bridge/port clone (~250–300 lines) by importing `lab_midi_common` (already extracted for long-loopback). Then split oversized `run_lab` / `build_parser` and pack push/dump/`_run_all_scenarios` multi-arg signatures into dataclasses. Extract `sysex_matrix_mid_loop_lib.py` only if useful lines remain above ~700. Keep Matrix SysEx fixtures/scenarios behaviorally identical.

**Bridge fail needles:** Kept local subset in `sysex_matrix_mid_loop_lib.BRIDGE_FAIL_NEEDLES` (Ask First / Pass/Fail parity). Did **not** switch to `lab_midi_common.BRIDGE_FAIL_NEEDLES`.

**After findings:** `--all` filtered to this path + companion → **0** findings. Useful lines: entrypoint ~306, lib ~531 (both under ~700). `lint-touched.py` (touched) exit 0; `--help` flags/defaults preserved vs pre-refactor.

## Verification

**Commands:**
- `python scripts/quality/lint-touched.py --all` -- expected: no lines mentioning `scripts/lab/sysex-matrix-mid-loop.py` (or new companion)
- `python scripts/quality/lint-touched.py` -- expected: exit 0 on this ticket’s diff
- `python scripts/lab/sysex-matrix-mid-loop.py --help` -- expected: documented flags/defaults unchanged

**Manual checks (if no CLI):**
- Diff limited to target (+ optional one companion) + checklist + this spec; no second checklist file refactored

## Suggested Review Order

**Entrypoint orchestration**

- Thin `run_lab` routes Bridge / fresh MIDI / single session after mode checks.
  [`sysex-matrix-mid-loop.py:335`](../../scripts/lab/sysex-matrix-mid-loop.py#L335)

- Bridge start loop preserves dump/push include flags and local fail scan.
  [`sysex-matrix-mid-loop.py:219`](../../scripts/lab/sysex-matrix-mid-loop.py#L219)

- Local Bridge fail needles recorded before Stop (parity with pre-refactor).
  [`sysex-matrix-mid-loop.py:154`](../../scripts/lab/sysex-matrix-mid-loop.py#L154)

**Matrix companion (scenarios + CLI)**

- Local `BRIDGE_FAIL_NEEDLES` subset kept on purpose (not common’s fuller set).
  [`sysex_matrix_mid_loop_lib.py:26`](../../scripts/lab/sysex_matrix_mid_loop_lib.py#L26)

- Scenario runners packed into dataclasses under the params gate.
  [`sysex_matrix_mid_loop_lib.py:198`](../../scripts/lab/sysex_matrix_mid_loop_lib.py#L198)

- Dump/push pairs + pass summaries stay in `run_all_scenarios`.
  [`sysex_matrix_mid_loop_lib.py:405`](../../scripts/lab/sysex_matrix_mid_loop_lib.py#L405)

- Parser builders split; public flags/defaults unchanged.
  [`sysex_matrix_mid_loop_lib.py:577`](../../scripts/lab/sysex_matrix_mid_loop_lib.py#L577)

**Tracking**

- Vague A checkbox + pending-commit note.
  [`scripts-quality-cleanup-checklist.md:97`](../../docs/dev/scripts-quality-cleanup-checklist.md#L97)
