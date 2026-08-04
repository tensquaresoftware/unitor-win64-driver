---
stepsCompleted:
  - step-01-validate-prerequisites
  - step-02-design-epics
  - step-03-create-stories
  - step-04-final-validation
inputDocuments:
  - _bmad-output/specs/spec-unitor-win64-driver/SPEC.md
  - _bmad-output/specs/spec-unitor-win64-driver/glossary.md
  - _bmad-output/specs/spec-unitor-win64-driver/validation-matrix.md
  - _bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md
  - _bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/addendum.md
  - _bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/matrix-control-sysex-extract.md
  - _bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md
  - _bmad-output/planning-artifacts/briefs/brief-unitor-win64-driver-2026-08-04/brief.md
  - _bmad-output/planning-artifacts/briefs/brief-unitor-win64-driver-2026-08-04/addendum.md
  - conventions.md
status: complete
project: unitor-win64-driver
created: 2026-08-04
updated: 2026-08-04
uxContract: none-v1
note: UX installer bar inherited from Architecture AD-12 checklist only; no bmad-ux DESIGN/EXPERIENCE
storyCount: 22
epicCount: 5
validation:
  frCoverage: pass
  architecture: pass
  storyQuality: pass
  epicStructure: pass
  dependencies: pass
  uxDrCoverage: n/a-ad12-via-story-4-1
---

# unitor-win64-driver - Epic Breakdown

## Overview

This document provides the complete epic and story breakdown for unitor-win64-driver, decomposing the requirements from the PRD, Spec capabilities, and Architecture Spine into implementable stories. UX design contract is absent for V1; installer UX inherits Architecture AD-12 checklist only.

## Requirements Inventory

### Functional Requirements

FR-1: WinUSB binding for MT4 (`VID 086A` / `PID 0003`) via installer/documented path; Zadig is contributor fallback only — CAP-1
FR-2: C++17 usermode Bridge maintains WinUSB session, Emagic cable multiplex/demultiplex, creates/destroys Virtual Ports via VirtualMIDI SDK — CAP-2
FR-3: Bridge Auto-Starts with Windows and/or on MT4 USB arrival; daily use needs no Admin — CAP-3
FR-4: Each MT4 exposes 2 input + 4 output Virtual Ports (physical shape) — CAP-4
FR-5: Stable Port Names: unit 1 `MT4 Port N`; unit K≥2 `MT4 #K Port N`; distinguishable multi-MT4 — CAP-5
FR-6: Transport notes, CC, and common channel/system MIDI — CAP-6
FR-7: Transport MIDI clock (`0xF8`), Start/Stop/Continue, and MTC (quarter-frame + full-frame) — CAP-7
FR-8: Transparent SysEx at editor/librarian scale; Matrix-Control validates, not a runtime dependency — CAP-8
FR-9: Multi-client: DAW + ShowMIDI concurrent on same relevant ports (no Bridge exclusive lock) — CAP-9
FR-10: Multi-MT4: two independent DeviceSessions / port sets (not Emagic cascade) — CAP-10
FR-11: Hot-plug recovery without Windows reboot (rescan / supervised Bridge restart OK) — CAP-11
FR-12: Public Installer UX bar + VirtualMIDI prerequisite fail-closed messaging — CAP-12
FR-13: User docs enable first MIDI and first SysEx (prereqs, install, Auto-Start, troubleshooting, works/does-not-work) — CAP-13
FR-14: Technical docs + license honesty (MIT ≠ VirtualMIDI ≠ future Windows MIDI Services); no GPL Linux sources vendored — CAP-14
FR-15: Authenticode strongly recommended; unsigned public build allowed only with SmartScreen docs — (trust / CAP-14 adjacent)
FR-16: Multi-DeviceProfile architecture; V1 validates MT4 only; cousins not promised without hardware — CAP-15

### Spec Capabilities (CAP inventory — machine contract)

CAP-1: Guided WinUSB bind without Zadig as primary path
CAP-2: Live C++17 Bridge session + VirtualMIDI SDK ports
CAP-3: Auto-Start (logon / USB arrival); no daily Admin
CAP-4: 2 IN + 4 OUT Virtual Ports per MT4
CAP-5: Locked port naming (AD-5 / AD-6)
CAP-6: Channel / system MIDI transport
CAP-7: Clock + Start/Stop/Continue + MTC
CAP-8: Transparent SysEx; Matrix-Control pass vectors
CAP-9: Multi-client DAW + ShowMIDI
CAP-10: Two independent MT4 sessions
CAP-11: Hot-plug without Windows reboot
CAP-12: Public Installer + VirtualMIDI prerequisite UX (AD-12)
CAP-13: Shipped user docs for UJ-1 / UJ-2
CAP-14: License / backend honesty; no GPL fork base
CAP-15: Declarative DeviceProfile boundary; MT4-only V1 product
CAP-16: MIDI Path harness + Studio-Done Gate (provisional anchors)
CAP-17: ~4h session stability including SysEx; buffered librarian dumps

### NonFunctional Requirements

