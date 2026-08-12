---
title: 'Scripts quality — verify-installer-contract.py'
type: 'refactor'
created: '2026-08-12'
status: 'done'
review_loop_iteration: 0
baseline_commit: 'e356408'
context:
  - '{project-root}/conventions.md'
  - '{project-root}/docs/dev/scripts-quality-cleanup-checklist.md'
  - '{project-root}/scripts/quality/lint-touched.py'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** `scripts/packaging/verify-installer-contract.py` fails the scripts quality gate (§3.4) with two lizard findings on a monolithic `main` (nloc≈183, ccn≈48). That blocks the next packaging checkbox without a scripts big-bang.

**Approach:** Structural refactor of this file only — split `main` into focused domain check helpers and small shared assert helpers — so all `[scripts:…]` findings for this path disappear under `--all`, while preserving offline contract behavior and exit codes. No new shared packaging/lab module unless a second caller appears.

## Boundaries & Constraints

**Always:**
- Touch primarily `scripts/packaging/verify-installer-contract.py` (Boy Scout: optional micro-helpers in the same file).
- Preserve checked needles/paths, Pass/Fail messages shape (`OK:` / `FAIL:`), and exits (`0` pass, `1` fail via `SystemExit`).
- No public argparse today — keep bare CLI: `python scripts/packaging/verify-installer-contract.py`.
- English only in code, logs, and failure strings.
- End of ticket: `lint-touched.py` exit 0 on the diff; zero `[scripts:…]` findings for this path under `--all`.
- Update checklist checkbox + note (`pending commit` until a hash is known).
- No commit unless Guillaume explicitly asks.

**Ask First:**
- Changing which files or needles the contract asserts (behavior change, not structure).
- Changing §3.4 thresholds or `lint-touched.py` behavior (HALT on suspected linter bug).
- Extracting a shared packaging module used by other callers beyond this file’s need.

**Never:**
- Second checklist file in this conversation.
- New SysEx scenarios, overnight suites, epic 6, or C++ Bridge changes.
- Mass reformat / cleanup of all `scripts/packaging/*` or `scripts/lab/*`.
- Hardware / ISCC installer runs (script is offline-only).

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Happy path | Repo contract files present and match needles | Prints `OK: installer contract checks passed`; exit 0 | N/A |
| Missing required file | One listed ISS/INF/PS1/doc path absent | `FAIL: missing required file: …` on stderr; exit 1 | `SystemExit(1)` |
| Missing needle | File present but required substring gone | `FAIL: <relpath> missing …` on stderr; exit 1 | `SystemExit(1)` |
| Gate touched | Diff touches only this ticket’s files | `lint-touched.py` exit 0 | Any new `[scripts:…]` on hunks → exit 1 |
| Gate all (target) | `--all` filtered to this path | Zero findings for `verify-installer-contract.py` | Other dirty scripts may still report |

</frozen-after-approval>

## Code Map

- `scripts/packaging/verify-installer-contract.py` — sole target; today almost all logic in `main`
- `scripts/quality/lint-touched.py` — enforcer (§3.4); do not change unless bug HALT
- `conventions.md` §3.4 — packaging glue ~90 nloc, CCN≤14, nesting≤5, params≤4, useful file ~700
- `docs/dev/scripts-quality-cleanup-checklist.md` — Vague A item; mark done + note after green
- `installer/*`, `docs/tests/smoke-epic4-public-installer-mt4.md`, CMake/version headers — read-only contract inputs (do not edit for this ticket)

### Baseline findings (`--all`, this path only, pre-refactor)

- `[scripts:function-length]` `main` nloc=183 > 90 (L27–233)
- `[scripts:complexity]` `main` ccn=48 > 14 (L27)
- Score ~1183, findings=2
- Useful lines ~201 (under file-size cap; no file-size finding)

## Tasks & Acceptance

**Execution:**
- [x] `scripts/packaging/verify-installer-contract.py` -- Split `main` into domain checkers (ISS, INF/GUID/HWID, build script, CMake/version, smoke guide, autostart helpers, Authenticode surfaces) plus small require/contain helpers; keep needle tables as data -- Clear length + CCN findings without changing assert set
- [x] `docs/dev/scripts-quality-cleanup-checklist.md` -- Check Vague A box for this path; add note (`pending commit` or hash later) -- Track cleanup progress

**Acceptance Criteria:**
- Given `--all` output filtered to `verify-installer-contract.py`, when the refactor is done, then `[scripts:…]` findings for that path = 0
- Given a ticket diff that only touches this cleanup, when `python scripts/quality/lint-touched.py` runs, then exit 0
- Given the same repo fixtures as before, when the script runs bare, then exit 0 and the same OK line if contracts still hold; missing file/needle still exits 1 with `FAIL:`

## Spec Change Log

- 2026-08-12 — Human approved needle sync for Authenticode policy doc drift (`strongly recommended` / `Not a hard V1 gate` → `no certificate purchase` / `Not a hard packaging gate`) so the offline contract matches `docs/dev/authenticode-and-smartscreen.md` after the hobby posture course correction. Avoids known-bad: green lizard with a permanently failing contract on `main`.

## Design Notes

**Before → after (this path only):**
- Before: 2 findings (`main` nloc=183, ccn=48), score ~1183
- After: 0 findings for this path under `--all`; `main` is thin orchestration over domain `check_*` helpers

**Needle sync (approved Ask First):** Authenticode policy table phrases updated to match post-`f6d881d` hobby docs; other needles unchanged.

## Verification

**Commands:**
- `python scripts/quality/lint-touched.py --all` -- expected: no lines mentioning `verify-installer-contract.py` as a finding
- `python scripts/quality/lint-touched.py` -- expected: exit 0 on this ticket’s diff
- `python scripts/packaging/verify-installer-contract.py` -- expected: exit 0 + `OK: installer contract checks passed` on a healthy tree
- `python scripts/packaging/verify-installer-contract.py --help` -- N/A (no argparse); do not add flags in this ticket

## Suggested Review Order

- Thin `main` orchestration restores pre-refactor check order after the split
  [`verify-installer-contract.py:291`](../../scripts/packaging/verify-installer-contract.py#L291)

- Domain helpers + needle tables clear lizard length/CCN without a shared packaging module
  [`verify-installer-contract.py:16`](../../scripts/packaging/verify-installer-contract.py#L16)

- Authenticode hobby needles synced to current policy doc (approved Ask First)
  [`verify-installer-contract.py:271`](../../scripts/packaging/verify-installer-contract.py#L271)

- Checklist Vague A item marked cleared with pending-commit note
  [`scripts-quality-cleanup-checklist.md:94`](../../docs/dev/scripts-quality-cleanup-checklist.md#L94)
