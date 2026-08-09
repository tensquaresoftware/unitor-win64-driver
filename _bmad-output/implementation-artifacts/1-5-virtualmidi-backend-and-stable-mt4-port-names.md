---
baseline_commit: 5e13e4f
---

# Story 1.5: VirtualMIDI backend and stable MT4 Port names

Status: done

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a DAW user,
I want the Bridge to create 2 IN + 4 OUT Virtual Ports named `MT4 Port N` via the VirtualMIDI SDK,
so that Ableton Live 12 / Reason / MIDI-OX / Matrix-Control can select physical-shaped endpoints.

## Acceptance Criteria

1. **Given** a live DeviceSession (Story 1.4) and the VirtualMIDI driver present on the machine  
   **When** the session starts  
   **Then** Virtual Ports are created programmatically via the VirtualMIDI SDK (not manual loopMIDI-only workflow) — FR-2 / AD-7

2. **And** exactly 2 input and 4 output endpoints appear per MT4 — FR-4 / CAP-4

3. **And** unit-1 display names are exactly `MT4 Port 1` … `MT4 Port 4` (same label on IN and OUT sides for cable N) — FR-5 / AD-5

4. **And** ordinal `K` / names are supplied by `DeviceSessionManager` into `MidiBackend`; the backend does not invent its own ordinal — AD-5 / AD-6

5. **And** only a live DeviceSession creates/destroys that unit’s ports through `MidiBackend` APIs — AD-9

6. **And** if VirtualMIDI is missing, the Bridge fails closed with an obvious fix path (no empty port list presented as success) — AD-12

7. **And** `MidiBackend` is an abstract seam; `VirtualMidiBackend` is the only V1 implementation (future Windows MIDI Services not shipped) — AD-2 / NFR-Q3

8. **And** `aaron1a12/virtual-midi` is not forked as project base (integration proof only) — CAP-14 / AD-7

**Traces:** FR-2, FR-4, FR-5, CAP-2, CAP-4, CAP-5, AD-5, AD-6, AD-7, AD-9  
*(Also cited in AC: AD-12 fail-closed, AD-2 / NFR-Q3 seam, CAP-14 no fork)*

## Tasks / Subtasks

- [x] Task 1: Define `MidiBackend` abstract seam in `src/Midi/` (AC: 5, 7)
  - [x] Add `MidiBackend.h` — abstract interface only; **no** VirtualMIDI / WinUSB / Protocol includes
  - [x] Normative behavioral API (names illustrative — keep Allman / `bool` + `errorOut`):
    - `CreatePortSet(const PortNameSet& names, std::string& errorOut)` — create the unit’s Virtual Port set
    - `DestroyPortSet() noexcept` — destroy that set (idempotent if never created / already destroyed)
    - Optional seam methods for Story **1.6** (declare now, may stub): e.g. `SendToHost` / host→device callback registration — **do not** implement notes/CC round-trip proof here
  - [x] Define a small `PortNameSet` (or equivalent) that carries **ready-made** display strings + direction counts — **not** USB serial / topology / ordinal inventing
  - [x] English comments only; document that backends must not re-derive unit ordinal `K`

- [x] Task 2: Implement `VirtualMidiBackend` (AC: 1, 2, 6, 7, 8)
  - [x] Add `VirtualMidiBackend.h` / `VirtualMidiBackend.cpp` under `src/Midi/`
  - [x] Create **6** directional endpoints matching FR-4 / AD-5:
    - IN: `MT4 Port 1`, `MT4 Port 2` (product IN cables 0,1)
    - OUT: `MT4 Port 1` … `MT4 Port 4` (product OUT cables 0..3)
  - [x] Prefer **runtime** load of `teVirtualMIDI.dll` (`LoadLibrary` / `GetProcAddress`) so the repo does **not** need vendored proprietary `.lib` / SDK drop for CI and macOS smoke — fail closed with English fix path if DLL/driver missing (`ERROR_PATH_NOT_FOUND`-class → “install loopMIDI or rtpMIDI so the VirtualMIDI driver is present”)
  - [x] Do **not** commit Tobias proprietary SDK binaries or GPL trees; do **not** clone/fork `aaron1a12/virtual-midi` into this repo
  - [x] Re-declare the minimal C API surface needed (create/close + flags) in a thin project-owned header under `src/Midi/` if required — do not paste large third-party samples
  - [x] Use SDK flags so IN-only vs OUT-only ports match product topology (e.g. instantiate TX for Port 1–2 product IN / app-visible MIDI IN, RX for Port 1–4 product OUT / app-visible MIDI OUT; avoid inventing IN endpoints for Port 3/4)
  - [x] No exclusive-open / single-client policy layered on top (AD-8 design — full multi-client proof is Epic 3 / Story 3.3)
  - [x] Non-Windows / missing-driver paths: `CreatePortSet` returns false + clear English `errorOut`; never claim success with zero ports

