---
baseline_commit: 2ecc1e85ae189983fa0cac5c0b3b137fbd52b6a0
---

# Story 1.3: WinUSB bind path and transport open

Status: done

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a Windows MT4 user (or contributor validating on hardware),
I want the MT4 associated with WinUSB and opened by the Bridge via the project DeviceInterfaceGUID,
so that the Bridge talks to the real device without Zadig as the primary community path.

## Acceptance Criteria

1. **Given** MT4 hardware `VID 086A` / `PID 0003` and the install/bind materials in-repo (INF or co-installer scripts under `installer/`)  
   **When** the guided bind path is applied on Win10 x64 or Win11 x64  
   **Then** Device Manager shows the MT4 associated with WinUSB per install docs — FR-1 / CAP-1

2. **And** `WinUsbTransport` opens the device using DeviceInterfaceGUID `aa209017-cf8a-49ad-a0e7-701187ff7e05` (not VID/PID string match alone) — AD-12  
   **And** open path respects the MT4 profile’s `ifnum` (`2` from Story 1.2 / AD-3) so the Bridge does not attach to the wrong composite interface

3. **And** docs state Zadig as contributor fallback only, not the primary community path

4. **And** opening fails closed with a clear English diagnostic if WinUSB bind / GUID is missing (no silent “success” with empty I/O)

**Traces:** FR-1, CAP-1, AD-12 (transport + bind portion)

## Tasks / Subtasks

- [x] Task 1: Ship in-repo WinUSB bind materials under `installer/` (AC: 1, 3)
  - [x] Create `installer/` tree (kebab-case packaging scripts; INF filename free but English)
  - [x] Custom INF associating MT4 `USB\VID_086A&PID_0003` (and composite interface as required — expect `&MI_02` for `ifnum=2`) with Microsoft **WinUSB**
  - [x] Register project DeviceInterfaceGUID exactly: `{aa209017-cf8a-49ad-a0e7-701187ff7e05}` via `DeviceInterfaceGUIDs` AddReg under the `.HW` section (Microsoft WinUSB custom-INF pattern)
  - [x] Prefer modern Win10/11 package shape: `Include`/`Needs` against in-box `winusb.inf` — **do not** ship obsolete WDF/WinUSB co-installer DLLs
  - [x] Guided bind instructions (pnputil / Device Manager update-driver steps) in docs (see Task 4) — this story is **bind materials**, not the polished Public Installer UX bar (Story 4.1)
  - [x] English-only comments / catalog strings in INF where comments are used

- [x] Task 2: Implement `WinUsbTransport` in `src/Usb/` (AC: 2, 4)
  - [x] Add PascalCase sources — e.g. `WinUsbTransport.h` / `WinUsbTransport.cpp`
  - [x] Single GUID constant owned by Usb layer (must match INF byte-for-byte)
  - [x] **Primary open path:** enumerate with `SetupDiGetClassDevs` + device-interface filter on the project GUID → open device path → `WinUsb_Initialize` (and RAII close / `WinUsb_Free` on teardown)
  - [x] After init, validate / select the USB interface consistent with `DeviceProfile::ifnum` (MT4 = 2). Fail closed if the bound interface does not match the profile
  - [x] Consume `findDeviceProfile` / MT4 constants from `Profile/DeviceProfile.h` — **do not** re-hardcode VID/PID/ifnum literals in Usb beyond the GUID + INF hardware ID
  - [x] **Forbidden as default path:** VID/PID-only `SetupDi` walk without GUID (adversarial Finding 5 failure mode)
  - [x] Optional contributor escape hatch only: explicit compile flag **or** documented runtime switch (e.g. `--dev-zadig`) for Zadig-bound machines that lack the project GUID — default builds must still be GUID-first and fail closed without it
  - [x] English diagnostics on every failure path (missing device, GUID not registered, open failed, wrong ifnum, WinUSB API error codes where useful)
  - [x] RAII for OS handles; Allman / 4 spaces / `#pragma once` — `conventions.md` §6
  - [x] Keep Profile isolation: **no** WinUSB includes under `src/Profile/` (Usb → Profile is OK; reverse is not)

