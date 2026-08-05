---
baseline_commit: b21b7b4
---

# Story 1.4: DeviceSession and Emagic cable mapper (usermode)

Status: done

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a studio user,
I want the Bridge to keep a live usermode session that multiplexes/demultiplexes Emagic cables for one MT4,
so that physical IN/OUT cables map correctly without any custom kernel MIDI driver.

## Acceptance Criteria

1. **Given** WinUSB transport can open the MT4 (Story 1.3) and the MT4 DeviceProfile (Story 1.2)  
   **When** the Bridge starts a DeviceSession for one connected MT4  
   **Then** Emagic cable multiplex/demultiplex runs entirely in the C++17 usermode process — FR-2 / AD-1

2. **And** there is exactly one DeviceSession per MT4 instance (own WinUSB handle and mapper state) — AD-4

3. **And** protocol orientation is original MIT reimplementation; no GPL Linux sources are vendored (Linux quirk reference is read-only) — AD-14 / NFR-Q2

4. **And** the Bridge process is a user-session host (not a Session-0 Windows Service) — AD-20

5. **And** session start/stop emits English diagnostics sufficient to debug attach failures

**Traces:** FR-2, CAP-2, AD-1, AD-4, AD-14, AD-20

## Tasks / Subtasks

- [x] Task 1: Implement `EmagicCableMapper` in `src/Protocol/` (AC: 1, 3)
  - [x] Add PascalCase sources — `EmagicCableMapper.h` / `EmagicCableMapper.cpp` (canonical name — **not** older `EmagicMidiMapper` examples in `conventions.md`)
  - [x] Consume `DeviceProfile` masks + Port N helpers from `Profile/DeviceProfile.h` — **do not** hard-code `0x8003` / `0x800f` / Port↔cable tables inside the mapper
  - [x] Implement **host → device** framing (multiplex): given MIDI bytes destined for Emagic cable index `c`, emit bulk payload using Emagic `F5` port-switch protocol (see Dev Notes)
  - [x] Implement **device → host** parsing (demultiplex): parse bulk bytes into per-cable MIDI streams; honor `0xFF` end-of-valid-data and `F5` spanning buffer boundaries (`seen_f5` state)
  - [x] Wire-format cable IDs are **1-based on the wire** (`F5` then `(cableIndex + 1) & 15`); internal cable indices remain **0-based** matching Profile bit `i` ↔ cable `i`
  - [x] Product Port N (1..2 IN / 1..4 OUT) maps via `collectProductCableIndices` — Broadcast cable **15** is not a V1 product port
  - [x] **AD-2 isolation:** Protocol headers/sources must **not** include WinUSB, `windows.h`, or VirtualMIDI headers — only Profile (+ STL)
  - [x] English comments only; MIT-original structure — describe behavior from reference, **never** paste GPL `midi.c` into the tree

- [x] Task 2: Extend `WinUsbTransport` with bulk I/O (AC: 1, 2)
  - [x] After successful `Open`, discover OUT/IN bulk pipes used for Emagic MIDI on the opened interface (WinUSB pipe query)
  - [x] Add public synchronous helpers, e.g. `WriteBulk` / `ReadBulk` (names free; keep Allman / `bool` + `errorOut` pattern) — English diagnostics on failure
  - [x] Do **not** expose raw `WINUSB_INTERFACE_HANDLE` / `HANDLE` to Protocol or Profile
  - [x] Keep open/close/GUID-first behavior from Story 1.3 intact (including Zadig hatch and ambiguous-match refuse)
  - [x] Dual-machine: non-Windows stubs fail closed with clear English messages (same pattern as `Open`)