NFR-P1: Bridge-added MIDI Path latency healthy target ≤ 4–5 ms p99; do-not-ship-worse ~8–10 ms p99 (provisional → Studio-Done Gate) — CAP-16
NFR-P2: MIDI Path jitter ≤ 1–2 ms p99 (provisional); excessive jitter is not a usermode alibi — CAP-16
NFR-P3: Measure MIDI Path only (not ASIO); publish method/results; harness design = Architecture AD-11 — CAP-16
NFR-R1: ~4 hours continuous studio/editor use including SysEx without mandatory Bridge restart — CAP-17
NFR-R2: Hot-plug recovery without Windows reboot — CAP-11 / FR-11
NFR-R3: SysEx bursts buffered for Matrix-Control librarian vectors (incomplete/corrupt dump = fail) — CAP-8 / CAP-17
NFR-D1: Win10 x64 + Win11 x64; Win10 mandatory in Validation Matrix
NFR-D2: Admin elevation at install only; not daily use
NFR-D3: Dual-machine loop (edit macOS / validate Windows); CI minimum = Windows build
NFR-S1: Authenticode strongly recommended; unsigned OK with SmartScreen docs if certificate lags
NFR-S2: No custom kernel driver attack surface in V1
NFR-Q1: Touched C++ passes `scripts/quality/lint-touched.py` per `conventions.md`
NFR-Q2: MIT original reimplementation; no vendored GPL Linux sources
NFR-Q3: `MidiBackend` abstraction for future Windows MIDI Services (Win11-only) without rewriting cable-mapping core

### Additional Requirements

#### Architecture decisions (AD-1…AD-20 — adopt, do not rewrite)

- AD-1: Usermode device-session pipeline; no custom kernel MIDI; no home-grown kernel VirtualMIDI Plan B
- AD-2: Module boundaries / dependency direction (mapper & DeviceProfile isolated from VirtualMIDI/WinUSB headers)
- AD-3: Declarative multi-PID DeviceProfile; V1 MT4 masks `in_cables=0x8003`, `out_cables=0x800f`, `ifnum=2`
- AD-4: One DeviceSession per MT4 instance (two units = two sessions)
- AD-5: Port naming spelling locked (`MT4 Port N` / `MT4 #K Port N`); ordinal K from DeviceSessionManager only
- AD-6: Stable unit identity (USB serial preferred; topology-path map fallback); sole owner of K
- AD-7: VirtualMIDI SDK is V1 MidiBackend; driver must be present; `aaron1a12/virtual-midi` = proof only (do not fork)
- AD-8: Multi-client via VirtualMIDI (up to 8 clients/port); Bridge must not add exclusive-open policy
- AD-9: Port lifecycle tied exclusively to live DeviceSession create/destroy via MidiBackend
- AD-10: Auto-Start + hot-plug recovery without Windows reboot
- AD-11: In-repo MIDI Path harness under `tools/midi-path-harness/`; results under `docs/dev/measurements/`
- AD-12: Installer = WinUSB INF/co-installer + DeviceInterfaceGUID `aa209017-cf8a-49ad-a0e7-701187ff7e05` + VirtualMIDI check + short UX checklist (8 items)
- AD-13: Windows compile CI every PR/main; pin runner image (prefer `windows-2022`); artifacts under `builds/`; CMake 3.20+ assumed
- AD-14: Emagic protocol = original MIT reimplementation; Linux reference read-only; no GPL in tree
- AD-15: Quality gate when C++ exists (lint-touched + PascalCase under `src/`)
- AD-16: Transparent SysEx transport (no Oberheim framing interpretation)
- AD-17: Full MIDI transport coverage (channel + clock + transport realtime + MTC + SysEx)
- AD-18: Session reliability + SysEx buffering for ~4h / librarian dumps
- AD-19: User docs, Ten Square Software facade, SmartScreen honesty if unsigned
- AD-20: Bridge runs as user-session process (not Session-0 Windows Service)

#### Structural seed (Epic 1 / foundation impact)

- Scaffold greenfield layout from Architecture Structural Seed: `src/App|Device|Profile|Usb|Protocol|Midi/`, `tools/midi-path-harness/`, `installer/`, `docs/user|dev/`, `builds/`
- Stack: C++17, WinUSB, VirtualMIDI SDK, CMake → `builds/`
- WinUsbTransport must open via DeviceInterfaceGUID (not VID/PID string alone)

#### Validation Matrix (inherit — do not reopen hosts)

- Hosts: Ableton Live 12, Reason Studios 12, Matrix-Control, ShowMIDI
- OS: Win10 x64 (mandatory) + Win11 x64
- Hardware: ≥1 MT4 `086A:0003`; second unit when available for FR-10 proof
- SysEx minimum pass vectors: Device Inquiry; single patch dump (~275 B); master dump (~351 B); edit-buffer/patch push; live editor stream; optional bank stress; mixed-wire tolerance (see Spec `validation-matrix.md` + PRD extract)
- Timing: MIDI Path harness required for Studio-Done Gate (CAP-16)

#### Success metrics (acceptance anchors for epic “done”)

- SM-1: Studio MIDI operability — Ableton Live 12 + Reason Studios 12 notes/CC/clock/Start-Stop-Continue/MTC on Win10 + Win11
- SM-2: SysEx operability — Matrix-Control minimum pass vectors on both OS targets
- SM-3: ~4h session stability including SysEx without mandatory Bridge restart
- SM-4: Hot-plug recovery without Windows reboot
- SM-5: Install + Auto-Start path to first MIDI without daily admin
- SM-6: Community release honesty — MIT vs VirtualMIDI; MSI clearance before redistributable Public Installer; SmartScreen docs if unsigned
- SM-7: Multi-client — DAW + ShowMIDI concurrent
- SM-8: Multi-MT4 design present; honest docs if only one unit physically validated
- SM-9: Studio-Done Gate — MIDI Path harness published; provisional anchors confirmed or revised

Counter-metrics (do not optimize in stories): cousin-device checkboxes over MT4 reliability; ASIO buffer as MIDI proof; “ports sometimes visible” MVP; jitter excused because usermode.

