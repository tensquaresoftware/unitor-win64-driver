---
title: 'Lean C++ unit tests without hardware'
type: 'feature'
created: '2026-08-05'
status: 'done'
review_loop_iteration: 0
baseline_commit: '4266cf52e48eef456017e963c1d2e2fcfa6ed9a6'
context:
  - '{project-root}/conventions.md'
  - '{project-root}/docs/dev/windows-ci-toolchain.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** The Windows merge gate proves compile + touched-diff lint, but pure Emagic cable and DeviceProfile logic can regress with no automatic signal.

**Approach:** Add a lean Catch2 suite (`BridgeTests`) covering Profile + Protocol only, run it after the Release build in the existing `windows-build.yml` job, and document CI vs manual coverage.

## Boundaries & Constraints

**Always:**
- Out-of-source builds under `builds/` only (never repo-root `build/`)
- Catch2 via CMake FetchContent (pinned tag); C++17; English diagnostics in tests
- Tests link only `DeviceProfile` + `EmagicCableMapper` (+ optional shared smoke vectors) — no WinUSB, VirtualMIDI, DeviceSession, or DAW mocks
- Extend `.github/workflows/windows-build.yml`; failing tests fail `build` so `ci-success` stays red
- Runner stays `windows-2022` for C++; no macOS/Linux CI jobs
- Keep `Bridge --test-mapper` working (thin CLI over same vectors or equivalent)

**Ask First:**
- Switching away from Catch2 / FetchContent
- Adding a second workflow or non-Windows CI runner for tests
- Introducing WinUSB/VirtualMIDI mocks

**Never:**
- Hardware, Zadig, live VirtualMIDI, DAW, SysEx/clock/soak automation
- Authenticode / packaging / release workflow work
- Cosmetic 100% coverage or AMT8/Unitor8 abstractions beyond declarative profile lookup already in code
- Duplicating MapperSmoke vectors wholesale without reuse/extract

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| MT4 profile hit | VID `0x086A`, PID `0x0003` | Non-null profile; MT4 ifnum/masks | N/A |
| Unknown identity | Wrong VID or PID | `findDeviceProfile` → `nullptr` | Fail closed |
| Product OUT ports | MT4 `outCables` | Indices `{0,1,2,3}`; cable 15 excluded | Truncate if `maxCount` small |
| Encode cable 0 note | Note-on, OUT cable 0 | MIDI + `0xFF`; no leading F5 | N/A |
| Encode switched note | Note-on, OUT cable 2 | `F5 0x03` + MIDI + `0xFF` | N/A |
| Decode F5 note | Bulk: F5, port, note, FF, pad | Sink sees cable 1 + note bytes | Ignore pad after FF |
| Split F5 | F5 alone then data in 2nd decode | Sticky pending F5; correct cable | N/A |
| Invalid encode | Non-OUT cable or tiny buffer | `EncodeToDevice` false | English error string |

</frozen-after-approval>

## Code Map

- `CMakeLists.txt` — add `enable_testing`, FetchContent Catch2, `BridgeTests` executable + `add_test`
- `tests/` — Catch2 cases for DeviceProfile + EmagicCableMapper (and shared vectors if extracted)
- `src/Profile/DeviceProfile.*` — declarative VID/PID, masks, `collectProductCableIndices`
- `src/Protocol/EmagicCableMapper.*` — F5 encode/decode under test
- `src/App/MapperSmoke.*` — existing `--test-mapper` vectors; reuse/extract, do not abandon CLI
- `.github/workflows/windows-build.yml` — run tests after Release build
- `docs/dev/windows-ci-toolchain.md` — CI now runs unit tests; shrink/remove deferred section
- `_bmad-output/implementation-artifacts/deferred-work.md` — mark this deferred item resolved only if workflow requires; prefer leave historical entry unless step says update

## Tasks & Acceptance