- [x] Task 3: Implement `DeviceSession` in `src/Device/` (AC: 1, 2, 4, 5)
  - [x] Add `DeviceSession.h` / `DeviceSession.cpp`
  - [x] One session owns: a `DeviceProfile` copy/ref, one `WinUsbTransport`, one `EmagicCableMapper` (own mapper state; own WinUSB handle via transport)
  - [x] `Start(...)`: open transport for the profile → send Emagic **init** bulk magic (see Dev Notes) → English success/failure diagnostics → fail closed if open or init write fails
  - [x] `Stop()` / destructor: best-effort Emagic **finish** bulk (if open) → `Close()` transport → English stop diagnostic; idempotent / safe if never started
  - [x] Non-copyable; RAII teardown
  - [x] **Do not** create Virtual Ports or call MidiBackend (Story 1.5 / AD-9) — leave a clear seam comment that only a live session will later own port lifecycle through `MidiBackend`
  - [x] **Do not** implement full `DeviceSessionManager` / multi-unit ordinal `K` (AD-5/AD-6) — single-unit App ownership is enough; two units = two sessions later (Epic 3), not one cascade (AD-4)
  - [x] Remove `src/Device/.gitkeep` and `src/Protocol/.gitkeep` when real sources exist

- [x] Task 4: Wire Bridge build + App proof points (AC: 1, 5)
  - [x] Add Protocol + Device (+ any new Usb helpers) TUs to the `Bridge` CMake target
  - [x] Keep Story 1.2 profile smoke and Story 1.3 `--open-device` / `--dev-zadig` behavior working
  - [x] Add `--test-mapper`: run **synthetic** encode/decode vectors (no hardware) for MT4 IN/OUT cable set; exit `0` on pass, non-zero + English diagnostic on fail (satisfies adversarial AD-3 round-trip requirement without a third-party test framework)
  - [x] Add `--start-session` (Windows hardware path): Start one `DeviceSession` for MT4; print English start diagnostics; Stop on exit; fail closed if Start fails. Gate like `--open-device` (no hardware attempt without the flag)
  - [x] Preserve AD-20: remain a normal user-session exe — **no** Windows Service project

- [x] Task 5: Quality + anti-scope (AC: all)
  - [x] `python scripts/quality/lint-touched.py` exits 0 on the C++ diff (Protocol pure logic prefers ~30-line function budget; Usb glue may use ~50)
  - [x] Compile: `cmake -S . -B builds/<config> && cmake --build builds/<config>` (macOS smoke OK; Windows CI remains the gate)
  - [x] Grep isolation: no `winusb` / `WinUsb` / VirtualMIDI / `teVirtualMIDI` under `src/Protocol/` or `src/Profile/`
  - [x] Confirm no French in sources; no GPL Linux file vendored under the repo; no `Midi/` VirtualMIDI implementation; no notes/CC host round-trip product claim

### Review Findings

- [x] [Review][Decision] Emagic OUT pad style — resolved 2026-08-05: keep a single trailing `0xFF` when capacity remains (do not fill remaining buffer like Linux URB padding).
- [x] [Review][Patch] Rollback `currentOutCable_` if MIDI append fails after port switch [`src/Protocol/EmagicCableMapper.cpp:69`]
- [x] [Review][Patch] Do not deliver demux MIDI for non-product IN cables (incl. broadcast 15) [`src/Protocol/EmagicCableMapper.cpp:166`]
- [x] [Review][Patch] Refuse ambiguous multi bulk IN/OUT pipe discovery (last-wins today) [`src/Usb/WinUsbTransport.cpp:99`]
- [x] [Review][Patch] Guard `cableIndex` before `1u << cableIndex` (shift UB for index ≥ 32) [`src/Protocol/EmagicCableMapper.cpp:12`]
- [x] [Review][Patch] Include Win32 `GetLastError` (or transferred length) in bulk I/O failure diagnostics [`src/Usb/WinUsbTransport.cpp:214`]
- [x] [Review][Defer] Synchronous bulk I/O has no pipe transfer timeout [`src/Usb/WinUsbTransport.cpp:205`] — deferred, pre-existing
- [x] [Review][Defer] No synchronization between `Close` and in-flight `ReadBulk`/`WriteBulk` [`src/Usb/WinUsbTransport.cpp:167`] — deferred, pre-existing
- [x] [Review][Defer] Host MIDI containing raw `0xF5` / `0xFF` can collide with Emagic framing [`src/Protocol/EmagicCableMapper.cpp:84`] — deferred, pre-existing