#### Explicit non-goals (scope fence for stories)

- Patch mode, LTC/VITC, Fast Mode / AMT
- Emagic cascaded / stacked multi-interface topologies
- Guaranteed AMT8 / Unitor8 products without hardware
- Windows MIDI Services as V1 backend; custom kernel MIDI driver; MIDI 2.0 claims
- Matrix-Control as Bridge runtime dependency
- Vendoring GPL Linux sources; forking `aaron1a12/virtual-midi` as base
- Inventing final latency thresholds before harness; blocking epics on Tobias MSI reply

#### Deferred / open (do not block epic design)

- OQ-1 / release gate: VirtualMIDI MSI redistribution — Public Installer release only
- OQ-2: Final latency/jitter thresholds after harness
- OQ-3: Authenticode personal vs org before first public release
- AQ-1: USB serial vs topology-path for unit ordinal K
- AQ-2: Hot-plug UX acknowledge vs silent recreate (lifecycle already AD-9)
- AQ-3: Pin VirtualMIDI SDK version
- AQ-4: Win11 dynamic ports / Windows MIDI Services coexistence notes

### UX Design Requirements

None — no bmad-ux `DESIGN.md` / `EXPERIENCE.md` for V1.

Installer UX work items inherited as Architecture checklist only (treat as UX-adjacent acceptance, not a UX contract):

UX-AD12-1: Few install steps
UX-AD12-2: Visible progress
UX-AD12-3: Clear success screen
UX-AD12-4: VirtualMIDI prerequisite explicit (fail closed if missing)
UX-AD12-5: WinUSB association succeeds
UX-AD12-6: Auto-Start wired
UX-AD12-7: One-time admin OK / daily admin not required
UX-AD12-8: Minimal jargon

Port naming is product-locked (AD-5 / AD-6) — do not invent alternate schemes in stories.

### FR Coverage Map

FR-1: Epic 1 — WinUSB bind for MT4 (guided path; Zadig fallback only)
FR-2: Epic 1 — Usermode Bridge session + VirtualMIDI SDK ports
FR-3: Epic 3 — Auto-Start (logon / USB arrival; no daily Admin)
FR-4: Epic 1 — 2 IN + 4 OUT Virtual Ports per MT4
FR-5: Epic 1 — Locked Port Names (`MT4 Port N`); Epic 3 verifies multi-unit `MT4 #K Port N`
FR-6: Epic 1 — Notes / CC / common channel MIDI
FR-7: Epic 2 — MIDI clock + Start/Stop/Continue + MTC
FR-8: Epic 2 — Transparent SysEx (Matrix-Control pass vectors)
FR-9: Epic 3 — Multi-client DAW + ShowMIDI
FR-10: Epic 3 — Two independent MT4 DeviceSessions
FR-11: Epic 3 — Hot-plug recovery without Windows reboot
FR-12: Epic 4 — Public Installer UX bar + VirtualMIDI prerequisite
FR-13: Epic 4 — User documentation (UJ-1 / UJ-2)
FR-14: Epic 4 — Technical docs + license honesty
FR-15: Epic 4 — Authenticode strongly recommended / SmartScreen if unsigned
FR-16: Epic 1 — Declarative multi-DeviceProfile boundary (MT4 validated)

CAP-16 / NFR-P1..P3 / SM-9: Epic 5 — MIDI Path harness + Studio-Done Gate
CAP-17 / NFR-R1 / NFR-R3: Epic 2 — Session longevity + SysEx buffering
NFR-Q1..Q3 / AD-13 / AD-15: Epic 1 — Scaffold, CI Windows build, lint gate, MidiBackend seam
NFR-D1..D3: Cross-cutting (Epic 1 CI + Epic 4/5 validation loop docs)
AD-12 checklist: Epic 4 (installer); partial WinUSB GUID bind also in Epic 1

## Epic List

### Epic 1: First Working MT4 MIDI
After this epic, a user (or contributor on the Windows box) can bind the MT4 to WinUSB, run the C++17 Bridge, see stable `MT4 Port N` endpoints (2 IN / 4 OUT), and exchange notes/CC — without a custom kernel driver.
**FRs covered:** FR-1, FR-2, FR-4, FR-5 (unit-1 naming), FR-6, FR-16
**Also:** CAP-1..2, CAP-4..6, CAP-15; AD-1..7, AD-13..15, AD-20 (process host baseline); scaffold + Windows CI + lint; `MidiBackend` seam
**User journeys:** Foundation for UJ-1

### Epic 2: Studio Transport and SysEx
After this epic, Validation Matrix DAWs can use clock / Start-Stop-Continue / MTC through the Bridge, and Matrix-Control can complete the minimum SysEx pass vectors without Bridge restart for normal librarian use — with buffering designed for ~4h sessions.
**FRs covered:** FR-7, FR-8
**Also:** CAP-7, CAP-8, CAP-17; AD-16, AD-17, AD-18; NFR-R1, NFR-R3; SM-1 (clock/MTC part), SM-2, SM-3
**User journeys:** Completes UJ-1 MIDI depth + UJ-2

### Epic 3: Daily Studio Resilience
After this epic, daily use needs no manual Bridge launch; unplug/replug restores ports without a Windows reboot; a DAW and ShowMIDI can share ports; two MT4s get distinguishable `MT4 #K Port N` names via separate sessions.
**FRs covered:** FR-3, FR-9, FR-10, FR-11 (plus FR-5 multi-unit verification)
**Also:** CAP-3, CAP-9..11; AD-8, AD-9, AD-10, AD-6 (ordinal stability); SM-4, SM-5 (Auto-Start part), SM-7, SM-8
**User journeys:** UJ-3, UJ-4; hardens UJ-1/UJ-2 for real sessions

