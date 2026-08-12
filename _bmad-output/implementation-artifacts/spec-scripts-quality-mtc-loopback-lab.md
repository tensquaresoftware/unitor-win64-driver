---
title: 'Scripts quality — mtc-loopback-lab.py'
type: 'refactor'
created: '2026-08-12'
status: 'done'
review_loop_iteration: 0
baseline_commit: '10cd8f7'
context:
  - '{project-root}/conventions.md'
  - '{project-root}/docs/dev/scripts-quality-cleanup-checklist.md'
  - '{project-root}/scripts/quality/lint-touched.py'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** `scripts/lab/mtc-loopback-lab.py` fails the scripts quality gate (§3.4) with two lizard findings (fat `main` CCN, high-param `_run_lab`). That blocks Vague B cleanup without a lab big-bang.

**Approach:** Structural refactor of this MTC quarter-frame / full-frame DIN loopback lab only (options/dataclass, split run phases, thin `main`) so all `[scripts:…]` findings for this path disappear under `--all`, while preserving public CLI flags, Pass/Fail scoring, Bridge/no-Bridge topology, log layout, and exit codes. Keep Bridge/port helpers local (same behavior lock as midi-clock); optional thin companion under `scripts/lab/` only if in-file splits are insufficient.

## Boundaries & Constraints

**Always:**
- Touch primarily `scripts/lab/mtc-loopback-lab.py`; optional companion only if needed for this file’s findings.
- Preserve documented CLI flags, defaults, validation, Pass/Fail rules (≥98% quarter-frames and ≥1 full-frame; Bridge fail needles flip to fail), Bridge argv (`--dev-zadig`), log path pattern (`tests/lab-logs/mtc-loopback/…`), and exit codes (`0` pass/list, `2` fail).
- Keep local Bridge fail-needle matching and port-enumeration behavior unless an approved note documents a deliberate change.
- English only in code, logs, and argparse help.
- End of ticket: `python scripts/quality/lint-touched.py` exit 0 on the diff; zero `[scripts:…]` findings for this path under `--all`.
- Update checklist checkbox + note (`pending commit` until a hash is known).
- No commit unless Guillaume explicitly asks.

**Ask First:**
- Renaming or removing a public CLI flag (document justification first).
- Changing §3.4 thresholds or `lint-touched.py` behavior (HALT on suspected linter bug).
- Expanding fail needles or switching to shared `lab_midi_common` if that would change Pass/Fail vs today.
- Migrating other realtime labs onto shared helpers beyond what this file needs.

**Never:**
- Second checklist file in this conversation.
- New SysEx scenario, overnight suite redesign, epic 6, or C++ Bridge changes.
- Mass reformat / cleanup of all `scripts/lab/*`.
- Hardware lab runs unless public CLI risk forces a `--help` / smoke check.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Help | `python scripts/lab/mtc-loopback-lab.py --help` | Same documented flags/defaults as before refactor | N/A |
| List ports | `--list-ports` | Enumerate MIDI ports; exit `0` | Missing mido → same error path as before |
| Gate touched | Diff touches only this ticket’s files | `lint-touched.py` exit 0 | Any new `[scripts:…]` on hunks → exit 1 |
| Gate all (target) | `--all` filtered to this path | Zero findings for this path (+ companion if any) | Other dirty labs may still report |

</frozen-after-approval>

## Code Map

- `scripts/lab/mtc-loopback-lab.py` — MTC QF + full-frame DIN loopback lab (target)
- `scripts/lab/lab_midi_common.py` — existing Bridge/port helpers (reference only; do not adopt unless Ask First)
- `scripts/lab/midi-clock-loopback-lab.py` — sibling realtime lab (pattern reference; do not change)
- `scripts/quality/lint-touched.py` — enforcer (§3.4); do not change unless bug HALT
- `conventions.md` §3.4 — lab glue ~90 nloc, CCN≤14, nesting≤5, params≤4, useful file ~700
- `docs/dev/scripts-quality-cleanup-checklist.md` — Vague B item for this path
- `scripts/lab/mtc_loopback_lab_lib.py` — optional companion if in-file split is insufficient

### Baseline findings (`--all`, this path only, pre-refactor)