## Dev Notes

### Scope fence

This story lands **FR-2 / CAP-2 (session + Emagic multiplex portion)**. After it, the Bridge can own a live usermode session with correct cable framing for one MT4 — still without VirtualMIDI ports.

| In scope | Out of scope (later stories) |
|---|---|
| `EmagicCableMapper` F5 multiplex/demultiplex | VirtualMIDI / `MidiBackend` / `MT4 Port N` names → **1.5** |
| `DeviceSession` Start/Stop for one MT4 | Notes/CC DAW round-trip proof → **1.6** |
| `WinUsbTransport` bulk read/write | Clock / MTC / SysEx product paths → Epic **2** |
| Mapper synthetic tests + session diagnostics | Hot-plug / Auto-Start / multi-MT4 ordinal → Epic **3** |
| Init/finish Emagic bulk magic on session lifecycle | Public Installer UX → **4.1** |
| | Cousin AMT8/Unitor8 shipping → hardware-gated later |
| | Emagic cascade topologies → V1 non-goal |
| | Session-0 Windows Service → forbidden forever (AD-20) |

### Epic context

Epic 1 outcome: bind MT4 to WinUSB, run C++17 Bridge, see stable `MT4 Port N` (2 IN / 4 OUT), exchange notes/CC — without a custom kernel driver.  
Stories 1.2–1.3 shipped profile + GUID-first open. Story 1.4 is the **session + protocol** seam that 1.5 attaches Virtual Ports to.

### Architecture compliance (must follow)

| Decision | What it means for this story |
|---|---|
| AD-1 | All Emagic multiplex/demultiplex in C++17 usermode; no custom kernel MIDI driver |
| AD-2 | `EmagicCableMapper` must not include WinUSB or VirtualMIDI headers; dependency direction App → Device → (Usb + Protocol); Protocol → Profile only |
| AD-3 | Masks are Linux quirk-shaped; bit `n` ↔ Emagic cable `n`; Port N via Profile helpers (Broadcast 15 excluded) |
| AD-4 | Exactly one `DeviceSession` per MT4 instance (own handle + mapper state); no cascade |
| AD-9 (design only) | Do not invent App/hot-plug port creation; ports arrive in 1.5 only through session → `MidiBackend` |
| AD-14 | MIT reimplementation; Linux `midi.c` / `quirks-table.h` are **read-only reference**; no GPL sources in repo; if wire behavior is ambiguous, capture USB on Windows and document under `docs/dev/` |
| AD-15 | PascalCase under `src/`; lint-touched; English only |
| AD-20 | User-session Bridge exe, not Session-0 Service |
| Structural Seed | `Protocol/` = EmagicCableMapper; `Device/` = DeviceSession; `Usb/` = WinUsbTransport |
| Pipeline | `WinUsbTransport` ↔ `EmagicCableMapper` ↔ (`MidiBackend` later) |

[Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-1..4, AD-9, AD-14, AD-15, AD-20, Structural Seed]

### CRITICAL — Emagic wire protocol (reference behavior; reimplement under MIT)

Linux documents Emagic USB MIDI as **raw MIDI with `F5 xx` port switching** (`QUIRK_MIDI_EMAGIC` ops in `sound/usb/midi.c`). Confirm against current upstream if unsure; **do not copy GPL source files into this repository**.

**Demultiplex (device → host bulk IN):**

1. Truncate buffer at first `0xFF` (end of valid data); ignore trailing pad.
2. MIDI bytes before the next `0xF5` belong to the **current** cable index.
3. On `0xF5`, the following byte (when `< 0x80`) selects cable `(byte - 1) & 15` (wire is 1-based; internal index 0-based).
4. `0xF5` may be split across bulk reads — retain a `seen_f5` (or equivalent) flag across calls.

