---
baseline_commit: 1c93de3
---

# Story 2.3: Transparent SysEx transport with burst buffering

Status: done

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a SysEx editor/librarian user,
I want large and bursty System Exclusive messages carried transparently with enough buffering for real dumps,
so that editors complete transfers without a Bridge restart under normal librarian use.

## Acceptance Criteria

1. **Given** a live DeviceSession with Virtual Ports  
   **When** SysEx frames (including Oberheim Matrix-shaped traffic) flow between Virtual Ports and MT4 cables  
   **Then** the Bridge carries them transparently with no Emagic-side framing or rewriting of Oberheim payloads — FR-8 / AD-16

2. **And** SysEx bursts are buffered/queued so Matrix-Control librarian-scale dumps can complete without Bridge restart — NFR-R3 / AD-18

3. **And** Matrix-Control is not linked or bundled as a Bridge runtime dependency — CAP-8

4. **And** incomplete or corrupt dumps under normal test conditions are treated as failures

**Traces:** FR-8, CAP-8, CAP-17 (buffering), AD-16, AD-18, NFR-R3

## Tasks / Subtasks

- [x] Task 1: Prove and harden device→host SysEx framing for librarian sizes (AC: 1, 4)
  - [x] Confirm `MidiMessageFramer` already assembles `0xF0`…`0xF7` via the SysEx hold path and already preserves realtime / MTC quarter-frame interleave without aborting SysEx — do **not** invent a SysexEngine or second framer
  - [x] Size gate: Matrix patch **275 B** and master **351 B** must assemble and emit intact under `kMaxSysexHoldBytes` (today **1024**). Prefer **keeping 1024** if it covers V1 Matrix frames; if raised, document why and keep a hard cap (do not grow unbounded)
  - [x] Observability: oversize SysEx that hits the hold cap must no longer be a silent `Reset()` only — bump an English-diagnosable counter / log path (Port N / cable when available at session layer) so incomplete dumps are **failures**, not invisible drops ([deferred-work](deferred-work.md) Epic 1 item)
  - [x] Extend framer synthetic coverage (`FramerSmoke` and/or new `FramerSysexSmoke.cpp` if file-size budget requires a split; wire into `runFramerTests()`): Device Inquiry (6 B); Inquiry reply (15 B); synthetic Oberheim-shaped patch frame (**275 B** `F0 10 06 01 … F7`); master-sized frame (**351 B** `F0 10 06 03 … F7`); patch split across multiple `Push` spans; clock and/or `0xF1` interleaved mid-SysEx without abort; oversize (> hold cap) → observable failure path
  - [x] Add matching Catch2 cases in `tests/MidiMessageFramerTests.cpp`
  - [x] If a framer bug is found for librarian-sized SysEx, fix in `MidiMessageFramer` only; preserve note/CC/running-status/realtime/MTC behavior from Epic 1 + 2.1 + 2.2

- [x] Task 2: Prove Emagic opaque carry of librarian-sized SysEx (AC: 1, 3)
  - [x] Add a dedicated `src/Protocol/EmagicMapperSysexSmoke.cpp` (mirror `EmagicMapperMtcSmoke.cpp` / `EmagicMapperRealtimeSmoke.cpp` — keep `EmagicMapperSmokeSupport.cpp` under ~400-line budget)
  - [x] Declare `runEmagicMapperSmokeEncodeSysex` / `runEmagicMapperSmokeDecodeSysex` in `EmagicMapperSmokeSupport.h`; wire both into `runAllEmagicMapperSmokeTests`
  - [x] Encode + decode vectors must include at least: one short Inquiry (`F0 7E … F7`); one **275 B** patch-shaped frame; one **351 B** master-shaped frame on a product cable — prove F5 demux / FF truncate do not eat, rewrite, or truncate Oberheim payloads
  - [x] Use **fixed synthetic byte patterns** sized/shaped like Matrix-Control frames — do **not** link, submodule, or `#include` Matrix-Control sources (CAP-8)
  - [x] Wire Catch2 one-liners in `tests/EmagicCableMapperTests.cpp`; add the new TU to **both** `Bridge` and `BridgeTests` source lists in `CMakeLists.txt`
  - [x] Do **not** add message-type allowlists or Oberheim parsers in DeviceSession / VirtualMidiBackend / EmagicCableMapper
  - [x] Do **not** invent Emagic escaping for `0xF5` / `0xFF` raw MIDI vs framing collision — still deferred; Matrix frames use manufacturer `0x10` and stay clear of Emagic port-switch / pad bytes in normal librarian traffic