- [x] Task 3: Add minimal `DeviceSessionManager` naming authority (AC: 3, 4)
  - [x] Add `DeviceSessionManager.h` / `.cpp` under `src/Device/`
  - [x] Sole owner of unit ordinal `K` for naming (AD-6) — **this story** may hard-wire single-unit `K = 1` (multi-unit serial/topology persistence = Epic 3 / Story 3.4 / AQ-1)
  - [x] Provide pure helper e.g. `formatPortDisplayName(unsigned unitOrdinalK, unsigned portN)` producing exact AD-5 strings:
    - `K == 1` → `MT4 Port N`
    - `K >= 2` → `MT4 #K Port N` (implement now so MidiBackend never formats; Epic 3 verifies multi-unit)
  - [x] Build a `PortNameSet` for one MT4 from Profile product port counts (2 IN / 4 OUT) using ready-made strings — **do not** ask MidiBackend to invent names
  - [x] App (or manager) wires `VirtualMidiBackend` as the V1 concrete `MidiBackend` at composition root only

- [x] Task 4: Extend `DeviceSession` lifecycle to own ports via `MidiBackend` (AC: 1, 5, 6)
  - [x] UPDATE `DeviceSession` — accept a `MidiBackend&` (or non-owning pointer) + ready-made `PortNameSet` on `Start` (exact signature free; preserve fail-closed English diagnostics)
  - [x] Normative sequence (AD-9): open WinUSB → Emagic init magics → **`MidiBackend::CreatePortSet`** → mark running. If port create fails → teardown transport + fail closed (no “session up, empty ports” success)
  - [x] `Stop` / destructor: **`DestroyPortSet`** then Emagic finish + transport close (destroy ports — do **not** “mark unavailable while keeping ports alive”)
  - [x] **Only** `DeviceSession` calls create/destroy — App must not create ports before/outside session; no hot-plug watcher creating ports (AD-9)
  - [x] Preserve Story 1.4 behaviors: one session per MT4, init×2 / finish magics, mapper ownership, AD-20 user-session exe
  - [x] Do **not** implement continuous USB↔MIDI byte pump or DAW notes/CC proof (Story **1.6**)

- [x] Task 5: Wire Bridge build + App proof points (AC: all)
  - [x] Add Midi (+ DeviceSessionManager) TUs to the `Bridge` CMake target; remove `src/Midi/.gitkeep` when real sources exist
  - [x] Keep Story 1.2–1.4 flags working: profile smoke, `--test-mapper`, `--open-device`, `--dev-zadig`, `--start-session`
  - [x] Extend `--start-session` (or add `--start-session-ports`): on Windows with driver present, Start creates ports then Stop destroys them; print English diagnostics listing expected names `MT4 Port 1`…`4` and IN/OUT counts; fail closed if VirtualMIDI missing
  - [x] Add `--test-port-names` (synthetic, no hardware / no DLL required): assert `formatPortDisplayName` for `K=1` and at least one `K=2` sample (`MT4 #2 Port 3`); exit `0`/`1`
  - [x] Dual-machine: macOS stubs fail closed for CreatePortSet with English message (same pattern as WinUSB)