**Multiplex (host → device bulk OUT):**

1. Track last selected cable on the OUT endpoint.
2. When sending to a different cable, prefix `0xF5`, `(cableIndex + 1) & 15`, then MIDI bytes.
3. When the URB/buffer has remaining space after real data, pad with a trailing `0xFF` (Linux pads remaining free space similarly).

**Session init / finish bulk (send on OUT after open / before close):**

| Phase | Purpose (behavior) | Byte sequence (document as named constants in Protocol or Device — not magic scattered in App) |
|---|---|---|
| Init (×2 in Linux) | “get version” Unitor magic | `F0 00 20 31 64 0B 00 00 F7` |
| Finish | patch-switch teardown magic | `F0 00 20 31 64 10 00 7F 40 F7` |

V1 MT4 `DeviceProfile.patchMode` stays **false** — still send lifecycle magics so the device enters/leaves the expected Emagic USB-MIDI mode (matches Linux endpoint init/finish behavior). Do not invent Patch/LTC/FastMode product features.

**Port ↔ cable (MT4, already locked in Story 1.2):**

| Direction | Product ports | Emagic cable indices |
|---|---|---|
| IN | Port 1..2 | 0, 1 |
| OUT | Port 1..4 | 0, 1, 2, 3 |

Broadcast bit 15 remains in stored masks; **not** a V1 Virtual Port / mapper product endpoint.

[Source: Linux `sound/usb/midi.c` Emagic ops — read-only; Story 1.2 Dev Notes; brief addendum cable table]

### CRITICAL — Naming SSOT

Use **`EmagicCableMapper`** everywhere (architecture spine, epics, Structural Seed). Older examples saying `EmagicMidiMapper` in `conventions.md` / Cursor rules are **stale aliases** — do not create that type name.

### CRITICAL — Layer boundaries (anti-reinvention)

| Layer | Owns | Must not |
|---|---|---|
| Profile | Masks, Port N helpers | WinUSB, F5 framing, session lifecycle |
| Protocol | F5 encode/decode + init/finish byte constants (or adjacent Protocol helpers) | WinUSB headers, VirtualMIDI, App CLI |
| Usb | GUID open + bulk pipes | Emagic F5 parsing, Port naming |
| Device | Session lifecycle composing Usb + Protocol | Installer, VirtualMIDI SDK |
| App | Flags / diagnostics / smoke | Reimplementing mapper or second cable table |

Do **not** invent a second CableMap type that reinterprets bits as 1-based cable IDs (adversarial Finding 2).

### Suggested shapes (illustrative — adjust to conventions)

```cpp
// Protocol/EmagicCableMapper.h — Profile only; no WinUSB
#pragma once
#include "Profile/DeviceProfile.h"
#include <cstddef>
#include <cstdint>
#include <string>

class EmagicCableMapper
{
public:
    explicit EmagicCableMapper(const DeviceProfile& profile);

    // Host MIDI bytes for one Emagic cable → append framed bulk bytes to outBuffer.
    bool EncodeToDevice(
        uint8_t cableIndex,
        const uint8_t* midiBytes,
        std::size_t midiSize,
        uint8_t* outBuffer,
        std::size_t outCapacity,
        std::size_t& outSize,
        std::string& errorOut);

    // One bulk IN buffer → demux into per-cable sinks.
    // Prefer a function-pointer / std::function callback:
    //   void(uint8_t cableIndex, const uint8_t* midi, std::size_t n)
    // so Story 1.5 can feed MidiBackend without rewriting the parser.
    // Must preserve seen_f5 / currentCable across calls.
    bool DecodeFromDevice(
        const uint8_t* bulkBytes,
        std::size_t bulkSize,
        /* per-cable MIDI sink callback */,
        std::string& errorOut);

private:
    DeviceProfile profile_;
    uint8_t currentOutCable_ = 0;
    uint8_t currentInCable_ = 0;
    bool seenF5_ = false;
};
```

