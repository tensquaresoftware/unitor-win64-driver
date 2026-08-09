---
baseline_commit: 4814f82
---

# Story 2.2: MTC quarter-frame and full-frame

Status: done

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a studio user syncing timecode,
I want MTC quarter-frame and full-frame messages used for sync to pass through the Bridge,
so that MIDI Time Code workflows work on the same MT4 Virtual Ports as performance MIDI.

## Acceptance Criteria

1. **Given** the clock/transport path from Story 2.1  
   **When** MTC quarter-frame (`0xF1` + data) and MTC full-frame (Universal Real Time SysEx `F0 7F … 01 01 hh mm ss ff F7`) messages used for sync are sent or observed through an MT4 Virtual Port in a Validation Matrix DAW  
   **Then** they are carried without Bridge-induced dropouts under the same class of short smoke as Story 2.1 — FR-7 / CAP-7

2. **And** MTC is treated as required V1 transport coverage (not optional polish) — AD-17

3. **And** failures identify the Virtual Port / cable in English diagnostics

**Traces:** FR-7, CAP-7, AD-17, SM-1 (MTC portion)

## Tasks / Subtasks

- [x] Task 1: Prove and harden device→host MTC framing (AC: 1, 2)
  - [x] Confirm `MidiMessageFramer` already frames `0xF1` as a 2-byte System Common message (`channelDataLength` returns 2) and already assembles short SysEx via the existing `0xF0`…`0xF7` hold path — do **not** invent an MtcEngine or second framer
  - [x] Extend `FramerSmoke` (invoked via `Bridge --test-mapper`) with synthetic vectors: lone quarter-frame (`0xF1` + one data nibble); a full 8-message quarter-frame sequence (message types 0–7); full-frame SysEx (`F0 7F 7F 01 01 hr mn sc fr F7`); quarter-frame interleaved mid-note; quarter-frame during an open SysEx hold that does **not** abort SysEx assembly; full-frame after / interleaved with Timing Clock without corrupting either
  - [x] Add matching Catch2 cases in `tests/MidiMessageFramerTests.cpp` (CI-enforced, same style as 2.1 realtime cases)
  - [x] If a framer bug is found for `0xF1` or short Universal Real Time SysEx, fix in `MidiMessageFramer` only; preserve note/CC/running-status/realtime (`>= 0xF8`) behavior from Epic 1 + 2.1

- [x] Task 2: Prove Emagic opaque carry of MTC bytes (AC: 1, 2)
  - [x] Add a dedicated `src/Protocol/EmagicMapperMtcSmoke.cpp` (mirror `EmagicMapperRealtimeSmoke.cpp` file-split — keep `EmagicMapperSmokeSupport.cpp` under ~400-line budget)
  - [x] Declare `runEmagicMapperSmokeEncodeMtc` / `runEmagicMapperSmokeDecodeMtc` in `EmagicMapperSmokeSupport.h`; wire both into `runAllEmagicMapperSmokeTests`
  - [x] Encode + decode vectors must include at least: one `0xF1 <data>` span and one full-frame SysEx on a product cable — prove F5 demux / FF truncate do not eat or corrupt those bytes
  - [x] Wire Catch2 one-liners in `tests/EmagicCableMapperTests.cpp`; add the new TU to **both** `Bridge` and `BridgeTests` source lists in `CMakeLists.txt`
  - [x] Do **not** add message-type allowlists or filters in DeviceSession / VirtualMidiBackend / EmagicCableMapper
  - [x] Do **not** invent Emagic escaping for `0xFF` System Reset vs pad collision — still deferred; MTC quarter-frame and full-frame payloads do not use `0xF5`/`0xFF` as MIDI data bytes in normal sync traffic