- [x] Task 6: Quality + anti-scope (AC: all)
  - [x] `python scripts/quality/lint-touched.py` exits 0 on the C++ diff
  - [x] Compile: `cmake -S . -B builds/<config> && cmake --build builds/<config>` (macOS smoke OK; Windows CI remains the gate)
  - [x] Grep isolation: no `teVirtualMIDI` / `VirtualMIDI` / `winusb` under `src/Protocol/` or `src/Profile/`
  - [x] Confirm no French in sources; no GPL `aaron1a12` tree; no MSI redistributable / Public Installer work; no notes/CC Validation Matrix claim; no Windows MIDI Services backend

### Review Findings

- [x] [Review][Decision] Confirm VirtualMIDI TX/RX mapping vs DAW-visible IN/OUT — resolved: keep code mapping (IN=`INSTANTIATE_TX`, OUT=`INSTANTIATE_RX`); Task 2 example aligned; still confirm live 2+4 on Windows later
- [x] [Review][Patch] Restrict `teVirtualMIDI.dll` load to system directories [`src/Midi/VirtualMidiBackend.cpp:122`]
- [x] [Review][Patch] Validate `PortNameSet` counts against profile cable masks in `DeviceSession::Start` [`src/Device/DeviceSession.cpp:41`]
- [x] [Review][Patch] Reject empty/whitespace port display names in `validatePortNameSet` [`src/Midi/VirtualMidiBackend.cpp:93`]
- [x] [Review][Patch] Assert OUT Port 2 and Port 3 names in `testBuiltMt4PortNameSet` [`src/App/Main.cpp:327`]
- [x] [Review][Defer] No recovery for stale Virtual Ports left after a crashed Bridge [`src/Midi/VirtualMidiBackend.cpp:246`] — deferred, pre-existing
- [x] [Review][Defer] Same AD-5 display names on IN and OUT need Windows collision proof with teVirtualMIDI [`src/Midi/VirtualMidiBackend.cpp:255`] — deferred, pre-existing
- [x] [Review][Defer] `--start-session` prints expected names but does not enumerate live Windows MIDI endpoints [`src/App/Main.cpp:358`] — deferred, pre-existing
- [x] [Review][Defer] Hand-rolled teVirtualMIDI flag constants unpinned to an SDK version/checksum [`src/Midi/TeVirtualMidiApi.h:14`] — deferred, pre-existing
- [x] [Review][Defer] Shared `MidiBackend` across concurrent `DeviceSession` instances is not rejected [`src/Device/DeviceSession.cpp:51`] — deferred, pre-existing

## Dev Notes

### Scope fence

This story lands **FR-2 / CAP-2 (VirtualMIDI port lifecycle portion)**, **FR-4 / CAP-4**, and **FR-5 / CAP-5 (unit-1 naming)**. After it, a live session creates stable `MT4 Port N` endpoints — still without proving notes/CC through the DAW.

| In scope | Out of scope (later stories) |
|---|---|
| `MidiBackend` + `VirtualMidiBackend` | Notes/CC host round-trip on all ports → **1.6** |
| Programmatic create/destroy of 2 IN + 4 OUT | Continuous USB↔MIDI I/O pump / reader thread → **1.6** (may declare seam APIs now) |
| Unit-1 names `MT4 Port 1`…`4` via `DeviceSessionManager` | Multi-unit `MT4 #K` proof + serial/topology registry → **3.4** / AQ-1 |
| Fail closed if VirtualMIDI driver/DLL missing | Public Installer VirtualMIDI UX / MSI embed → **4.1** / OQ-1 |
| AD-9 session-owned lifecycle | Hot-plug product recovery → **3.2** |
| AD-8 “no exclusive lock” design note | Concurrent DAW + MIDI-OX acceptance → **3.3** |
| | Clock / MTC / SysEx product paths → Epic **2** |
| | Windows MIDI Services backend → post-V1 |
| | Session-0 Windows Service → forbidden forever (AD-20) |

### Epic context

