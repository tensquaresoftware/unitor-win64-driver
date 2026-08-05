---
baseline_commit: 559cc96865b90318f5bb9df71127c482781390d7
---

# Story 1.2: Declarative MT4 DeviceProfile

Status: done

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a Bridge developer,
I want a declarative DeviceProfile for MT4 (`086A:0003`) with cable masks and interface number,
so that cable mapping and future cousin devices do not hard-code PID logic in the mapper.

## Acceptance Criteria

1. **Given** the scaffold from Story 1.1  
   **When** the MT4 DeviceProfile is loaded by the Bridge  
   **Then** it declares at least `vid=0x086A`, `pid=0x0003`, `in_cables=0x8003`, `out_cables=0x800f`, `ifnum=2`, with Patch / LTC / FastMode capability flags **off** — AD-3

2. **And** Port N is defined as the N-th present **product** cable bit in ascending order (IN Ports 1..2, OUT Ports 1..4) — see Dev Notes mask semantics (Broadcast bit excluded from Virtual Port N)

3. **And** Profile layer code does not include VirtualMIDI or WinUSB headers — AD-2  
   (`EmagicCableMapper` is **out of scope** for this story; do not create it yet)

4. **And** cousin DeviceProfile stubs may exist as data only; V1 acceptance does not require shipping working AMT8 / Unitor8 — FR-16 / CAP-15

**Traces:** FR-16, CAP-15, AD-2, AD-3

## Tasks / Subtasks

- [x] Task 1: Define `DeviceProfile` type in `src/Profile/` (AC: 1, 3)
  - [x] Add PascalCase header (and `.cpp` only if non-inline helpers need a TU) — e.g. `DeviceProfile.h`
  - [x] Fields at minimum: `vid`, `pid` (`uint16_t`), `ifnum` (`uint8_t`), `inCables` / `outCables` (`uint16_t` Linux quirk masks), capability flags (Patch / LTC / FastMode — all false/off for MT4)
  - [x] Use project naming (`lowerCamelCase` members, `k` constants) — `conventions.md` §6.3; prefer `inCables` over snake_case `in_cables` in C++ symbols while documenting Linux field names in comments
  - [x] **No** `#include` of WinUSB, `winusb.h`, VirtualMIDI, or Windows MIDI headers anywhere under `src/Profile/`
  - [x] English-only identifiers and comments

- [x] Task 2: Ship validated MT4 row + optional cousin data stubs (AC: 1, 4)
  - [x] Declarative table / constexpr array keyed by USB identity (header or `.cpp` resource — Architecture: “DeviceProfiles as declarative data”)
  - [x] **MT4 validated row (must match exactly):** VID `0x086A`, PID `0x0003`, `inCables=0x8003`, `outCables=0x800f`, `ifnum=2`, capabilities off
  - [x] Optional stubs (data only, not product-validated): AMT8 `0x086A:0x0002` masks `0x80ff` / `0x80ff` `ifnum=2`; Unitor8 `0x086A:0x0001` same masks — mark clearly as stubs
  - [x] Lookup API: find profile by `(vid, pid)` returning pointer / optional; missing PID fails closed (null / empty) — no silent fake MT4

- [x] Task 3: Port N / cable-index helpers owned by Profile (AC: 2)
  - [x] Provide a small pure helper (same layer) that, given a cable mask, yields ordered Emagic cable indices for **product Virtual Ports**
  - [x] **Binding mask semantics (prevents AD-3 divergence):** store Linux quirk literals **including** bit 15 (`0x8000`); when enumerating Port N for V1 Virtual Ports, **ignore cable index 15** (Linux names this Emagic “Broadcast”). Then ascending set bits yield MT4 IN Port 1..2 (cables 0,1) and OUT Port 1..4 (cables 0,1,2,3)
  - [x] Document in a short English comment that bit 15 remains in the stored mask for Linux fidelity / future Broadcast work — not exposed as `MT4 Port N` in V1
  - [x] Do **not** invent a second competing CableMap type in `Protocol/` this story — mapper builds on these helpers in Story 1.4

- [x] Task 4: Wire into Bridge build + prove “loaded by the Bridge” (AC: 1)
  - [x] Add Profile sources to the `Bridge` CMake target (`target_sources` / list next to `Main.cpp`)
  - [x] Keep `target_include_directories(... src)` so includes look like `#include "Profile/DeviceProfile.h"`
  - [x] From `src/App/Main.cpp`, call the lookup for MT4 (`0x086A`, `0x0003`) so the profile is referenced by the executable (not dead data). Still exit 0; no USB / MIDI I/O
  - [x] Remove `src/Profile/.gitkeep` once real sources exist

