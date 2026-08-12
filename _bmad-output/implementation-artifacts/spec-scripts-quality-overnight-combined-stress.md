---
title: 'Scripts quality — overnight-combined-stress.py'
type: 'refactor'
created: '2026-08-12'
status: 'done'
review_loop_iteration: 0
baseline_commit: '18463eb'
context:
  - '{project-root}/conventions.md'
  - '{project-root}/docs/dev/scripts-quality-cleanup-checklist.md'
  - '{project-root}/scripts/quality/lint-touched.py'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** `scripts/lab/overnight-combined-stress.py` fails the scripts quality gate (§3.4) with three lizard findings (oversized / high-CCN `run_overnight`, `_run_child` with 5 params). That blocks Vague A cleanup without a lab big-bang.

**Approach:** Structural refactor of this wrapper only (options/dataclass for child runs, split overnight cycle glue) so all `[scripts:…]` findings for this path disappear under `--all`, while preserving public CLI flags, soft exit 0, child topology (mid → bank → long), journal lines, and sleep-hold behavior. Optional thin companion under `scripts/lab/` only if needed — no other caller migration.

## Boundaries & Constraints

**Always:**
- Touch primarily `scripts/lab/overnight-combined-stress.py`; optional companion only if useful for this file’s findings.
- Preserve documented CLI flags, defaults, validation, soft harness exit `0`, Ctrl+C / gap interrupt behavior, journal path pattern, and child argv shapes.
- English only in code, logs, and argparse help.
- End of ticket: `python scripts/quality/lint-touched.py` exit 0 on the diff; zero `[scripts:…]` findings for this path under `--all`.
- Update checklist checkbox + note (`pending commit` until a hash is known).
- No commit unless Guillaume explicitly asks.

**Ask First:**
- Renaming or removing a public CLI flag (document justification first).
- Changing §3.4 thresholds or `lint-touched.py` behavior (HALT on suspected linter bug).
- Changing soft-exit semantics (always 0) or aborting overnight on a failed child cycle.
- Migrating other overnight / lab callers onto a new shared module beyond what this file needs.

**Never:**
- Second checklist file in this conversation.
- New SysEx scenario, overnight suite redesign, epic 6, or C++ Bridge changes.
- Mass reformat / cleanup of all `scripts/lab/*`.
- Hardware lab runs unless public CLI risk forces a `--help` / smoke check.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Help | `python scripts/lab/overnight-combined-stress.py --help` | Same documented flags/defaults as before refactor | N/A |
| Validation | `--hours 0` / empty port / bad counts | Same `SystemExit` messages as before | Non-zero via `SystemExit` |
| Gate touched | Diff touches only this ticket’s files | `lint-touched.py` exit 0 | Any new `[scripts:…]` on hunks → exit 1 |
| Gate all (target) | `--all` filtered to this path | Zero findings for this path (+ companion) | Other dirty labs may still report |

</frozen-after-approval>

## Code Map

- `scripts/lab/overnight-combined-stress.py` — overnight wrapper (mid → bank → long children)
- `scripts/lab/sysex-matrix-mid-loop.py` / `sysex-matrix-bank-loop.py` / `sysex-long-loopback.py` — child labs (unchanged)
- `scripts/quality/lint-touched.py` — enforcer (§3.4); do not change unless bug HALT
- `conventions.md` §3.4 — lab glue ~90 nloc, CCN≤14, nesting≤5, params≤4, useful file ~700
- `docs/dev/scripts-quality-cleanup-checklist.md` — Vague A item for this path
- `scripts/lab/overnight_combined_stress_lib.py` — optional companion if in-file split is insufficient

### Baseline findings (`--all`, this path only, pre-refactor)

- `[scripts:complexity]` `run_overnight` ccn=22 > 14 (L99)
- `[scripts:function-length]` `run_overnight` nloc=164 > 90 (L99–275)
- `[scripts:parameters]` `_run_child` params=5 > 4 (L81)
- Score ~1022, findings=3

