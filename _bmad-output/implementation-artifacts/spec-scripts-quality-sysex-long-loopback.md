---
title: 'Scripts quality — sysex-long-loopback.py'
type: 'refactor'
created: '2026-08-12'
status: 'done'
review_loop_iteration: 0
baseline_commit: 'ca5c7bd'
context:
  - '{project-root}/conventions.md'
  - '{project-root}/docs/dev/scripts-quality-cleanup-checklist.md'
  - '{project-root}/scripts/quality/lint-touched.py'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** `scripts/lab/sysex-long-loopback.py` fails the scripts quality gate (§3.4) with eight lizard findings (oversized `run_lab` / `build_parser` / `_run_loopback_session`, high nesting/params, file ~1087 useful lines). That blocks per-file cleanup without a lab big-bang.

**Approach:** Structural refactor of this file only (helpers, options/dataclass, split glue) so all `[scripts:…]` findings for this path disappear under `--all`, while preserving public CLI flags, Pass/Fail/Abort exits, MIDI topology, and log shape. Extract one minimal shared module under `scripts/lab/` only if required to clear file-size or unblock this file — do not migrate other callers unless needed for this ticket.

## Boundaries & Constraints

**Always:**
- Touch primarily `scripts/lab/sysex-long-loopback.py`; shared extract only if indispensable for this file’s findings.
- Preserve documented CLI flags, defaults, exit codes (`0` pass, `2` fail, `3` abort), log headers/line patterns, and lab behavior.
- English only in code, logs, and argparse help.
- End of ticket: `python scripts/quality/lint-touched.py` exit 0 on the diff; zero `[scripts:…]` findings for this path under `--all`.
- Update checklist checkbox + note (`pending commit` until a hash is known).
- No commit unless Guillaume explicitly asks.

**Ask First:**
- Renaming or removing a public CLI flag (document justification first).
- Changing §3.4 thresholds or `lint-touched.py` behavior (HALT on suspected linter bug).
- Migrating more than this one primary caller onto a new shared module beyond what this file needs.

**Never:**
- Second checklist file in this conversation.
- New SysEx scenario (Request All `04H` type 0), overnight suites, epic 6, or C++ Bridge changes.
- Mass reformat / cleanup of all `scripts/lab/*`.
- Hardware lab runs unless public CLI risk forces a `--help` / smoke check.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Help | `python scripts/lab/sysex-long-loopback.py --help` | Same documented flags/defaults as before refactor | N/A |
| List ports | `--list-ports` | Lists MIDI in/out names; exit 0 | Missing mido → clear error, non-zero |
| Gate touched | Diff touches only this ticket’s files | `lint-touched.py` exit 0 | Any new `[scripts:…]` on hunks → exit 1 |
| Gate all (target) | `--all` filtered to this path | Zero findings for `sysex-long-loopback.py` | Other dirty labs may still report |

</frozen-after-approval>

## Code Map

- `scripts/lab/sysex-long-loopback.py` — orchestration entrypoint (after: useful ≈544)
- `scripts/lab/lab_midi_common.py` — BridgeSession + MIDI port helpers (new extract)
- `scripts/lab/sysex_long_loopback_lib.py` — assembler/wait/synthetic/CLI builders (new extract)
- `scripts/quality/lint-touched.py` — enforcer (§3.4); do not change unless bug HALT
- `conventions.md` §3.4 — scripts thresholds (lab glue ~90 nloc, CCN≤14, nesting≤5, params≤4, useful file ~700)
- `docs/dev/scripts-quality-cleanup-checklist.md` — Vague A first item; marked done + note
- `scripts/lab/*.py` — Bridge/MIDI helpers still duplicated elsewhere; not migrated this ticket

### Baseline findings (`--all`, this path only, pre-refactor)

- `[scripts:function-length]` `run_lab` nloc=253 > 90 (L778–1046)
- `[scripts:complexity]` `run_lab` ccn=47 > 14 (L778)
- `[scripts:file-size]` useful lines 1087 > 700
- `[scripts:function-length]` `build_parser` nloc=152 > 90 (L1049–1200)
- `[scripts:nesting]` `_run_loopback_session` nesting~=8 > 5 (L596)
- `[scripts:nesting]` `run_lab` nesting~=8 > 5 (L778)
- `[scripts:parameters]` `_run_loopback_session` params=7 > 4 (L596)
- `[scripts:function-length]` `_run_loopback_session` nloc=114 > 90 (L596–714)
- Score ~1253, findings=8