### Epic 4: Community Install and Trust
After this epic, a new community user can install via the Public Installer UX bar (AD-12 checklist), follow shipped docs to first MIDI and first SysEx, and see honest MIT vs VirtualMIDI vs future Windows MIDI Services messaging — with Authenticode strongly recommended and SmartScreen documented if unsigned.
**FRs covered:** FR-12, FR-13, FR-14, FR-15
**Also:** CAP-12..14; AD-12, AD-19; SM-5, SM-6; OQ-1 noted as Public Installer release gate only
**User journeys:** Makes UJ-1/UJ-2 community-ready

### Epic 5: MIDI Path Proof (Studio-Done Gate)
After this epic, the project has an in-repo MIDI Path harness, published measurement method/results, and a clear path to confirm or revise provisional latency/jitter anchors — without using ASIO buffer size as proof.
**FRs covered:** (none numbered — Spec CAP-16 / NFR-P1..P3 / SM-9)
**Also:** AD-11; CAP-16; tools under `tools/midi-path-harness/`; results under `docs/dev/measurements/`
**User journeys:** Studio credibility gate (SM-9); does not invent final thresholds (OQ-2)

## Epic 1: First Working MT4 MIDI

After this epic, a user (or contributor on the Windows box) can bind the MT4 to WinUSB, run the C++17 Bridge, see stable `MT4 Port N` endpoints (2 IN / 4 OUT), and exchange notes/CC — without a custom kernel driver.

**FRs covered:** FR-1, FR-2, FR-4, FR-5 (unit-1), FR-6, FR-16  
**CAPs:** CAP-1, CAP-2, CAP-4, CAP-5, CAP-6, CAP-15  
**ADs:** AD-1..7, AD-13..15, AD-20 (baseline)

### Story 1.1: Scaffold Bridge project and Windows build gate

As a contributor,
I want a CMake C++17 Bridge skeleton that builds to `builds/` on Windows CI with the project lint gate wired,
So that every later story lands in a compileable, convention-compliant tree.

**Acceptance Criteria:**

**Given** a clean checkout on a Windows x64 environment (or GitHub Actions Windows runner)
**When** the CMake project is configured and built
**Then** artifacts land under `builds/` (not `build/` at repo root) and the Bridge executable target compiles
**And** the Structural Seed folders exist under `src/` (`App/`, `Device/`, `Profile/`, `Usb/`, `Protocol/`, `Midi/`) with PascalCase placeholder sources as needed
**And** CI runs a Windows compile on every PR / main push using a pinned runner image (prefer `windows-2022`, not floating `windows-latest`) — AD-13
**And** touched C++ under `src/` can be checked with `python scripts/quality/lint-touched.py` per `conventions.md` §3 — AD-15 / NFR-Q1
**And** no French appears in source; no custom kernel driver project is introduced — AD-1 / NFR-S2

**Traces:** FR-2 (runtime host baseline), NFR-D3, NFR-Q1, AD-13, AD-15, AD-20

### Story 1.2: Declarative MT4 DeviceProfile

As a Bridge developer,
I want a declarative DeviceProfile for MT4 (`086A:0003`) with cable masks and interface number,
So that cable mapping and future cousin devices do not hard-code PID logic in the mapper.

**Acceptance Criteria:**

**Given** the scaffold from Story 1.1
**When** the MT4 DeviceProfile is loaded by the Bridge
**Then** it declares at least `vid=086A`, `pid=0003`, `in_cables=0x8003`, `out_cables=0x800f`, `ifnum=2`, with Patch/LTC/FastMode capability flags off — AD-3
**And** Port N is defined as the N-th present cable bit in ascending order (IN Ports 1..2, OUT Ports 1..4)
**And** `EmagicCableMapper` / profile code does not include VirtualMIDI or WinUSB headers — AD-2
**And** cousin DeviceProfile stubs may exist as data only; V1 acceptance does not require shipping working AMT8/Unitor8 — FR-16 / CAP-15

**Traces:** FR-16, CAP-15, AD-2, AD-3

### Story 1.3: WinUSB bind path and transport open

As a Windows MT4 user (or contributor validating on hardware),
I want the MT4 associated with WinUSB and opened by the Bridge via the project DeviceInterfaceGUID,
So that the Bridge talks to the real device without Zadig as the primary community path.

**Acceptance Criteria:**

**Given** MT4 hardware `VID 086A` / `PID 0003` and the install/bind materials in-repo (INF or co-installer scripts under `installer/`)
**When** the guided bind path is applied on Win10 x64 or Win11 x64
**Then** Device Manager shows the MT4 associated with WinUSB per install docs — FR-1 / CAP-1
**And** `WinUsbTransport` opens the device using DeviceInterfaceGUID `aa209017-cf8a-49ad-a0e7-701187ff7e05` (not VID/PID string match alone) — AD-12
**And** docs state Zadig as contributor fallback only, not the primary community path
**And** opening fails closed with a clear diagnostic if WinUSB bind/GUID is missing (no silent “success” with empty I/O)

**Traces:** FR-1, CAP-1, AD-12 (transport + bind portion)

### Story 1.4: DeviceSession and Emagic cable mapper (usermode)

As a studio user,
I want the Bridge to keep a live usermode session that multiplexes/demultiplexes Emagic cables for one MT4,
So that physical IN/OUT cables map correctly without any custom kernel MIDI driver.