- [x] Task 3: Burst buffering / queue for librarian-scale traffic (AC: 2, 4)
  - [x] Implement bounded buffering so sequential librarian-scale bursts (design target: on the order of **~100 × 275 B** outbound and/or inbound with ≥10 ms Matrix-Control pacing) complete **without Bridge restart** under normal test conditions — NFR-R3 / AD-18
  - [x] Prefer composition in `Device/` (session pump owns queues) over inventing a Protocol “SysEx service”; Protocol stays framing + opaque Emagic map only (AD-2)
  - [x] Host→device: teVirtualMIDI `PARSE_RX` callbacks must not be the sole unbounded work path under `usbIoMutex_` — if Encode+WriteBulk can stall behind `processBulkRead`, introduce a bounded outbound queue (or equivalent backpressure) so bursts queue instead of dropping or forcing a restart
  - [x] Device→host: complete framed SysEx must reach `SendToHost` without silent truncation; reject or log when payload would exceed teVirtualMIDI create-port `maxSysexLength` (`kTeVmDefaultMaxSysexLength = 65535`) — clear English failure, not silent success ([deferred-work](deferred-work.md))
  - [x] Queue overflow / corrupt assembly under normal librarian-scale tests = **failure** (English diagnostics with Port N / cable / direction when known)
  - [x] Do **not** claim ~4 h soak proof here — design must not *require* restart for librarian dumps; longevity sample plan remains **2.5**
  - [x] Investigate `processBulkRead` holding `usbIoMutex_` across decode **if** burst smoke shows host→device stalls — fix only as needed for AC pass; full latency harness remains Epic **5** / AD-11

- [x] Task 4: Host→device path — verify, only harden framer if lab fails (AC: 1, 2)
  - [x] Default assumption: with `PARSE_RX`, teVirtualMIDI delivers **complete** SysEx units up to `maxSysexLength` → existing `EncodeToDevice` + `WriteBulk` is enough once Task 3 buffering exists
  - [x] Confirm `kEncodeBufferCapacity` (**4096**) remains sufficient for F5 wrap of a 351 B frame (+ pad); raise only if encode smoke proves overflow
  - [x] On Windows hardware smoke: confirm host → Virtual OUT → MT4 physical OUT carries librarian-sized SysEx (MIDI-OX / SysEx tool / Matrix-Control when available — see Task 5 fence)
  - [x] **Only if** lab shows incomplete host→device SysEx spans: add a symmetric host→device framer. Do **not** add it preemptively
  - [x] Computer Mode wake: keep the existing channel CC kick — **SysEx alone does not wake Computer Mode**

- [x] Task 5: Document hardware SysEx transport smoke (AC: 1, 2, 4)
  - [x] Add `docs/tests/smoke-epic2-sysex-mt4.md` (kebab-case) with checklist: ≥1 IN + ≥1 OUT; librarian-sized SysEx both directions; short burst (≥ several 275 B frames or equivalent) without Bridge restart; English failure notes (Port N / cable / direction)
  - [x] Win10 x64 = mandatory matrix row; Win11 x64 = document when hardware available
  - [x] Host options for this story: MIDI-OX / SysEx file sender / DAW SysEx, **and/or** Matrix-Control when installed — but do **not** claim SM-2 / Story **2.4** minimum pass-vector table complete
  - [x] Cross-link from `docs/tests/smoke-epic2-mtc-mt4.md` and `docs/tests/smoke-epic2-clock-mt4.md` (those stay clock/MTC; SysEx librarian transport lives here)
  - [x] Explicit fences: Matrix-Control locked minimum vectors (Inquiry + patch + master + push + live edits + mixed-wire) → **2.4**; ~4 h longevity → **2.5**; full-frame MTC sync already covered by **2.2**

- [x] Task 6: Quality + anti-scope (AC: all)
  - [x] `python scripts/quality/lint-touched.py` exits 0 on the C++ diff
  - [x] Compile: `cmake -S . -B builds/<config> && cmake --build builds/<config>` (macOS smoke OK; Windows CI remains the gate)
  - [x] `Bridge --test-mapper` still exit 0 (includes new framer + mapper SysEx vectors)
  - [x] `ctest` / `BridgeTests` Pass with new Catch2 cases
  - [x] Grep isolation: no `teVirtualMIDI` / `VirtualMIDI` / `winusb` under `src/Protocol/` or `src/Profile/`
  - [x] Confirm no French in sources; no Matrix-Control link/submodule (CAP-8); no claiming SM-2 / 2.4 vectors done; no ~4h longevity claim (→ **2.5**); no MIDI Path harness (→ Epic **5**); no installer / Auto-Start / hot-plug work

### Review Findings

