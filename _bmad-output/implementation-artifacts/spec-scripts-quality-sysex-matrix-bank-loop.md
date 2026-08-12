---
title: 'Scripts quality — sysex-matrix-bank-loop.py'
type: 'refactor'
created: '2026-08-12'
status: 'done'
review_loop_iteration: 0
baseline_commit: 'b8f5fbf'
context:
  - '{project-root}/conventions.md'
  - '{project-root}/docs/dev/scripts-quality-cleanup-checklist.md'
  - '{project-root}/scripts/quality/lint-touched.py'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** `scripts/lab/sysex-matrix-bank-loop.py` fails the scripts quality gate (§3.4) with six lizard findings (oversized `run_lab` / `build_parser`, high nesting, `_run_bank_burst` params, file ~767 useful lines). That blocks Vague A cleanup without a lab big-bang.

**Approach:** Structural refactor of this file only (reuse existing `lab_midi_common` for Bridge/ports, options/dataclass for bank-burst args, split glue) so all `[scripts:…]` findings for this path disappear under `--all`, while preserving public CLI flags, Pass/Fail exits, MIDI topology, and log shape. Add at most one thin Matrix bank companion under `scripts/lab/` if useful for maintainability after common reuse — do not migrate other callers.

## Boundaries & Constraints

**Always:**
- Touch primarily `scripts/lab/sysex-matrix-bank-loop.py`; optional companion extract only if useful for this file’s findings / consistency with mid-loop.
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
| Help | `python scripts/lab/sysex-matrix-bank-loop.py --help` | Same documented flags/defaults as before refactor | N/A |
| List ports | `--list-ports` | Lists MIDI in/out names; exit 0 | Missing mido → clear error, non-zero |
| Gate touched | Diff touches only this ticket’s files | `lint-touched.py` exit 0 | Any new `[scripts:…]` on hunks → exit 1 |
| Gate all (target) | `--all` filtered to this path | Zero findings for `sysex-matrix-bank-loop.py` (+ any new companion) | Other dirty labs may still report |

</frozen-after-approval>

## Code Map

- `scripts/lab/sysex-matrix-bank-loop.py` — Matrix bank-loop lab entrypoint (orchestration + burst today)
- `scripts/lab/lab_midi_common.py` — existing BridgeSession + MIDI port helpers (reuse; do not re-extract)
- `scripts/lab/sysex_matrix_bank_loop_lib.py` — optional companion for bank SysEx / CLI / burst helpers if useful after common reuse
- `scripts/quality/lint-touched.py` — enforcer (§3.4); do not change unless bug HALT
- `conventions.md` §3.4 — scripts thresholds (lab glue ~90 nloc, CCN≤14, nesting≤5, params≤4, useful file ~700)
- `docs/dev/scripts-quality-cleanup-checklist.md` — Vague A item for this path
- `scripts/lab/sysex-matrix-mid-loop.py` — prior pattern: thin entrypoint + lib + `lab_midi_common` (orchestration model only)

### Baseline findings (`--all`, this path only, pre-refactor)

- `[scripts:complexity]` `run_lab` ccn=31 > 14 (L533)
- `[scripts:function-length]` `run_lab` nloc=168 > 90 (L533–712)
- `[scripts:function-length]` `build_parser` nloc=125 > 90 (L715–839)
- `[scripts:nesting]` `run_lab` nesting~=8 > 5 (L533)
- `[scripts:file-size]` useful lines 767 > 700
- `[scripts:parameters]` `_run_bank_burst` params=6 > 4 (L421)
- Score ~1031, findings=6

## Tasks & Acceptance

**Execution:**
- [x] `scripts/lab/sysex-matrix-bank-loop.py` -- Import `lab_midi_common` for Bridge/ports; pack bank-burst args into a dataclass; split `run_lab` / `build_parser`; flatten nesting -- Clear length/CCN/params/nesting findings without changing behavior
- [x] `scripts/lab/sysex_matrix_bank_loop_lib.py` (only if needed) -- Move bank SysEx assembler / CLI builders / burst helpers -- Keep entrypoint lean and mirror mid-loop; Boy Scout: no other caller migration
- [x] `docs/dev/scripts-quality-cleanup-checklist.md` -- Check `sysex-matrix-bank-loop.py` + note (`pending commit`) -- Track Vague A progress
- [x] `_bmad-output/implementation-artifacts/spec-scripts-quality-sysex-matrix-bank-loop.md` -- Record after-findings in Design Notes when gate is green -- Close the before/after trail