- [x] Task 3: Wire into Bridge build + prove open path from App (AC: 2, 4)
  - [x] Add Usb TU(s) to the `Bridge` CMake target; link Windows libs when `_WIN32` (`winusb`, `setupapi` — adjust if VS requires additional import libs)
  - [x] Dual-machine compile: macOS edit host must still configure/build. Guard Windows-only APIs with `#ifdef _WIN32` (or equivalent); non-Windows stub of `Open` fails closed with a clear “WinUSB requires Windows” diagnostic — do not fake success
  - [x] From `src/App/Main.cpp`: keep Story 1.2 profile smoke. **Recommended policy:** gate hardware open behind an explicit flag (e.g. `--open-device`). Without the flag, do not call `Open` (profile smoke only, exit 0). With the flag on Windows: call `Open`; success → exit 0; failure → print English diagnostic to stderr and exit non-zero. Never print or imply open success when `IsOpen()` is false. Document the flag in the WinUSB bind doc.
  - [x] Remove `src/Usb/.gitkeep` once real sources exist

- [x] Task 4: Contributor / bind documentation (AC: 1, 3)
  - [x] Add English docs under `docs/dev/` (and/or thin `docs/user/` seed if needed) covering: primary INF/pnputil bind path, how to verify WinUSB + GUID in Device Manager, Bridge open expectations
  - [x] Explicit Zadig section labeled **contributor fallback only** — not the primary community path (FR-1 / AD-12)
  - [x] Do **not** invent the full Story 4.1 Public Installer UX checklist UI here (progress screens, Auto-Start wiring, VirtualMIDI prerequisite MSI gate)

- [x] Task 5: Quality + anti-scope (AC: all)
  - [x] `python scripts/quality/lint-touched.py` exits 0 on the C++ diff (Usb glue paths may use the ~50-line function budget per `conventions.md` §3)
  - [x] Compile: `cmake -S . -B builds/<config> && cmake --build builds/<config>` (macOS smoke OK; Windows CI remains the gate)
  - [x] Windows CI still green (`windows-2022`) — no WDK/KMDF project, no custom kernel driver
  - [x] Confirm no French in sources; no EmagicCableMapper / DeviceSession / VirtualMIDI / notes-CC path introduced

### Review Findings

- [x] [Review][Patch] Fail closed when several present interfaces match the project WinUSB GUID + ifnum (no silent first-match) [`src/Usb/WinUsbOpenDetail.cpp:openByDeviceInterfaceGuid`] — decided: refuse if ambiguous
- [x] [Review][Patch] Zadig fallback re-opens via global GUID enum after hardware-ID match [`src/Usb/WinUsbOpenDetail.cpp:tryOpenMatchedUsbDevice`]
- [x] [Review][Patch] Dual GUID sources can drift (string constant unused; binary GUID separate) [`src/Usb/WinUsbTransport.cpp:kProjectDeviceInterfaceGuid`]
- [x] [Review][Patch] Successful Open leaves prior `errorOut` text (Zadig success path) [`src/Usb/WinUsbTransport.cpp:Open`]
- [x] [Review][Patch] Registry GUID read fragile (fixed buffer, no type check, singular parse failure blocks multi-sz, only first multi-sz entry) [`src/Usb/WinUsbOpenDetail.cpp:readDeviceInterfaceGuidFromRegistry`]
- [x] [Review][Patch] Zadig hatch may miss GUID under Device Parameters (DIREG_DEV only) [`src/Usb/WinUsbOpenDetail.cpp:readDeviceInterfaceGuidFromRegistry`]
- [x] [Review][Patch] Hardware-ID match uses substring (`wcsstr`) instead of exact multi-sz entry [`src/Usb/WinUsbOpenDetail.cpp:hardwareIdListContains`]
- [x] [Review][Patch] Default fail message always says GUID missing even when `ifnum` validation failed [`src/Usb/WinUsbTransport.cpp:Open`]

## Dev Notes

### Scope fence

