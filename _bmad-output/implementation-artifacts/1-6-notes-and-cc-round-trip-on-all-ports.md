---
baseline_commit: 49b78a0
---

# Story 1.6: Notes and CC round-trip on all ports

Status: done

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a studio user,
I want notes and CC to flow both ways between MT4 physical cables and the Virtual Ports,
so that I can play and automate through the Bridge on every IN/OUT.

## Acceptance Criteria

1. **Given** Virtual Ports are live with a connected MT4 (Stories 1.3–1.5)  
   **When** notes and CC are sent/received on each of the 2 IN and 4 OUT ports  
   **Then** round-trips succeed for Validation Matrix hosts that are available in the test loop (at minimum one DAW or MIDI-OX smoke) — FR-6 / CAP-6

2. **And** common channel/system messages required for basic playability are carried (not notes-only stubs that drop obvious channel traffic) — FR-6 / AD-17 (channel portion)

3. **And** no Bridge restart is required for sustained note/CC smoke of a normal short session

4. **And** failures are diagnosable via English logs (which port/cable failed)

**Traces:** FR-6, CAP-6, AD-17 (channel portion), SM-1 (notes/CC foundation)

## Tasks / Subtasks

- [x] Task 1: Complete VirtualMIDI data path in `VirtualMidiBackend` (AC: 1, 2, 4)
  - [x] Extend `TeVirtualMidiApi.h` with runtime decls for `virtualMIDISendData` (and keep existing create/close + callback typedef). Do **not** commit proprietary SDK binaries
  - [x] Load `virtualMIDISendData` via `GetProcAddress` in `ensureApiLoaded`; fail closed with English fix path if export missing
  - [x] Implement `SendToHost(inPortIndex, midiBytes, byteCount, errorOut)` using `virtualMIDISendData` on the matching **IN** handle (`inPorts_[inPortIndex]`). Reject OOB index / null / empty / ports-not-created with English `errorOut` naming the port index
  - [x] On **OUT** port create, pass a real `TeVmMidiDataCallback` (not `nullptr`) + cookie that identifies `this` and `outPortIndex`. Callback invokes `HostToDeviceSink` when set
  - [x] **CRITICAL:** never call `virtualMIDIClosePort` / `DestroyPortSet` from inside the data callback (deadlock risk per teVirtualMIDI docs)
  - [x] Keep Story 1.5 TX/RX mapping: product IN = `INSTANTIATE_TX` (Bridge → host apps); product OUT = `INSTANTIATE_RX` (host apps → Bridge)
  - [x] Non-Windows stubs: `SendToHost` fails closed; `SetHostToDeviceSink` may still store pointers for compile symmetry

- [x] Task 2: Wire DeviceSession continuous USB ↔ MIDI pump (AC: 1, 2, 3, 4)
  - [x] UPDATE `DeviceSession` — after successful `CreatePortSet`, register `SetHostToDeviceSink` and start a **device→host** reader loop (dedicated thread on Windows)
  - [x] **Host → device:** sink receives `(outPortIndex, midiBytes)` → map index → Emagic cable via `collectProductCableIndices(profile.outCables)` → `EmagicCableMapper::EncodeToDevice` → `WinUsbTransport::WriteBulk`. English log on failure including Port N / cable index
  - [x] **Device → host:** `ReadBulk` → `DecodeFromDevice` with `MidiCableSink` → map product IN cable → `inPortIndex` → `MidiBackend::SendToHost`. Ignore / do not forward non-product cables (mapper already filters; do not invent Broadcast Virtual Ports)
  - [x] Own cable↔port tables from Profile helpers only — **do not** hard-code `{0,1}` / `{0,1,2,3}` literals outside Profile reuse
  - [x] Stop / destructor sequence (normative): signal pump stop → join reader thread → clear host→device sink → `DestroyPortSet` → Emagic finish → transport `Close`. Must not leak threads or leave ports after Stop
  - [x] Address Story 1.4 deferred I/O hazards **enough to run safely**: set a bulk IN `PIPE_TRANSFER_TIMEOUT` (or equivalent cancel path) so Stop can wake a blocked `ReadBulk`; do not Close while the reader thread still owns in-flight reads without a join
  - [x] Preserve AD-2: Protocol stays free of VirtualMIDI / WinUSB; Midi stays free of Emagic F5; Usb stays free of port naming
  - [x] Do **not** claim clock / MTC / SysEx Validation Matrix proof (Epic 2) — raw byte path may carry them, but product acceptance for FR-7 / FR-8 is out of scope