- `[scripts:complexity]` `main` ccn=19 > 14 (L436)
- `[scripts:parameters]` `_run_lab` params=7 > 4 (L303)
- Score ~119, findings=2

## Tasks & Acceptance

**Execution:**
- [x] `scripts/lab/mtc-loopback-lab.py` -- Pack `_run_lab` args into dataclass(es); split send/settle/score phases; thin `main` via `run_lab(args) -> int` -- Clear CCN/params findings without changing behavior
- [x] `scripts/lab/mtc_loopback_lab_lib.py` (only if needed) -- Not required; in-file helpers cleared all findings -- Keep entrypoint lean; no other caller migration
- [x] `docs/dev/scripts-quality-cleanup-checklist.md` -- Check `mtc-loopback-lab.py` + note (`pending commit`) -- Track Vague B progress
- [x] `_bmad-output/implementation-artifacts/spec-scripts-quality-mtc-loopback-lab.md` -- Record after-findings in Design Notes when gate is green -- Close the before/after trail

**Acceptance Criteria:**
- Given the target path under `python scripts/quality/lint-touched.py --all`, when findings are filtered to this path (and any new companion), then count is 0
- Given this ticket’s diff only, when `python scripts/quality/lint-touched.py` runs, then exit code is 0
- Given `--help` (and existing documented flags), when invoked after refactor, then flag names/defaults match pre-refactor unless a rename was approved and noted
- Given checklist Vague B, when the ticket finishes, then the `mtc-loopback-lab.py` box is checked with a note line

## Spec Change Log

## Design Notes

**Strategy:** Pack `_run_lab`’s seven args into a frozen options/dataclass object (≤4 params). Split the MTC sequence (sanity note-on → quarter-frame flood → full-frame → dense tail → settle → score) so nesting and future length stay under lab glue limits. Extract orchestration from fat `main` into a thin `run_lab(args) -> int` (bridge start/ready, ports, needles, log write, exit). Prefer one file; keep local `BRIDGE_FAIL_NEEDLES` + full-text match + cwd-based fresh enum (do not silently adopt `lab_midi_common`). Companion only if a helper still trips thresholds. Do not change `--qf-interval-ms` units (/1000) or Pass math (`qf >= 98%` and `full >= 1`).

**After findings:** `--all` filtered to this path → **0** findings (no companion; local `BRIDGE_FAIL_NEEDLES` + full-text match + cwd-based fresh enum kept). `lint-touched.py` (touched) exit 0; `--help` flags/defaults preserved vs pre-refactor.

## Verification

**Commands:**
- `python scripts/quality/lint-touched.py --all` -- expected: zero `[scripts:…]` lines for `mtc-loopback-lab.py` (and companion if any)
- `python scripts/quality/lint-touched.py` -- expected: exit 0 on this ticket’s diff
- `python scripts/lab/mtc-loopback-lab.py --help` -- expected: same flags/defaults as pre-refactor

**Manual checks (if no CLI):**
- Checklist Vague B item for this path checked with a note line

## Suggested Review Order

**Param packing**

- Frozen dataclasses cut helper params under the lab gate.
  [`mtc-loopback-lab.py:287`](../../scripts/lab/mtc-loopback-lab.py#L287)

**Phase split**

- QF flood → full-frame → dense tail order preserved.
  [`mtc-loopback-lab.py:374`](../../scripts/lab/mtc-loopback-lab.py#L374)

- Score still requires ≥98% QF and ≥1 full-frame.
  [`mtc-loopback-lab.py:421`](../../scripts/lab/mtc-loopback-lab.py#L421)

**Bridge ownership**

- Stop Bridge if ready-wait fails after start (pre-refactor contract).
  [`mtc-loopback-lab.py:547`](../../scripts/lab/mtc-loopback-lab.py#L547)

- Thin `run_lab` owns ports, needles, log, exit codes.
  [`mtc-loopback-lab.py:604`](../../scripts/lab/mtc-loopback-lab.py#L604)

**Tracking**

- Vague B checkbox + pending-commit note.
  [`scripts-quality-cleanup-checklist.md:119`](../../docs/dev/scripts-quality-cleanup-checklist.md#L119)