- [x] Task 3: Host→device path — verify, only harden if lab fails (AC: 1, 2)
  - [x] Default assumption: teVirtualMIDI `PARSE_RX` delivers complete System Common (`0xF1` + data) and complete SysEx full-frame units → existing `EncodeToDevice` + `WriteBulk` path is enough
  - [x] On Windows hardware smoke: confirm host → Virtual OUT → MT4 physical OUT → DIN loopback → Virtual IN carries quarter-frame + full-frame on **Out2→In2** (2026-08-09 `mtc-loopback-lab.py` after IN demux OUT-hint fix; Python harness accepted in lieu of Scarlett/DAW; future real-DAW UAT guide deferred)
  - [x] **Only if** lab shows incomplete host→device spans (split `0xF1` without data, truncated SysEx, or garbled full-frame): add a symmetric host→device framer (mirror device→host). Do **not** add it preemptively
  - [x] Dense quarter-frame short smoke (~125 QF/s, 72 frames + 1 full-frame) — 72/72 QF + full-frame Pass; no mutex fix required this turn

- [x] Task 4: Document hardware MTC smoke (AC: 1, 3)
  - [x] Add `docs/tests/smoke-epic2-mtc-mt4.md` (kebab-case) with checklist: ≥1 IN + ≥1 OUT; Ableton Live 12 **or** Reason Studios 12; observe/slave quarter-frame + full-frame used for sync; short session without Bridge restart; English failure notes (Port N / cable / direction)
  - [x] Win10 x64 = mandatory matrix row; Win11 x64 = document when hardware available (same pattern as `smoke-epic2-clock-mt4.md`)
  - [x] Cross-link lightly from `docs/tests/smoke-epic2-clock-mt4.md` (clock stays 2.1; MTC proof lives here) and from `docs/tests/smoke-epic1-mt4.md` if still pointing only at clock for Epic 2
  - [x] Explicitly state: full-frame MTC is SysEx-shaped but **this story’s AC is MTC sync only** — Matrix-Control / librarian SysEx remains **2.3** / **2.4**