- [x] Task 3: App CLI long-running MIDI smoke path (AC: 1, 3, 4)
  - [x] Extend `--start-session` (or add `--run-midi` kept behind the same Start wiring) so that after Start succeeds, the process **stays alive** with I/O running until Ctrl+C / console cancel, then Stop cleanly
  - [x] Print English diagnostics at start: 2 IN / 4 OUT expected names, “MIDI I/O running — notes/CC smoke ready”, and on Stop “MIDI I/O stopped”
  - [x] On pump failures that kill the session, print which Port N / cable / direction failed and exit non-zero
  - [x] Keep prior flags working: `--test-mapper`, `--test-port-names`, `--open-device`, `--dev-zadig`
  - [x] Optional synthetic helper (no hardware): assert IN/OUT port-index ↔ cable-index tables for MT4 (`0→0,1→1` IN; `0..3→0..3` OUT) if it keeps Main CCN under lint budget — otherwise document mapping in Dev Notes only and cover via mapper + hardware smoke

- [x] Task 4: Quality + anti-scope (AC: all)
  - [x] `python scripts/quality/lint-touched.py` exits 0 on the C++ diff
  - [x] Compile: `cmake -S . -B builds/<config> && cmake --build builds/<config>` (macOS smoke OK; Windows CI remains the gate)
  - [x] Grep isolation: no `teVirtualMIDI` / `VirtualMIDI` / `winusb` under `src/Protocol/` or `src/Profile/`
  - [x] Confirm no French in sources; no GPL trees; no Windows MIDI Services backend; no Public Installer / MSI; no `tools/midi-path-harness/` work; no Auto-Start / hot-plug product claims

## Dev Notes

### Scope fence

This story lands **FR-6 / CAP-6** (notes, CC, common channel/system MIDI) and the **SM-1 notes/CC foundation**. It is the last Epic 1 story: after it, a contributor on Windows can play and automate through all MT4 Virtual Ports.

| In scope | Out of scope (later stories) |
|---|---|
| Continuous USB ↔ VirtualMIDI byte pump | MIDI clock / Start-Stop-Continue product proof → **2.1** |
| `SendToHost` + host→device callback path | MTC quarter/full frame product proof → **2.2** |
| Notes + CC + common channel/system on **all** 2 IN + 4 OUT | Transparent SysEx / Matrix-Control vectors → **2.3** / **2.4** |
| Short session without Bridge restart | ~4h longevity design → **2.5** |
| English diagnostics with Port N / cable | Auto-Start → **3.1**; hot-plug → **3.2** |
| Safe Stop vs blocked `ReadBulk` (timeout/join) | Multi-client DAW+MIDI-OX acceptance → **3.3** |
| | Multi-MT4 `MT4 #K` proof → **3.4** |
| | Public Installer / MSI → **4.1** / OQ-1 |
| | MIDI Path latency harness → Epic **5** / AD-11 |
| | Windows MIDI Services backend → post-V1 |

### Epic context

Epic 1 outcome: bind MT4 to WinUSB, run C++17 Bridge, see stable `MT4 Port N` (2 IN / 4 OUT), **exchange notes/CC** — without a custom kernel driver.  
Stories 1.2–1.5 shipped profile, GUID-first WinUSB, DeviceSession + Emagic mapper, and Virtual Port create/destroy. Story **1.6** connects the pipeline so MIDI bytes actually move.

Pipeline (architecture spine):

