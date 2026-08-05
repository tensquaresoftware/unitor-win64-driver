---
title: 'Strengthen Windows CI/CD merge gate'
type: 'chore'
created: '2026-08-05'
status: 'done'
baseline_commit: '5142a875d49499ab177cf3d312826088e88dd860'
review_loop_iteration: 0
context:
  - '{project-root}/docs/dev/windows-ci-toolchain.md'
  - '{project-root}/conventions.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** PRs and pushes to `main` only prove that the Bridge compiles on Windows. The project quality gate (`lint-touched`) stays local, there is no single merge-check job, and contributors lack a clear map of what CI covers versus what still needs a Windows PC with MT4 hardware.

**Approach:** Extend the existing Windows workflow into one pipeline: quality-gate on the touched C++ diff, Windows x64 Release build (unchanged toolchain pin), and a `ci-success` job as the sole green merge signal. Document the hardware/manual gap and defer C++ unit-test scaffolding to a follow-up ticket. Patterns from Matrix-Control (quality-gate, `ci-success`, checkout v5) — adapted to Windows-only usermode Bridge, no multi-OS or release pipeline.

## Boundaries & Constraints

**Always:**
- Keep runner pin `windows-2022` for the build job (AD-13); do not switch to `windows-latest`.
- Artefacts under `builds/` only; configure with `cmake -S . -B builds/ci -A x64`, Release build, assert `Bridge.exe`.
- Quality gate uses existing `scripts/quality/lint-touched.py` + `scripts/quality/requirements.txt` (no second analyser).
- Single workflow file (extend or rename the current one — no duplicate overlapping workflows).
- Harmonize `actions/checkout` to v5 wherever this workflow is touched.
- Docs for CI/manual loop in English; chat stays French.
- Commits only when Guillaume explicitly asks.

**Ask First:**
- Adding Authenticode, packaging, or a public release workflow.
- Adding macOS/Linux CI runners.
- Introducing a C++ unit-test framework / CMake `Tests` target in this ticket.
- Changing required-check names after `ci-success` exists if Guillaume already configured GitHub branch protection outside the repo.

**Never:**
- Blind copy of Matrix multi-OS matrix, JUCE checkout, draft-PR fast tier, or `release.yml`.
- Full-tree Boy Scout lint (`--all` as a failing CI gate).
- Kernel driver / WDK CI.
- Inventing a large test factory when no CMake test target exists yet.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| PR with C++ under `src/` | Diff vs base branch | `quality-gate` runs lint on touched hunks; `build` compiles Release; both green → `ci-success` green | Lint findings → quality-gate fail; compile error → build fail; `ci-success` stays red |
| Push to `main` | Push range / `before` SHA when available | Same three jobs; lint base = push `before` or `HEAD~1` fallback (Matrix pattern) | Invalid/missing `before` → fallback base, still run lint |
| PR touching only docs/workflows | No C++ in diff | `quality-gate` exits 0 (nothing to lint); build still runs | N/A |
| Lint violation on touched function | Over-budget NLOC/CCN/etc. | `quality-gate` fails with lizard findings | Job red; no merge via `ci-success` |
| Build succeeds, quality fails | Mixed results | `ci-success` must **not** pass (`always()` + explicit `needs.*.result == 'success'`) | Prevents false green merge gate |

</frozen-after-approval>

## Code Map

- `.github/workflows/windows-build.yml` -- sole CI workflow to extend (jobs: quality-gate, build, ci-success)
- `scripts/quality/lint-touched.py` -- SSOT quality analyser; supports `--base REF`
- `scripts/quality/requirements.txt` -- `lizard` pin for quality-gate
- `docs/dev/windows-ci-toolchain.md` -- update for new jobs, commands, checkout v5, merge gate
- `docs/tests/smoke-epic1-mt4.md` -- existing manual hardware smoke (link from CI doc; do not duplicate checklist)
- `CMakeLists.txt` -- Bridge-only target today (no Tests); cite as unit-test gap
- `src/Protocol/EmagicCableMapper.*`, `src/Profile/DeviceProfile.*`, `src/App/MapperSmoke.*` -- pure-logic candidates for a **follow-up** unit-test ticket (out of scope here)
- Matrix reference (read-only inspiration): `/Volumes/Guillaume/Dev/Projects/JUCE/Matrix-Control/.github/workflows/build-and-test.yml`