## Tasks & Acceptance

**Execution:**
- [x] `scripts/lab/overnight-combined-stress.py` -- Pack child-run args into a dataclass; split `run_overnight` into setup / per-phase / summary helpers -- Clear length/CCN/params findings without changing behavior
- [x] `scripts/lab/overnight_combined_stress_lib.py` (only if needed) -- Not required; in-file helpers cleared all findings -- Keep entrypoint lean; no other caller migration
- [x] `docs/dev/scripts-quality-cleanup-checklist.md` -- Check `overnight-combined-stress.py` + note (`pending commit`) -- Track Vague A progress
- [x] `_bmad-output/implementation-artifacts/spec-scripts-quality-overnight-combined-stress.md` -- Record after-findings in Design Notes when gate is green -- Close the before/after trail

**Acceptance Criteria:**
- Given the target path under `python scripts/quality/lint-touched.py --all`, when findings are filtered to this path (and any new companion), then count is 0
- Given this ticket’s diff only, when `python scripts/quality/lint-touched.py` runs, then exit code is 0
- Given `--help` (and existing documented flags), when invoked after refactor, then flag names/defaults match pre-refactor unless a rename was approved and noted
- Given checklist Vague A, when the ticket finishes, then the `overnight-combined-stress.py` box is checked with a note line

## Spec Change Log

## Design Notes

**Strategy:** Pack `_run_child`’s five args into one options dataclass (≤4 params). Split `run_overnight` into small helpers (resolve paths / mid|bank|long phase / gap / journal) so nloc and CCN fall under lab glue limits. Prefer one file (~394 lines); extract a companion only if a helper still trips thresholds. Preserve soft exit 0, awake hold/release, and child argv flags.

**After findings:** `--all` filtered to this path → **0** findings (no companion). Useful lines ~383 (under ~700). `lint-touched.py` (touched) exit 0; `--help` flags/defaults preserved vs pre-refactor. Review patch: `_should_stop` restores `monotonic < deadline` semantics (NaN hours); gap uses `gap_s > 0` like pre-refactor (NaN skips sleep).

## Suggested Review Order

**Child run packing**

- Frozen ChildRun packs python/script/argv/log_dir so `_run_child` stays ≤4 params.
  [`overnight-combined-stress.py:82`](../../scripts/lab/overnight-combined-stress.py#L82)

- Thin `_run_child` keeps RUN log + KeyboardInterrupt → 130.
  [`overnight-combined-stress.py:107`](../../scripts/lab/overnight-combined-stress.py#L107)

**Stop / cycle glue**

- Deadline stop matches pre-refactor `<` semantics (incl. NaN hours).
  [`overnight-combined-stress.py:119`](../../scripts/lab/overnight-combined-stress.py#L119)

- One cycle: mid → bank → long with stop gates + gap `> 0`.
  [`overnight-combined-stress.py:291`](../../scripts/lab/overnight-combined-stress.py#L291)

- Thin `run_overnight` owns awake hold and soft exit 0.
  [`overnight-combined-stress.py:320`](../../scripts/lab/overnight-combined-stress.py#L320)

**Tracking**

- Vague A checkbox + pending-commit note.
  [`scripts-quality-cleanup-checklist.md:105`](../../docs/dev/scripts-quality-cleanup-checklist.md#L105)


## Verification

**Commands:**
- `python scripts/quality/lint-touched.py --all` -- expected: no lines mentioning `overnight-combined-stress.py` (or new companion)
- `python scripts/quality/lint-touched.py` -- expected: exit 0 on this ticket’s diff
- `python scripts/lab/overnight-combined-stress.py --help` -- expected: documented flags/defaults unchanged

**Manual checks (if no CLI):**
- Diff limited to target (+ optional companion) + checklist + this spec; no second checklist file refactored