```text
WinUsbTransport ↔ EmagicCableMapper ↔ MidiBackend (VirtualMidiBackend) ↔ Virtual Ports ↔ DAW/MIDI-OX
```

### Architecture compliance (must follow)

| Decision | What it means for this story |
|---|---|
| AD-1 | All Emagic multiplex stays usermode; no custom kernel MIDI |
| AD-2 | Protocol / Profile free of VirtualMIDI + WinUSB; Midi free of F5; composition root wires backends |
| AD-3 | Port N / cable indices from Profile `collectProductCableIndices` only |
| AD-4 | One session / one pump / one port set per MT4 |
| AD-5 / AD-6 | Do not rename ports; names already come from DeviceSessionManager |
| AD-7 | Data path uses teVirtualMIDI SDK APIs (`SendData` + create callback); still runtime LoadLibrary |
| AD-8 | Do not add exclusive-open policy (full concurrent-host proof = 3.3) |
| AD-9 | Only live DeviceSession owns ports + I/O lifecycle |
| AD-14 | No GPL `midi.c` paste; F5 behavior already in EmagicCableMapper |
| AD-15 | PascalCase under `src/`; lint-touched; English only |
| AD-17 | Channel portion in-scope (notes/CC/common); clock/MTC acceptance deferred to Epic 2 |
| AD-20 | Remain user-session Bridge exe |
| Structural Seed | Pump lives in `Device/`; VirtualMIDI I/O in `Midi/`; framing in `Protocol/`; bulk in `Usb/` |

[Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-1..9, AD-14..17, AD-20, pipeline diagram]

### CRITICAL — Directionality (do not invert TX/RX)

Locked in Story 1.5 code review — keep this mapping:

| Product side | Virtual Port role | teVirtualMIDI flags | Data API |
|---|---|---|---|
| MT4 physical **IN** → host apps (DAW MIDI **IN**) | Bridge transmits into the virtual IN | `PARSE_TX` + `INSTANTIATE_TX` | `virtualMIDISendData` via `SendToHost` |
| Host apps (DAW MIDI **OUT**) → MT4 physical **OUT** | Bridge receives from the virtual OUT | `PARSE_RX` + `INSTANTIATE_RX` | create-port **callback** → `HostToDeviceSink` |

There is **no** product IN for Port 3/4. Out-of-range `inPortIndex` / `outPortIndex` must fail closed.

### CRITICAL — Port index ↔ Emagic cable (MT4)

Reuse Profile helpers — illustrative MT4 result:

| Direction | Port N (display) | Backend index | Emagic cable index |
|---|---|---|---|
| IN | `MT4 Port 1` | 0 | 0 |
| IN | `MT4 Port 2` | 1 | 1 |
| OUT | `MT4 Port 1` | 0 | 0 |
| OUT | `MT4 Port 2` | 1 | 1 |
| OUT | `MT4 Port 3` | 2 | 2 |
| OUT | `MT4 Port 4` | 3 | 3 |

Broadcast cable 15 is never a Virtual Port and must not receive host traffic.

```cpp
// Illustrative — build once at Start from profile masks
uint8_t inCables[kMaxMidiBackendInPorts] = {};
uint8_t outCables[kMaxMidiBackendOutPorts] = {};
const std::size_t inN = collectProductCableIndices(profile.inCables, inCables, kMaxMidiBackendInPorts);
const std::size_t outN = collectProductCableIndices(profile.outCables, outCables, kMaxMidiBackendOutPorts);
// outPortIndex i → outCables[i]; inPortIndex i ← match demux cableIndex
```

### CRITICAL — Call graph (I/O)

```text
Start:
  Open → init magics → CreatePortSet → SetHostToDeviceSink(session) → start reader thread

Host → device (VirtualMIDI callback thread):
  OUT callback(outPortIndex, bytes)
    → DeviceSession sink
    → EncodeToDevice(cable=outCables[outPortIndex], bytes)
    → WriteBulk(framed)

Device → host (reader thread):
  ReadBulk(buffer)
    → DecodeFromDevice(MidiCableSink)
    → for product IN cable: SendToHost(inPortIndex, midi)

Stop:
  signal stop → join reader → SetHostToDeviceSink(nullptr)
    → DestroyPortSet → finish magic → Close
```

