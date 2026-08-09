---
baseline_commit: 7934b75
---

# Story 2.1: MIDI clock and transport realtime

Status: done

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a DAW user,
I want MIDI clock and Start/Stop/Continue to pass through the Bridge without dropouts under normal session use,
so that sequencers can slave or observe tempo/transport from MT4 Virtual Ports.

## Acceptance Criteria

1. **Given** Epic 1 notes/CC path is working on at least one IN and one OUT Virtual Port  
   **When** a Validation Matrix DAW (Ableton Live 12 or Reason Studios 12) sends or observes MIDI clock (`0xF8`) and Start / Stop / Continue through an MT4 Virtual Port  
   **Then** those messages are carried without Bridge-induced dropouts under a normal short sequencing smoke — FR-7 / CAP-7 / AD-17

2. **And** dropping clock or transport realtime is treated as a V1 defect (not a deferred nicety) — AD-17

3. **And** the smoke is documented for both Win10 x64 and Win11 x64 when hardware is available (Win10 mandatory in matrix)

**Traces:** FR-7, CAP-7, AD-17, SM-1 (clock/transport portion)

## Tasks / Subtasks

- [x] Task 1: Prove and harden device→host realtime framing (AC: 1, 2)
  - [x] Confirm `MidiMessageFramer` already emits every `status >= 0xF8` as an immediate 1-byte message without clearing running status or SysEx hold — do **not** invent a second framer or ClockEngine
  - [x] Extend `FramerSmoke` (invoked via `Bridge --test-mapper`) with synthetic vectors: lone `0xF8`; `0xFA` / `0xFB` / `0xFC`; clock interleaved mid-note (between status and data); realtime during an open SysEx hold that does **not** abort SysEx assembly
  - [x] Prefer keeping framer coverage in `FramerSmoke` (and/or Catch2 if a small `MidiMessageFramerTests.cpp` stays under lint budget) — no new third-party test framework
  - [x] If a framer bug is found for F8/FA/FB/FC, fix in `MidiMessageFramer` only; preserve existing note/CC/running-status behavior

- [x] Task 2: Prove Emagic opaque carry of clock/transport (AC: 1, 2)
  - [x] Add at least one encode + one decode synthetic vector in `EmagicMapperSmokeSupport` / Catch2 mapper tests that includes `0xF8` and one of `0xFA`/`0xFC` on a product cable — prove F5 demux does not eat realtime bytes
  - [x] Do **not** add message-type allowlists or filters in DeviceSession / VirtualMidiBackend / EmagicCableMapper
  - [x] Do **not** invent Emagic escaping for `0xFF` System Reset vs pad collision — still deferred (Linux-parity limitation); `0xF8`–`0xFC` are safe vs F5/FF framing

- [x] Task 3: Host→device path — verify, only harden if lab fails (AC: 1, 2)
  - [x] Default assumption: teVirtualMIDI `PARSE_RX` delivers Start/Stop/Continue/Clock as complete single-byte commands → existing `EncodeToDevice` + `WriteBulk` path is enough
  - [x] On Windows hardware smoke: confirm DAW → Virtual OUT → MT4 physical OUT carries clock/transport (DIN LED / slave device / loopback observation) — 2026-08-10 PC-only DIN Out2→In2 via `midi-clock-loopback-lab.py` (same honesty barème as story 2.2); Scarlett/DAW UAT deferred
  - [x] **Only if** lab shows interleaved realtime inside multi-byte spans or incomplete host→device commands: add a symmetric host→device framer (mirror device→host). Do **not** add it preemptively
  - [x] If dense clock (24 ppqn) shows Bridge-induced dropouts under short smoke, investigate known load edge (`processBulkRead` holding `usbIoMutex_` across decode) — fix only if required for AC pass; full latency harness remains Epic **5** / AD-11