- [x] Task 5: Quality + anti-scope (AC: 3, 4)
  - [x] `python scripts/quality/lint-touched.py` exits 0 on the diff
  - [x] Compile smoke: `cmake -S . -B builds/<config> && cmake --build builds/<config>` (macOS smoke OK; Windows CI remains the gate)
  - [x] Confirm no French in sources; no WinUSB/VirtualMIDI/INF/mapper/session code introduced

### Review Findings

- [x] [Review][Patch] Centralize MT4 AD-3 literals as named Profile constants shared by the table and Main smoke [`src/Profile/DeviceProfile.h` / `src/App/Main.cpp`]
- [x] [Review][Patch] Smoke-test `collectProductCableIndices` for exact MT4 cable sequences `{0,1}` IN and `{0,1,2,3}` OUT [`src/App/Main.cpp`]

## Dev Notes

### Scope fence

This story is **data + Profile-layer API only**. After it, the Bridge compiles with a known MT4 identity table and Port N rules.  
**Do not** open WinUSB, implement `EmagicCableMapper`, create Virtual Ports, or write INF.

| In scope | Out of scope (later stories) |
|---|---|
| `DeviceProfile` type + MT4 row | WinUSB open / GUID bind → 1.3 |
| Optional AMT8/Unitor8 **data stubs** | Working cousin products → hardware-gated |
| Port N ↔ cable index helpers (excl. Broadcast) | Full Emagic multiplex → 1.4 |
| CMake + Main load smoke | VirtualMIDI / port names → 1.5 |
| lint + compile | Notes/CC I/O → 1.6 |

### Epic context

Epic 1 outcome: bind MT4 to WinUSB, run C++17 Bridge, see stable `MT4 Port N` (2 IN / 4 OUT), exchange notes/CC — without a custom kernel driver.  
Story 1.2 lands **FR-16 / CAP-15 / AD-3** so Stories 1.3–1.4 consume one declarative profile instead of scattering `0x8003` literals.

### Architecture compliance (must follow)

| Decision | What it means for this story |
|---|---|
| AD-3 | Declarative per-PID profile; MT4 masks/ifnum/capabilities as specified; Port N from ascending present bits (product ports) |
| AD-2 | `Profile/` must not include VirtualMIDI or WinUSB headers; dependency direction: mapper → profile (mapper not built yet) |
| AD-15 | PascalCase C++ under `src/`; lint-touched on touched C++; English only |
| Structural Seed | Profiles live in `src/Profile/` |
| Consistency | DeviceProfiles as declarative data — not magic numbers in App/Usb/Midi |

[Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-2, AD-3, Structural Seed, Consistency]

### CRITICAL — Cable mask / Port N semantics (read before coding)

Linux `quirks-table.h` stores MT4 as `in_cables=0x8003`, `out_cables=0x800f` (`QUIRK_MIDI_EMAGIC`, `ifnum=2`).  
Bit layout:

| Mask | Set bits (0-based cable index) | Linux ALSA ports | V1 Virtual Ports (`MT4 Port N`) |
|---|---|---|---|
| `0x8003` IN | 0, 1, **15** | 3 ports (last = Broadcast) | **Port 1..2** only (cables 0,1) |
| `0x800f` OUT | 0, 1, 2, 3, **15** | 5 ports (last = Broadcast) | **Port 1..4** only (cables 0..3) |

Linux `midi.c` labels Emagic cable/port index used as Broadcast via `EXTERNAL_PORT` (MT4 OUT sequential port 4 ↔ cable 15).  
**Product FR-4 requires 2 IN + 4 OUT** — Broadcast is **not** a V1 Virtual Port.

**Binding rules for implementers (and for Story 1.4 mapper):**

1. Store masks as Linux literals (including `0x8000`).
2. `bit i` (0-based) ↔ Emagic cable index `i` (same as Linux quirk).
3. Port N (1-based) = N-th set bit in ascending order **after excluding cable 15**.
4. Never re-interpret masks as “1-based cable IDs” or opaque blobs — that was the adversarial divergence Finding 2 failure mode.

Optional `static_assert` / constexpr test in Profile that MT4 IN count == 2 and OUT count == 4 under rule (3).