Forbidden: App calling `SendToHost` / inventing a second pump; Protocol including VirtualMIDI; closing ports from the VirtualMIDI callback; leaving `ReadBulk` blocked across `Close`.

### CRITICAL — VirtualMIDI data APIs (eval path)

| Topic | Rule |
|---|---|
| Send to DAW (IN ports) | `virtualMIDISendData(port, bytes, length)` — prefer one complete MIDI message per call when practical (teVirtualMIDI guidance) |
| Receive from DAW (OUT ports) | Prefer **callback** registered at `virtualMIDICreatePortEx2` (already typed in `TeVirtualMidiApi.h`) over blocking `GetData` |
| Close safety | Never `virtualMIDIClosePort` from inside the callback |
| Load strategy | Same runtime `LoadLibraryEx(..., SYSTEM32)` as Story 1.5; add `GetProcAddress("virtualMIDISendData")` |
| Driver prerequisite | loopMIDI or rtpMIDI still required; fail closed English fix path unchanged |
| MSI / redistribute | Still **out of scope** (OQ-1 / Story 4.1) |

Public eval docs: https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html

### CRITICAL — Channel MIDI coverage (AC #2)

Carry **raw channel MIDI** through the mapper without a notes-only filter. Minimum playability set for smoke:

- Note On / Note Off (`0x8n` / `0x9n`)
- Control Change (`0xBn`)
- Program Change, Channel Pressure, Pitch Bend, Poly Aftertouch when present in the smoke

Do **not** implement a message-type allowlist that drops non-note channel traffic. Do **not** expand acceptance to claim FR-7 clock/MTC or FR-8 SysEx librarian vectors — those are Epic 2 ACs even if bytes may transit.

### CRITICAL — Threading / Stop (Story 1.4 deferred items)

From `deferred-work.md` (1.4 review) — **must be handled for this continuous pump**:

1. **Blocked ReadBulk:** set WinUSB pipe policy `PIPE_TRANSFER_TIMEOUT` on the bulk IN pipe (reasonable short timeout, e.g. tens–hundreds of ms) so the reader loop can observe a stop flag between reads. Document the chosen timeout in an English comment.
2. **Close vs in-flight I/O:** Stop must **join** the reader thread before `transport_.Close()` / `DestroyPortSet`.
3. **Host→device concurrency:** serialize Encode+WriteBulk against Stop (mutex or equivalent) so teardown cannot race the VirtualMIDI callback.
4. **F5 / 0xFF collision** in raw MIDI remains a known Linux-parity limitation — do **not** invent an escaping layer in this story (still deferred).

### Layer boundaries (anti-reinvention)

| Layer | Owns | Must not |
|---|---|---|
| Profile | Masks, `collectProductCableIndices` | Pump, VirtualMIDI, F5 |
| Protocol | F5 encode/decode, init/finish constants | Threads, VirtualMIDI, WinUSB |
| Usb | GUID open, bulk R/W, pipe timeout policy | Port names, Emagic parse, VirtualMIDI |
| Midi | `SendToHost`, OUT callback → sink, DLL exports | Emagic F5, WinUSB open, inventing `K` |
| Device | Session lifecycle + **I/O pump** + cable↔port map | Proprietary SDK includes (keep in Midi) |
| App | CLI long-run / Ctrl+C | Second naming scheme or parallel pump |

### Existing code being modified — current state

**After Stories 1.1–1.5 (done):**