**Acceptance Criteria:**

**Given** WinUSB transport can open the MT4 (Story 1.3) and the MT4 DeviceProfile (Story 1.2)
**When** the Bridge starts a DeviceSession for one connected MT4
**Then** Emagic cable multiplex/demultiplex runs entirely in the C++17 usermode process — FR-2 / AD-1
**And** there is exactly one DeviceSession per MT4 instance (own WinUSB handle and mapper state) — AD-4
**And** protocol orientation is original MIT reimplementation; no GPL Linux sources are vendored (Linux quirk reference is read-only) — AD-14 / NFR-Q2
**And** the Bridge process is a user-session host (not a Session-0 Windows Service) — AD-20
**And** session start/stop emits English diagnostics sufficient to debug attach failures

**Traces:** FR-2, CAP-2, AD-1, AD-4, AD-14, AD-20

### Story 1.5: VirtualMIDI backend and stable MT4 Port names

As a DAW user,
I want the Bridge to create 2 IN + 4 OUT Virtual Ports named `MT4 Port N` via the VirtualMIDI SDK,
So that Ableton Live 12 / Reason / ShowMIDI / Matrix-Control can select physical-shaped endpoints.

**Acceptance Criteria:**

**Given** a live DeviceSession (Story 1.4) and the VirtualMIDI driver present on the machine
**When** the session starts
**Then** Virtual Ports are created programmatically via the VirtualMIDI SDK (not manual loopMIDI-only workflow) — FR-2 / AD-7
**And** exactly 2 input and 4 output endpoints appear per MT4 — FR-4 / CAP-4
**And** unit-1 display names are exactly `MT4 Port 1` … `MT4 Port 4` (same label on IN and OUT sides for cable N) — FR-5 / AD-5
**And** ordinal `K` / names are supplied by `DeviceSessionManager` into `MidiBackend`; the backend does not invent its own ordinal — AD-5 / AD-6
**And** only a live DeviceSession creates/destroys that unit’s ports through `MidiBackend` APIs — AD-9
**And** if VirtualMIDI is missing, the Bridge fails closed with an obvious fix path (no empty port list presented as success) — AD-12
**And** `MidiBackend` is an abstract seam; `VirtualMidiBackend` is the only V1 implementation (future Windows MIDI Services not shipped) — AD-2 / NFR-Q3
**And** `aaron1a12/virtual-midi` is not forked as project base (integration proof only) — CAP-14 / AD-7

**Traces:** FR-2, FR-4, FR-5, CAP-2, CAP-4, CAP-5, AD-5, AD-6, AD-7, AD-9

### Story 1.6: Notes and CC round-trip on all ports

As a studio user,
I want notes and CC to flow both ways between MT4 physical cables and the Virtual Ports,
So that I can play and automate through the Bridge on every IN/OUT.

**Acceptance Criteria:**

**Given** Virtual Ports are live with a connected MT4 (Stories 1.3–1.5)
**When** notes and CC are sent/received on each of the 2 IN and 4 OUT ports
**Then** round-trips succeed for Validation Matrix hosts that are available in the test loop (at minimum one DAW or ShowMIDI smoke) — FR-6 / CAP-6
**And** common channel/system messages required for basic playability are carried (not notes-only stubs that drop obvious channel traffic)
**And** no Bridge restart is required for sustained note/CC smoke of a normal short session
**And** failures are diagnosable via English logs (which port/cable failed)

**Traces:** FR-6, CAP-6, AD-17 (channel portion), SM-1 (notes/CC foundation)

## Epic 2: Studio Transport and SysEx

After this epic, Validation Matrix DAWs can use clock / Start-Stop-Continue / MTC through the Bridge, and Matrix-Control can complete the minimum SysEx pass vectors without Bridge restart for normal librarian use — with buffering designed for ~4h sessions.

**FRs covered:** FR-7, FR-8  
**CAPs:** CAP-7, CAP-8, CAP-17  
**ADs:** AD-16, AD-17, AD-18  
**NFRs:** NFR-R1, NFR-R3

### Story 2.1: MIDI clock and transport realtime

As a DAW user,
I want MIDI clock and Start/Stop/Continue to pass through the Bridge without dropouts under normal session use,
So that sequencers can slave or observe tempo/transport from MT4 Virtual Ports.

**Acceptance Criteria:**

**Given** Epic 1 notes/CC path is working on at least one IN and one OUT Virtual Port
**When** a Validation Matrix DAW (Ableton Live 12 or Reason Studios 12) sends or observes MIDI clock (`0xF8`) and Start / Stop / Continue through an MT4 Virtual Port
**Then** those messages are carried without Bridge-induced dropouts under a normal short sequencing smoke — FR-7 / CAP-7 / AD-17
**And** dropping clock or transport realtime is treated as a V1 defect (not a deferred nicety)
**And** the smoke is documented for both Win10 x64 and Win11 x64 when hardware is available (Win10 mandatory in matrix)

**Traces:** FR-7, CAP-7, AD-17, SM-1 (clock/transport portion)

### Story 2.2: MTC quarter-frame and full-frame

As a studio user syncing timecode,
I want MTC quarter-frame and full-frame messages used for sync to pass through the Bridge,
So that MIDI Time Code workflows work on the same MT4 Virtual Ports as performance MIDI.

**Acceptance Criteria:**

**Given** the clock/transport path from Story 2.1
**When** MTC quarter-frame and full-frame messages used for sync are sent or observed through an MT4 Virtual Port in a Validation Matrix DAW
**Then** they are carried without Bridge-induced dropouts under the same class of smoke as Story 2.1 — FR-7 / CAP-7
**And** MTC is in required V1 transport coverage (not optional polish) — AD-17
**And** failures identify the Virtual Port / cable in English diagnostics