Epic 1 outcome: bind MT4 to WinUSB, run C++17 Bridge, see stable `MT4 Port N` (2 IN / 4 OUT), exchange notes/CC — without a custom kernel driver.  
Stories 1.2–1.4 shipped profile, GUID-first WinUSB open, and DeviceSession + Emagic mapper. Story 1.5 attaches Virtual Ports. Story 1.6 proves MIDI bytes through those ports.

### Architecture compliance (must follow)

| Decision | What it means for this story |
|---|---|
| AD-2 | `EmagicCableMapper` / `DeviceProfile` stay free of VirtualMIDI headers; Midi types stay in `Midi/`; composition root wires `VirtualMidiBackend` |
| AD-3 | Port N from Profile helpers — IN 1..2 / OUT 1..4; Broadcast cable 15 never a Virtual Port |
| AD-4 | One session / one Virtual Port set per MT4 |
| AD-5 | Exact spelling; IN and OUT share label for cable N; names ready-made from Device → Midi |
| AD-6 | `DeviceSessionManager` sole owner of `K` — MidiBackend must not invent ordinal from USB descriptors |
| AD-7 | VirtualMIDI SDK = V1 backend; driver present (eval: loopMIDI/rtpMIDI); no GPL fork base |
| AD-8 | Do not add exclusive-open policy (full concurrent-host proof later) |
| AD-9 | Only live `DeviceSession` create/destroy via `MidiBackend`; destroy on teardown (no keep-alive unavailable mode) |
| AD-12 | Fail closed + obvious fix path when VirtualMIDI missing (Bridge-side; installer UX is 4.1) |
| AD-15 | PascalCase under `src/`; lint-touched; English only |
| AD-20 | Remain user-session Bridge exe |
| Structural Seed | `src/Midi/` = MidiBackend + VirtualMidiBackend; `Device/` = DeviceSession + DeviceSessionManager |
| Pipeline | `WinUsbTransport` ↔ `EmagicCableMapper` ↔ `MidiBackend` (V1: VirtualMIDI) |

[Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-2..9, AD-12, AD-15, AD-20, Structural Seed]

### CRITICAL — Port naming (do not invent alternate schemes)

| Unit ordinal K | Display pattern | Example |
|---|---|---|
| 1 | `MT4 Port N` | `MT4 Port 1` … `MT4 Port 4` |
| ≥ 2 | `MT4 #K Port N` | `MT4 #2 Port 1` … `MT4 #2 Port 4` |

- Same label on IN and OUT for cable `N` (Windows shows separate selectable endpoints).
- There is **no** `MT4 Port 3` IN — only Port 1..2 IN and Port 1..4 OUT.
- `N` = product Port N from Story 1.2 (`collectProductCableIndices`) — ascending set bits excluding Broadcast 15.
- MidiBackend receives **strings**, not “please format from serial”.

[Source: ARCHITECTURE-SPINE AD-5 / AD-6; epics Story 1.5; adversarial Finding 1 + Finding 6]

### CRITICAL — Call graph (AD-9)

```
App → DeviceSessionManager (assign K, build PortNameSet)
    → DeviceSession::Start(profile, midiBackend, names)
        → WinUsbTransport::Open
        → Emagic init magics
        → MidiBackend::CreatePortSet(names)
    → DeviceSession::Stop
        → MidiBackend::DestroyPortSet
        → Emagic finish + Close
```

Forbidden: App/hot-plug creating ports before a session exists; MidiBackend assigning `K`; dual create/destroy owners.

### CRITICAL — VirtualMIDI integration (eval path)

| Topic | Rule |
|---|---|
| Product path | Bridge creates/destroys ports via SDK API — not “user only clicks loopMIDI UI” |
| Driver prerequisite | loopMIDI or rtpMIDI preinstalled for **dev/eval** (driver + `teVirtualMIDI.dll`) |
| MSI redistributable | **Out of scope** — OQ-1 / Story 4.1 release gate only |
| Recommended link strategy | Runtime `LoadLibrary("teVirtualMIDI.dll")` + `GetProcAddress` — avoids committing proprietary `.lib`; CI/macOS still compile |
| API surface (illustrative) | `virtualMIDICreatePortEx2` / `virtualMIDIClosePort` (+ later `GetData`/`SendData` for 1.6) |
| Fail closed | Missing DLL/driver → English message with fix path; exit non-zero; never empty success |
| AQ-3 | Pin exact SDK version when Tobias/eval freeze allows — document observed DLL behavior in `docs/dev/` if useful; do not block coding on MSI reply |
| AQ-4 | Win11 + Windows MIDI Services may affect dynamic ports — validate on Windows soak; document quirks; do not fork lifecycle policy |