- `MidiBackend` already declares `SendToHost` + `SetHostToDeviceSink` — Windows `SendToHost` returns `"reserved for Story 1.6"`; callback pointer is stored but unused; OUT ports created with **`callback = nullptr`**
- `VirtualMidiBackend` creates 2 TX IN + 4 RX OUT ports; runtime DLL load of create/close only
- `DeviceSession::Start` = Open → init×2 → `CreatePortSet`; **no** reader thread; **no** sink registration; `Stop` destroys ports then finish+close
- `EmagicCableMapper` has working `EncodeToDevice` / `DecodeFromDevice` + `MidiCableSink`; `--test-mapper` synthetic vectors pass
- `WinUsbTransport::ReadBulk` / `WriteBulk` are synchronous with **no** pipe timeout and **no** Close/read sync
- `Main.cpp` `--start-session` Starts then **immediately Stops** (lifecycle proof only — not a MIDI smoke)

**What this story changes:**

- Real VirtualMIDI send + OUT callback
- DeviceSession owns continuous bidirectional MIDI pump
- Transport timeout / join so Stop is safe
- CLI stays up for hardware notes/CC smoke

**What must be preserved:**

- AD-5 names / create-destroy ownership / fail-closed VirtualMIDI missing
- Profile / Protocol isolation
- GUID-first open + `--dev-zadig` hatch
- Init×2 / finish magics
- `--test-mapper` / `--test-port-names` / profile smoke
- User-session exe; `builds/` output; no kernel driver

**Must not break:**

- Windows CI compile without proprietary SDK in-repo
- macOS configure/build smoke
- CreatePortSet fail-closed when DLL missing
- Mapper synthetic tests

### Suggested shapes (illustrative — adjust to conventions)

```cpp
// TeVirtualMidiApi.h — add:
typedef BOOL (CALLBACK* TeVmSendDataFn)(
    TeVmMidiPortHandle midiPort, LPBYTE midiDataBytes, DWORD length);

// VirtualMidiBackend — OUT create uses callback + cookie { backend*, outIndex }
// SendToHost → sendData_(inPorts_[inPortIndex], ...)

// DeviceSession private (illustrative):
void onHostMidi(std::size_t outPortIndex, const uint8_t* midi, std::size_t n);
void readerLoop(); // while (!stop_) { ReadBulk; DecodeFromDevice; }
std::thread readerThread_;
std::atomic<bool> stopPump_{true};
std::mutex usbIoMutex_;
uint8_t inCableByPort_[kMaxMidiBackendInPorts];
uint8_t outCableByPort_[kMaxMidiBackendOutPorts];
std::size_t inPortCount_ = 0;
std::size_t outPortCount_ = 0;
```

### Technical requirements

- **Language:** C++17 (`std::thread`, `std::atomic`, `std::mutex` OK)
- **Build:** extend existing Bridge TUs only if new files appear; prefer updating existing Midi/Device/Usb sources
- **Style:** Allman, 4 spaces, `#pragma once`, English — `conventions.md` §6
- **Quality:** `scripts/quality/lint-touched.py` SSOT; keep functions within CCN/length budgets (split pump helpers if Main/DeviceSession grow)
- **No** new third-party test frameworks; no committed proprietary SDK drop

### Library / framework requirements

| Use | Do not use |
|---|---|
| Existing `MidiBackend` seam methods | New parallel MIDI API beside MidiBackend |
| `virtualMIDISendData` + create callback | Polling-only design that blocks Stop without timeout |
| `EmagicCableMapper` encode/decode | Reimplementing F5 in Midi or Device |
| Profile `collectProductCableIndices` | Hard-coded second cable table in Midi |
| MIDI-OX and/or one Validation Matrix DAW for smoke | Claiming full SM-1 clock/MTC done |
| Fail-closed English diagnostics | Silent drop of an entire Port with success claim |

### File structure requirements

#### UPDATE (primary)

| Path | Current state | This story |
|---|---|---|
| `src/Midi/TeVirtualMidiApi.h` | create/close + callback typedef | Add `virtualMIDISendData` typedef |
| `src/Midi/VirtualMidiBackend.h/.cpp` | SendToHost stub; OUT callback nullptr | Real send + OUT callback → sink |
| `src/Device/DeviceSession.h/.cpp` | Start/Stop ports only | Pump, sink, cable maps, thread join |
| `src/Usb/WinUsbTransport.h/.cpp` | Bulk R/W no timeout | IN pipe timeout (and any cancel needed for Stop) |
| `src/App/Main.cpp` | `--start-session` immediate Stop | Long-run I/O until Ctrl+C; English smoke diagnostics |
| `CMakeLists.txt` | Bridge sources listed | Only if new TUs are added |