**Traces:** FR-7, CAP-7, AD-17, SM-1 (MTC portion)

### Story 2.3: Transparent SysEx transport with burst buffering

As a SysEx editor/librarian user,
I want large and bursty System Exclusive messages carried transparently with enough buffering for real dumps,
So that editors complete transfers without a Bridge restart under normal librarian use.

**Acceptance Criteria:**

**Given** a live DeviceSession with Virtual Ports
**When** SysEx frames (including Oberheim Matrix-shaped traffic) flow between Virtual Ports and MT4 cables
**Then** the Bridge carries them transparently with no Emagic-side framing or rewriting of Oberheim payloads — FR-8 / AD-16
**And** SysEx bursts are buffered/queued so Matrix-Control librarian-scale dumps can complete without Bridge restart — NFR-R3 / AD-18
**And** Matrix-Control is not linked or bundled as a Bridge runtime dependency — CAP-8
**And** incomplete or corrupt dumps under normal test conditions are treated as failures

**Traces:** FR-8, CAP-8, CAP-17 (buffering), AD-16, AD-18, NFR-R3

### Story 2.4: Matrix-Control minimum SysEx pass vectors

As Guillaume validating Matrix-Control on Windows,
I want the locked minimum SysEx pass vectors to succeed over the Bridge on Win10 x64 and Win11 x64,
So that first-party editor/librarian traffic is proven for V1.

**Acceptance Criteria:**

**Given** Story 2.3 SysEx transport and a connected MT4 with Matrix-Control installed
**When** the minimum pass vectors from Spec `validation-matrix.md` / PRD extract are executed
**Then** all of the following succeed without Bridge restart for normal librarian completion:
1. Device Inquiry round-trip (`F0 7E 7F 06 01 F7` → Universal reply incl. Oberheim/Matrix identity)
2. Single patch dump (~275 B response)
3. Master dump (~351 B response)
4. Edit-buffer / patch push (outbound ~275 B; slot `01` and/or edit-buffer `0D`)
5. Live editor stream (short 7 B / 9 B remote edits at normal Matrix-Control spacing)
6. Mixed-wire tolerance (non-patch SysEx during a dump must not permanently block a later valid patch frame)
**And** optional bank stress (~100× 275 B) may be recorded when hardware/time allow but is not a hard gate
**And** results are noted for Win10 x64 (mandatory) and Win11 x64 — FR-8 / CAP-8 / SM-2

**Traces:** FR-8, CAP-8, SM-2, AD-16; companion `matrix-control-sysex-extract.md`

### Story 2.5: Session longevity design for ~4h studio use

As a studio user,
I want the Bridge designed so a ~4-hour session including SysEx activity does not require a mandatory restart for normal use,
So that long writing/editing days stay trustworthy.

**Acceptance Criteria:**

**Given** channel MIDI, clock/MTC, and SysEx paths from Stories 2.1–2.4
**When** the Bridge is run under a continuous studio/editor scenario including SysEx Sessions
**Then** the implementation is designed and documented for about **4 hours** continuous use without mandatory Bridge restart for normal operation — NFR-R1 / CAP-17 / AD-18
**And** a stability sample plan exists for at least Win10 x64 (matrix mandatory), including SysEx activity
**And** known leak/restart failure modes discovered in the sample are logged as defects (not accepted as “usermode limits”)
**And** this story does not invent final latency thresholds (those remain Epic 5 / OQ-2)

**Traces:** CAP-17, NFR-R1, AD-18, SM-3

## Epic 3: Daily Studio Resilience

After this epic, daily use needs no manual Bridge launch; unplug/replug restores ports without a Windows reboot; a DAW and ShowMIDI can share ports; two MT4s get distinguishable `MT4 #K Port N` names via separate sessions.

**FRs covered:** FR-3, FR-9, FR-10, FR-11 (FR-5 multi-unit verification)  
**CAPs:** CAP-3, CAP-9, CAP-10, CAP-11  
**ADs:** AD-6, AD-8, AD-9, AD-10, AD-20

### Story 3.1: Auto-Start without daily Administrator

As a studio user,
I want the Bridge to start with Windows logon and/or when the MT4 arrives on USB,
So that Virtual Ports are ready without launching the Bridge by hand every session.

**Acceptance Criteria:**

**Given** Epic 1–2 Bridge functionality installed on a user Windows session
**When** the machine reboots with MT4 connected, or the user plugs the MT4 after login
**Then** Virtual Ports become available on the happy path without a manual Bridge launch — FR-3 / CAP-3 / AD-10
**And** Auto-Start uses user-session registration (Task Scheduler at logon and/or Run-key equivalent and/or USB device-arrival start) — not a Session-0 Windows Service — AD-20
**And** daily operation does not require Administrator elevation — NFR-D2
**And** missing VirtualMIDI still fails closed with an obvious fix path (no silent empty success)

**Traces:** FR-3, CAP-3, AD-10, AD-20, SM-5 (Auto-Start portion)

### Story 3.2: Hot-plug recovery without Windows reboot

As a studio user who moves gear mid-session,
I want usable Virtual Ports to return after unplug/replug without rebooting Windows,
So that a rack move does not kill the whole PC session.

**Acceptance Criteria:**