Public eval docs: https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html  
Driver presence via loopMIDI/rtpMIDI — SDK itself is not freeware; do not redistribute linking apps without clearance (OQ-1).

### CRITICAL — Layer boundaries (anti-reinvention)

| Layer | Owns | Must not |
|---|---|---|
| Profile | Masks, Port N cable helpers | VirtualMIDI, display-name formatting, session lifecycle |
| Protocol | F5 encode/decode | VirtualMIDI headers, port create |
| Usb | GUID open + bulk | Port names, VirtualMIDI |
| Device | Session + **DeviceSessionManager** naming/`K` | Proprietary SDK includes (keep SDK in Midi) |
| Midi | `MidiBackend` + `VirtualMidiBackend` | Inventing `K`, Emagic F5, WinUSB open |
| App | Flags / composition root wiring | Second naming scheme or pre-session port create |

### Suggested shapes (illustrative — adjust to conventions)

```cpp
// Midi/MidiBackend.h — no VirtualMIDI includes
#pragma once
#include <string>

struct PortNameSet
{
    // Ready-made AD-5 strings. Example sizes for MT4: 2 IN + 4 OUT.
    // Prefer fixed arrays + counts over inventing ordinals here.
    const wchar_t* const* inNames = nullptr;   // or UTF-8 + convert at VirtualMIDI boundary
    std::size_t inCount = 0;
    const wchar_t* const* outNames = nullptr;
    std::size_t outCount = 0;
};

class MidiBackend
{
public:
    virtual ~MidiBackend() = default;
    virtual bool CreatePortSet(const PortNameSet& names, std::string& errorOut) = 0;
    virtual void DestroyPortSet() noexcept = 0;
};
```

```cpp
// Device/DeviceSessionManager — naming authority (K=1 for this story)
std::string formatPortDisplayName(unsigned unitOrdinalK, unsigned portN);
// "MT4 Port 1" / "MT4 #2 Port 3" — single formatting SSOT
```

```cpp
// DeviceSession::Start — after init magic success:
if (!midiBackend_.CreatePortSet(names, errorOut))
{
    Stop(); // must DestroyPortSet-safe + close transport
    return false;
}
```

### Technical requirements

- **Language:** C++17
- **Build:** extend root `CMakeLists.txt`; outputs under `builds/`
- **Windows APIs:** `LoadLibrary` / `GetProcAddress` / `FreeLibrary` for VirtualMIDI DLL behind Midi layer only
- **Style:** Allman, 4 spaces, `#pragma once`, English — `conventions.md` §6
- **Quality:** `scripts/quality/lint-touched.py` SSOT
- **Encoding:** VirtualMIDI port names are wide strings (`LPCWSTR`) — convert from UTF-8/`std::string` at the VirtualMidiBackend boundary if Device stores UTF-8
- **No** new third-party deps in-repo (no Catch2; no vendored GPL; no committed proprietary SDK drop required)

### Library / framework requirements

| Use | Do not use |
|---|---|
| Abstract `MidiBackend` + only `VirtualMidiBackend` in V1 | Windows MIDI Services as V1 backend |
| Runtime teVirtualMIDI.dll (loopMIDI/rtpMIDI present) | Fork/copy `aaron1a12/virtual-midi` |
| `DeviceSessionManager` for `K` + display strings | MidiBackend formatting `MT4 #K` from USB serial |
| Story 1.4 `DeviceSession` Start/Stop extended | App creating ports outside session |
| `--test-port-names` + `--start-session` diagnostics | Claiming Validation Matrix notes/CC proof (1.6) |
| Fail-closed English fix path | Empty port list as success |