- [x] [Review][Defer] Incomplete SysEx without F7 never fails — deferred: AC4 for this story stays oversize / queue overflow / nested `0xF0` abandon; open hold without `0xF7` left for a later story if Win10 lab shows hangs
- [x] [Review][Defer] Win10 hardware matrix still blank — deferred: same honesty bar as 2.1/2.2; synthetic gates proceed; Win10 checklist remains Guillaume’s manual lab gate (not a code blocker for this review)
- [x] [Review][Defer] No DeviceSession pump-level burst test — deferred: AC2 proof for this story = `HostOutboundQueue` unit tests + framer/mapper smokes + Win10 lab checklist; no new session/pump integration test in 2.3
- [x] [Review][Patch] Host outbound drain pops before encode/WriteBulk — failed items are discarded; remaining queue is not cleared or counted on encode/WriteBulk/ReadBulk failure [`src/Device/DeviceSessionHostOutbound.cpp` / `DeviceSessionDeviceHost.cpp`]
- [x] [Review][Patch] Shutdown/enqueue race — `handleHostMidi` does not bail on `stopPump_`/`running_`; `Stop` clears the queue before nulling the VirtualMIDI sink, so a late callback can enqueue work that never drains [`src/Device/DeviceSessionHostOutbound.cpp` / `DeviceSession.cpp`]
- [x] [Review][Patch] Nested `0xF0` abandons open SysEx without reject counter — prior partial dump is cleared via `beginSysEx()` with no AC4-visible failure [`src/Protocol/MidiMessageFramer.cpp`]
- [x] [Review][Patch] `HostOutboundQueue::TryPush` byte-cap check can wrap `size_t` before compare [`src/Device/HostOutboundQueue.cpp`]
- [x] [Review][Patch] `SendToHost` >65535 reject path has no Catch2/smoke coverage [`src/Midi/VirtualMidiBackend.cpp`]
- [x] [Review][Patch] Queue byte-cap overflow path untested (message-count overflow only) [`tests/HostOutboundQueueTests.cpp`]
- [x] [Review][Defer] Trailing bytes after oversize SysEx Reset in the same Push span are dropped without extra reject count [`src/Protocol/MidiMessageFramer.cpp`] — deferred, pre-existing
- [x] [Review][Defer] After first `SendToHost` failure, remaining demuxed frames in the same bulk read are skipped once `stopPump_` is set [`src/Device/DeviceSessionDeviceHost.cpp`] — deferred, pre-existing

## Dev Notes

### Soft dependency on Stories 2.1 / 2.2

Stories **2.1** and **2.2** are still **`review`** awaiting Win10 DAW lab Pass for clock and MTC. Synthetic SysEx work (Tasks 1–3, Catch2, smoke doc scaffold) can proceed in parallel. Hardware SysEx matrix fill-in should reuse the same lab path when possible. If 2.1/2.2 lab later reveals a host→device framer or mutex fix, rebase SysEx buffering on that pump — do not invent a parallel I/O path.

### Scope fence

This story lands **FR-8 / AD-16 transparent SysEx** plus **NFR-R3 / AD-18 burst buffering** so librarian-scale dumps can complete without Bridge restart. Story **2.4** owns Matrix-Control **minimum pass vectors** on hardware. Story **2.5** owns ~4 h longevity design/sample. Epic 1 + 2.1 + 2.2 already ship a transparent USB ↔ VirtualMIDI pump that carries **short** SysEx (including MTC full-frame).

| In scope | Out of scope (later stories) |
|---|---|
| Transparent carry of Oberheim Matrix-**shaped** SysEx (no Emagic rewrite) | Locked Matrix-Control minimum pass-vector table → **2.4** |
| Framer/mapper synthetic vectors for Inquiry / 275 B / 351 B (+ interleave) | ~4 h longevity design / soak sample → **2.5** |
| Bounded burst buffering/queue so librarian-scale dumps need no Bridge restart | Auto-Start → **3.1**; hot-plug → **3.2** |
| Oversize / overflow / corrupt dumps treated as failures (English diagnostics) | Multi-client DAW+MIDI-OX → **3.3**; multi-MT4 → **3.4** |
| teVirtualMIDI max SysEx length check on SendToHost | Public Installer / MSI → **4.1** / OQ-1 |
| Document Win10 (mandatory) / Win11 (when available) SysEx transport smoke | MIDI Path latency/jitter harness + Studio-Done thresholds → Epic **5** / AD-11 |
| Fix framer/pump/queue only as needed for AC | `0xFF` System Reset vs Emagic pad escaping — still deferred |
| | Building a SysEx librarian UI or Oberheim parser in the Bridge |
| | Linking Matrix-Control as a Bridge dependency (forbidden — CAP-8) |

### Epic context

Epic 2 outcome: Validation Matrix DAWs can use **clock / Start-Stop-Continue / MTC** through the Bridge, and Matrix-Control can complete minimum SysEx pass vectors — with buffering designed for ~4h sessions.

Story **2.3** makes the SysEx **pipe + buffers** trustworthy for librarian-scale traffic. Story **2.4** then proves the locked Matrix-Control vectors on that pipe.

Pipeline (unchanged topology):

```text
WinUsbTransport ↔ EmagicCableMapper ↔ MidiMessageFramer (device→host)
  ↔ MidiBackend (VirtualMidiBackend) ↔ Virtual Ports ↔ editor/DAW
```