## Tasks & Acceptance

**Execution:**
- [x] `.github/workflows/windows-build.yml` -- Add `quality-gate` job (ubuntu-latest OK): checkout@v5 with adequate fetch depth, pip install requirements, run `lint-touched.py` with PR base vs push-before/`HEAD~1` logic mirrored from Matrix; keep `build` on `windows-2022` with checkout@v5 + existing cmake/Release/`Bridge.exe` steps; add `ci-success` needing both, `if: always()` and success checks only -- single merge gate without draft tier
- [x] `docs/dev/windows-ci-toolchain.md` -- Document jobs, triggers, local lint command, merge-gate name `ci-success`, Matrix reuse vs omit list, explicit hardware/manual gap (point to smoke doc), and deferred C++ unit-test surface (mapper/profile/MapperSmoke)
- [x] `_bmad-output/implementation-artifacts/deferred-work.md` -- Append one entry for follow-up C++ unit tests without hardware (Emagic mapper, DeviceProfile, MapperSmoke / CMake Tests target)

**Acceptance Criteria:**
- Given a PR or push to `main`, when the workflow runs, then jobs `quality-gate`, `build`, and `ci-success` exist and `ci-success` is green only if both upstream jobs succeed.
- Given C++ changes that violate `conventions.md` §3 on touched hunks, when `quality-gate` runs, then the job fails and `ci-success` does not succeed.
- Given a clean configure/build on `windows-2022`, when `build` finishes, then `Bridge.exe` is still asserted under `builds/ci`.
- Given updated toolchain docs, when a contributor reads them, then they know: CI commands, that hardware/DAW/SysEx/VirtualMIDI stay manual on a Windows PC, and that C++ unit tests are deferred.
- Given this ticket’s scope, when comparing to Matrix-Control, then checkout v5 + quality-gate + ci-success are reused in spirit; multi-OS matrix, draft tier, JUCE, and release.yml are omitted.

## Spec Change Log

## Design Notes

**Workflow shape (lean):**

```yaml
jobs:
  quality-gate:  # ubuntu-latest, Python + lizard
  build:         # needs: quality-gate; windows-2022; cmake builds/ci; Bridge.exe
  ci-success:
    needs: [quality-gate, build]
    if: always() && needs.quality-gate.result == 'success' && needs.build.result == 'success'
```

Also: `permissions: contents: read`; concurrency cancel-in-progress; PR `git fetch` of base ref must succeed (no `|| true`).

**Omit draft tier:** only one OS; fast vs full would barely save cost and adds label/gh complexity.

**quality-gate on Ubuntu:** lint is OS-agnostic; `build` waits on it so Windows minutes are not spent when lint already failed.

## Verification

**Commands:**
- `python -m pip install -r scripts/quality/requirements.txt && python scripts/quality/lint-touched.py --base origin/main` -- expected: exit 0 on conforming tree/diff
- YAML review of `.github/workflows/windows-build.yml` -- expected: three jobs, checkout@v5, no draft tier, no macOS/Linux build
- Inspect `docs/dev/windows-ci-toolchain.md` -- expected: merge gate + manual gap + deferred unit tests documented

**Manual checks (if no CLI):**
- After push/PR: GitHub Actions shows `quality-gate`, `build`, `ci-success`; recommend branch protection require `ci-success` only (repo settings — Guillaume)

## Suggested Review Order

**Merge gate**

- Entry point: three jobs, concurrency, least-privilege token
  [`windows-build.yml:13`](../../.github/workflows/windows-build.yml#L13)

- Lint on PR/push diff; fetch base must succeed
  [`windows-build.yml:34`](../../.github/workflows/windows-build.yml#L34)

- Windows build only after quality-gate is green
  [`windows-build.yml:52`](../../.github/workflows/windows-build.yml#L52)

- Sole merge signal when both legs succeed
  [`windows-build.yml:85`](../../.github/workflows/windows-build.yml#L85)

**Docs & deferred**

- Jobs, manual hardware gap, Matrix reused vs omitted
  [`windows-ci-toolchain.md:1`](../../docs/dev/windows-ci-toolchain.md#L1)

- Follow-up unit-test ticket tracked here
  [`deferred-work.md:42`](deferred-work.md#L42)