### Suggested shape (illustrative — adjust names to conventions)

```cpp
// Profile/DeviceProfile.h — no WinUSB / VirtualMIDI includes
#pragma once

#include <cstdint>
#include <optional>

struct DeviceProfile {
    uint16_t vid;
    uint16_t pid;
    uint8_t ifnum;
    uint16_t inCables;   // Linux quirk bitmask (may include 0x8000)
    uint16_t outCables;
    bool patchMode;      // off for V1 MT4
    bool ltc;
    bool fastMode;
};

// Lookup + helpers that skip cable index 15 when building Port N lists
const DeviceProfile* findDeviceProfile(uint16_t vid, uint16_t pid) noexcept;
// e.g. fill cable indices for ports 1..N into a caller buffer / small array
```

Do not vendor GPL Linux sources. Reading quirks as reference is fine; reimplement under MIT — AD-14 / NFR-Q2.

### Technical requirements

- **Language:** C++17
- **Build:** extend existing root `CMakeLists.txt` Bridge target; outputs still under `builds/`
- **Includes:** project headers via `src/` root already configured in Story 1.1
- **Style:** Allman braces, 4 spaces, `#pragma once`, English — `conventions.md` §6
- **Quality:** `scripts/quality/lint-touched.py` SSOT — do not invent a second analyser
- **No** new third-party deps

### Library / framework requirements

- **Do not** add VirtualMIDI SDK, WinUSB linkage, or test frameworks beyond compile + lint (Architecture does not mandate a unit-test harness for 1.2)
- **Do not** vendor GPL `midi.c` / `quirks-table.h`
- Cousin stubs are optional data rows — not `#ifdef` product features

### File structure requirements

#### NEW (create)

| Path | Purpose |
|---|---|
| `src/Profile/DeviceProfile.h` (required) | Type + declarations |
| `src/Profile/DeviceProfile.cpp` (if needed) | Table + lookup + Port N helpers |
| Optional tiny header for flags/constants | Only if it clarifies; avoid YAGNI sprawl |

#### UPDATE (existing)

| Path | Current state | This story |
|---|---|---|
| `CMakeLists.txt` | Bridge = `Main.cpp` only | Add Profile TU(s) |
| `src/App/Main.cpp` | `main` returns 0 | Load/lookup MT4 profile (smoke) |
| `src/Profile/.gitkeep` | Empty layer marker | Delete when real sources exist |

#### OUT OF SCOPE paths

- `src/Usb/*` WinUSB → 1.3
- `src/Protocol/*` EmagicCableMapper → 1.4
- `src/Device/*` DeviceSession → 1.4
- `src/Midi/*` VirtualMIDI → 1.5
- `installer/` INF → 1.3 / 4.1

### Existing code being modified — current state

**After Story 1.1 (done):**

- `CMakeLists.txt` — C++17 `Bridge` exe; `target_include_directories` → `src/`
- `src/App/Main.cpp` — trivial `main` return 0
- Layer dirs with `.gitkeep` only: `Device/`, `Profile/`, `Usb/`, `Protocol/`, `Midi/`
- CI: `.github/workflows/windows-build.yml` on `windows-2022`
- Quality: `scripts/quality/lint-touched.py` unchanged

**Preserve:** Structural Seed layout; `builds/` out-of-source; AD-20 user-session exe (not a Service); English/MIT orientation; no kernel driver files.

**Must not break:** Windows CI compile; lint-touched exit 0 on conforming Profile code; Main still exits cleanly.

### Testing requirements

| Check | How |
|---|---|
| Compile | `cmake -S . -B builds/<cfg> && cmake --build builds/<cfg>` |
| Profile load | Main (or linked code) resolves MT4 profile; values match AD-3 literals |
| Port counts | Helper / static_assert: MT4 → 2 IN + 4 OUT product ports (Broadcast excluded) |
| Isolation | Grep `src/Profile/`: no `WinUsb`, `winusb`, VirtualMIDI, `teVirtualMIDI` |
| Lint | `pip install -r scripts/quality/requirements.txt` then `python scripts/quality/lint-touched.py` |
| Cousins | If stubs present: data only; no claim of validated product behavior |

No Validation Matrix hosts, no hardware, no MIDI Path harness for this story.

### Previous story intelligence

From Story 1.1 (done):