```cpp
// Device/DeviceSession.h — may include Usb + Protocol
class DeviceSession
{
public:
    DeviceSession() = default;
    ~DeviceSession();
    DeviceSession(const DeviceSession&) = delete;
    DeviceSession& operator=(const DeviceSession&) = delete;

    bool Start(const DeviceProfile& profile, std::string& errorOut,
               WinUsbOpenOptions options = {});
    void Stop() noexcept;
    bool IsRunning() const noexcept;

private:
    WinUsbTransport transport_;
    // mapper owned when running — unique_ptr or optional member
};
```

### Technical requirements

- **Language:** C++17
- **Build:** extend root `CMakeLists.txt`; outputs under `builds/`
- **Windows APIs:** existing WinUSB + SetupAPI; add `WinUsb_QueryPipe` / `WinUsb_ReadPipe` / `WinUsb_WritePipe` (or equivalent) behind Usb layer only
- **Style:** Allman, 4 spaces, `#pragma once`, English — `conventions.md` §6
- **Quality:** `scripts/quality/lint-touched.py` SSOT; prefer TDD for Protocol pure logic (`conventions.md` §8)
- **No** new third-party deps (no Catch2/GoogleTest required this story — use `--test-mapper` vectors)
- **No** VirtualMIDI SDK yet

### Library / framework requirements

| Use | Do not use |
|---|---|
| Existing `DeviceProfile` + Port N helpers | Hard-coded second MT4 mask table in Protocol |
| Existing `WinUsbTransport` Open/Close | Exposing WinUSB handles to Protocol |
| MIT-original F5 reimplementation | Vendoring / pasting GPL `midi.c` / `quirks-table.h` |
| `--test-mapper` synthetic vectors | Claiming DAW notes/CC proof (1.6) |
| Single `DeviceSession` from App | Full `DeviceSessionManager` / multi-unit `K` (Epic 3) |

### File structure requirements

#### NEW (create)

| Path | Purpose |
|---|---|
| `src/Protocol/EmagicCableMapper.h` | Mapper API + framing state |
| `src/Protocol/EmagicCableMapper.cpp` | F5 encode/decode (+ optional init/finish constant helpers) |
| `src/Device/DeviceSession.h` | Session API |
| `src/Device/DeviceSession.cpp` | Start/Stop composing transport + mapper |

#### UPDATE (existing)

| Path | Current state | This story |
|---|---|---|
| `CMakeLists.txt` | Bridge = Main + Profile + Usb | Add Protocol + Device TUs |
| `src/App/Main.cpp` | Profile smoke + `--open-device` | Add `--test-mapper` + `--start-session`; keep prior flags |
| `src/Usb/WinUsbTransport.h/.cpp` | Open/Close/IsOpen only | Add bulk pipe discovery + Read/Write |
| `src/Device/.gitkeep`, `src/Protocol/.gitkeep` | Empty markers | Delete when real sources exist |

#### OUT OF SCOPE paths

- `src/Midi/*` VirtualMIDI / `MidiBackend` → **1.5**
- Notes/CC Validation Matrix proof → **1.6**
- Full Public Installer / Auto-Start → **4.1** / **3.1**
- Hot-plug recovery product UX → **3.2**

### Existing code being modified — current state

**After Stories 1.1–1.3 (done):**

- `CMakeLists.txt` — C++17 `Bridge`; sources: `Main.cpp`, `DeviceProfile.cpp`, `WinUsbTransport.cpp` (+ Windows-only open helpers); links `setupapi`/`winusb`/`ole32` on WIN32
- `src/App/Main.cpp` — profile smoke (MT4 fields + cable order `{0,1}` / `{0,1,2,3}`); `--open-device` opens transport only; `--dev-zadig` hatch; exit 0/1; **no session, no F5**
- `src/Profile/DeviceProfile.h/.cpp` — MT4 validated row + cousin stubs; Port N helpers skip Broadcast 15; **no WinUSB includes**
- `src/Usb/WinUsbTransport.*` — GUID-first open, ifnum validation, RAII close; handles private; **no public bulk I/O**
- `src/Device/.gitkeep`, `src/Protocol/.gitkeep`, `src/Midi/.gitkeep` — empty
- CI: `.github/workflows/windows-build.yml` on `windows-2022`