### File structure requirements

#### NEW (create)

| Path | Purpose |
|---|---|
| `src/Midi/MidiBackend.h` | Abstract seam |
| `src/Midi/VirtualMidiBackend.h` | V1 concrete backend |
| `src/Midi/VirtualMidiBackend.cpp` | SDK/DLL create/destroy |
| `src/Midi/TeVirtualMidiApi.h` (optional) | Minimal project-owned API decls for dynamic load |
| `src/Device/DeviceSessionManager.h` | `K` + `formatPortDisplayName` + start wiring helper |
| `src/Device/DeviceSessionManager.cpp` | Implementation (unit-1 sufficient) |

#### UPDATE (existing)

| Path | Current state | This story |
|---|---|---|
| `CMakeLists.txt` | Bridge = App + DeviceSession + Profile + Protocol + Usb | Add Midi + DeviceSessionManager TUs |
| `src/Device/DeviceSession.h/.cpp` | Start/Stop USB+mapper only; seam comment for MidiBackend | Own CreatePortSet/DestroyPortSet via `MidiBackend` |
| `src/App/Main.cpp` | `--test-mapper`, `--start-session`, `--open-device` | Wire manager + backend; port-name test; session creates ports |
| `src/Midi/.gitkeep` | Empty marker | Delete when real sources exist |

#### OUT OF SCOPE paths

- Notes/CC Validation Matrix / I/O pump → **1.6**
- Hot-plug / Auto-Start product → **3.1** / **3.2**
- Multi-MT4 ordinal persistence → **3.4**
- Public Installer / MSI → **4.1**
- `tools/midi-path-harness/` → Epic **5**

### Existing code being modified — current state

**After Stories 1.1–1.4 (done):**

- `CMakeLists.txt` — C++17 `Bridge`; sources: Main, DeviceSession, DeviceProfile, EmagicCableMapper, WinUsbTransport (+ Win32 open helpers); links `setupapi`/`winusb`/`ole32` on WIN32
- `src/Device/DeviceSession.*` — Start opens transport, builds mapper, sends init×2; Stop finish+close; **no** MidiBackend yet; header seam comment points here
- `src/Protocol/EmagicCableMapper.*` — F5 encode/decode; `MidiCableSink` callback ready for 1.5/1.6 feed; **no** VirtualMIDI includes
- `src/Profile/DeviceProfile.*` — MT4 masks + `collectProductCableIndices` (IN `{0,1}`, OUT `{0,1,2,3}`); no display-name helper yet
- `src/App/Main.cpp` — profile smoke, `--test-mapper`, `--open-device`, `--dev-zadig`, `--start-session`
- `src/Midi/.gitkeep` — empty (Structural Seed placeholder)
- No `DeviceSessionManager` yet (explicitly deferred in Story 1.4)

**What this story changes:**

- Introduce Midi seam + VirtualMIDI create/destroy
- Introduce minimal DeviceSessionManager as naming/`K` owner
- DeviceSession Start/Stop gain port lifecycle through MidiBackend

**What must be preserved:**

- Profile / Protocol isolation (AD-2)
- GUID-first open, bulk I/O, mapper F5 behavior, init/finish magics
- Fail-closed English diagnostics; clear `errorOut` on success
- User-session exe (AD-20); `builds/` out-of-source; no kernel driver
- Prior CLI flags still work

**Must not break:**

- Windows CI compile (even without VirtualMIDI driver on the runner — runtime fail-closed is OK; link/load must not require proprietary files in-repo)
- macOS configure/build smoke
- `Bridge --test-mapper` and profile smoke
- `--open-device` without forcing VirtualMIDI when only transport open is requested

### Testing requirements