**Given** a live session with Virtual Ports and a Validation Matrix host open (DAW and/or ShowMIDI)
**When** the MT4 is unplugged and then replugged
**Then** usable Virtual Ports return without requiring a Windows reboot — FR-11 / CAP-11 / NFR-R2
**And** recovery is a **new** DeviceSession that recreates ports under AD-6 identity; teardown destroys ports via `MidiBackend` (no orphan ports) — AD-9
**And** host port rescan and/or supervised Bridge restart are allowed and documented; requiring reboot is a V1 failure — AD-10
**And** AQ-2 (silent recreate vs acknowledge restart) may be noted as UX preference only — lifecycle ownership stays AD-9 (do not invent a second port authority)

**Traces:** FR-11, CAP-11, AD-9, AD-10, SM-4; deferred AQ-2

### Story 3.3: Multi-client DAW plus ShowMIDI

As a studio user,
I want a DAW and ShowMIDI to open the same relevant Virtual Ports at once,
So that I can monitor MIDI while sequencing without exclusive-lock dead ends.

**Acceptance Criteria:**

**Given** Virtual Ports are live
**When** Ableton Live 12 or Reason Studios 12 and ShowMIDI open the same relevant ports concurrently
**Then** both observe MIDI activity per VirtualMIDI multi-client semantics — FR-9 / CAP-9 / AD-8
**And** the Bridge does not add an exclusive-open policy on top of VirtualMIDI
**And** user/tech docs mention the VirtualMIDI ceiling of up to **8** clients per port
**And** any host-specific quirks discovered are documented when observed (they do not change the rule)

**Traces:** FR-9, CAP-9, AD-8, SM-7

### Story 3.4: Two MT4 units with stable distinguishable names

As a studio user with two MT4 interfaces,
I want each unit to get its own session and clearly named port set,
So that DAWs recall the right ports and units never cross-wire after relaunch/replug.

**Acceptance Criteria:**

**Given** two MT4 devices enumerated on one PC (or a simulated dual-instance test if only one physical unit is available — then docs must state validation status honestly)
**When** both units are connected and the Bridge runs
**Then** each unit has an independent DeviceSession (own WinUSB handle, mapper state, Virtual Port set) — FR-10 / AD-4
**And** unit 1 uses `MT4 Port N`; unit `K≥2` uses `MT4 #K Port N` exactly — FR-5 / AD-5
**And** `DeviceSessionManager` is the sole owner of ordinal `K` (USB serial preferred; topology-path map fallback) — AD-6
**And** ordinal `K` does not reshuffle for a known unit when another unit is (un)plugged
**And** Emagic cascade/stacked topologies are not implemented or claimed — non-goal
**And** AQ-1 (serial vs topology primary) is deferred to first dual-unit enumeration notes — do not block the story design

**Traces:** FR-5, FR-10, CAP-5, CAP-10, AD-4, AD-5, AD-6, SM-8; deferred AQ-1

## Epic 4: Community Install and Trust

After this epic, a new community user can install via the Public Installer UX bar (AD-12 checklist), follow shipped docs to first MIDI and first SysEx, and see honest MIT vs VirtualMIDI vs future Windows MIDI Services messaging — with Authenticode strongly recommended and SmartScreen documented if unsigned.

**FRs covered:** FR-12, FR-13, FR-14, FR-15  
**CAPs:** CAP-12, CAP-13, CAP-14  
**ADs:** AD-12, AD-19  
**UX:** AD-12 checklist items UX-AD12-1…8 (no bmad-ux contract)

### Story 4.1: Public Installer meeting AD-12 UX bar

As a community Windows user,
I want a short, clear Public Installer that binds WinUSB, installs the Bridge, wires Auto-Start, and checks VirtualMIDI,
So that I can reach first MIDI the same evening without a developer toolchain feel.

**Acceptance Criteria:**

**Given** a clean Win10 x64 or Win11 x64 machine and MT4 available
**When** the user runs the Public Installer path
**Then** the AD-12 UX checklist holds: (1) few steps, (2) visible progress, (3) clear success screen, (4) VirtualMIDI prerequisite explicit, (5) WinUSB association succeeds, (6) Auto-Start wired, (7) one-time admin OK / daily admin not required, (8) minimal jargon — FR-12 / CAP-12
**And** if VirtualMIDI is missing, install blocks with an obvious fix path (eval: loopMIDI/rtpMIDI; licensed MSI when cleared) — not an empty port list as success
**And** installer technology (WiX / Inno / other) is an implementation choice under this checklist only
**And** OQ-1 (Tobias MSI redistribution clearance) is a **release gate for redistributable Public Installer embedding VirtualMIDI only** — not a blocker to implement the installer path / eval prerequisite messaging

**Traces:** FR-12, CAP-12, AD-12, SM-5; release gate OQ-1

### Story 4.2: End-user documentation for first MIDI and SysEx

As a new MT4 owner on Windows,
I want shipped user docs that cover prerequisites, install, Auto-Start, first MIDI test, first SysEx test, troubleshooting, and works/does-not-work,
So that I can complete UJ-1 and UJ-2 without tribal knowledge.

**Acceptance Criteria:**

**Given** the Public Installer / Bridge from Story 4.1 and Epic 1–3 capabilities
**When** a new user follows only shipped `docs/user/` materials (plus named external prerequisites)
**Then** they can complete first MIDI (UJ-1) and first SysEx with Matrix-Control or documented equivalent path (UJ-2) — FR-13 / CAP-13 / AD-19
**And** docs cover VirtualMIDI prerequisites, install, Auto-Start, first MIDI test, first SysEx test, troubleshooting, and an explicit works / does-not-work list
**And** hot-plug recovery expectations (rescan / supervised restart OK; reboot = fail) are stated
**And** multi-MT4 validation honesty is stated if only one physical unit was proven