This story lands **FR-1 / CAP-1 / AD-12 (bind + transport open)**. After it, a Windows machine can bind MT4 to WinUSB with the project GUID and the Bridge can open a real WinUSB handle.

| In scope | Out of scope (later stories) |
|---|---|
| `installer/` INF + guided bind scripts/docs | Polished Public Installer UX bar (WiX/Inno, Auto-Start, VirtualMIDI MSI) → **4.1** |
| `WinUsbTransport` GUID-first open + fail-closed diagnostics | Emagic multiplex / DeviceSession → **1.4** |
| Use MT4 `DeviceProfile.ifnum` | VirtualMIDI ports / names → **1.5** |
| Zadig documented as contributor fallback | Notes/CC round-trip → **1.6** |
| Dual-machine compile stubs | MIDI Path harness → Epic 5 |

**Intentional layering (readiness):** Story 1.3 = transport bind materials + open API; Story 4.1 = community installer product. Do not merge 4.1 into this ticket.

### Epic context

Epic 1 outcome: bind MT4 to WinUSB, run C++17 Bridge, see stable `MT4 Port N` (2 IN / 4 OUT), exchange notes/CC — without a custom kernel driver.  
Story 1.2 already shipped the declarative MT4 row. Story 1.3 is the first hardware I/O seam.

### Architecture compliance (must follow)

| Decision | What it means for this story |
|---|---|
| AD-12 | Custom INF → WinUSB + GUID `aa209017-cf8a-49ad-a0e7-701187ff7e05`; `WinUsbTransport` opens via that GUID; Zadig contributor-only; fail closed |
| AD-3 / Story 1.2 | Read `ifnum` (and identity) from `DeviceProfile` — MT4 `ifnum=2` |
| AD-2 | Profile must not include WinUSB headers; Usb may include Profile |
| AD-1 | Usermode WinUSB only — no custom kernel driver |
| AD-15 | PascalCase under `src/`; lint-touched; English only |
| AD-20 | Bridge remains a user-session exe (not Session-0 Service) |
| Structural Seed | `src/Usb/` = WinUsbTransport; `installer/` = INF + packaging scripts |
| Consistency | Fail closed on missing WinUSB bind; English diagnostics; VID/PID uppercase in docs (`086A:0003`) |

[Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-1, AD-2, AD-3, AD-12, AD-15, AD-20, Structural Seed, Consistency]

### CRITICAL — GUID-first open (adversarial Finding 5)

Architecture review documented a divergence trap:

- **Wrong:** enumerate `USB\VID_086A&PID_0003` and open the first WinUSB interface without the project GUID → wrong composite interface / wrong unit when two MT4s are present; Zadig machines “work” while INF machines diverge.
- **Right:** enumerate **device interfaces** for `{aa209017-cf8a-49ad-a0e7-701187ff7e05}`; only then open + `WinUsb_Initialize`; validate `ifnum` from profile.

VID/PID string match alone is **not** an acceptable primary path (story AC + AD-12).

[Source: `architecture/.../reviews/review-adversarial-divergence.md` — Finding 5]

### CRITICAL — Composite interface / hardware ID

MT4 is a composite USB device. Linux / AD-3 place Emagic MIDI on **interface 2**.  
INF hardware ID should target the MIDI interface node (commonly `USB\VID_086A&PID_0003&MI_02`), not only the parent device, unless verification on hardware shows a single non-composite binding that still exposes interface 2 correctly.  
Document the verified hardware ID string in install docs after the first successful Windows bind.

### Suggested shape (illustrative — adjust names to conventions)

```cpp
// Usb/WinUsbTransport.h — may include Profile/DeviceProfile.h; Windows headers only in .cpp or behind ifdefs
#pragma once

#include "Profile/DeviceProfile.h"

#include <string>

// Must match installer INF DeviceInterfaceGUIDs value exactly.
inline constexpr const char* kMt4WinUsbDeviceInterfaceGuid =
    "{aa209017-cf8a-49ad-a0e7-701187ff7e05}";

class WinUsbTransport
{
public:
    WinUsbTransport() = default;
    ~WinUsbTransport();

    WinUsbTransport(const WinUsbTransport&) = delete;
    WinUsbTransport& operator=(const WinUsbTransport&) = delete;

    // GUID-first open. On failure, writes English diagnostic to errorOut and returns false.
    bool Open(const DeviceProfile& profile, std::string& errorOut);
    void Close() noexcept;
    bool IsOpen() const noexcept;

private:
    // WINUSB_INTERFACE_HANDLE / HANDLE stored only in .cpp (pimpl or opaque)
};
```