- [x] Task 5: Quality + anti-scope (AC: all)
  - [x] `python scripts/quality/lint-touched.py` exits 0 on the C++ diff
  - [x] Compile: `cmake -S . -B builds/<config> && cmake --build builds/<config>` (macOS smoke OK; Windows CI remains the gate)
  - [x] `Bridge --test-mapper` still exit 0 (includes new framer + mapper MTC vectors)
  - [x] `ctest` / `BridgeTests` Pass with new Catch2 cases
  - [x] Grep isolation: no `teVirtualMIDI` / `VirtualMIDI` / `winusb` under `src/Protocol/` or `src/Profile/`
  - [x] Confirm no French in sources; no SysEx librarian / Matrix-Control claim (→ **2.3**/**2.4**); no ~4h longevity claim (→ **2.5**); no MIDI Path harness (→ Epic **5**); no installer / Auto-Start / hot-plug work

### Review Findings

- [x] [Review][Patch] Nested 0xF1 while outer incomplete 0xF1 starts interrupt instead of replace — can emit a spurious extra quarter-frame [src/Protocol/MidiMessageFramer.cpp:103]
- [x] [Review][Patch] Task 3 hardware-confirm / dense-rate subtasks marked [x] without Win10 lab evidence — closed 2026-08-09 via `mtc-loopback-lab.py` on real Out2→In2 after Emagic IN demux OUT-hint fix (`hintInCableFromOut`); 72/72 QF + full-frame; DAW/Scarlett UAT deferred
- [x] [Review][Patch] Add running-status + quarter-frame interleave coverage (smoke + Catch2) [src/App/FramerMtcSmoke.cpp]
- [x] [Review][Patch] Add Timing Clock between 0xF1 and its data byte coverage (smoke + Catch2) [src/App/FramerMtcSmoke.cpp]
- [x] [Review][Patch] Remove unused interruptLen_ bookkeeping [src/Protocol/MidiMessageFramer.h:46]
- [x] [Review][Patch] Soften header comment equating QF interrupt with one-byte realtime [src/Protocol/MidiMessageFramer.h:42]
- [x] [Review][Defer] Duplicated MTC vectors in FramerMtcSmoke vs MidiMessageFramerTests — deferred, pattern parity with 2.1; consolidate harnesses later

## Dev Notes

### Soft dependency on Story 2.1

Story 2.1 remains **`review`** for formal English checklist / DAW sign-off (operator guide §3 already ✅). Story 2.2 hardware closeout used a Python DIN-loopback harness on physical **Out2→In2** (2026-08-09) after fixing Emagic IN demux mis-attribution (unlabeled echo stuck on In 1). Real-DAW + Scarlett UAT is deferred to a later dedicated UAT guide.

### Scope fence

This story lands the **FR-7 / CAP-7 MTC** portion of AD-17 / SM-1. Story 2.1 already proved Timing Clock + Start/Stop/Continue. Epic 1 already ships a transparent USB ↔ VirtualMIDI pump. **Primary deliverable is product proof + synthetic hardening + smoke docs** — not a greenfield MTC parser or timecode engine.

| In scope | Out of scope (later stories) |
|---|---|
| Prove MTC quarter-frame (`0xF1` + data) both directions under short DAW smoke | Transparent SysEx librarian / burst buffering → **2.3** |
| Prove MTC full-frame SysEx used for sync (`F0 7F … 01 01 … F7`) both directions | Matrix-Control minimum SysEx vectors → **2.4** |
| Framer/mapper synthetic vectors for QF + full-frame (+ interleave with notes/clock) | ~4h longevity design → **2.5** |
| Treat Bridge-induced MTC dropouts as defects | Auto-Start → **3.1**; hot-plug → **3.2** |
| Document Win10 (mandatory) / Win11 (when available) smoke | Multi-client DAW+ShowMIDI → **3.3**; multi-MT4 → **3.4** |
| Fix framer/pump only if smoke fails | Public Installer / MSI → **4.1** / OQ-1 |
| | MIDI Path latency/jitter harness + Studio-Done thresholds → Epic **5** / AD-11 |
| | `0xFF` System Reset vs Emagic pad escaping — still deferred |
| | SMPTE user-bits SysEx / MMC / MTC cueing variants beyond full-frame sync — not required by AC |
| | Building an MTC generator/decoder UI or SMPTE display — Bridge carries bytes only |

### Epic context

Epic 2 outcome: Validation Matrix DAWs can use **clock / Start-Stop-Continue / MTC** through the Bridge, and Matrix-Control can complete minimum SysEx pass vectors — with buffering designed for ~4h sessions.

Story **2.2** extends the same path as 2.1 to MTC. Full-frame is SysEx-shaped and therefore **touches** the AD-16 transparent-SysEx pipe, but this story only claims **MTC sync messages** — not librarian buffering or Matrix-Control.

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
| AD-16 | Full-frame rides transparent SysEx carry — no Emagic-side framing of MTC payloads; do **not** claim Matrix-Control librarian done |
| AD-17 | MTC quarter-frame + full-frame are **required** V1 transport — dropouts = defect |
| AD-20 | Remain user-session Bridge exe |
| Structural Seed | Framing stays in `Protocol/`; pump in `Device/`; VirtualMIDI I/O in `Midi/` — no new MTC module |

[Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-16, AD-17, pipeline]

### CRITICAL — Reuse, do not reinvent

Already in tree (Epic 1 + Story 2.1):

1. Bidirectional USB ↔ VirtualMIDI byte pump (`DeviceSession` + `VirtualMidiBackend`)
2. Opaque Emagic encode/decode (no message-type filter)
3. `MidiMessageFramer` with `0xF1` length-2 System Common path + SysEx hold (`kMaxSysexHoldBytes = 1024`)
4. Realtime interleave proven for `>= 0xF8` (must remain green)
5. Smoke patterns: `FramerSmoke`, `EmagicMapperRealtimeSmoke.cpp`, Catch2 `MidiMessageFramerTests` / `EmagicCableMapperTests`, `docs/tests/smoke-epic2-clock-mt4.md`
6. Long-run CLI `--start-session` / `--run-midi` for hardware smoke

**Forbidden reinventing:**

- A new `MtcEngine` / `TimecodeFilter` / message-type allowlist module
- A second I/O pump beside DeviceSession
- Replacing VirtualMIDI with Windows MIDI Services
- Claiming SysEx librarian / Matrix-Control (2.3/2.4) done because full-frame SysEx flowed
- Parsing SMPTE fields in production code beyond what tests need for fixed byte vectors

### CRITICAL — Message map (this story)

| Bytes | Name | Device→host framer today | Host→device today | Product AC 2.2 |
|---|---|---|---|---|
| `0xF1 <nn>` | MTC quarter-frame (System Common, 2 bytes) | Length-2 path exists; **unproven for product** | Opaque encode | **Required** |
| `F0 7F cc 01 01 hr mn sc fr F7` | MTC full-frame (Universal Real Time SysEx, 10 bytes) | SysEx hold path exists; **unproven for MTC product** | Opaque encode | **Required** |
| `0xF8` / `0xFA`/`0xFB`/`0xFC` | Clock / transport | Proven in 2.1 | Opaque | Regression — must stay green |
| Other SysEx (Matrix / librarian) | — | Assembled | Opaque | **Out → 2.3+** |
| `0xFF` | System Reset / Emagic pad | Framer would emit | Decode truncates at first `0xFF` | **Out / deferred collision** |

Canonical layouts (do not invent variants in production):

- **Quarter-frame:** status `0xF1`, data byte `0nnndddd` where `nnn` is message type 0–7 and `dddd` is a time nibble. A complete SMPTE time needs eight QF messages (types 0→7 forward). Rate ≈ 4× SMPTE frame rate (denser than notes; similar class of short-smoke stress as 24 ppqn clock).
- **Full-frame:** `F0 7F <device-id> 01 01 hr mn sc fr F7` — typically `device-id = 0x7F` (broadcast) in DAW sync cues. `hr` encodes hours + frame-rate type bits. Used for locate/cue when time is not free-running.

MIDI 1.0 allows System Common and SysEx to interleave with realtime. Framer must emit complete `0xF1` commands and complete full-frame SysEx without aborting interrupted note/running-status/SysEx state incorrectly — Task 1 locks this with tests.

### CRITICAL — Directionality (do not invert TX/RX)

Locked since Story 1.5 — unchanged:

| Product side | teVirtualMIDI | Data API |
|---|---|---|
| MT4 physical **IN** → host (DAW MIDI IN) | `PARSE_TX` + `INSTANTIATE_TX` | `virtualMIDISendData` via `SendToHost` (complete commands) |
| Host (DAW MIDI OUT) → MT4 physical **OUT** | `PARSE_RX` + `INSTANTIATE_RX` | create-port callback → `HostToDeviceSink` |

Prefer one complete MIDI message per `virtualMIDISendData` call. Quarter-frame must be emitted as a **2-byte** command (not glued inside another message). Full-frame must be one complete SysEx per send.

### CRITICAL — Computer Mode wake

`DeviceSession` already sends a channel CC kick so USB enters Computer Mode. **SysEx and realtime alone do not wake Computer Mode**. Do not remove the channel kick; do not assume MTC-only traffic will open the device path on a cold session.

### Existing code being modified — current state

**After Epic 1 + Story 2.1 (synthetic green; 2.1 hardware lab pending):**

- `MidiMessageFramer` — `channelDataLength(0xF1) == 2`; SysEx hold for `0xF0`…`0xF7`; realtime `>= 0xF8` immediate emit; **no dedicated MTC vectors yet**
- `DeviceSessionDeviceHost.cpp` — demux → per-IN `inFramers_[].Push` → `SendToHost`
- `DeviceSession` — host→device Encode+WriteBulk under `usbIoMutex_`; reader thread; Computer Mode kick
- `EmagicCableMapper` — opaque MIDI copy; F5 port switch; `0xFF` end-of-valid-data truncate — **no MTC-specific logic (correct)**
- `FramerSmoke` — note/CC + clock/transport interleave from 2.1 — **no `0xF1` / full-frame vectors yet**
- `EmagicMapperRealtimeSmoke.cpp` — F8/FA/FB/FC opaque carry — **template for MTC smoke TU**
- `docs/tests/smoke-epic2-clock-mt4.md` — explicitly fences MTC → story 2.2

**What this story changes:**

- Synthetic proof that framer + mapper preserve quarter-frame + full-frame (including interleave with notes/clock)
- Hardware/DAW smoke documentation for MTC
- Code changes only where proof reveals defects or host→device needs a framer after lab evidence

**What must be preserved:**

- Notes/CC round-trip on all ports
- Clock + Start/Stop/Continue synthetic vectors and (when lab-filled) 2.1 smoke path
- TX/RX mapping, Port N naming, Profile cable helpers
- Fail-closed VirtualMIDI missing
- Init×2 / finish / Computer Mode kick
- Layer isolation (Protocol free of VirtualMIDI/WinUSB)
- `builds/` output; user-session exe; no kernel driver

**Must not break:**

- `Bridge --test-mapper` / `--test-port-names`
- Existing Catch2 realtime + mapper cases from 2.1
- Windows CI without proprietary SDK in-repo
- macOS configure/build smoke
- Epic 1 hardware notes/CC path

### Known load / deferred edges (do not expand scope blindly)

From `deferred-work.md` and Story 2.1 review:

| Edge | Relevance to 2.2 |
|---|---|
| `processBulkRead` holds `usbIoMutex_` across decode | Dense QF (~4× frame rate) may amplify host→device stall — investigate **only if** dropouts appear |
| SysEx over 1024 bytes silently dropped | Irrelevant for 10-byte full-frame; do not “fix” observability here (→ Epic 2 SysEx stories) |
| `SendToHost` vs teVirtualMIDI max SysEx length | Full-frame is tiny; revisit with **2.3** if librarian sizes matter |
| F5 / `0xFF` raw MIDI collision | Not in AC; do not invent escaping |
| Incomplete host→device spans | Unlikely with PARSE_RX; add host framer only if lab proves need |
| NFR-P1/P2 latency/jitter numbers | Epic **5** measurement — do not invent thresholds claims in this story’s smoke doc |

### Technical requirements

- **Language:** C++17; Allman; 4 spaces; `#pragma once`; English — `conventions.md` §6
- **Quality:** `scripts/quality/lint-touched.py` SSOT on touched C++
- **File size:** Prefer new `EmagicMapperMtcSmoke.cpp` over growing `EmagicMapperSmokeSupport.cpp` past ~400 useful lines (same split as 2.1 realtime)
- **Build:** list new Protocol smoke TU in **both** Bridge and BridgeTests in `CMakeLists.txt`
- **No** committed proprietary VirtualMIDI SDK; no French in sources

### Library / framework requirements

| Use | Do not use |
|---|---|
| Existing `MidiMessageFramer` + DeviceSession pump | New MtcEngine / message allowlist |
| Existing `EmagicCableMapper` opaque path | Reimplementing F5 for MTC |
| Ableton Live 12 or Reason Studios 12 for smoke | Claiming full SM-1 SysEx librarian |
| ShowMIDI as optional observer / loopback helper | ShowMIDI alone as sole MTC-slave proof if DAW available |
| `FramerSmoke` / Catch2 already in tree | New third-party test frameworks |
| English diagnostics with Port N / cable | Silent success when MTC is dropped |
| Fixed byte vectors for QF + full-frame | Runtime SMPTE math library |

### File structure requirements

#### UPDATE (primary)

| Path | Current state | This story |
|---|---|---|
| `src/App/FramerSmoke.cpp` | Note/CC + clock/transport | Add QF + full-frame (+ interleave) vectors; fold into `runFramerTests()` |
| `src/Protocol/EmagicMapperSmokeSupport.h` | Declares clock encode/decode | Declare MTC encode/decode runners |
| `src/Protocol/EmagicMapperSmokeSupport.cpp` | `runAll…` calls clock runners | Wire MTC runners into `runAllEmagicMapperSmokeTests` |
| `src/Protocol/MidiMessageFramer.cpp` (+ `.h` if needed) | `0xF1` + SysEx already implemented | Fix only if tests/lab find defects |
| `tests/MidiMessageFramerTests.cpp` | Realtime cases from 2.1 | Add QF + full-frame cases |
| `tests/EmagicCableMapperTests.cpp` | Clock/transport wrappers | Add MTC encode/decode wrappers |
| `docs/tests/smoke-epic2-clock-mt4.md` | MTC out of scope | Light cross-link to MTC smoke doc |
| `docs/tests/smoke-epic1-mt4.md` | Points Epic 2 at clock | Ensure MTC doc also discoverable |
| `CMakeLists.txt` | Bridge + BridgeTests list RealtimeSmoke | Add `EmagicMapperMtcSmoke.cpp` to **both** executables |

#### NEW

| Path | Why |
|---|---|
| `src/Protocol/EmagicMapperMtcSmoke.cpp` | MTC encode/decode synthetic vectors (file-size split) |
| `docs/tests/smoke-epic2-mtc-mt4.md` | Hardware/DAW checklist for AC 1 + 3 |

#### Likely untouched

| Path | Why |
|---|---|
| `src/Midi/VirtualMidiBackend.*` | Already SendData + PARSE flags |
| `src/Profile/DeviceProfile.*` | No MTC capability bit required |
| `src/Protocol/EmagicCableMapper.*` core | Opaque carry already correct |
| `src/Usb/WinUsbTransport.*` | Bulk path sufficient unless timeout issues under QF rate |
| `tools/midi-path-harness/` | Epic **5** |

#### Conditional UPDATE (lab-gated)

| Path | When |
|---|---|
| `src/Device/DeviceSession.cpp` / `DeviceSessionDeviceHost.cpp` | Host→device framer needed, or mutex/load fix for proven dropouts |
| `src/App/MidiSessionCli.cpp` | Optional English “MTC smoke ready” wording — only if within lint CCN |

### Testing requirements

| Check | How |
|---|---|
| Compile (macOS) | `cmake -S . -B builds/<cfg> && cmake --build builds/<cfg>` |
| Compile (Windows CI) | Existing workflow green |
| Framer synthetic | `Bridge --test-mapper` includes new QF + full-frame vectors |
| Mapper synthetic | `0xF1` + full-frame SysEx survive encode/decode on a product cable |
| Catch2 | `BridgeTests` / `ctest` include new MTC cases |
| Hardware MTC (Windows) | WinUSB-bound MT4 + VirtualMIDI present → `--start-session`; Ableton **or** Reason: send/observe quarter-frame + at least one full-frame locate/cue on ≥1 IN and ≥1 OUT |
| Short session | Normal short sync smoke without Bridge restart |
| Regression | Notes/CC still work; clock/transport synthetic vectors still pass |
| OS matrix | Win10 x64 documented (mandatory); Win11 when available |
| Isolation | Grep Protocol/Profile: no VirtualMIDI / WinUSB |
| Lint | `python scripts/quality/lint-touched.py` on touched C++ |

**Dropout definition for this story:** Bridge-induced loss or stall of MTC quarter-frame / full-frame under a normal short sync smoke (not a multi-hour soak, not Epic 5 p99 thresholds). If the DAW loses timecode lock or full-frame cues never arrive while notes still flow, that is a fail.

Validation Matrix hosts: Ableton Live 12, Reason Studios 12 — **minimum for this story** is one of those DAWs exercising MTC send and/or slave/observe. ShowMIDI may observe bytes but does not replace DAW MTC proof for SM-1 when a matrix DAW is available.

**DAW tip (lab):** Prefer a port dedicated to MTC when the DAW allows — MTC can be chatty; Bridge must still carry it on a shared Virtual Port without dropouts under short smoke.

### Previous story intelligence

From Story **2.1** (`review`, synthetic green):

- Pattern that worked: prove existing framer behavior with `FramerSmoke` + Catch2; opaque mapper vectors in a **separate** `EmagicMapperRealtimeSmoke.cpp`; hardware checklist in `docs/tests/smoke-epic2-clock-mt4.md`; no ClockEngine
- Explicit fence: MTC was out → **this story**
- Review kept status `review` until Win10 DAW Pass — same honesty bar for MTC hardware rows (synthetic alone ≠ done)
- Deferred dense-clock mutex stall — same edge for dense QF
- Anti-patterns still binding: no allowlists, no preemptive host framer, no F5/FF escaping, no SysEx librarian claim

From Story **1.6** / deferred-work:

- Continuous pump + framer landed; FR-7 MTC deferred to Epic 2
- SysEx max-length / observability deferred — full-frame is tiny, safe for this story
- Hardware notes/CC all-ports green (lab 2026-08-05) — baseline for MTC lab

### Git intelligence summary

Recent relevant commits:

- `4814f82` — Prove MIDI clock and transport realtime for story 2.1
- `7934b75` — Align mapper smoke encode expectations with initial Out 1 F5
- `41cccdb` — Catch2 BridgeTests for Emagic mapper and DeviceProfile
- `204fc4a` — Record Epic 1 IN mute lab fix and Ableton smoke results

Patterns to extend: split Protocol smoke TUs by topic, Catch2 thin wrappers over shared smoke runners, English lab docs under `docs/tests/`, prove-don’t-reinvent.

### Latest tech information

- MTC quarter-frame is System Common (`0xF1` + 1 data byte), **not** realtime (`>= 0xF8`) — do not fold QF into the realtime immediate-emit path; use the existing length-2 System Common path
- Eight quarter-frames encode one complete SMPTE time (updated every two frames); forward play uses types 0→7
- Full-frame is Universal Real Time SysEx: `F0 7F <id> 01 01 hr mn sc fr F7` (typically 10 bytes) — well under `kMaxSysexHoldBytes` (1024)
- Full-frame is for locate/cue; free-running sync uses quarter-frames — both are **required** by AD-17 / FR-7 for V1
- teVirtualMIDI: prefer complete commands per `virtualMIDISendData`; with PARSE_RX, host→device callbacks are usually complete units — verify under Ableton/Reason before adding a host framer
- Do not use ASIO buffer size as MTC proof (SM-C2 / AD-11)
- Provisional latency/jitter anchors remain **unmeasured** until Epic 5 — do not publish them as proven in this story’s smoke doc

### Project context reference

- `conventions.md` §3 quality gate, §6 C++ style
- No `project-context.md` in-repo yet — follow architecture spine + conventions
- Keep hardware/protocol names in English (MT4, WinUSB, VirtualMIDI, MIDI, MTC, Ableton, Reason)

### Anti-patterns to forbid

- Building an MtcEngine, TimecodeDecoder, or message-type allowlist
- Claiming SysEx librarian / Matrix-Control (2.3/2.4) done because full-frame flowed
- Treating full-frame as “already covered by 2.3” and skipping it here — AD-17 requires MTC in this story
- Inventing Emagic `0xFF` escaping “for completeness”
- Preemptive host→device framer without lab evidence
- Building `tools/midi-path-harness/` or quoting Studio-Done latency numbers as proven
- Inverting TX/RX flags
- French comments; kebab-case sources under `src/`
- Committing proprietary VirtualMIDI SDK binaries
- Treating ShowMIDI-only observation as full DAW MTC proof when a matrix DAW is available
- Growing `EmagicMapperSmokeSupport.cpp` instead of a dedicated MTC smoke TU

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Story 2.2, Epic 2]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-16, AD-17, AD-11]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — FR-7, SM-1, Validation Matrix §10]
- [Source: `_bmad-output/implementation-artifacts/2-1-midi-clock-and-transport-realtime.md` — prove pattern, scope fence, review honesty]
- [Source: `_bmad-output/implementation-artifacts/deferred-work.md` — mutex load, SysEx observability, F5/FF]
- [Source: `docs/tests/smoke-epic2-clock-mt4.md` — clock checklist template; MTC out of scope]
- [Source: `src/Protocol/MidiMessageFramer.cpp` — `channelDataLength` `0xF1`, SysEx hold]
- [Source: `src/Protocol/EmagicMapperRealtimeSmoke.cpp` — file-split template for MTC smoke]
- [Source: `src/Device/DeviceSessionDeviceHost.cpp` — framer before SendToHost]
- [Source: `conventions.md` — §3 quality gate, §6 C++ standards]
- [Source: http://midi.teragonaudio.com/tech/mtc.htm — quarter-frame + full-frame layouts]
- [Source: https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html — complete-command guidance]

## Dev Agent Record

### Agent Model Used

Composer (Cursor agent router)

### Debug Log References

- Framer mid-note / mid-SysEx `0xF1` aborted outer assembly before fix → nested quarter-frame interrupt path in `MidiMessageFramer`
- Mapper encode smoke: second encode on same cable omitted F5 → separate mapper instances per encode vector

### Completion Notes List

- Synthetic MTC proof green: FramerSmoke + FramerMtcSmoke + Catch2 + EmagicMapperMtcSmoke; `Bridge --test-mapper` exit 0; `ctest` Pass; `lint-touched.py` exit 0; Protocol/Profile isolation grep clean
- Hardened `MidiMessageFramer` so MTC quarter-frame can interrupt an incomplete note or open SysEx without aborting them (product parity with realtime interleave); idle `0xF1` and full-frame SysEx still use existing length-2 / SysEx-hold paths
- No host→device framer added (lab-gated); assumption documented in `docs/tests/smoke-epic2-mtc-mt4.md`
- Hardware Win10 DAW matrix rows left blank for lab fill-in (same honesty bar as story 2.1)
- No MtcEngine / allowlist / Matrix-Control / librarian / Epic 5 harness

### File List

- `CMakeLists.txt`
- `src/App/FramerSmoke.cpp`
- `src/App/FramerMtcSmoke.cpp` (new)
- `src/Protocol/MidiMessageFramer.h`
- `src/Protocol/MidiMessageFramer.cpp`
- `src/Protocol/EmagicMapperSmokeSupport.h`
- `src/Protocol/EmagicMapperSmokeSupport.cpp`
- `src/Protocol/EmagicMapperMtcSmoke.cpp` (new)
- `tests/MidiMessageFramerTests.cpp`
- `tests/EmagicCableMapperTests.cpp`
- `docs/tests/smoke-epic2-mtc-mt4.md` (new)
- `docs/tests/smoke-epic2-clock-mt4.md`
- `docs/tests/smoke-epic1-mt4.md`
- `_bmad-output/implementation-artifacts/sprint-status.yaml`
- `_bmad-output/implementation-artifacts/2-2-mtc-quarter-frame-and-full-frame.md`

## Change Log

- 2026-08-05 — Story context created (ready-for-dev)
- 2026-08-05 — Implemented MTC quarter-frame / full-frame synthetic proof + framer interrupt harden + smoke doc; status → review
- 2026-08-05 — Code review patches: nested-F1 replace, running-status/QF + clock-between-F1 tests, interruptLen_ removal, Task 3 lab honesty; status remains review (Win10 MTC lab pending)
- 2026-08-09 — Win10 MTC hardware Pass via `scripts/lab/mtc-loopback-lab.py` on Out2→In2 after `EmagicCableMapper::hintInCableFromOut` (qf 72/72 + full-frame); status → done; DAW/Scarlett UAT deferred