**Acceptance Criteria:**
- Given the target path under `python scripts/quality/lint-touched.py --all`, when findings are filtered to `sysex-matrix-bank-loop.py` (and any new companion), then count is 0
- Given this ticket’s diff only, when `python scripts/quality/lint-touched.py` runs, then exit code is 0
- Given `--help` (and existing documented flags), when invoked after refactor, then flag names/defaults match pre-refactor unless a rename was approved and noted
- Given checklist Vague A, when the ticket finishes, then the `sysex-matrix-bank-loop.py` box is checked with a note line

## Spec Change Log

## Design Notes

**Strategy:** Delete the local Bridge/port clone (~230–260 lines) by importing `lab_midi_common` (already extracted for long-loopback / mid-loop). Pack `_run_bank_burst` multi-arg signature into a dataclass. Split oversized `run_lab` / `build_parser` and flatten nesting. Extract `sysex_matrix_bank_loop_lib.py` if useful for maintainability / mid-loop consistency (size alone may clear ~700 after common reuse). Keep Matrix bank SysEx fixtures and Pass/Fail behavior identical.

**Bridge fail needles:** Keep the **local** five-needle subset (same Ask First / Pass/Fail parity as mid-loop). Do **not** switch to `lab_midi_common.BRIDGE_FAIL_NEEDLES` without approval.

**After findings:** `--all` filtered to this path + companion → **0** findings. Useful lines: entrypoint ~284, lib ~343 (both under ~700). `lint-touched.py` (touched) exit 0; `--help` flags/defaults preserved vs pre-refactor.

## Verification

**Commands:**
- `python scripts/quality/lint-touched.py --all` -- expected: no lines mentioning `scripts/lab/sysex-matrix-bank-loop.py` (or new companion)
- `python scripts/quality/lint-touched.py` -- expected: exit 0 on this ticket’s diff
- `python scripts/lab/sysex-matrix-bank-loop.py --help` -- expected: documented flags/defaults unchanged

**Manual checks (if no CLI):**
- Diff limited to target (+ optional one companion) + checklist + this spec; no second checklist file refactored

## Suggested Review Order

**Entrypoint orchestration**

- Thin `run_lab` routes Bridge / fresh MIDI / single session after mode checks.
  [`sysex-matrix-bank-loop.py:308`](../../scripts/lab/sysex-matrix-bank-loop.py#L308)

- Bridge start loop preserves fail scan before Stop and local needle subset.
  [`sysex-matrix-bank-loop.py:160`](../../scripts/lab/sysex-matrix-bank-loop.py#L160)

- Fresh-process child flags stay bank-only (count / slot / interval / timeouts).
  [`sysex-matrix-bank-loop.py:104`](../../scripts/lab/sysex-matrix-bank-loop.py#L104)

**Bank companion (burst + CLI)**

- Local `BRIDGE_FAIL_NEEDLES` subset kept on purpose (not common’s fuller set).
  [`sysex_matrix_bank_loop_lib.py:23`](../../scripts/lab/sysex_matrix_bank_loop_lib.py#L23)

- Bank burst packed into `BankBurstOpts` under the params gate.
  [`sysex_matrix_bank_loop_lib.py:150`](../../scripts/lab/sysex_matrix_bank_loop_lib.py#L150)

- Parser builders split; public flags/defaults unchanged.
  [`sysex_matrix_bank_loop_lib.py:358`](../../scripts/lab/sysex_matrix_bank_loop_lib.py#L358)

**Tracking**

- Vague A checkbox + pending-commit note.
  [`scripts-quality-cleanup-checklist.md:101`](../../docs/dev/scripts-quality-cleanup-checklist.md#L101)