Do **not** perform Emagic cable multiplex or MIDI framing here — transport open/handle ownership only. Bulk read/write helpers may be added as thin private/public methods **only if** needed to prove the handle is alive; prefer deferring sustained I/O loops to Story 1.4+.

### Technical requirements

- **Language:** C++17
- **Build:** extend root `CMakeLists.txt`; outputs under `builds/`
- **Windows APIs:** WinUSB (`winusb.h` / `winusb.dll`), SetupAPI for device-interface enumeration
- **INF:** Microsoft custom WinUSB INF pattern (Include/Needs `WINUSB.SYS` / `winusb.inf`); no co-installer DLLs on Win10+
- **Style:** Allman, 4 spaces, English — `conventions.md` §6
- **Quality:** `scripts/quality/lint-touched.py` SSOT; Usb paths use glue-path line budget (~50)
- **No** new third-party deps; **no** VirtualMIDI SDK yet

### Library / framework requirements

| Use | Do not use |
|---|---|
| System WinUSB + SetupAPI on Windows | Custom KMDF/WDM driver project |
| In-box `winusb.inf` Include/Needs | WdfCoInstaller / WinUsbCoInstaller redistributables |
| Existing `DeviceProfile` lookup | Hard-coded second copy of MT4 ifnum/masks in Usb |
| Optional Zadig **docs** + guarded escape hatch | Zadig as documented primary community path |

### File structure requirements

#### NEW (create)

| Path | Purpose |
|---|---|
| `src/Usb/WinUsbTransport.h` | Transport API + GUID constant |
| `src/Usb/WinUsbTransport.cpp` | GUID enumeration + WinUSB open/close (Windows); stub otherwise |
| `installer/*.inf` (+ optional `.cat` later / scripts) | WinUSB association + DeviceInterfaceGUIDs |
| `installer/` helper script(s) optional | e.g. `pnputil` guided bind — kebab-case names |
| `docs/dev/winusb-bind.md` (name free, kebab-case) | Primary bind path + Zadig fallback + verify steps |

#### UPDATE (existing)

| Path | Current state | This story |
|---|---|---|
| `CMakeLists.txt` | Bridge = Main + DeviceProfile | Add Usb TU(s); Windows link libs |
| `src/App/Main.cpp` | Profile smoke only | Wire transport open policy (see Task 3) |
| `src/Usb/.gitkeep` | Empty layer marker | Delete when real sources exist |

#### OUT OF SCOPE paths

- `src/Protocol/*` EmagicCableMapper → 1.4
- `src/Device/*` DeviceSession → 1.4
- `src/Midi/*` VirtualMIDI → 1.5
- Full Public Installer UX / Auto-Start / VirtualMIDI prerequisite product → 4.1
- Authenticode / catalog signing policy → 4.4 (unsigned INF OK for contributor bind; document if test-signing / pnputil force needed)

### Existing code being modified — current state

**After Stories 1.1–1.2 (done):**

- `CMakeLists.txt` — C++17 `Bridge` exe; includes `src/`; sources: `Main.cpp`, `DeviceProfile.cpp`
- `src/App/Main.cpp` — loads MT4 via `findDeviceProfile`, validates AD-3 fields + product cable order `{0,1}` / `{0,1,2,3}`, fails closed on unknown PID; exit 0; **no USB I/O**
- `src/Profile/DeviceProfile.h/.cpp` — MT4 validated row + cousin stubs; `kMt4InterfaceNumber = 2`; Port N helpers skip Broadcast cable 15; **no WinUSB includes**
- `src/Usb/.gitkeep` only — no transport yet
- `installer/` — **does not exist** (create this story)
- CI: `.github/workflows/windows-build.yml` on `windows-2022`