#### Likely untouched (reuse as-is)

| Path | Why |
|---|---|
| `src/Protocol/EmagicCableMapper.*` | Already encodes/decodes; extend only if a proven bug blocks notes/CC |
| `src/Profile/DeviceProfile.*` | Cable helpers already correct |
| `src/Device/DeviceSessionManager.*` | Naming/`K` already correct |
| `src/Midi/MidiBackend.h` | Seam methods already declared |

#### OUT OF SCOPE paths

- `tools/midi-path-harness/` → Epic **5**
- `installer/` MSI / VirtualMIDI redistributable → **4.1**
- Auto-Start / hot-plug watchers → Epic **3**
- Cousin AMT8/Unitor8 product shipping → hardware-gated later

### Testing requirements

| Check | How |
|---|---|
| Compile (macOS) | `cmake -S . -B builds/<cfg> && cmake --build builds/<cfg>` |
| Compile (Windows CI) | Existing workflow green; no proprietary SDK committed |
| Mapper regression | `Bridge --test-mapper` still exit 0 |
| Port names regression | `Bridge --test-port-names` still exit 0 |
| Hardware notes/CC (Windows) | WinUSB-bound MT4 + loopMIDI/rtpMIDI → `--start-session` (or `--run-midi`) stays up; smoke **each** of 2 IN + 4 OUT with MIDI-OX and/or Ableton/Reason — notes + CC both directions |
| Common channel traffic | At least one non-note channel message type (e.g. CC + pitch bend or program change) observed through a port — proves not notes-only |
| Short session | Several minutes of play/automation without Bridge restart |
| Fail diagnostics | Force a failure path (bad index in unit-level if available, or unplug during run) → English log mentions Port/cable/direction |
| Fail closed (no driver) | Start still fails with VirtualMIDI fix-path message |
| Isolation | Grep Protocol/Profile: no VirtualMIDI / WinUSB |
| Lint | `python scripts/quality/lint-touched.py` on touched C++ |

Validation Matrix hosts (PRD): Ableton Live 12, Reason Studios 12, MIDI-OX, Matrix-Control — **minimum for this story** is one DAW **or** MIDI-OX smoke covering all ports. Full multi-host / multi-OS matrix depth continues across later epics.

### Previous story intelligence

From Story **1.5** (done):

- Seam stubs intentionally left for 1.6 — implement them; do not invent a second interface
- TX/RX mapping confirmed in code review — do not “fix” by swapping flags without Windows evidence
- Deferred: stale ports after crash; IN/OUT same-name collision proof; live endpoint enumeration; unpinned SDK flags; shared MidiBackend across sessions — **still deferred** unless they block notes/CC
- `LoadLibraryEx(..., SYSTEM32)` and blank-name validation must remain

From Story **1.4** (done):

- `MidiCableSink` is the demux feed point — wire it in the pump
- Deferred bulk timeout + Close/in-flight sync — **in scope to resolve for the pump**
- F5/0xFF collision escaping — still deferred
- Keep Main helpers small for lint CCN

From Story **1.3** (done):

- Dual-machine `#ifdef _WIN32` stubs; fail-closed English — mirror for any new USB timeout helpers

From Story **1.2** (done):

- Port N / cable binding locked — reuse `collectProductCableIndices`; never invent a second table in Midi

### Git intelligence summary

Recent implementation commits:

- `49b78a0` — VirtualMIDI backend + stable `MT4 Port N` endpoints (create/destroy; SendToHost stub)
- `5e13e4f` — DeviceSession + Emagic cable mapper + WinUSB bulk I/O
- `b21b7b4` — WinUSB bind path + GUID-first MT4 transport open
- `2ecc1e8` — Declarative MT4 DeviceProfile + Port N helpers
- `559cc96` — Bridge CMake scaffold + Windows CI