**Execution:**
- [x] `CMakeLists.txt` -- Add Catch2 FetchContent (pinned), `BridgeTests` from Profile+Protocol (+ tests sources), `enable_testing`/`add_test` -- lean target without Usb/Midi/App device stack
- [x] `tests/*.cpp` (+ optional shared vector header under `src/` or `tests/`) -- Implement matrix cases; prefer extracting MapperSmoke expected bytes into a shared place over copy-paste
- [x] `src/App/MapperSmoke.*` -- Keep `--test-mapper` green; call shared vectors or stay equivalent
- [x] `.github/workflows/windows-build.yml` -- After Release build (+ Bridge.exe check), run `BridgeTests` / `ctest -C Release` so failure fails the job
- [x] `docs/dev/windows-ci-toolchain.md` -- Document unit-test step; move mapper/profile from “deferred” into “what CI covers”; keep hardware checklist manual

**Acceptance Criteria:**
- Given a clean configure into `builds/…`, when building `BridgeTests`, then the target links without winusb/setupapi
- Given `BridgeTests` runs, when the I/O matrix cases execute, then all pass with zero hardware
- Given CI `build` on `windows-2022`, when tests fail, then the job fails and `ci-success` does not pass
- Given docs updated, when reading the toolchain page, then CI coverage vs manual Epic 1 smoke is explicit
- Given `lint-touched.py` on the C++ diff, when conventions §3 apply, then the gate is clean

## Spec Change Log

## Design Notes

**Catch2 over GoogleTest:** single-header-friendly FetchContent, lighter ceremony for ~8 cases, no need for gmock.

**Target shape (illustrative):**
```cmake
add_executable(BridgeTests
  tests/BridgeTestsMain.cpp  # or Catch2 WithMain
  tests/DeviceProfileTests.cpp
  tests/EmagicCableMapperTests.cpp
  src/Profile/DeviceProfile.cpp
  src/Protocol/EmagicCableMapper.cpp)
```

Prefer one Catch2 main translation unit; avoid linking `Main.cpp` / Usb / Midi.

**CI step (illustrative):** after Release build, `ctest --test-dir builds/ci -C Release --output-on-failure` or run the `BridgeTests.exe` path under `builds/ci`.

## Verification

**Commands:**
- `cmake -S . -B builds/unit-tests -A x64` (Windows) -- expected: configure OK with Catch2 fetched
- `cmake --build builds/unit-tests --config Release --target BridgeTests` -- expected: success
- `ctest --test-dir builds/unit-tests -C Release --output-on-failure` -- expected: all tests pass
- `python scripts/quality/lint-touched.py --base origin/main` -- expected: exit 0 on touched C++

**Manual checks (if no CLI):**
- On macOS, optional: configure/build only Profile+Protocol test sources if toolchain allows — not a CI requirement
- Confirm `docs/dev/windows-ci-toolchain.md` no longer lists this suite as deferred

## Suggested Review Order

**CMake target**

- Catch2 pinned by commit; `BridgeTests` links Profile + Protocol only
  [`CMakeLists.txt:49`](../../CMakeLists.txt#L49)

- Registers `ctest` with per-config `TARGET_FILE` for VS output dirs
  [`CMakeLists.txt:77`](../../CMakeLists.txt#L77)

**Shared smoke (no duplication)**

- Shared encode/decode vectors used by CLI and Catch2
  [`EmagicMapperSmokeSupport.h:10`](../../src/Protocol/EmagicMapperSmokeSupport.h#L10)

- `--test-mapper` stays a thin wrapper over the shared runner
  [`MapperSmoke.cpp:9`](../../src/App/MapperSmoke.cpp#L9)

**CI gate**

- After Release `Bridge.exe`, failing `ctest` fails the `build` job
  [`windows-build.yml:85`](../../.github/workflows/windows-build.yml#L85)

**Docs**

- CI automated coverage vs hardware checklist spelled out
  [`windows-ci-toolchain.md:43`](../../docs/dev/windows-ci-toolchain.md#L43)

**Tests**

- Profile VID/PID, OUT/IN ports, truncation edges
  [`DeviceProfileTests.cpp:7`](../../tests/DeviceProfileTests.cpp#L7)

- Mapper smoke + invalid encode (non-OUT / tiny buffer)
  [`EmagicCableMapperTests.cpp:28`](../../tests/EmagicCableMapperTests.cpp#L28)
