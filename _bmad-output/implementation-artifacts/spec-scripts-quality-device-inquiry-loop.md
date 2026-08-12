---
title: 'Scripts quality — device-inquiry-loop.py'
type: 'refactor'
created: '2026-08-12'
status: 'done'
review_loop_iteration: 0
baseline_commit: '8b29a91'
context:
  - '{project-root}/conventions.md'
  - '{project-root}/docs/dev/scripts-quality-cleanup-checklist.md'
  - '{project-root}/scripts/quality/lint-touched.py'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** `scripts/lab/device-inquiry-loop.py` fails the scripts quality gate (§3.4) with two lizard findings on `_run_inquiry_loop` (params=5 > 4, nesting≈6 > 5). That blocks Vague B cleanup without a lab big-bang.

**Approach:** Structural refactor of this Device Inquiry lab only (pack loop options into a small dataclass / namespace; extract late-reply / cycle helpers so nesting stays ≤5) so all `[scripts:…]` findings for this path disappear under `--all`, while preserving public CLI flags, Pass/Fail threshold, Bridge optional start/stop, fresh WinMM MIDI child process, and log layout. Optional thin companion under `scripts/lab/` only if needed — no other caller migration.

## Boundaries & Constraints

**Always:**
- Touch primarily `scripts/lab/device-inquiry-loop.py`; optional companion only if useful for this file’s findings.
- Preserve documented CLI flags, defaults, validation, exit codes (`0` pass / `2` fail / non-zero via `SystemExit`), Bridge session + fresh MIDI process topology, inquiry bytes, port matching, and log path pattern (`tests/lab-logs/device-inquiry/…`).
- English only in code, logs, and argparse help.
- End of ticket: `python scripts/quality/lint-touched.py` exit 0 on the diff; zero `[scripts:…]` findings for this path under `--all`.
- Update checklist checkbox + note (`pending commit` until a hash is known).
- No commit unless Guillaume explicitly asks.

**Ask First:**
- Renaming or removing a public CLI flag (document justification first).
- Changing §3.4 thresholds or `lint-touched.py` behavior (HALT on suspected linter bug).
- Changing Pass/Fail semantics (`--pass-percent`, exit `2` on fail) or inquiry / Identity Reply matching.
- Migrating other labs onto `lab_midi_common.py` (or a new shared module) beyond what this file needs to clear findings.

**Never:**
- Second checklist file in this conversation.
- New SysEx scenario, overnight suite, epic 6, or C++ Bridge changes.
- Mass reformat / cleanup of all `scripts/lab/*`.
- Hardware lab runs unless public CLI risk forces a `--help` / smoke check.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Help | `python scripts/lab/device-inquiry-loop.py --help` | Same documented flags/defaults as before refactor | N/A |
| Validation | `--count 0` / `--interval 0` / bad timeouts | Same `SystemExit` messages as before | Non-zero via `SystemExit` |
| Gate touched | Diff touches only this ticket’s files | `lint-touched.py` exit 0 | Any new `[scripts:…]` on hunks → exit 1 |
| Gate all (target) | `--all` filtered to this path | Zero findings for this path (+ companion) | Other dirty labs may still report |

</frozen-after-approval>

## Code Map

- `scripts/lab/device-inquiry-loop.py` — Device Inquiry host MIDI loop; optional Bridge start/stop + fresh WinMM child
- `scripts/lab/lab_midi_common.py` — existing shared Bridge/MIDI helpers; reference only unless this file must import to clear findings
- `scripts/quality/lint-touched.py` — enforcer (§3.4); do not change unless bug HALT
- `conventions.md` §3.4 — lab glue ~90 nloc, CCN≤14, nesting≤5, params≤4, useful file ~700
- `docs/dev/scripts-quality-cleanup-checklist.md` — Vague B item for this path

### Baseline findings (`--all`, this path only, pre-refactor)

- `[scripts:nesting]` `_run_inquiry_loop` nesting≈6 > 5 (L355)
- `[scripts:parameters]` `_run_inquiry_loop` params=5 > 4 (L355)
- Score ~16, findings=2