Patterns to extend: single `Bridge` exe, flag-gated hardware paths, fail-closed stubs on non-Windows, layer isolation greps, runtime VirtualMIDI load (no vendored `.lib`).

### Latest tech information

- teVirtualMIDI: `virtualMIDICreatePortEx2` accepts optional `LPVM_MIDI_DATA_CB`; with NULL callback, apps may poll `virtualMIDIGetData` — **prefer callback** for OUT ports in this Bridge
- `virtualMIDISendData` expects complete MIDI commands when practical; do not intermingle realtime octets inside other commands (relevant later for Epic 2 clock — for 1.6, send channel messages as complete units)
- Driver still via loopMIDI/rtpMIDI for eval; SDK not freeware; no redistribute without Tobias clearance (OQ-1)
- WinUSB: `WinUsb_SetPipePolicy(..., PIPE_TRANSFER_TIMEOUT, ...)` is the standard way to bound blocking reads for shutdown
- Do not treat Windows MIDI Services loopback as a substitute for Emagic-shaped Bridge ports

### Project context reference

- `conventions.md` §3 quality gate, §6 C++ style, RAII for threads/handles
- No `project-context.md` in-repo yet — follow architecture spine + conventions
- Clarity: keep hardware/protocol names in English (MT4, WinUSB, VirtualMIDI, MIDI)

### Anti-patterns to forbid

- Inverting TX/RX so DAW IN/OUT appear swapped
- Notes-only filter that drops CC / pitch bend / program change
- Claiming FR-7 / FR-8 / Epic 2 done “because bytes flowed”
- Calling `virtualMIDIClosePort` from the data callback
- Closing WinUSB while the reader thread is still in `ReadBulk` without join/timeout
- Hard-coding cable tables in Midi instead of Profile helpers
- VirtualMIDI / WinUSB includes in Protocol or Profile
- App-owned second pump bypassing DeviceSession
- Building `tools/midi-path-harness/` or installer MSI work here
- French comments; kebab-case sources under `src/`; Session-0 Windows Service
- Forking `aaron1a12/virtual-midi` or committing proprietary SDK binaries

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Story 1.6, Epic 1]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-1..9, AD-14..17, AD-20, pipeline]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — FR-6, SM-1 notes/CC portion, Validation Matrix §10]
- [Source: `_bmad-output/implementation-artifacts/1-5-virtualmidi-backend-and-stable-mt4-port-names.md` — TX/RX mapping, seam stubs, deferred items]
- [Source: `_bmad-output/implementation-artifacts/1-4-devicesession-and-emagic-cable-mapper-usermode.md` — mapper + deferred timeout/Close races]
- [Source: `_bmad-output/implementation-artifacts/deferred-work.md` — 1.4/1.5 deferred carry-overs]
- [Source: `src/Midi/MidiBackend.h` — `SendToHost` / `HostToDeviceSink` seam]
- [Source: `src/Midi/VirtualMidiBackend.cpp` — create with null callback; SendToHost stub]
- [Source: `src/Protocol/EmagicCableMapper.h` — `EncodeToDevice` / `DecodeFromDevice` / `MidiCableSink`]
- [Source: `conventions.md` — §3 quality gate, §6 C++ standards]
- [Source: https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html — SDK eval model]

## Dev Agent Record

### Agent Model Used

Composer (Cursor agent router)

### Debug Log References

- macOS: `cmake -S . -B builds/debug && cmake --build builds/debug` OK
- `Bridge --test-mapper` / `Bridge --test-port-names` exit 0
- `python scripts/quality/lint-touched.py` exit 0
- Protocol/Profile isolation grep clean

### Completion Notes List

