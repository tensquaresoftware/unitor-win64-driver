---
title: 'Scripts quality — midi-concurrent-in-stress.py'
type: 'refactor'
created: '2026-08-12'
status: 'done'
review_loop_iteration: 0
baseline_commit: 'ee0191f'
context:
  - '{project-root}/conventions.md'
  - '{project-root}/docs/dev/scripts-quality-cleanup-checklist.md'
  - '{project-root}/scripts/quality/lint-touched.py'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** `scripts/lab/midi-concurrent-in-stress.py` fails the scripts quality gate (§3.4) with two lizard nesting findings (`_run_stress` and nested `listen`). That blocks Vague B cleanup without a lab big-bang.

**Approach:** Structural refactor of this dual-IN demux stress lab only (flatten the listen/SysEx parse path and thin `_run_stress`) so all `[scripts:…]` findings for this path disappear under `--all`, while preserving public CLI flags, Pass/Fail gates, Bridge/no-Bridge topology, log layout, and exit codes. Optional thin companion under `scripts/lab/` only if in-file splits are insufficient.

## Boundaries & Constraints

**Always:**
- Touch primarily `scripts/lab/midi-concurrent-in-stress.py`; optional companion only if needed for this file’s findings.
- Preserve documented CLI flags, defaults (`--rounds` 10, `--log-dir` `tests/lab-logs/midi-concurrent-in`), Bridge argv (`--start-session`, `--dev-zadig`), Pass/Fail math (notes ≥ rounds×4, dumps ≥ max(1, rounds//2), zero cross-talk), log path pattern (`concurrent-in-{stamp}.log`), and exit codes (`0` pass, `2` fail).
- English only in code, logs, and argparse help.
- End of ticket: `python scripts/quality/lint-touched.py` exit 0 on the diff; zero `[scripts:…]` findings for this path under `--all`.
- Update checklist checkbox + note (`pending commit` until a hash is known).
- No commit unless Guillaume explicitly asks.

**Ask First:**
- Renaming or removing a public CLI flag (document justification first).
- Changing §3.4 thresholds or `lint-touched.py` behavior (HALT on suspected linter bug).
- Changing Pass/Fail thresholds or marker notes (60 / 72).
- Migrating other labs onto a new shared module beyond what this file needs.

**Never:**
- Second checklist file in this conversation.
- New SysEx scenario, overnight suite redesign, epic 6, or C++ Bridge changes.
- Mass reformat / cleanup of all `scripts/lab/*`.
- Hardware lab runs unless public CLI risk forces a `--help` / smoke check.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Help | `python scripts/lab/midi-concurrent-in-stress.py --help` | Same documented flags/defaults as before refactor | N/A |
| Gate touched | Diff touches only this ticket’s files | `lint-touched.py` exit 0 | Any new `[scripts:…]` on hunks → exit 1 |
| Gate all (target) | `--all` filtered to this path | Zero findings for this path (+ companion if any) | Other dirty labs may still report |

</frozen-after-approval>

## Code Map

- `scripts/lab/midi-concurrent-in-stress.py` — dual-IN demux stress (Out2→In2 notes + Matrix dump Out1→In1)
- `scripts/lab/lab_midi_common.py` — existing Bridge/port helpers (reference only; do not adopt unless Ask First)
- `scripts/quality/lint-touched.py` — enforcer (§3.4); do not change unless bug HALT
- `conventions.md` §3.4 — lab glue ~90 nloc, CCN≤14, nesting≤5, params≤4, useful file ~700
- `docs/dev/scripts-quality-cleanup-checklist.md` — Vague B item for this path
- `scripts/lab/midi_concurrent_in_stress_lib.py` — optional companion if in-file split is insufficient

### Baseline findings (`--all`, this path only, pre-refactor)

- `[scripts:nesting]` `_run_stress` nesting~=8 > 5 (L250)
- `[scripts:nesting]` `_run_stress.listen` nesting~=7 > 5 (L260)
- Score ~108, findings=2

## Tasks & Acceptance

**Execution:**
- [x] `scripts/lab/midi-concurrent-in-stress.py` -- Extract listen/SysEx-frame helpers to cut nesting; keep `_run_stress` orchestration and Pass gates identical -- Clear nesting findings without changing behavior
- [x] `scripts/lab/midi_concurrent_in_stress_lib.py` (only if needed) -- Not required; in-file helpers cleared all findings -- Keep entrypoint lean; no other caller migration
- [x] `docs/dev/scripts-quality-cleanup-checklist.md` -- Check `midi-concurrent-in-stress.py` + note (`pending commit`) -- Track Vague B progress
- [x] `_bmad-output/implementation-artifacts/spec-scripts-quality-midi-concurrent-in-stress.md` -- Record after-findings in Design Notes when gate is green -- Close the before/after trail

**Acceptance Criteria:**
- Given the target path under `python scripts/quality/lint-touched.py --all`, when findings are filtered to this path (and any new companion), then count is 0
- Given this ticket’s diff only, when `python scripts/quality/lint-touched.py` runs, then exit code is 0
- Given `--help` (and existing documented flags), when invoked after refactor, then flag names/defaults match pre-refactor unless a rename was approved and noted
- Given checklist Vague B, when the ticket finishes, then the `midi-concurrent-in-stress.py` box is checked with a note line

## Spec Change Log

## Design Notes

**Strategy:** Nesting comes from a nested `listen` with deep SysEx reassembly (`while` + `for` + `elif` + inner `while`). Lift `listen` to module level (or a small helper class) and extract frame-drain / message-dispatch helpers so both lizard nesting counts fall ≤5. Keep marker notes 72/60, Pass math, BridgeSession, and CLI unchanged. Prefer one file; companion only if a helper still trips thresholds.

**After findings:** `--all` filtered to this path → **0** findings (no companion). Nesting cleared via `_score_sysex_frame` / `_drain_sysex_hold` / `_dispatch_midi_msg` / `_listen_port(ListenTarget)`; params kept ≤4 with `ListenTarget`. `lint-touched.py` (touched) exit 0; `--help` flags/defaults preserved vs pre-refactor.

## Verification

**Commands:**
- `python scripts/quality/lint-touched.py --all` -- expected: zero `[scripts:…]` lines for `midi-concurrent-in-stress.py` (and companion if any)
- `python scripts/quality/lint-touched.py` -- expected: exit 0 on this ticket’s diff
- `python scripts/lab/midi-concurrent-in-stress.py --help` -- expected: same flags/defaults as pre-refactor

## Suggested Review Order

**Nesting flatten**

- Module-level `_listen_port` + `ListenTarget` replaces nested `_run_stress.listen`.
  [`midi-concurrent-in-stress.py:294`](../../scripts/lab/midi-concurrent-in-stress.py#L294)

- SysEx hold/drain extracted so nesting ≤5.
  [`midi-concurrent-in-stress.py:258`](../../scripts/lab/midi-concurrent-in-stress.py#L258)

**Behavior lock**

- Thin `_run_stress` keeps listen → send → settle → score order.
  [`midi-concurrent-in-stress.py:352`](../../scripts/lab/midi-concurrent-in-stress.py#L352)

- Pass gates unchanged: notes ≥ rounds×4, dumps ≥ max(1, rounds//2), zero cross-talk.
  [`midi-concurrent-in-stress.py:326`](../../scripts/lab/midi-concurrent-in-stress.py#L326)

**Tracking**

- Vague B checkbox + pending-commit note in the cleanup checklist.
  [`scripts-quality-cleanup-checklist.md:127`](../../docs/dev/scripts-quality-cleanup-checklist.md#L127)