**Preserve:**

- Profile layer purity (AD-2)
- MT4 constants / lookup API — Usb consumes them, does not fork them
- User-session Bridge exe (AD-20)
- `builds/` out-of-source; English/MIT orientation; no kernel driver files
- Story 1.2 Main profile assertions (do not regress)

**Must not break:**

- Windows CI compile
- macOS configure/build smoke without WinUSB
- `lint-touched.py` exit 0 on conforming Usb/App code
- Profile smoke behavior for known/unknown PID

### Testing requirements

| Check | How |
|---|---|
| Compile (macOS) | `cmake -S . -B builds/<cfg> && cmake --build builds/<cfg>` — stub path OK |
| Compile (Windows CI) | Existing workflow green with Usb sources linked |
| INF GUID | INF `DeviceInterfaceGUIDs` value matches `kMt4WinUsbDeviceInterfaceGuid` exactly |
| Open path | On bound hardware: `Open(mt4Profile)` succeeds; `IsOpen()==true` |
| Fail closed | Without bind / without device: Open returns false + English diagnostic; no success claim |
| GUID-first | Code review / grep: primary enumeration uses interface GUID, not VID/PID-only walk |
| Isolation | Grep `src/Profile/`: still no `WinUsb` / `winusb` / SetupAPI includes |
| Lint | `pip install -r scripts/quality/requirements.txt` then `python scripts/quality/lint-touched.py` |
| Docs | Primary path = INF; Zadig labeled contributor fallback |

Hardware AC (Device Manager shows WinUSB) is validated on the Windows box — macOS cannot substitute that check.

No Validation Matrix DAW hosts, VirtualMIDI, or MIDI Path harness for this story.

### Previous story intelligence

From Story 1.2 (done):

- Prefer real PascalCase sources over decorative stubs; replace `src/Usb/.gitkeep`
- Include root is `src/` → `#include "Usb/WinUsbTransport.h"` / `"Profile/DeviceProfile.h"`
- Centralize literals as named constants (review lesson) — do the same for the DeviceInterfaceGUID
- Fail-closed patterns already used in Profile lookup / Main unknown-PID check — mirror for missing WinUSB bind
- lint CCN budget: keep `Open` / enumeration helpers split so CCN stays ≤10 where possible; Usb glue may use ~50-line function budget
- Anti-scope worked: 1.2 deferred WinUSB entirely — implement it **here**, not inside Profile

From Story 1.1 (done):

- Dual-machine loop: edit on macOS, validate USB on Windows
- Empty layers stay `.gitkeep` until needed — Usb is now needed
- Do not invent a second quality analyser

### Git intelligence summary

Latest implementation commits:

- `2ecc1e8` — Declarative MT4 DeviceProfile + Port N helpers (Profile + Main smoke)
- `559cc96` — Bridge CMake scaffold + Windows CI

Patterns to extend: single root `CMakeLists.txt`, one `Bridge` executable, minimal App entry, layer folders under `src/`.  
No prior Usb or `installer/` code — establish GUID-first transport + INF cleanly for 1.4+.

### Latest tech information

- Microsoft Learn: [WinUSB installation for developers](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-installation) — custom INF with `[DDInstall.HW]` + `DeviceInterfaceGUIDs` REG_MULTI_SZ; apps enumerate via `SetupDiGetClassDevs` then `WinUsb_Initialize`
- Win10/11: remove WDF/WinUSB **co-installer** references — use `Include`/`Needs` against in-box WinUSB (WDK 11 22H2+ no longer ships those co-installers)
- Firmware OS-descriptor auto-WinUSB is **not** assumed (AD-12) — custom INF remains required
- Zadig still valid as contributor fallback (v2.x) — document only, do not make it primary
- Project GUID is product-fixed: `aa209017-cf8a-49ad-a0e7-701187ff7e05` (do not generate a new one)

### Project context reference