**Traces:** FR-13, CAP-13, AD-19, SM-5

### Story 4.3: Technical docs and three-way license honesty

As a contributor or community evaluator,
I want technical docs and public messaging that keep MIT, VirtualMIDI, and future Windows MIDI Services claims honest,
So that the project stays credible and legally clear.

**Acceptance Criteria:**

**Given** the public repository and README/docs surfaces
**When** a reader checks license and backend claims
**Then** materials state clearly: MIT (this repo) ≠ VirtualMIDI (proprietary, separate) ≠ Windows MIDI Services (future Win11-only backend, not V1) — FR-14 / CAP-14 / AD-19
**And** no GPL Linux sources are vendored; `aaron1a12/virtual-midi` is cited as integration proof only, not a fork base
**And** contributor docs describe the dual-machine loop (edit on macOS / validate on Windows x64; Win10 mandatory in matrix) — NFR-D3 / AD-13
**And** public facade is **Ten Square Software**

**Traces:** FR-14, CAP-14, AD-13, AD-19, SM-6

### Story 4.4: Authenticode policy and SmartScreen honesty

As a community downloader,
I want signed builds when possible, and clear SmartScreen guidance if a public build ships unsigned,
So that trust issues do not silently kill adoption.

**Acceptance Criteria:**

**Given** a public community build pipeline for the Bridge/installer
**When** Authenticode signing is available
**Then** signed builds use the Ten Square Software / chosen certificate path documented for the release — FR-15 / NFR-S1
**And** if a public build ships unsigned because the certificate lags, docs explain SmartScreen behavior and mitigation steps — AD-19
**And** Authenticode remains strongly recommended but **not** a hard V1 gate
**And** OQ-3 (personal vs org certificate) stays deferred to Guillaume before first tagged public community release — do not block this story

**Traces:** FR-15, NFR-S1, AD-19, SM-6; deferred OQ-3

## Epic 5: MIDI Path Proof (Studio-Done Gate)

After this epic, the project has an in-repo MIDI Path harness, published measurement method/results, and a clear path to confirm or revise provisional latency/jitter anchors — without using ASIO buffer size as proof.

**FRs covered:** (none numbered — Spec CAP-16 / NFR-P1..P3 / SM-9)  
**CAPs:** CAP-16  
**ADs:** AD-11  
**NFRs:** NFR-P1, NFR-P2, NFR-P3

### Story 5.1: In-repo MIDI Path harness scaffold

As a contributor measuring studio timing,
I want a C++17 MIDI Path harness in-repo that builds to `builds/`,
So that latency/jitter claims can be measured reproducibly instead of “feels fine.”

**Acceptance Criteria:**

**Given** the Bridge project scaffold and VirtualMIDI available on the Windows validation machine
**When** `tools/midi-path-harness/` is built
**Then** the harness compiles as C++17 with artifacts under `builds/` — AD-11 / CAP-16
**And** it can inject/observe timestamped MIDI using high-resolution clocks (`QueryPerformanceCounter` or equivalent)
**And** the harness measures the **MIDI Path** only — never ASIO buffer size — NFR-P3
**And** first iteration supports a Bridge-mediated Virtual Port software loop; hardware loopback is optional when a physical loop setup exists — AD-11 assumption
**And** Windows CI builds the harness when present (same pinned runner policy as the Bridge) — AD-13

**Traces:** CAP-16, NFR-P3, AD-11, SM-9 (harness exists)

### Story 5.2: Publish measurement method and baseline tables

As a project maintainer,
I want published method notes and latest measurement tables under `docs/dev/measurements/`,
So that Studio-Done discussions share the same evidence.

**Acceptance Criteria:**

**Given** the harness from Story 5.1 can run a software-loop (and hardware-loop when available)
**When** a measurement run completes on at least Win10 x64
**Then** `docs/dev/measurements/` contains method documentation plus latest tables including host OS, Bridge build, VirtualMIDI presence, and path type (software-loop vs hardware-loop) — AD-11
**And** reported metrics include bridge-relevant latency and jitter at p99 (or clearly labeled equivalent percentiles)
**And** results are explicitly labeled **provisional** until Studio-Done Gate revises anchors — NFR-P1 / NFR-P2
**And** ASIO buffer size is never cited as MIDI Path proof

**Traces:** CAP-16, NFR-P1, NFR-P2, NFR-P3, AD-11, SM-9

### Story 5.3: Studio-Done Gate decision record for timing anchors

As Guillaume (product owner),
I want a clear Studio-Done Gate checklist that confirms or revises the provisional latency/jitter anchors after harness evidence,
So that V1 does not claim “studio-done timing” on planning numbers alone.

**Acceptance Criteria:**

**Given** published measurements from Story 5.2
**When** the Studio-Done Gate is evaluated
**Then** the gate records one of: (a) confirm healthy targets ≤4–5 ms p99 latency and ≤1–2 ms p99 jitter, (b) revise anchors with rationale, or (c) defer ship-timing claim while staying under the do-not-ship-worse ceiling ~8–10 ms p99 unless an explicit product decision allows otherwise — NFR-P1 / NFR-P2 / OQ-2
**And** excessive jitter is not excused merely because the path is usermode
**And** this story does **not** invent final thresholds before measurement — it only defines the decision record / release note update path
**And** SM-9 is satisfiable: method exists; anchors confirmed or explicitly revised before calling timing “done”

**Traces:** CAP-16, NFR-P1, NFR-P2, SM-9, OQ-2, AD-11
