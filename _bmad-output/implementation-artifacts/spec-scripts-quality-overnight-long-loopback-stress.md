---
title: 'Scripts quality — overnight-long-loopback-stress.py'
type: 'refactor'
created: '2026-08-12'
status: 'done'
review_loop_iteration: 0
baseline_commit: 'a2c634d'
context:
  - '{project-root}/conventions.md'
  - '{project-root}/docs/dev/scripts-quality-cleanup-checklist.md'
  - '{project-root}/scripts/quality/lint-touched.py'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** `scripts/lab/overnight-long-loopback-stress.py` fails the scripts quality gate (§3.4) with one lizard finding (`_run_child` has 7 params). That blocks Vague B cleanup without a lab big-bang.

**Approach:** Structural refactor of this overnight long SysEx DIN loopback wrapper only (pack child-run args into a small options dataclass so `_run_child` stays ≤4 params) so all `[scripts:…]` findings for this path disappear under `--all`, while preserving public CLI flags, soft exit 0, child topology (Out2→In2 DIN via `sysex-long-loopback.py` + Bridge), journal layout, awake hold, and gap interrupt behavior. Optional thin companion under `scripts/lab/` only if needed — no other caller migration.

## Boundaries & Constraints

**Always:**
- Touch primarily `scripts/lab/overnight-long-loopback-stress.py`; optional companion only if useful for this file’s findings.
- Preserve documented CLI flags, defaults, validation, soft harness exit `0`, Ctrl+C / gap interrupt behavior, journal path pattern (`tests/lab-logs/overnight-long-loopback/…`), child argv shape (`--with-bridge`, configurable out/in ports, `--pass-percent 100`, Bridge path, long-lab sizes/count/interval/timeout), and Windows awake hold/release.
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
| Help | `python scripts/lab/overnight-long-loopback-stress.py --help` | Same documented flags/defaults as before refactor | N/A |
| Validation | `--hours 0` / bad counts / bad interval / empty ports | Same `SystemExit` messages as before | Non-zero via `SystemExit` |
| Gate touched | Diff touches only this ticket’s files | `lint-touched.py` exit 0 | Any new `[scripts:…]` on hunks → exit 1 |
| Gate all (target) | `--all` filtered to this path | Zero findings for this path (+ companion) | Other dirty labs may still report |

</frozen-after-approval>

## Code Map

- `scripts/lab/overnight-long-loopback-stress.py` — overnight wrapper looping `sysex-long-loopback.py` until `--hours` elapses (DIN Out2→In2)
- `scripts/lab/overnight-matrix-stress.py` — sibling overnight pattern (`ChildRun` packing); reference only, do not change
- `scripts/lab/sysex-long-loopback.py` — child lab (unchanged)
- `scripts/quality/lint-touched.py` — enforcer (§3.4); do not change unless bug HALT
- `conventions.md` §3.4 — lab glue ~90 nloc, CCN≤14, nesting≤5, params≤4, useful file ~700
- `docs/dev/scripts-quality-cleanup-checklist.md` — Vague B item for this path
- `scripts/lab/overnight_long_loopback_stress_lib.py` — optional companion if in-file split is insufficient

### Baseline findings (`--all`, this path only, pre-refactor)

- `[scripts:parameters]` `_run_child` params=7 > 4 (L73)
- Score ~107, findings=1

## Tasks & Acceptance

**Execution:**
- [x] `scripts/lab/overnight-long-loopback-stress.py` -- Pack `_run_child` args into a frozen options dataclass (incl. out/in ports); keep cycle loop / soft exit / awake semantics identical -- Clear the params finding without changing behavior
- [x] `scripts/lab/overnight_long_loopback_stress_lib.py` (only if needed) -- Not required; in-file ChildRun/OvernightContext cleared all findings -- Keep entrypoint lean; no other caller migration
- [x] `docs/dev/scripts-quality-cleanup-checklist.md` -- Check `overnight-long-loopback-stress.py` + note (`pending commit`) -- Track Vague B progress
- [x] `_bmad-output/implementation-artifacts/spec-scripts-quality-overnight-long-loopback-stress.md` -- Record after-findings in Design Notes when gate is green -- Close the before/after trail

**Acceptance Criteria:**
- Given the target path under `python scripts/quality/lint-touched.py --all`, when findings are filtered to this path (and any new companion), then count is 0
- Given this ticket’s diff only, when `python scripts/quality/lint-touched.py` runs, then exit code is 0
- Given `--help` (and existing documented flags), when invoked after refactor, then flag names/defaults match pre-refactor unless a rename was approved and noted
- Given checklist Vague B, when the ticket finishes, then the `overnight-long-loopback-stress.py` box is checked with a note line

## Spec Change Log

## Design Notes

**Strategy:** Mirror the already-green overnight wrappers: pack `_run_child`’s seven args into one frozen options dataclass so the function takes ≤4 params (typically `run` + `stats`). Keep ports configurable on the dataclass (unlike matrix’s fixed In1/Out1). Prefer one file; extract a companion only if something else trips thresholds. Preserve soft exit 0, child argv (`--with-bridge`, `--pass-percent 100`, long sizes/count), Windows awake hold, and gap `> 0` sleep semantics. After packing alone raised nesting on `run_overnight`, split into `_run_one_cycle` / gap / journal helpers (same shape as overnight-matrix).

**After findings:** `--all` filtered to this path → **0** findings (no companion). Useful lines ~281 (under ~700). `lint-touched.py` (touched) exit 0; `--help` flags/defaults preserved vs pre-refactor.

## Suggested Review Order

**Child run packing**

- Frozen ChildRun packs python/script/extra-argv/ports/log_dir so `_run_child` stays ≤4 params.
  [`overnight-long-loopback-stress.py:73`](../../scripts/lab/overnight-long-loopback-stress.py#L73)

- Thin `_run_child` keeps Bridge/port prefix + RUN log + KeyboardInterrupt → 130.
  [`overnight-long-loopback-stress.py:99`](../../scripts/lab/overnight-long-loopback-stress.py#L99)

**Stop / cycle glue**

- Deadline stop matches pre-refactor `<` semantics (incl. NaN hours).
  [`overnight-long-loopback-stress.py:124`](../../scripts/lab/overnight-long-loopback-stress.py#L124)

- One cycle: long child + result note + gap `> 0`.
  [`overnight-long-loopback-stress.py:200`](../../scripts/lab/overnight-long-loopback-stress.py#L200)

- Thin `run_overnight` owns soft exit 0; journal always written; awake released in `finally`.
  [`overnight-long-loopback-stress.py:226`](../../scripts/lab/overnight-long-loopback-stress.py#L226)

**Tracking**

- Vague B checkbox + pending-commit note.
  [`scripts-quality-cleanup-checklist.md:131`](../../docs/dev/scripts-quality-cleanup-checklist.md#L131)

## Verification

**Commands:**
- `python scripts/quality/lint-touched.py --all` filtered to `overnight-long-loopback-stress` -- expected: 0 findings for this path (+ companion if any)
- `python scripts/quality/lint-touched.py` -- expected: exit 0 on this ticket’s diff
- `python scripts/lab/overnight-long-loopback-stress.py --help` -- expected: same flags/defaults as pre-refactor