- No `project-context.md` in repo yet — use this story + `conventions.md` + Architecture Spine as SSOT
- Kickoff brief: `docs/dev/prompt-demarrage-projet-bmad.md` (WinUSB + optional INF; Zadig historically for personal/dev)
- Quality glossary: `docs/dev/software-development-quality-principles.md`
- Readiness note: dual WinUSB paths 1.3 vs 4.1 are intentional layering

### Anti-patterns to forbid

- Opening by VID/PID string match alone as the default path
- Ignoring `DeviceProfile.ifnum` and binding the wrong composite interface
- Putting WinUSB / SetupAPI includes in `src/Profile/`
- Claiming open success when no handle / no pipes are available
- Shipping WdfCoInstaller010xx / WinUsbCoInstaller DLLs “because old tutorials did”
- Implementing EmagicCableMapper / DeviceSession / VirtualMIDI “while we’re here”
- Making Zadig the documented primary community install path
- Building the full Story 4.1 Public Installer UX in this ticket
- French comments; GPL Linux source drops; kebab-case `win-usb-transport.cpp` under `src/`
- Second quality script replacing `lint-touched.py`
- Custom kernel driver / WDK KMDF project

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Story 1.3, Epic 1]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-12, Structural Seed, Consistency, Stack]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/reviews/review-adversarial-divergence.md` — Finding 5 GUID vs VID/PID]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — FR-1]
- [Source: `_bmad-output/planning-artifacts/implementation-readiness-report-2026-08-04.md` — FR-1 → 1.3; dual path vs 4.1]
- [Source: `_bmad-output/implementation-artifacts/1-2-declarative-mt4-deviceprofile.md` — prior Profile API + deferred Usb]
- [Source: `_bmad-output/implementation-artifacts/1-1-scaffold-bridge-project-and-windows-build-gate.md` — scaffold / CI patterns]
- [Source: `conventions.md` — §3 quality gate, §6 C++ standards, Usb glue line budget]
- [Source: Microsoft Learn WinUSB installation — custom INF + DeviceInterfaceGUIDs]

## Dev Agent Record

### Agent Model Used

Composer (Cursor agent router)

### Debug Log References

- macOS: `cmake -S . -B builds/debug && cmake --build builds/debug` — OK
- `Bridge` (no flags) exit 0; `Bridge --open-device` fail-closed with `WinUSB requires Windows`
- `python scripts/quality/lint-touched.py` exit 0 after splitting WinUSB helpers
- Profile isolation grep: no WinUSB/SetupAPI under `src/Profile/`

### Completion Notes List

- Shipped `installer/mt4-winusb.inf` (MI_02 + project DeviceInterfaceGUID, in-box winusb.inf Include/Needs) and `installer/bind-mt4-winusb.ps1`.
- Implemented GUID-first `WinUsbTransport::Open` with ifnum validation from `DeviceProfile`; `--dev-zadig` optional fallback only.
- Non-Windows stub fails closed; App gates hardware open behind `--open-device`.
- Docs: `docs/dev/winusb-bind.md` — INF primary path; Zadig labeled contributor fallback only.
- Lint-touched OK; no Protocol/Device/Midi scope creep.

### File List

- CMakeLists.txt
- docs/dev/winusb-bind.md
- installer/bind-mt4-winusb.ps1
- installer/mt4-winusb.inf
- src/App/Main.cpp
- src/Usb/.gitkeep (deleted)
- src/Usb/WinUsbOpenDetail.cpp
- src/Usb/WinUsbOpenDetail.h
- src/Usb/WinUsbOpenSupport.cpp
- src/Usb/WinUsbOpenSupport.h
- src/Usb/WinUsbTransport.cpp
- src/Usb/WinUsbTransport.h
- _bmad-output/implementation-artifacts/1-3-winusb-bind-path-and-transport-open.md
- _bmad-output/implementation-artifacts/sprint-status.yaml

### Change Log

- 2026-08-05: Implemented WinUSB INF bind materials, GUID-first transport open, Bridge `--open-device` wiring, and bind docs; status → review.
- 2026-08-05: Code review patches — fail-closed on ambiguous GUID matches, Zadig HWID-scoped open, single GUID source, registry robustness, clearer diagnostics; status → done.
