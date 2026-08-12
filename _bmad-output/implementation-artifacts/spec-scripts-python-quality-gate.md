---
title: 'Python scripts anti-drift quality gate'
type: 'feature'
created: '2026-08-12'
status: 'done'
baseline_commit: '785cef45f6d64117bede5fa3eb1863c6b8851388'
review_loop_iteration: 0
context:
  - '{project-root}/conventions.md'
  - '{project-root}/scripts/quality/lint-touched.py'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** Lab/CLI Python under `scripts/` already carries long files and duplicated helpers; new edits can silently worsen that debt while the C++ gate stays silent.

**Approach:** Extend the existing touched-diff quality gate so Python under `scripts/` is checked with slightly looser lizard thresholds, failing only on regression in the ticket diff (`--all` remains diagnostic).

## Boundaries & Constraints

**Always:**
- Single entrypoint: `python scripts/quality/lint-touched.py` (C++ + Python).
- Python scope: `scripts/**/*.py` only (not `_bmad/`, not `.agents/`, not `tools/`).
- Touched mode: exit 1 on script findings that intersect changed hunks (or file-size growth past threshold); `--all`: report findings, exit 0.
- Preserve existing C++ thresholds, scopes (`src|tests|apps|lib|tools`), and exit semantics.
- English only in linter messages and `conventions.md`.
- Document scripts thresholds in `conventions.md` §3.x; historical lab debt stays a separate chore.
- Ticket finish criteria: scripts (+ C++) gate green on this ticket’s diff.

**Ask First:**
- Changing C++ thresholds or making `--all` fail-closed.
- Expanding Python scope beyond `scripts/`.
- Mass lab refactor / shared `BridgeSession` extraction.

**Never:**
- Big-bang Clean Code of `scripts/lab/*`.
- Oberheim `04H` type `0`, new overnight/pre-epic-6 suites, mandatory CI wiring.
- French in code, linter output, or conventions quality text.
- Boy Scout refactors outside the touched zone.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Clean touched diff | No `.py` under `scripts/` over scripts thresholds on changed hunks | Exit 0; report may be empty or C++-only | N/A |
| Script function drift | Touched `.py` function intersects hunks and exceeds scripts nloc/params/ccn/nesting | Exit 1; finding labeled as scripts/Python | Message cites metric + path |
| Oversized file growth | Existing `.py` useful-line count grows and stays above scripts file max | Exit 1 file-size finding | No fail if file shrank or stayed flat while still over threshold |
| New oversized file | New `scripts/**/*.py` above file max | Exit 1 | N/A |
| `--all` debt | Many lab files over thresholds | Findings printed; exit 0 | Must not fail the process |
| Untouched debt | Large lab unchanged in ticket | No fail from that file | Hunk filter excludes it |
| C++ regression guard | Only C++ changed; scripts clean | Same C++ behavior as before | Scripts path must not alter C++ thresholds |

</frozen-after-approval>

## Code Map

- `conventions.md` -- SSOT §3 C++ gate; add §3.x Scripts quality gate (scope, thresholds, touched vs `--all`, WET→DRY, finish criteria).
- `scripts/quality/lint-touched.py` -- extend with Python collect/analyse; keep C++ path; distinguish report language.
- `scripts/quality/requirements.txt` -- already pins lizard; no change unless import needs it.

## Tasks & Acceptance

**Execution:**
- [x] `conventions.md` -- add §3.x Scripts quality gate (paths, metrics, thresholds below, touched vs `--all`, WET→DRY labs, historical debt note, finish criteria via same command) -- SSOT for agents
- [x] `scripts/quality/lint-touched.py` -- detect `scripts/**/*.py`; lizard + indent nesting + useful-line growth; scripts thresholds; hunk-aware; `--all` includes `scripts/` diagnostic; report tags C++ vs scripts; do not change C++ constants/behavior -- anti-drift enforcement
- [x] `_bmad-output/implementation-artifacts/spec-scripts-python-quality-gate.md` -- keep Design Notes + Verification demos accurate after impl -- proof artifact

**Acceptance Criteria:**
- Given a clean worktree (or diff with no script metric hits), when `python scripts/quality/lint-touched.py` runs, then exit 0 and C++ gate still behaves as today.
- Given a touched `scripts/**/*.py` hunk that intersects a function over scripts thresholds, when the gate runs without `--all`, then exit 1 with a scripts-labeled finding.
- Given historical oversized labs unchanged, when the gate runs without `--all`, then those files do not fail the run.
- Given `--all`, when labs exceed thresholds, then findings print and exit code is 0.
- Given this ticket’s own diff, when the gate runs in touched mode, then it exits 0 (Boy Scout only on touched lines if needed).

## Spec Change Log

## Design Notes

**Why one tool:** Same finish command agents already run; avoids dual criteria drift.

**Scripts thresholds (approved 2026-08-12)** — looser than C++ core for lab/CLI pragmatism (KISS/ETC); still blocks 1000+ line growth on new work:

| Metric | C++ | Scripts |
|---|---|---|
| Function nloc | 40 / 50 glue | **70** / **90** glue |
| Params | 4 | **4** |
| CCN | 10 / 12 | **12** / **14** |
| Nesting | 4 | **5** (indent depth) |
| Useful file lines | ~400 `.cpp` | **~700** `.py` |

**Glue paths (scripts):** treat as glue when path contains `/lab/` or `/packaging/` (argparse + I/O harnesses). Other `scripts/` (e.g. `quality/`, `dev/`) use core scripts limits.

**File-size rule:** same spirit as C++ — fail only if new file over max, or useful-line count **grew** vs `--base` and remains over max.

**Historical labs:** `--all` documents debt; no fail-closed until a dedicated chore.

## Verification

**Commands:**
- `python scripts/quality/lint-touched.py` -- expected: exit 0 on this ticket’s diff
- `python scripts/quality/lint-touched.py --all` -- expected: may list lab debt; exit 0
- Touch-only sanity: unchanged C++ thresholds still applied when a `.cpp` under scope is analysed

**Manual checks:**
- Report distinguishes C++ vs scripts findings when both present
- `conventions.md` §3.x matches implemented constants

## Suggested Review Order

**Contract (SSOT)**

- Scripts thresholds, scope, and touched vs `--all` finish criteria
  [`conventions.md:145`](../../conventions.md#L145)

**Gate implementation**

- Scripts constants next to unchanged C++ limits
  [`lint-touched.py:56`](../../scripts/quality/lint-touched.py#L56)

- Python scope under `scripts/` only
  [`lint-touched.py:165`](../../scripts/quality/lint-touched.py#L165)

- Dual-language analyse with hunk filter and parse-error guard
  [`lint-touched.py:493`](../../scripts/quality/lint-touched.py#L493)

- Exclude lizard `self`/`cls` from Python param count
  [`lint-touched.py:470`](../../scripts/quality/lint-touched.py#L470)

- Indent nesting estimate for Python
  [`lint-touched.py:244`](../../scripts/quality/lint-touched.py#L244)

- Report tags `[scripts:…]` vs `[cpp:…]`
  [`lint-touched.py:590`](../../scripts/quality/lint-touched.py#L590)

- `--all` includes `tools/` + `scripts/` diagnostic (exit 0)
  [`lint-touched.py:196`](../../scripts/quality/lint-touched.py#L196)