| Check | How |
|---|---|
| Compile (macOS) | `cmake -S . -B builds/<cfg> && cmake --build builds/<cfg>` — VirtualMIDI stubs OK |
| Compile (Windows CI) | Existing workflow green; no proprietary SDK committed |
| Name formatting | `Bridge --test-port-names` — `MT4 Port 1`…`4` for K=1; `MT4 #2 Port 3` sample for K=2 |
| Port create (hardware + driver) | Windows: loopMIDI/rtpMIDI installed + WinUSB-bound MT4 → `--start-session` creates 2 IN + 4 OUT named exactly; Stop destroys; English diagnostics |
| Fail closed | Windows without VirtualMIDI DLL/driver: Start fails with fix-path message; exit non-zero; no success claim |
| Isolation | Grep `src/Protocol/` + `src/Profile/`: no VirtualMIDI / teVirtualMIDI / WinUSB |
| Lint | `python scripts/quality/lint-touched.py` on touched C++ |
| Anti-fork | Repo has no `aaron1a12/virtual-midi` subtree / vendored GPL MIDI samples |

No Validation Matrix DAW notes/CC proof and no MIDI Path harness for this story.

### Previous story intelligence

From Story **1.4** (done):

- Explicit seam: “Virtual ports / MidiBackend arrive in Story 1.5 — only a live session will own that lifecycle”
- `DeviceSessionManager` / multi-unit `K` were deferred — **introduce minimal manager here** for AD-5/AD-6 AC
- Mapper exposes `MidiCableSink` for later MidiBackend feed — wire byte path in **1.6**, not as AC here
- Review deferred: bulk timeouts + Close vs in-flight I/O — still deferred until continuous pump (1.6+)
- Keep Main helpers small for lint CCN when adding flags

From Story **1.3** (done):

- Dual-machine `#ifdef _WIN32` stubs; fail-closed English — mirror for VirtualMIDI missing
- Hardware paths flag-gated — keep `--start-session` gated; do not auto-open ports on every Bridge launch yet (Auto-Start is Epic 3)

From Story **1.2** (done):

- Port N / cable binding already locked — reuse `collectProductCableIndices` / counts; do not invent a second cable table in Midi
- Display names are **new** (DeviceSessionManager), distinct from cable-index helpers in Profile

From Story **1.1** (done):

- `src/Midi/.gitkeep` awaits real PascalCase sources — replace now
- Include root is `src/` → `#include "Midi/MidiBackend.h"`

### Git intelligence summary

Recent implementation commits:

- `5e13e4f` — DeviceSession + Emagic cable mapper + WinUSB bulk I/O
- `b21b7b4` — WinUSB bind path + GUID-first MT4 transport open
- `2ecc1e8` — Declarative MT4 DeviceProfile + Port N helpers
- `559cc96` — Bridge CMake scaffold + Windows CI

Patterns to extend: single root `CMakeLists.txt`, one `Bridge` executable, flag-gated hardware paths, fail-closed stubs on non-Windows, layer isolation greps.

### Latest tech information

- Tobias Erichsen VirtualMIDI SDK: programmatic create/destroy of freely nameable ports; driver comes from loopMIDI/rtpMIDI for eval; MSI merge module only for licensees (OQ-1)
- Typical C API: `virtualMIDICreatePortEx2(name, callback, cookie, queueSize, flags)` / `virtualMIDIClosePort` — confirm against the SDK header you obtain for eval; prefer dynamic load
- Instantiation flags commonly distinguish RX/TX/BOTH — use them so Port 3/4 are OUT-only (no phantom IN 3/4)
- Marketing pages still say “Windows 7 up to Windows 10”; project still targets Win10+Win11 — treat Win11 + Windows MIDI Services dynamic-port quirks as AQ-4 ops validation, not a V1 backend change
- Do not treat Windows MIDI Services built-in loopback as a substitute for Emagic-shaped Bridge ports

### Project context reference

- `conventions.md` §3 quality gate, §6 C++ style, protocol/backends stay swappable behind an interface
- No `project-context.md` in-repo yet — follow architecture spine + conventions
- Clarity: keep hardware/protocol names in English (MT4, WinUSB, VirtualMIDI, MIDI)

### Anti-patterns to forbid