## Tasks & Acceptance

**Execution:**
- [x] `scripts/lab/device-inquiry-loop.py` -- Pack `_run_inquiry_loop` args into a frozen options dataclass (ports + count/interval/timeout/pass); extract cycle / late-reply helpers so nesting ≤5 and params ≤4 -- Clear both findings without changing behavior
- [x] `scripts/lab/*_lib.py` or import from `lab_midi_common.py` (only if needed) -- Not required; in-file InquiryLoopOpts/InquiryCycle cleared all findings -- Keep entrypoint lean; no other caller migration
- [x] `docs/dev/scripts-quality-cleanup-checklist.md` -- Check `device-inquiry-loop.py` + note (`pending commit`) -- Track Vague B progress
- [x] `_bmad-output/implementation-artifacts/spec-scripts-quality-device-inquiry-loop.md` -- Record after-findings in Design Notes when gate is green -- Close the before/after trail

**Acceptance Criteria:**
- Given the target path under `python scripts/quality/lint-touched.py --all`, when findings are filtered to this path (and any new companion), then count is 0
- Given this ticket’s diff only, when `python scripts/quality/lint-touched.py` runs, then exit code is 0
- Given `--help` (and existing documented flags), when invoked after refactor, then flag names/defaults match pre-refactor unless a rename was approved and noted
- Given checklist Vague B, when the ticket finishes, then the `device-inquiry-loop.py` box is checked with a note line

## Spec Change Log

## Design Notes

**Strategy:** Clear the two `_run_inquiry_loop` findings only. Pack ports + timing/pass fields into frozen `InquiryLoopOpts`; hold open ports/`mido`/lines in `InquiryCycle`; route late VirtualMIDI slack through `_resolve_inquiry_outcome` + `_run_one_inquiry` so nesting ≤5 and params ≤4. Prefer one file; do **not** migrate BridgeSession onto `lab_midi_common.py` unless an in-file split still trips thresholds. Preserve CLI, exit `0`/`2`, Bridge + fresh MIDI child, and log layout.

**After findings:** `--all` filtered to this path → **0** findings (no companion). `lint-touched.py` (touched) exit 0; `--help` flags/defaults preserved vs pre-refactor.

## Verification

**Commands:**
- `python scripts/quality/lint-touched.py --all` filtered to `device-inquiry-loop` — expected: 0 findings for this path
- `python scripts/quality/lint-touched.py` — expected: exit 0 on ticket diff
- `python scripts/lab/device-inquiry-loop.py --help` — expected: same flags/defaults as pre-refactor

## Suggested Review Order

**Inquiry loop packing**

- Frozen opts pack ports + timing/pass so `_run_inquiry_loop` stays ≤4 params.
  [`device-inquiry-loop.py:358`](../../scripts/lab/device-inquiry-loop.py#L358)

- Cycle + pending pack open ports and first-wait result for helpers.
  [`device-inquiry-loop.py:368`](../../scripts/lab/device-inquiry-loop.py#L368)

- Late VirtualMIDI slack / TIMEOUT / LATE logging (Bridge SendToHost rationale kept).
  [`device-inquiry-loop.py:389`](../../scripts/lab/device-inquiry-loop.py#L389)

- One inquiry: send, wait, resolve, interval sleep when more remain.
  [`device-inquiry-loop.py:423`](../../scripts/lab/device-inquiry-loop.py#L423)

- Thin loop: open ports, tally got/late nested, same summary Pass/Fail.
  [`device-inquiry-loop.py:443`](../../scripts/lab/device-inquiry-loop.py#L443)

- `run_lab` builds opts from argparse Namespace (CLI unchanged).
  [`device-inquiry-loop.py:529`](../../scripts/lab/device-inquiry-loop.py#L529)

**Tracking**

- Vague B checkbox + pending-commit note.
  [`scripts-quality-cleanup-checklist.md:135`](../../docs/dev/scripts-quality-cleanup-checklist.md#L135)