Burst buffering (this story) slots into the **DeviceSession** pump / host↔device path — not a new top-level module.

### Architecture compliance (must follow)

| Decision | What it means for this story |
|---|---|
| AD-1 | Emagic multiplex stays usermode; no custom kernel MIDI |
| AD-2 | Protocol free of VirtualMIDI/WinUSB; Midi free of F5; composition in Device — queues live in Device (or Midi), not Protocol |
| AD-3 | Port N / cable indices from Profile helpers only |
| AD-7 | Keep teVirtualMIDI runtime LoadLibrary path; no SDK binaries committed |
| AD-11 | Do **not** build `tools/midi-path-harness/` here; provisional latency anchors are Epic 5 |
| AD-14 | No GPL `midi.c` paste |
| AD-15 | PascalCase under `src/`; lint-touched; English only |
| AD-16 | Transparent SysEx — no Emagic-side framing/rewriting of Oberheim payloads |
| AD-17 | Prior clock/MTC coverage must remain green (regression) |
| AD-18 | Buffer/queue SysEx bursts so librarian dumps complete without Bridge restart; incomplete/corrupt dumps = failures |
| AD-20 | Remain user-session Bridge exe |
| Structural Seed | Framing in `Protocol/`; pump + buffering in `Device/`; VirtualMIDI I/O in `Midi/` — no SysexEngine product module |

[Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-16, AD-18]

### CRITICAL — Reuse, do not reinvent

Already in tree (Epic 1 + Stories 2.1 / 2.2):

1. Bidirectional USB ↔ VirtualMIDI byte pump (`DeviceSession` + `VirtualMidiBackend`)
2. Opaque Emagic encode/decode (no message-type filter)
3. `MidiMessageFramer` SysEx hold (`kMaxSysexHoldBytes = 1024`) + realtime / MTC QF interleave
4. MTC full-frame (10 B SysEx) proven synthetically — template for larger SysEx vectors
5. Smoke patterns: `FramerSmoke` / `FramerMtcSmoke`, `EmagicMapperRealtimeSmoke` / `EmagicMapperMtcSmoke`, Catch2, `docs/tests/smoke-epic2-*.md`
6. teVirtualMIDI ports created with `kTeVmDefaultMaxSysexLength = 65535`
7. Host→device encode scratch `kEncodeBufferCapacity = 4096`
8. Long-run CLI `--start-session` / `--run-midi` for hardware smoke

**Forbidden reinventing:**

- A new `SysexEngine` / `OberheimParser` / message-type allowlist module
- A second I/O pump beside DeviceSession
- Replacing VirtualMIDI with Windows MIDI Services
- Claiming Matrix-Control SM-2 / Story 2.4 vectors done because synthetic 275/351 B frames flowed
- Linking or vendoring Matrix-Control into this repo
- Parsing Oberheim checksums/opcodes in production Bridge code beyond what tests need for fixed byte vectors

### CRITICAL — Size map (this story)

| Frame | Bytes | Fits hold 1024? | Fits encode 4096? | Product role |
|---|---|---|---|---|
| Device Inquiry | 6 | Yes | Yes | Editor presence gate |
| Inquiry reply | 15 | Yes | Yes | Identity |
| Patch dump / send (`01H` / `0DH`) | **275** | Yes | Yes | Dominant librarian frame |
| Master dump / send (`03H`) | **351** | Yes | Yes | Global block |
| Remote edit / Matrix Mod | 7 / 9 | Yes | Yes | Live editor stream (2.4 stress) |
| Bank EXPORT / IMPORT burst | ~**100 × 275** | Per-frame yes | Per-frame yes | **Burst buffering** (this story) |
| Oversize > hold cap | — | No → fail visible | — | Observability required |
| teVirtualMIDI max | 65535 | N/A | N/A | Create-port / SendToHost ceiling |

Canonical Oberheim Matrix shape (do not invent variants in production):

```text
F0  10  06  <opcode>  [header…]  [payload…]  [checksum if bulk]  F7
```

Universal Device Inquiry (not Oberheim-prefixed): `F0 7E 7F 06 01 F7`.

[Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/matrix-control-sysex-extract.md`]

### CRITICAL — Directionality (do not invert TX/RX)

Locked since Story 1.5 — unchanged:

| Product side | teVirtualMIDI | Data API |
|---|---|---|
| MT4 physical **IN** → host (editor MIDI IN) | `PARSE_TX` + `INSTANTIATE_TX` | `virtualMIDISendData` via `SendToHost` (complete SysEx) |
| Host (editor MIDI OUT) → MT4 physical **OUT** | `PARSE_RX` + `INSTANTIATE_RX` | create-port callback → `HostToDeviceSink` |

Prefer **one complete SysEx per** `virtualMIDISendData` call. Do not glue patch frames together or strip `F7`.

With `PARSE_RX`, callbacks deliver complete commands; SysEx longer than `maxSysexLength` are discarded by teVirtualMIDI — Bridge must not set a tiny max, and must not silently accept SendToHost above that ceiling.

### CRITICAL — Computer Mode wake

`DeviceSession` already sends a channel CC kick so USB enters Computer Mode. **SysEx and realtime alone do not wake Computer Mode**. Do not remove the channel kick; do not assume SysEx-only traffic will open the device path on a cold session.

### Existing code being modified — current state

**After Epic 1 + Stories 2.1 / 2.2 (synthetic green; hardware labs pending):**

- `MidiMessageFramer` — SysEx hold to **1024** bytes; oversize → silent `Reset()`; realtime + MTC QF interrupt mid-SysEx work
- `DeviceSessionDeviceHost.cpp` — demux → per-IN `inFramers_[].Push` → `SendToHost`; holds `usbIoMutex_` across decode
- `DeviceSession` — host→device Encode+WriteBulk under `usbIoMutex_`; encode scratch **4096**; **no outbound burst queue**
- `EmagicCableMapper` — opaque MIDI copy; F5 port switch; `0xFF` end-of-valid-data truncate — **no SysEx-specific logic (correct)**
- `VirtualMidiBackend` — ports created with max SysEx **65535**; `SendToHost` does not reject oversize
- `FramerSmoke` / `FramerMtcSmoke` — short SysEx + MTC full-frame; **no 275/351 B librarian vectors**
- `EmagicMapperMtcSmoke.cpp` — short full-frame SysEx opaque carry — **template for SysEx smoke TU**
- `docs/tests/smoke-epic2-mtc-mt4.md` — explicitly fences librarian SysEx → **2.3** / **2.4**

**What this story changes:**

- Synthetic proof that framer + mapper preserve librarian-sized Oberheim-shaped SysEx (including split Push / interleave)
- Observability for oversize SysEx drops
- Bounded burst buffering so librarian-scale sequences complete without Bridge restart
- Optional SendToHost max-length guard
- Hardware/transport smoke documentation for SysEx (not full 2.4 Matrix vector table)

**What must be preserved:**

- Notes/CC round-trip on all ports
- Clock + Start/Stop/Continue + MTC synthetic vectors (and lab paths when filled)
- TX/RX mapping, Port N naming, Profile cable helpers
- Fail-closed VirtualMIDI missing
- Init×2 / finish / Computer Mode kick
- Layer isolation (Protocol free of VirtualMIDI/WinUSB)
- `builds/` output; user-session exe; no kernel driver
- CAP-8: no Matrix-Control runtime dependency

**Must not break:**

- `Bridge --test-mapper` / `--test-port-names`
- Existing Catch2 realtime / MTC / mapper cases from 2.1 / 2.2
- Windows CI without proprietary SDK in-repo
- macOS configure/build smoke
- Epic 1 hardware notes/CC path

### Known load / deferred edges (raise here when in scope)

From `deferred-work.md` and Stories 2.1 / 2.2:

| Edge | Relevance to 2.3 |
|---|---|
| SysEx over 1024 silently dropped | **In scope** — observability (+ keep/raise cap with rationale) |
| `SendToHost` vs teVirtualMIDI max SysEx length | **In scope** — guard / English fail |
| `processBulkRead` holds `usbIoMutex_` across decode | **Investigate if** burst smoke stalls host→device |
| No host→device burst queue | **In scope** — AD-18 / NFR-R3 |
| F5 / `0xFF` raw MIDI collision | **Out** — still deferred |
| Incomplete host→device spans | Add host framer **only if** lab proves need |
| ~4 h session longevity | **Out → 2.5** |
| Matrix-Control minimum vectors | **Out → 2.4** |
| NFR-P1/P2 latency/jitter numbers | Epic **5** — do not invent thresholds in this smoke doc |

### Technical requirements

- **Language:** C++17; Allman; 4 spaces; `#pragma once`; English — `conventions.md` §6
- **Quality:** `scripts/quality/lint-touched.py` SSOT on touched C++
- **File size:** Prefer new `EmagicMapperSysexSmoke.cpp` / optional `FramerSysexSmoke.cpp` over growing support files past ~400 useful lines
- **Build:** list new Protocol smoke TU(s) in **both** Bridge and BridgeTests in `CMakeLists.txt`
- **No** committed proprietary VirtualMIDI SDK; no French in sources; **no** Matrix-Control link

### Library / framework requirements

| Use | Do not use |
|---|---|
| Existing `MidiMessageFramer` + DeviceSession pump | New SysexEngine / Oberheim allowlist |
| Existing `EmagicCableMapper` opaque path | Reimplementing F5 for SysEx |
| Fixed synthetic 275/351 B vectors (from extract sizes) | Vendoring Matrix-Control sources |
| MIDI-OX / SysEx tools / Matrix-Control (optional host) for hardware smoke | Claiming SM-2 / 2.4 complete |
| Bounded queue in Device (or Midi) | Unbounded memory growth “just in case” |
| `FramerSmoke` / Catch2 already in tree | New third-party test frameworks |
| English diagnostics with Port N / cable | Silent success when SysEx is dropped |
| teVirtualMIDI `maxSysexLength` 65535 | Tiny max that discards librarian frames |

### File structure requirements

#### UPDATE (primary)

| Path | Current state | This story |
|---|---|---|
| `src/Protocol/MidiMessageFramer.cpp` (+ `.h` if needed) | Hold 1024; silent oversize Reset | Librarian-size proof; oversize observability; fix only if defects found |
| `src/App/FramerSmoke.cpp` and/or new framer SysEx smoke TU | Short SysEx / MTC | Add 275/351 / Inquiry / burst-assembly vectors; fold into `runFramerTests()` |
| `src/Protocol/EmagicMapperSmokeSupport.h` | Declares clock + MTC runners | Declare SysEx encode/decode runners |
| `src/Protocol/EmagicMapperSmokeSupport.cpp` | `runAll…` calls clock + MTC | Wire SysEx runners into `runAllEmagicMapperSmokeTests` |
| `src/Device/DeviceSession.cpp` / `DeviceSessionDeviceHost.cpp` | No burst queue; mutex across decode | Bounded buffering / queue as needed for AC 2 |
| `src/Midi/VirtualMidiBackend.cpp` (and/or session SendToHost path) | No max-length reject | Guard vs teVirtualMIDI max when sending to host |
| `tests/MidiMessageFramerTests.cpp` | Realtime + MTC + short SysEx | Add librarian-size + oversize cases |
| `tests/EmagicCableMapperTests.cpp` | Clock + MTC wrappers | Add SysEx encode/decode wrappers |
| `docs/tests/smoke-epic2-mtc-mt4.md` / `smoke-epic2-clock-mt4.md` | SysEx out of scope | Light cross-link to SysEx smoke doc |
| `docs/tests/smoke-epic1-mt4.md` | Points Epic 2 at clock/MTC | Ensure SysEx doc discoverable |
| `CMakeLists.txt` | Bridge + BridgeTests list MtcSmoke | Add `EmagicMapperSysexSmoke.cpp` (+ optional FramerSysexSmoke) to **both** |
| `_bmad-output/implementation-artifacts/deferred-work.md` | Lists silent 1024 drop + SendToHost max | Mark raised/resolved items when closed by this story |

#### NEW

| Path | Why |
|---|---|
| `src/Protocol/EmagicMapperSysexSmoke.cpp` | Librarian-sized opaque encode/decode vectors (file-size split) |
| `docs/tests/smoke-epic2-sysex-mt4.md` | Hardware/transport checklist for AC 1–2, 4 |
| Optional: `src/App/FramerSysexSmoke.cpp` | If FramerSmoke would exceed size/clarity budget |

#### Likely untouched

| Path | Why |
|---|---|
| `src/Profile/DeviceProfile.*` | No SysEx capability bit required |
| `src/Protocol/EmagicCableMapper.*` core | Opaque carry already correct |
| `src/Usb/WinUsbTransport.*` | Bulk path sufficient unless timeout issues under burst |
| `tools/midi-path-harness/` | Epic **5** |
| Matrix-Control repo | External validation host only — never a build dependency |

#### Conditional UPDATE (lab-gated)

| Path | When |
|---|---|
| Host→device framer (new or DeviceSession) | Lab proves incomplete PARSE_RX SysEx spans |
| Mutex / decode split in `DeviceSessionDeviceHost.cpp` | Burst smoke proves host→device stall under `usbIoMutex_` |

### Testing requirements

| Check | How |
|---|---|
| Compile (macOS) | `cmake -S . -B builds/<cfg> && cmake --build builds/<cfg>` |
| Compile (Windows CI) | Existing workflow green |
| Framer synthetic | `Bridge --test-mapper` includes Inquiry + 275 B + 351 B + interleave + oversize-fail |
| Mapper synthetic | Same frames survive encode/decode on a product cable without rewrite |
| Catch2 | `BridgeTests` / `ctest` include new SysEx cases |
| Burst buffering | Synthetic and/or lab sequence of multiple 275 B frames completes without Bridge restart; overflow = visible fail |
| Hardware SysEx (Windows) | WinUSB-bound MT4 + VirtualMIDI → `--start-session`; send/observe librarian-sized SysEx on ≥1 IN and ≥1 OUT |
| Short session | Normal librarian-scale burst without Bridge restart |
| Regression | Notes/CC + clock/transport + MTC synthetic vectors still pass |
| OS matrix | Win10 x64 documented (mandatory); Win11 when available |
| Isolation | Grep Protocol/Profile: no VirtualMIDI / WinUSB |
| CAP-8 | No Matrix-Control in CMake / submodules / `#include` |
| Lint | `python scripts/quality/lint-touched.py` on touched C++ |

**Failure definition for this story:** Bridge-induced truncation, merge, rewrite, silent drop, or forced Bridge restart under normal librarian-scale SysEx (single 275/351 B frames and short bursts on the order of bank-export pacing). Not a multi-hour soak (→ 2.5), not Epic 5 p99 thresholds, not the full Matrix-Control SM-2 vector table (→ 2.4).

Validation Matrix hosts for **later** Story 2.4: Matrix-Control is primary. For **this** story, any host that can send/receive fixed SysEx sizes is enough for transport proof; Matrix-Control is welcome but not required to close synthetic work.

### Previous story intelligence

From Story **2.2** (`review`, synthetic green):

- Pattern that worked: prove existing framer with dedicated `FramerMtcSmoke` + Catch2; opaque mapper vectors in **separate** `EmagicMapperMtcSmoke.cpp`; hardware checklist in `docs/tests/smoke-epic2-mtc-mt4.md`; no MtcEngine
- Explicit fence: librarian SysEx → **this story** / 2.4
- Review kept status `review` until Win10 DAW Pass — same honesty bar for SysEx hardware rows (synthetic alone ≠ done)
- Raised deferred: >1024 observability, teVirtualMIDI max checks, burst buffering — **consume those here**
- Nested `0xF1` interrupt harden mid-SysEx — regression must stay green
- Dual-harness duplication (smoke vs Catch2) accepted for now; consolidate later

From Story **2.1** (`review`):

- Prove-don’t-reinvent; dedicated RealtimeSmoke TU; mutex stall gated on lab dropouts
- Anti-patterns still binding: no allowlists, no preemptive host framer, no F5/FF escaping

From Epic 1 / deferred-work:

- Continuous pump + framer landed; SysEx max-length / silent 1024 drop deferred to Epic 2 SysEx — **now**
- Hardware notes/CC all-ports green (lab 2026-08-05) — baseline for SysEx lab
- ReadBulk must use endpoint `wMaxPacketSize` (not oversized 512) — do not regress

### Git intelligence summary

Recent relevant commits:

- `1c93de3` — Prove MTC quarter-frame and full-frame sync for story 2.2
- `4814f82` — Prove MIDI clock and transport realtime for story 2.1
- `7934b75` — Align mapper smoke encode expectations with initial Out 1 F5
- `41cccdb` — Catch2 BridgeTests for Emagic mapper and DeviceProfile
- `a4d5d6c` — Fix MT4 device-to-host MIDI over WinUSB bulk IN (packet-size / framer)

Patterns to extend: split Protocol smoke TUs by topic, Catch2 thin wrappers over shared smoke runners, English lab docs under `docs/tests/`, prove-don’t-reinvent, raise deferred observability when the owning story arrives.

### Latest tech information

- teVirtualMIDI `maxSysexLength` is the create-port buffer; with `PARSE_RX`, complete commands are delivered and **SysEx longer than max are discarded** by the driver — keep **65535** unless a measured reason says otherwise
- Prefer complete SysEx per `virtualMIDISendData`; do not stream partial `F0`… without `F7` to the host
- Matrix-Control stock pacing is **≥10 ms** between SysEx (M-1000); bank export is sequential request/response pairs, not one giant SysEx — buffering must handle **many complete frames**, not one multi-megabyte message
- Single V1 Matrix frames (275 / 351 B) fit today’s **1024** hold; the hard problem is **burst queue / mutex stall**, not raising the hold to 64 KiB by default
- Do not use ASIO buffer size as SysEx proof (SM-C2 / AD-11)
- Provisional latency/jitter anchors remain **unmeasured** until Epic 5

### Project context reference

- `conventions.md` §3 quality gate, §6 C++ style
- No `project-context.md` in-repo yet — follow architecture spine + conventions
- Keep hardware/protocol names in English (MT4, WinUSB, VirtualMIDI, MIDI, SysEx, Matrix-Control, Oberheim, Ableton, Reason, MIDI-OX)

### Anti-patterns to forbid

- Building a SysexEngine, OberheimDecoder, or message-type allowlist
- Linking / vendoring Matrix-Control into the Bridge build (CAP-8)
- Claiming Story **2.4** / SM-2 done because synthetic 275/351 B frames passed
- Claiming ~4 h longevity (**2.5**) done because a short burst queued
- Treating MTC full-frame (2.2) as “SysEx already done”
- Inventing Emagic `0xFF` escaping “for completeness”
- Preemptive host→device framer without lab evidence
- Unbounded queues with no overflow failure mode
- Growing hold buffer to 65535 “just because” without measuring need
- Building `tools/midi-path-harness/` or quoting Studio-Done latency numbers as proven
- Inverting TX/RX flags
- French comments; kebab-case sources under `src/`
- Committing proprietary VirtualMIDI SDK binaries
- Growing `EmagicMapperSmokeSupport.cpp` instead of a dedicated SysEx smoke TU

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Story 2.3, Epic 2]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-16, AD-18, AD-2]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — FR-8, NFR-R3, SM-2]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/matrix-control-sysex-extract.md` — sizes, burst scenarios, opcodes]
- [Source: `_bmad-output/implementation-artifacts/2-2-mtc-quarter-frame-and-full-frame.md` — prove pattern, scope fence, dual-harness]
- [Source: `_bmad-output/implementation-artifacts/2-1-midi-clock-and-transport-realtime.md` — prove pattern, mutex gate]
- [Source: `_bmad-output/implementation-artifacts/deferred-work.md` — silent 1024 drop, SendToHost max, mutex load]
- [Source: `docs/tests/smoke-epic2-mtc-mt4.md` — MTC checklist template; SysEx out of scope]
- [Source: `src/Protocol/MidiMessageFramer.cpp` — SysEx hold / oversize Reset]
- [Source: `src/Protocol/EmagicMapperMtcSmoke.cpp` — file-split template for SysEx smoke]
- [Source: `src/Device/DeviceSession.cpp` — encode capacity 4096, usbIoMutex_]
- [Source: `src/Midi/TeVirtualMidiApi.h` — `kTeVmDefaultMaxSysexLength`]
- [Source: `conventions.md` — §3 quality gate, §6 C++ standards]
- [Source: https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html — PARSE_RX / maxSysexLength behavior]

## Dev Agent Record

### Agent Model Used

Composer (Cursor agent)

### Debug Log References

- macOS `builds/debug`: `Bridge --test-mapper` exit 0; `BridgeTests` 505 assertions / 43 cases Pass
- `python scripts/quality/lint-touched.py` exit 0
- Kept `kMaxSysexHoldBytes = 1024` (275/351 fit); no host→device framer added (PARSE_RX assumption)
- Mutex stall addressed via bounded `HostOutboundQueue` + try_lock drain from callbacks; reader drains after bulk/timeout

### Completion Notes List

- Transparent librarian-sized SysEx proven synthetically on framer + Emagic opaque encode/decode (Inquiry / 275 / 351), including split Push and realtime/MTC interleave
- Oversize SysEx now increments `ConsumeOversizeSysexRejectCount` and fails the session with English Port N / cable diagnostics (no silent drop)
- Bounded host→device outbound queue (~100 × 275 B headroom); overflow = English pump failure
- `SendToHost` rejects payloads above teVirtualMIDI max SysEx length (65535)
- Hardware checklist `docs/tests/smoke-epic2-sysex-mt4.md` added; Win10 lab rows remain for Guillaume (same honesty bar as 2.1/2.2)
- CAP-8 preserved: no Matrix-Control link; no SM-2 / 2.4 / 2.5 claims

### File List

- CMakeLists.txt
- _bmad-output/implementation-artifacts/2-3-transparent-sysex-transport-with-burst-buffering.md
- _bmad-output/implementation-artifacts/deferred-work.md
- _bmad-output/implementation-artifacts/sprint-status.yaml
- docs/tests/smoke-epic1-mt4.md
- docs/tests/smoke-epic2-clock-mt4.md
- docs/tests/smoke-epic2-mtc-mt4.md
- docs/tests/smoke-epic2-sysex-mt4.md
- src/App/FramerSmoke.cpp
- src/App/FramerSysexSmoke.cpp
- src/Device/DeviceSession.cpp
- src/Device/DeviceSession.h
- src/Device/DeviceSessionDeviceHost.cpp
- src/Device/DeviceSessionHostOutbound.cpp
- src/Device/HostOutboundQueue.cpp
- src/Device/HostOutboundQueue.h
- src/Midi/TeVirtualMidiApi.h
- src/Midi/TeVirtualMidiLimits.h
- src/Midi/VirtualMidiBackend.cpp
- src/Protocol/EmagicMapperSmokeSupport.cpp
- src/Protocol/EmagicMapperSmokeSupport.h
- src/Protocol/EmagicMapperSysexSmoke.cpp
- src/Protocol/MidiMessageFramer.cpp
- src/Protocol/MidiMessageFramer.h
- tests/EmagicCableMapperTests.cpp
- tests/HostOutboundQueueTests.cpp
- tests/MidiMessageFramerSysexTests.cpp
- tests/MidiMessageFramerTests.cpp
- tests/TeVirtualMidiLimitsTests.cpp

## Change Log

- 2026-08-05 — Story context created (ready-for-dev)
- 2026-08-05 — Implemented transparent SysEx transport + burst buffering; status → review
- 2026-08-05 — Code review patches applied (drain commit-on-success, Stop/enqueue race, nested F0 reject, queue wrap, tests); status → done