- Forking or vendoring `aaron1a12/virtual-midi` (GPL) as project base
- MidiBackend inventing / sorting ordinal `K` from USB serial
- Creating ports from App or hot-plug **before** `DeviceSession::Start`
- Keeping ports alive after unplug/teardown (“mark unavailable”)
- Naming IN endpoints `MT4 Port 3` / `MT4 Port 4`
- Presenting empty port list as success when VirtualMIDI is missing
- Shipping Windows MIDI Services as V1 backend
- Embedding VirtualMIDI MSI / solving OQ-1 Tobias clearance in this story
- Implementing notes/CC round-trip Validation Matrix proof “while we’re here” (1.6)
- VirtualMIDI includes in Profile/Protocol
- French comments; kebab-case sources under `src/`; custom kernel VirtualMIDI Plan B
- Session-0 Windows Service host

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Story 1.5, Epic 1]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-2..9, AD-12, Structural Seed]
- [Source: `architecture/.../reviews/review-adversarial-divergence.md` — Findings 1, 4, 6 (K owner, CreatePortSet graph, IN Port 1..2)]
- [Source: `architecture/.../reviews/review-version-reality.md` — AQ-4 Win11/WMS ops note]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — FR-2, FR-4, FR-5, NFR-Q3, glossary VirtualMIDI]
- [Source: `prds/.../addendum.md` — VirtualMIDI licensing / eval vs MSI]
- [Source: `_bmad-output/implementation-artifacts/1-4-devicesession-and-emagic-cable-mapper-usermode.md` — prior session seam + deferred manager]
- [Source: `_bmad-output/implementation-artifacts/1-2-declarative-mt4-deviceprofile.md` — Port N / Broadcast exclusion]
- [Source: `conventions.md` — §3 quality gate, §6 C++ standards]
- [Source: https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html — SDK eval model]

## Dev Agent Record

### Agent Model Used

Composer (Cursor agent)

### Debug Log References

- macOS: `cmake -S . -B builds/debug-1-5 && cmake --build builds/debug-1-5`
- `Bridge --test-port-names` exit 0; `Bridge --test-mapper` exit 0; profile smoke exit 0
- `Bridge --start-session` fail-closed on macOS (WinUSB requires Windows) with expected port diagnostics printed first
- `python scripts/quality/lint-touched.py --base HEAD` exit 0
- Isolation grep: no VirtualMIDI/WinUSB under src/Protocol or src/Profile

### Completion Notes List

- MidiBackend abstract seam + PortNameSet with ready-made UTF-8 display names; Story 1.6 SendToHost / host→device sink stubs declared
- VirtualMidiBackend loads teVirtualMIDI.dll at runtime (no proprietary .lib in repo); IN uses INSTANTIATE_TX, OUT uses INSTANTIATE_RX; fail-closed English fix path when DLL/driver missing; non-Windows CreatePortSet fails closed
- DeviceSessionManager owns K=1 and formatPortDisplayName / buildPortNameSet (MT4 Port N / MT4 #K Port N)
- DeviceSession Start sequence: WinUSB → init magics → CreatePortSet; Stop destroys ports then finish+close
- CLI: --test-port-names; --start-session prints 2 IN / 4 OUT expected names and wires VirtualMidiBackend at composition root

### File List

- CMakeLists.txt
- src/App/Main.cpp
- src/Device/DeviceSession.cpp
- src/Device/DeviceSession.h
- src/Device/DeviceSessionManager.cpp
- src/Device/DeviceSessionManager.h
- src/Midi/.gitkeep (deleted)
- src/Midi/MidiBackend.h
- src/Midi/TeVirtualMidiApi.h
- src/Midi/VirtualMidiBackend.cpp
- src/Midi/VirtualMidiBackend.h
- _bmad-output/implementation-artifacts/1-5-virtualmidi-backend-and-stable-mt4-port-names.md
- _bmad-output/implementation-artifacts/sprint-status.yaml

## Change Log

- 2026-08-05 — Story context created (ready-for-dev)
- 2026-08-05 — Implemented VirtualMIDI backend + stable MT4 Port names; status → review
- 2026-08-05 — Code review: kept TX/RX mapping; patched DLL load, PortNameSet guards, OUT name tests; status → done