- [x] Task 4: Document hardware clock/transport smoke (AC: 1, 3)
  - [x] Add `docs/tests/smoke-epic2-clock-mt4.md` (kebab-case) with a clear checklist: ≥1 IN + ≥1 OUT; Ableton Live 12 **or** Reason Studios 12; observe/slave `0xF8` + Start/Stop/Continue; short sequencing session without Bridge restart; English failure notes (Port N / cable / direction)
  - [x] Win10 x64 = mandatory matrix row; Win11 x64 = document when hardware available (same pattern as Epic 1 smoke)
  - [x] Cross-link lightly from `docs/tests/smoke-epic1-mt4.md` (Epic 1 stays notes/CC; clock proof lives in Epic 2 doc)
  - [x] Optional CLI wording tweak in `MidiSessionCli` (“clock/transport smoke ready”) — only if it stays within lint CCN; not required for AC

- [x] Task 5: Quality + anti-scope (AC: all)
  - [x] `python scripts/quality/lint-touched.py` exits 0 on the C++ diff
  - [x] Compile: `cmake -S . -B builds/<config> && cmake --build builds/<config>` (macOS smoke OK; Windows CI remains the gate)
  - [x] `Bridge --test-mapper` still exit 0 (includes new framer + mapper vectors)
  - [x] Grep isolation: no `teVirtualMIDI` / `VirtualMIDI` / `winusb` under `src/Protocol/` or `src/Profile/`
  - [x] Confirm no French in sources; no MTC product claim (→ **2.2**); no SysEx librarian claim (→ **2.3**/**2.4**); no MIDI Path harness (→ Epic **5**); no installer / Auto-Start / hot-plug work

### Review Findings

- [x] [Review][Patch] Await Win10 DAW lab Pass before story done — Closed 2026-08-10 via `midi-clock-loopback-lab.py` on real Out2→In2 (`20260809T221926Z`, clock 108/108, Start/Continue/Stop Pass with stop≥2, no Bridge fail needles); Scarlett/DAW UAT deferred (same barème as 2.2).
- [x] [Review][Patch] Add Catch2 framer realtime vectors to CI [`tests/MidiMessageFramerTests.cpp` + `CMakeLists.txt`] — Applied 2026-08-05: lone F8, FA/FB/FC, mid-note clock, SysEx-hold clock, running-status-after-clock in `BridgeTests`.
- [x] [Review][Patch] Smoke guide softens Continue as optional [`docs/tests/smoke-epic2-clock-mt4.md`] — Applied 2026-08-05: Continue required; N/A + English reason only if DAW cannot emit.
- [x] [Review][Patch] Mapper smoke omits Continue (`0xFB`) [`src/Protocol/EmagicMapperRealtimeSmoke.cpp`] — Applied 2026-08-05: encode vector is F8+FB+FC.
- [x] [Review][Patch] Transport framer test silent on wrong byte value [`src/App/FramerSmoke.cpp`] — Applied 2026-08-05: English diagnostic on byte mismatch.
- [x] [Review][Defer] Dense-clock mutex stall risk under load [`src/Device/DeviceSessionDeviceHost.cpp` / prior deferred-work] — deferred, pre-existing: `processBulkRead` holding `usbIoMutex_` across decode can stall host→device; story intentionally gates investigation on lab dropouts (already tracked in deferred-work from Epic 1 CR).

## Dev Notes

### Scope fence

This story lands the **FR-7 / CAP-7 clock + Start/Stop/Continue** portion of AD-17 / SM-1. Epic 1 already ships a transparent USB ↔ VirtualMIDI pump with a device→host framer that treats all realtime statuses (`>= 0xF8`) as immediate single-byte emits. **Primary deliverable is product proof + synthetic hardening + smoke docs** — not a greenfield mapper.

| In scope | Out of scope (later stories) |
|---|---|
| Prove `0xF8` + Start (`0xFA`) / Continue (`0xFB`) / Stop (`0xFC`) both directions under short DAW smoke | MTC quarter-frame (`0xF1`) + full-frame → **2.2** |
| Framer/mapper synthetic vectors for realtime interleave | Transparent SysEx / Matrix-Control vectors → **2.3** / **2.4** |
| Treat Bridge-induced clock/transport dropouts as defects | ~4h longevity design → **2.5** |
| Document Win10 (mandatory) / Win11 (when available) smoke | Auto-Start → **3.1**; hot-plug → **3.2** |
| Fix framer/pump only if smoke fails | Multi-client DAW+ShowMIDI → **3.3**; multi-MT4 → **3.4** |
| | Public Installer / MSI → **4.1** / OQ-1 |
| | MIDI Path latency/jitter harness + Studio-Done thresholds → Epic **5** / AD-11 |
| | `0xFF` System Reset vs Emagic pad escaping — still deferred |
| | Active Sensing (`0xFE`) product acceptance — not required by AC (framer may already emit it) |

### Epic context

Epic 2 outcome: Validation Matrix DAWs can use **clock / Start-Stop-Continue / MTC** through the Bridge, and Matrix-Control can complete minimum SysEx pass vectors — with buffering designed for ~4h sessions.

Story **2.1** is the first Epic 2 story: prove tempo/transport realtime on the same Virtual Ports that already carry notes/CC. Story **2.2** extends the same path to MTC; do not pull MTC into this story even though the framer already knows `0xF1` length.

Pipeline (unchanged):

```text
WinUsbTransport ↔ EmagicCableMapper ↔ MidiMessageFramer (device→host)
  ↔ MidiBackend (VirtualMidiBackend) ↔ Virtual Ports ↔ DAW
```

### Architecture compliance (must follow)

| Decision | What it means for this story |
|---|---|
| AD-1 | Emagic multiplex stays usermode; no custom kernel MIDI |
| AD-2 | Protocol free of VirtualMIDI/WinUSB; Midi free of F5; composition in Device |
| AD-3 | Port N / cable indices from Profile helpers only |
| AD-7 | Keep teVirtualMIDI runtime LoadLibrary path; no SDK binaries committed |
| AD-11 | Do **not** build `tools/midi-path-harness/` here; provisional latency anchors are Epic 5 |
| AD-14 | No GPL `midi.c` paste |
| AD-15 | PascalCase under `src/`; lint-touched; English only |
| AD-17 | Clock + Start/Stop/Continue are **required** V1 transport — dropouts = defect |
| AD-20 | Remain user-session Bridge exe |
| Structural Seed | Framing stays in `Protocol/`; pump in `Device/`; VirtualMIDI I/O in `Midi/` |

[Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-17, pipeline]

### CRITICAL — Reuse, do not reinvent

Epic 1 already provides:

1. Bidirectional USB ↔ VirtualMIDI byte pump (`DeviceSession` + `VirtualMidiBackend`)
2. Opaque Emagic encode/decode (no message-type filter)
3. `MidiMessageFramer` with `isRealtimeStatus(status >= 0xF8)` → immediate 1-byte `SendToHost`
4. Long-run CLI `--start-session` / `--run-midi` for hardware smoke
5. Lab-proven notes/CC on all 2 IN + 4 OUT (Epic 1 smoke green)

**Forbidden reinventing:**

- A new `ClockTransport` / `RealtimeFilter` / allowlist module
- A second I/O pump beside DeviceSession
- Replacing VirtualMIDI with Windows MIDI Services
- Claiming MTC or SysEx librarian vectors done because clock bytes flowed

### CRITICAL — Message map (this story)

| Byte | Name | Device→host framer today | Host→device today | Product AC 2.1 |
|---|---|---|---|---|
| `0xF8` | Timing Clock | Emit 1 byte immediately | Opaque encode | **Required** |
| `0xFA` | Start | Emit 1 byte | Opaque encode | **Required** |
| `0xFB` | Continue | Emit 1 byte | Opaque encode | **Required** |
| `0xFC` | Stop | Emit 1 byte | Opaque encode | **Required** |
| `0xF1` | MTC quarter-frame | 2-byte system path exists | Opaque | **Out → 2.2** |
| `0xF0…F7` | SysEx | Assembled (1024 hold) | Opaque | **Out → 2.3+** |
| `0xFF` | System Reset / Emagic pad | Framer would emit | Decode truncates at first `0xFF` | **Out / deferred collision** |

MIDI 1.0 allows realtime bytes **between** bytes of other messages (including mid-note and mid-SysEx). The framer must process them without aborting the interrupted message — current code does this; Task 1 locks it with tests.

### CRITICAL — Directionality (do not invert TX/RX)

Locked since Story 1.5 — unchanged:

| Product side | teVirtualMIDI | Data API |
|---|---|---|
| MT4 physical **IN** → host (DAW MIDI IN) | `PARSE_TX` + `INSTANTIATE_TX` | `virtualMIDISendData` via `SendToHost` (complete commands) |
| Host (DAW MIDI OUT) → MT4 physical **OUT** | `PARSE_RX` + `INSTANTIATE_RX` | create-port callback → `HostToDeviceSink` |

Prefer one complete MIDI message per `virtualMIDISendData` call (teVirtualMIDI guidance). Realtime must not be glued inside another command’s byte array when the Bridge emits to the host.

### CRITICAL — Computer Mode wake

`DeviceSession` already sends a channel CC kick so USB enters Computer Mode. **SysEx and realtime alone do not wake Computer Mode** (documented in session code). Do not remove the channel kick; do not assume clock-only traffic will open the device path on a cold session.

### Existing code being modified — current state

**After Epic 1 (done) + lab patches:**

- `MidiMessageFramer` — realtime `>= 0xF8` immediate emit; SysEx hold; channel/running status; `0xF1`/`0xF2`/`0xF3`/`0xF6` lengths present for later stories
- `DeviceSessionDeviceHost.cpp` — demux → per-IN `inFramers_[].Push` → `SendToHost`
- `DeviceSession` — host→device Encode+WriteBulk under `usbIoMutex_`; reader thread; Computer Mode kick
- `EmagicCableMapper` — opaque MIDI copy; F5 port switch; `0xFF` end-of-valid-data truncate
- `FramerSmoke` — partial note, two messages, running status — **no realtime vectors yet**
- `docs/tests/smoke-epic1-mt4.md` — notes/CC green; clock/MTC/SysEx explicitly out of Epic 1 scope

**What this story changes:**

- Synthetic proof that framer + mapper preserve F8/FA/FB/FC (including interleave)
- Hardware/DAW smoke documentation for clock/transport
- Code changes only where proof reveals defects or host→device needs a framer after lab evidence

**What must be preserved:**

- Notes/CC round-trip on all ports
- TX/RX mapping, Port N naming, Profile cable helpers
- Fail-closed VirtualMIDI missing
- Init×2 / finish / Computer Mode kick
- Layer isolation (Protocol free of VirtualMIDI/WinUSB)
- `builds/` output; user-session exe; no kernel driver

**Must not break:**

- `Bridge --test-mapper` / `--test-port-names`
- Windows CI without proprietary SDK in-repo
- macOS configure/build smoke
- Epic 1 hardware notes/CC path

### Known load / deferred edges (do not expand scope blindly)

From `deferred-work.md` and Story 1.6 review:

| Edge | Relevance to 2.1 |
|---|---|
| `processBulkRead` holds `usbIoMutex_` across decode | Dense 24 ppqn clock may amplify host→device stall — investigate **only if** dropouts appear |
| F5 / `0xFF` raw MIDI collision | Not in AC; do not invent escaping |
| CTRL_CLOSE_EVENT may kill before Stop | Pre-existing; not a clock blocker |
| Incomplete host→device spans | Unlikely with PARSE_RX; add host framer only if lab proves need |
| NFR-P1/P2 latency/jitter numbers | Epic **5** measurement — do not invent thresholds claims in this story |

### Technical requirements

- **Language:** C++17; Allman; 4 spaces; `#pragma once`; English — `conventions.md` §6
- **Quality:** `scripts/quality/lint-touched.py` SSOT on touched C++
- **Build:** prefer updating existing TUs; new Catch2 TU only if needed and listed in CMake
- **No** committed proprietary VirtualMIDI SDK; no French in sources

### Library / framework requirements

| Use | Do not use |
|---|---|
| Existing `MidiMessageFramer` + DeviceSession pump | New ClockEngine / message allowlist |
| Existing `EmagicCableMapper` opaque path | Reimplementing F5 for realtime |
| Ableton Live 12 or Reason Studios 12 for smoke | Claiming full SM-1 including MTC |
| ShowMIDI as optional observer | ShowMIDI alone as sole clock-slave proof if DAW available |
| `FramerSmoke` / Catch2 already in tree | New third-party test frameworks |
| English diagnostics with Port N / cable | Silent success when clock is dropped |

### File structure requirements

#### UPDATE (primary)

| Path | Current state | This story |
|---|---|---|
| `src/App/FramerSmoke.cpp` | Note/CC/running-status only | Add F8/FA/FB/FC (+ interleave / mid-SysEx) vectors |
| `src/Protocol/EmagicMapperSmokeSupport.cpp` (+ Catch2 mapper tests) | Notes/CC + F5/pad | Add realtime-in-span encode/decode vector(s) |
| `src/Protocol/MidiMessageFramer.cpp` (+ `.h` if needed) | Realtime already implemented | Fix only if tests/lab find defects |
| `docs/tests/smoke-epic1-mt4.md` | Clock out of scope | Light cross-link to Epic 2 clock doc |
| `CMakeLists.txt` | Bridge + BridgeTests | Only if a new test TU is added |

#### NEW

| Path | Why |
|---|---|
| `docs/tests/smoke-epic2-clock-mt4.md` | Hardware/DAW checklist for AC 1 + 3 |
| Optionally `tests/MidiMessageFramerTests.cpp` | If Catch2 coverage is preferred alongside CLI smoke |

#### Likely untouched

| Path | Why |
|---|---|
| `src/Midi/VirtualMidiBackend.*` | Already SendData + PARSE flags |
| `src/Profile/DeviceProfile.*` | No clock capability bit required |
| `src/Protocol/EmagicCableMapper.*` core | Opaque carry already correct |
| `src/Usb/WinUsbTransport.*` | Bulk path sufficient unless timeout issues under clock rate |
| `tools/midi-path-harness/` | Epic **5** |

#### Conditional UPDATE (lab-gated)

| Path | When |
|---|---|
| `src/Device/DeviceSession.cpp` / `DeviceSessionDeviceHost.cpp` | Host→device framer needed, or mutex/load fix for proven dropouts |
| `src/App/MidiSessionCli.cpp` | Optional English “clock smoke ready” wording |

### Testing requirements

| Check | How |
|---|---|
| Compile (macOS) | `cmake -S . -B builds/<cfg> && cmake --build builds/<cfg>` |
| Compile (Windows CI) | Existing workflow green |
| Framer synthetic | `Bridge --test-mapper` includes new realtime vectors |
| Mapper synthetic | Realtime bytes survive encode/decode on a product cable |
| Hardware clock (Windows) | WinUSB-bound MT4 + VirtualMIDI present → `--start-session`; Ableton **or** Reason: slave or observe clock + Start/Stop/Continue on ≥1 IN and ≥1 OUT |
| Short session | Normal short sequencing smoke without Bridge restart |
| Regression | Notes/CC still work (spot-check at least one IN + one OUT) |
| OS matrix | Win10 x64 documented (mandatory); Win11 when available |
| Isolation | Grep Protocol/Profile: no VirtualMIDI / WinUSB |
| Lint | `python scripts/quality/lint-touched.py` on touched C++ |

**Dropout definition for this story:** Bridge-induced loss or stall of clock/transport under a normal short sequencing smoke (not a multi-hour soak, not Epic 5 p99 thresholds). If the DAW loses sync or Start/Stop/Continue is missing while notes still flow, that is a fail.

Validation Matrix hosts: Ableton Live 12, Reason Studios 12 — **minimum for this story** is one of those DAWs. ShowMIDI may observe but does not replace DAW slave/observe for SM-1 clock portion.

### Previous story intelligence

From Story **1.6** (done):

- Continuous pump + framer landed; product acceptance for FR-7 was **explicitly deferred** to 2.1 — claim it here with smoke, do not re-build the pump
- Review patches fixed mutex scope around `SendToHost`, Stop races, CC smoke — preserve those invariants
- Deferred: CTRL_CLOSE before Stop; SysEx max length; F5/FF collision — still deferred
- Hardware notes/CC all-ports green (lab 2026-08-05) — baseline for clock work

From Story **1.4** / deferred-work:

- Emagic `0xFF` pad vs System Reset collision remains known — out of 2.1 AC
- Busy IN holding `usbIoMutex_` can stall host→device — watch under dense clock

From Epic 1 smoke doc / git:

- Boot Camp + Ableton path proven for notes/CC
- `MidiMessageFramer` was part of the P0 IN-mute fix (complete commands to VirtualMIDI)
- Recent commits: mapper smoke F5 align, Catch2 BridgeTests — extend those patterns

### Git intelligence summary

Recent relevant commits:

- `7934b75` — Align mapper smoke encode expectations with initial Out 1 F5
- `41cccdb` — Catch2 BridgeTests for Emagic mapper and DeviceProfile
- `204fc4a` — Record Epic 1 IN mute lab fix and Ableton smoke results
- `a4d5d6c` — Fix MT4 device-to-host MIDI over WinUSB bulk IN
- `9c61ae3` — Force initial Emagic F5 on first Out 1 encode

Patterns to extend: synthetic smoke in App + Catch2, flag-gated hardware paths, English lab docs under `docs/tests/`, no reinvented protocol layer.

### Latest tech information

- MIDI 1.0 System Real-Time (`0xF8`–`0xFF`) are single-byte and may appear **between** bytes of other messages; receivers must handle them without losing the interrupted message state
- Timing Clock (`0xF8`) is typically 24 pulses per quarter note — denser than notes/CC; short smoke must survive that rate without Bridge-induced gaps
- teVirtualMIDI: prefer complete commands per `virtualMIDISendData`; with PARSE_RX, host→device callbacks are usually complete units — verify under Ableton/Reason before adding a host framer
- Do not use ASIO buffer size as MIDI clock proof (SM-C2 / AD-11)
- Provisional latency/jitter anchors (≤4–5 ms / ≤1–2 ms p99) remain **unmeasured** until Epic 5 — do not publish them as proven in this story’s smoke doc

### Project context reference

- `conventions.md` §3 quality gate, §6 C++ style
- No `project-context.md` in-repo yet — follow architecture spine + conventions
- Keep hardware/protocol names in English (MT4, WinUSB, VirtualMIDI, MIDI, Ableton, Reason)

### Anti-patterns to forbid

- Building a ClockEngine or message-type allowlist
- Claiming MTC (2.2) or SysEx librarian (2.3/2.4) done
- Inventing Emagic `0xFF` escaping “for completeness”
- Preemptive host→device framer without lab evidence
- Building `tools/midi-path-harness/` or quoting Studio-Done latency numbers as proven
- Inverting TX/RX flags
- French comments; kebab-case sources under `src/`
- Committing proprietary VirtualMIDI SDK binaries
- Treating ShowMIDI-only observation as full DAW clock-slave proof when a matrix DAW is available

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Story 2.1, Epic 2]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-17, AD-11]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — FR-7, SM-1, Validation Matrix §10]
- [Source: `_bmad-output/specs/spec-unitor-win64-driver/validation-matrix.md` — DAW clock pass rules]
- [Source: `_bmad-output/implementation-artifacts/1-6-notes-and-cc-round-trip-on-all-ports.md` — pump + FR-7 fence]
- [Source: `_bmad-output/implementation-artifacts/deferred-work.md` — mutex load, F5/FF collision]
- [Source: `docs/tests/smoke-epic1-mt4.md` — Epic 1 notes/CC green; clock deferred]
- [Source: `src/Protocol/MidiMessageFramer.cpp` — `isRealtimeStatus`]
- [Source: `src/Device/DeviceSessionDeviceHost.cpp` — framer before SendToHost]
- [Source: `conventions.md` — §3 quality gate, §6 C++ standards]
- [Source: https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html — complete-command guidance]

## Dev Agent Record

### Agent Model Used

Composer (Cursor agent)

### Debug Log References

- macOS build: `builds/macos-smoke-2-1` — Bridge + BridgeTests green
- `Bridge --test-mapper` exit 0 (mapper + framer realtime vectors)
- `ctest` BridgeTests Pass
- `python scripts/quality/lint-touched.py` OK after splitting realtime mapper smoke to stay under file-size budget
- Isolation grep Protocol/Profile: OK after removing VirtualMIDI mention from MidiMessageFramer.h comment
- CMake: always compile `WinUsbBulkProbe.cpp` (non-Windows stub) so macOS Bridge links `--probe-usb`

### Completion Notes List

- Confirmed existing `MidiMessageFramer` already emits `status >= 0xF8` as immediate 1-byte messages without clearing running status or SysEx hold — no ClockEngine, no framer bugfix required
- Extended `FramerSmoke` with lone F8, FA/FB/FC, mid-note clock interleave, realtime-during-SysEx, and running-status-after-clock vectors
- Added encode (F8+FC cable 1) and decode (F8+FA cable 0) opaque carry vectors in `EmagicMapperRealtimeSmoke.cpp`; Catch2 cases wired; no message-type filters
- Host→device: kept Encode+WriteBulk only; smoke doc documents lab-gated host framer / mutex investigation — not added preemptively
- Documented hardware smoke in `docs/tests/smoke-epic2-clock-mt4.md`; light cross-link from Epic 1 smoke; skipped optional MidiSessionCli wording (not required for AC)
- Hardware DAW matrix rows remain for lab fill-in (Win10 mandatory); synthetic gate is green
- Code review 2026-08-05: Catch2 framer realtime vectors; mapper encode includes Continue; smoke Continue wording tightened; FramerSmoke transport diagnostic; status stays `review` until Win10 DAW Pass
- 2026-08-10 PC-only closeout: `scripts/lab/midi-clock-loopback-lab.py --with-bridge` Pass stamp `20260809T220838Z` (Out2→In2); story → `done`; Scarlett/DAW UAT deferred

### File List

- CMakeLists.txt
- src/App/FramerSmoke.cpp
- src/Protocol/EmagicMapperSmokeSupport.cpp
- src/Protocol/EmagicMapperSmokeSupport.h
- src/Protocol/EmagicMapperRealtimeSmoke.cpp
- src/Protocol/MidiMessageFramer.h
- tests/EmagicCableMapperTests.cpp
- tests/MidiMessageFramerTests.cpp
- docs/tests/smoke-epic2-clock-mt4.md
- docs/tests/smoke-epic1-mt4.md
- docs/tests/smoke-epic2-mt4.md
- scripts/lab/midi-clock-loopback-lab.py
- _bmad-output/implementation-artifacts/2-1-midi-clock-and-transport-realtime.md
- _bmad-output/implementation-artifacts/sprint-status.yaml

## Change Log

- 2026-08-05 — Story context created (ready-for-dev)
- 2026-08-05 — Implemented clock/transport synthetic proof + smoke docs; status → review
- 2026-08-05 — Code review patches: Catch2 framer CI vectors, Continue mapper+docs, FramerSmoke diagnostic; await Win10 lab
- 2026-08-10 — Win10 PC-only DIN loopback harness Pass (`midi-clock-loopback-lab.py` `20260809T220838Z`); status → done; Scarlett/DAW UAT deferred