- VirtualMIDI data path complete: `virtualMIDISendData` via runtime load; OUT ports use create-time callback → `HostToDeviceSink`; TX/RX mapping unchanged from 1.5
- DeviceSession owns continuous bidirectional pump with Profile cable maps, reader thread, usbIo mutex vs Stop, and English Port N / cable failure diagnostics
- WinUSB bulk IN `PIPE_TRANSFER_TIMEOUT` = 100 ms so Stop can join a blocked `ReadBulk`
- `--start-session` / `--run-midi` stay alive until Ctrl+C; pump failures exit non-zero with direction/Port/cable detail
- MT4 cable↔port order still asserted in profile smoke (`matchesMt4ProductCableOrder`)
- Hardware notes/CC round-trip on all 2 IN + 4 OUT remains a Windows validation step (MIDI-OX or one Validation Matrix DAW)

### File List

- `CMakeLists.txt`
- `src/App/Main.cpp`
- `src/App/MapperSmoke.cpp` (new)
- `src/App/MapperSmoke.h` (new)
- `src/App/MidiSessionCli.cpp` (new)
- `src/App/MidiSessionCli.h` (new)
- `src/Device/DeviceSession.cpp`
- `src/Device/DeviceSession.h`
- `src/Midi/MidiBackend.h`
- `src/Midi/TeVirtualMidiApi.h`
- `src/Midi/VirtualMidiBackend.cpp`
- `src/Midi/VirtualMidiBackend.h`
- `src/Midi/VirtualMidiWinSupport.cpp` (new)
- `src/Midi/VirtualMidiWinSupport.h` (new)
- `src/Usb/WinUsbTransport.cpp`
- `src/Usb/WinUsbTransport.h`
- `_bmad-output/implementation-artifacts/sprint-status.yaml`

### Review Findings

- [x] [Review][Defer] Hardware AC1/AC2 proof vs review status — deferred: infra OK for story close; Windows MIDI-OX/DAW smoke on all 2 IN + 4 OUT remains a manual checklist (user choice 2026-08-05)
- [x] [Review][Patch] Close host→device startup window before `running_` is set [`src/Device/DeviceSession.cpp:294`]
- [x] [Review][Patch] Clear host→device sink under `usbIoMutex_` before DestroyPortSet [`src/Device/DeviceSession.cpp:312`]
- [x] [Review][Patch] Make `IsRunning()` false after pump fatal stop [`src/Device/DeviceSession.cpp:382`]
- [x] [Review][Patch] Stop device→host forwards once `stopPump_` is set mid-decode [`src/Device/DeviceSession.cpp:176`]
- [x] [Review][Patch] Do not hold `usbIoMutex_` across `SendToHost` / prefer encode-then-unlock-write for host→device [`src/Device/DeviceSession.cpp:237`]
- [x] [Review][Patch] Log English diagnostics for invalid host→device port index (AC4) [`src/Device/DeviceSession.cpp:152`]
- [x] [Review][Patch] Extend MapperSmoke with at least one CC (non-note channel) encode/decode vector [`src/App/MapperSmoke.cpp`]
- [x] [Review][Defer] CTRL_CLOSE_EVENT may kill process before `Stop()` finishes [`src/App/MidiSessionCli.cpp:30`] — deferred, pre-existing OS console-close constraint
- [x] [Review][Defer] Device→host may forward raw spans that are not complete MIDI messages [`src/Protocol/EmagicCableMapper.cpp:165`] — deferred, pre-existing demux design; message framer out of 1.6 scope
- [x] [Review][Defer] `SendToHost` does not reject payloads above VirtualMIDI default max Sysex length [`src/Midi/VirtualMidiBackend.cpp:361`] — deferred, pre-existing; Epic 2 SysEx path

## Change Log

- 2026-08-05 — Story context created (ready-for-dev)
- 2026-08-05 — Implemented notes/CC bidirectional pump + long-run CLI; status → review
- 2026-08-05 — Code review findings recorded (1 decision, 7 patches, 3 deferred)
- 2026-08-05 — Review patches applied (startup/Stop races, pump IsRunning, mutex scope, CC smoke); status → done