- Prefer real PascalCase sources over fake APIs; empty layers were `.gitkeep` until needed — **now replace Profile `.gitkeep`**
- Include root is `src/` → `#include "Profile/DeviceProfile.h"`
- Naming conflict resolved: layer dirs stay PascalCase (`Profile/`), non-layer dirs kebab-case
- Anti-scope worked: 1.1 explicitly deferred DeviceProfile tables to 1.2 — implement them here, nowhere else
- Optional CI lint step was skipped (YAGNI); keep lint local unless you choose to add it
- Completion note: “no fake APIs” — helpers for Port N are real contract for 1.4, not decorative stubs

### Git intelligence summary

Latest implementation commit: `559cc96` — Bridge CMake scaffold + Windows CI.  
Patterns to extend: single root `CMakeLists.txt`, one `Bridge` executable, minimal App entry.  
No prior Profile code — establish declarative table pattern cleanly for cousins later.

### Latest tech information

- Linux reference (read-only): Emagic entries in `sound/usb/quirks-table.h` (`0x086a` / `0x0001..0003`) and Emagic ops in `sound/usb/midi.c` — confirm masks still `0x8003` / `0x800f` for MT4; do not copy GPL code into the repo
- USB IDs in docs stay uppercase hex (`086A:0003`); C++ literals `0x086A`, `0x0003`
- No new library versions for this story

### Project context reference

- No `project-context.md` in repo yet — use this story + `conventions.md` + Architecture Spine as SSOT
- Hardware table: PRD / brief addendum; kickoff brief `docs/dev/prompt-demarrage-projet-bmad.md`
- Quality glossary: `docs/dev/software-development-quality-principles.md`

### Anti-patterns to forbid

- Hard-coding `0x8003` / `0x800f` inside future mapper **logic** instead of reading DeviceProfile (even “temporarily”)
- Counting Port N with bit 15 included → wrong 3 IN / 5 OUT vs FR-4
- Stripping `0x8000` from stored masks without documenting Linux divergence
- Including WinUSB/VirtualMIDI headers in `Profile/`
- Implementing `EmagicCableMapper` / DeviceSession “while we’re here”
- Shipping AMT8/Unitor8 as supported products without hardware stories
- French comments; GPL source drops; kebab-case `device-profile.cpp` under `src/`
- Second quality script replacing `lint-touched.py`

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Story 1.2, Epic 1]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-2, AD-3, Structural Seed, Consistency]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/reviews/review-adversarial-divergence.md` — Finding 2 mask vs CableMap]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — FR-4, FR-16]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/addendum.md` — hardware & cable masks]
- [Source: `_bmad-output/implementation-artifacts/1-1-scaffold-bridge-project-and-windows-build-gate.md` — prior learnings]
- [Source: `conventions.md` — §3 quality gate, §4.1 DeviceProfile preference, §6 C++ standards]
- [Source: Linux `sound/usb/quirks-table.h` / `midi.c` — reference only, no vendoring]

## Dev Agent Record

### Agent Model Used

Composer (Cursor agent router)

### Debug Log References

- lint-touched initially failed on `main` CCN 13 > 10; extracted `matchesValidatedMt4Profile` helper — re-lint OK

### Completion Notes List

- Added declarative `DeviceProfile` with MT4 validated row (`0x086A:0x0003`, masks `0x8003`/`0x800f`, `ifnum=2`, capabilities off)
- Cousin AMT8 / Unitor8 rows present as data-only stubs
- Port N helpers skip Emagic Broadcast (cable 15); `static_assert` locks MT4 to 2 IN / 4 OUT product ports
- Bridge `Main` loads MT4 via `findDeviceProfile` and fails closed on unknown PID; no USB/MIDI I/O
- macOS compile smoke + `lint-touched.py` exit 0; Profile layer has no WinUSB/VirtualMIDI includes

### File List

- `src/Profile/DeviceProfile.h` (new)
- `src/Profile/DeviceProfile.cpp` (new)
- `src/Profile/.gitkeep` (deleted)
- `src/App/Main.cpp` (modified)
- `CMakeLists.txt` (modified)
- `_bmad-output/implementation-artifacts/sprint-status.yaml` (modified)
- `_bmad-output/implementation-artifacts/1-2-declarative-mt4-deviceprofile.md` (modified)

### Change Log

- 2026-08-05: Implemented declarative MT4 DeviceProfile + Port N helpers; wired into Bridge build; status → review
- 2026-08-05: Code review patches — centralized MT4 AD-3 constants; smoke-tested product cable order; status → done