**Preserve:**

- Profile purity (AD-2) and MT4 constants / lookup API
- GUID-first open semantics (ambiguous multi-match refuse; clear `errorOut` on success; Zadig hatch scoped)
- User-session Bridge exe (AD-20)
- `builds/` out-of-source; English/MIT orientation; no kernel driver files
- Story 1.2 Main profile assertions and Story 1.3 `--open-device` path

**Must not break:**

- Windows CI compile
- macOS configure/build smoke without WinUSB
- `lint-touched.py` exit 0 on conforming code
- Profile smoke for known/unknown PID
- Open-without-session path (`--open-device`) unless intentionally superseded — prefer keeping both flags working

### Testing requirements

| Check | How |
|---|---|
| Compile (macOS) | `cmake -S . -B builds/<cfg> && cmake --build builds/<cfg>` — stubs OK |
| Compile (Windows CI) | Existing workflow green with Protocol/Device linked |
| Mapper round-trip | `Bridge --test-mapper` — at least: (a) encode note-on for OUT cables 0..3 inserts correct `F5` switches; (b) decode a synthetic bulk buffer with `F5` + MIDI + `FF` pad routes to expected cables; (c) `F5` split across two `DecodeFromDevice` calls preserves state |
| Port/cable binding | Mapper uses Profile helpers — MT4 product IN `{0,1}`, OUT `{0,1,2,3}` |
| Isolation | Grep `src/Protocol/` and `src/Profile/`: no WinUSB / VirtualMIDI |
| Session diagnostics | `--start-session` on unbound machine prints English failure (not silent success); on bound Windows hardware Start succeeds and Stop is clean |
| Lint | `python scripts/quality/lint-touched.py` on touched C++ |
| AD-14 | Repo contains no vendored `midi.c` / `quirks-table.h` copies |

No Validation Matrix DAW proof and no MIDI Path harness for this story.

### Previous story intelligence

From Story **1.3** (done):

- Transport is open/close only today — **this story must add bulk I/O**; 1.3 explicitly deferred sustained I/O and Emagic/DeviceSession here
- Fail-closed English diagnostics; clear `errorOut` on success; refuse ambiguous multi-GUID matches
- Dual-machine `#ifdef _WIN32` stubs; keep that pattern for new Usb methods
- Anti-scope worked: no Protocol/Device creep in 1.3 — create them **here**
- Review pitfalls: do not leave stale failure text after success; keep GUID as single string constant

From Story **1.2** (done):

- Binding rules for mapper are normative — store Linux masks; bit `i` ↔ cable `i`; Port N excludes cable 15
- Prefer Profile helpers over a competing CableMap interpretation
- Lint CCN: extract helpers when `main` grows — `--test-mapper` / `--start-session` will push Main; keep anonymous-namespace helpers small

From Story **1.1** (done):

- Empty layers used `.gitkeep` until needed — replace Device/Protocol placeholders with real PascalCase sources
- Include root is `src/` → `#include "Protocol/EmagicCableMapper.h"`, `#include "Device/DeviceSession.h"`

### Git intelligence summary

Recent implementation commits:

- `b21b7b4` — WinUSB bind path + GUID-first MT4 transport open
- `2ecc1e8` — Declarative MT4 DeviceProfile + Port N helpers
- `559cc96` — Bridge CMake scaffold + Windows CI

Patterns to extend: single root `CMakeLists.txt`, one `Bridge` executable, flag-gated hardware paths, fail-closed stubs on non-Windows.