## Tasks & Acceptance

**Execution:**
- [x] `scripts/lab/sysex-long-loopback.py` -- Split `run_lab` / `build_parser` / `_run_loopback_session` into focused helpers; pack multi-arg session state into a dataclass/Namespace; flatten nesting -- Clear length/CCN/params/nesting findings without changing behavior
- [x] `scripts/lab/lab_midi_common.py` + `scripts/lab/sysex_long_loopback_lib.py` -- Extract Bridge/MIDI glue and loopback/CLI helpers -- Drop useful-line count under ~700; Boy Scout: no other caller migration
- [x] `docs/dev/scripts-quality-cleanup-checklist.md` -- Check `sysex-long-loopback.py` + note (`pending commit`) -- Track Vague A progress
- [x] `_bmad-output/implementation-artifacts/spec-scripts-quality-sysex-long-loopback.md` -- Record after-findings in Design Notes when gate is green -- Close the before/after trail

**Acceptance Criteria:**
- Given the target path under `python scripts/quality/lint-touched.py --all`, when findings are filtered to `scripts/lab/sysex-long-loopback.py`, then count is 0
- Given this ticket’s diff only, when `python scripts/quality/lint-touched.py` runs, then exit code is 0
- Given `--help` (and existing documented flags), when invoked after refactor, then flag names/defaults match pre-refactor unless a rename was approved and noted
- Given checklist Vague A, when the ticket finishes, then the `sysex-long-loopback.py` box is checked with a note line

## Spec Change Log

## Design Notes

**Strategy:** In-file split was not enough for useful-line ≤700. Extracted `lab_midi_common.py` (BridgeSession + port match) and `sysex_long_loopback_lib.py` (assembler/wait/synthetic/CLI builders). Orchestration stays in `sysex-long-loopback.py`. Other labs still keep their local copies (no mass migrate).

**After findings (2026-08-12):** 0 `[scripts:…]` lines for `sysex-long-loopback.py`, `lab_midi_common.py`, or `sysex_long_loopback_lib.py` under `--all`. Useful lines ≈544 / ≈239 / ≈397. Touched mode exit 0.
## Verification

**Commands:**
- `python scripts/quality/lint-touched.py --all` -- expected: no lines mentioning `scripts/lab/sysex-long-loopback.py`
- `python scripts/quality/lint-touched.py` -- expected: exit 0 on this ticket’s diff
- `python scripts/lab/sysex-long-loopback.py --help` -- expected: documented flags/defaults unchanged

**Manual checks (if no CLI):**
- Diff limited to target (+ optional one shared module) + checklist + this spec; no second checklist file refactored

## Suggested Review Order

**Entrypoint orchestration**

- Thin `run_lab` routes Bridge / fresh MIDI / single session after auto wall.
  [`sysex-long-loopback.py:589`](../../scripts/lab/sysex-long-loopback.py#L589)

- Session dataclass packs former 7-arg loopback signature under the params gate.
  [`sysex-long-loopback.py:68`](../../scripts/lab/sysex-long-loopback.py#L68)

- Payload loop uses `OpenPorts` + `PayloadWork`; nesting flattened vs original.
  [`sysex-long-loopback.py:215`](../../scripts/lab/sysex-long-loopback.py#L215)

**Child / Bridge glue (behavior parity)**

- Fresh-process argv + wall inheritance restored to pre-refactor floor/`Launching` lines.
  [`sysex-long-loopback.py:237`](../../scripts/lab/sysex-long-loopback.py#L237)

- Bridge multi-start path writes the same header/footer shape as before.
  [`sysex-long-loopback.py:411`](../../scripts/lab/sysex-long-loopback.py#L411)

**Shared extracts**

- BridgeSession + port match live here so the entry file stays under ~700 useful lines.
  [`lab_midi_common.py:165`](../../scripts/lab/lab_midi_common.py#L165)

- Assembler/wait/synthetic + argparse builders; path bootstrap for sibling import.
  [`sysex_long_loopback_lib.py:27`](../../scripts/lab/sysex_long_loopback_lib.py#L27)

- Public CLI flags rebuilt via grouped helpers; defaults unchanged.
  [`sysex_long_loopback_lib.py:435`](../../scripts/lab/sysex_long_loopback_lib.py#L435)

**Tracking**

- Vague A checkbox + pending-commit note.
  [`scripts-quality-cleanup-checklist.md:90`](../../docs/dev/scripts-quality-cleanup-checklist.md#L90)