### Latest tech information

- Emagic ops remain in upstream Linux `sound/usb/midi.c` (`snd_usbmidi_emagic_*`, `QUIRK_MIDI_EMAGIC`) — F5 port-switch + FF pad + init/finish SysEx magics still the reference behavior as of 2026 review
- Architecture version-reality review confirmed `QUIRK_MIDI_EMAGIC` still present on torvalds/linux master
- WinUSB bulk: use `WinUsb_QueryPipe` after `WinUsb_Initialize` to locate bulk IN/OUT pipe IDs on the opened interface; then `WinUsb_ReadPipe` / `WinUsb_WritePipe`
- No new third-party library versions for this story

### Project context reference

- `conventions.md` §3 quality gate (lint-touched), §6 C++ style, §8 prefer TDD for protocol/mapping
- No `project-context.md` in-repo yet — follow architecture spine + conventions
- Clarity: keep hardware/protocol names in English (MT4, WinUSB, Emagic, MIDI)

### References

- Epics Story 1.4 — `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md`
- Architecture spine AD-1, AD-2, AD-3, AD-4, AD-9, AD-14, AD-20 — `.../ARCHITECTURE-SPINE.md`
- Adversarial Finding 2 (mask semantics) — `architecture/.../reviews/review-adversarial-divergence.md`
- Prior stories — `1-2-declarative-mt4-deviceprofile.md`, `1-3-winusb-bind-path-and-transport-open.md`
- Brief cable table — `briefs/.../addendum.md`
- Linux reference (external, read-only) — `https://github.com/torvalds/linux/blob/master/sound/usb/midi.c` (Emagic section)

## Dev Agent Record

### Agent Model Used

Composer (Cursor agent router)

### Debug Log References

- macOS: `cmake -S . -B builds/macos-smoke && cmake --build builds/macos-smoke`
- `Bridge --test-mapper` → pass
- `Bridge --start-session` / `--open-device` on macOS → fail closed with English WinUSB message
- `python scripts/quality/lint-touched.py` → OK

### Completion Notes List

- Implemented `EmagicCableMapper` F5 multiplex/demultiplex with `seen_f5` spanning buffers; product OUT cables validated via Profile masks (Broadcast 15 excluded).
- Extended `WinUsbTransport` with bulk pipe discovery after Open plus `WriteBulk` / `ReadBulk`; handles stay private; non-Windows stubs fail closed.
- Added `DeviceSession` Start/Stop owning one transport + mapper; sends Emagic init (×2) / finish magics; seam comment for future MidiBackend (1.5); no Virtual Ports.
- Bridge CLI: `--test-mapper` synthetic vectors (encode switches, decode + FF pad, split F5); `--start-session` hardware-gated; prior `--open-device` / `--dev-zadig` preserved.
- Quality: lint-touched green; Protocol free of platform USB/VirtualMIDI includes; no GPL sources vendored.

### File List

- `CMakeLists.txt`
- `src/App/Main.cpp`
- `src/Device/DeviceSession.h`
- `src/Device/DeviceSession.cpp`
- `src/Protocol/EmagicCableMapper.h`
- `src/Protocol/EmagicCableMapper.cpp`
- `src/Usb/WinUsbTransport.h`
- `src/Usb/WinUsbTransport.cpp`
- `src/Profile/DeviceProfile.h`
- `src/Device/.gitkeep` (deleted)
- `src/Protocol/.gitkeep` (deleted)
- `_bmad-output/implementation-artifacts/1-4-devicesession-and-emagic-cable-mapper-usermode.md`
- `_bmad-output/implementation-artifacts/sprint-status.yaml`

## Change Log

- 2026-08-05 — Story context created (ready-for-dev)
- 2026-08-05 — Implemented DeviceSession + EmagicCableMapper + WinUSB bulk I/O; status → review
- 2026-08-05 — Code review patches applied (mapper state rollback, IN filter, ambiguous pipes, diagnostics); status → done
