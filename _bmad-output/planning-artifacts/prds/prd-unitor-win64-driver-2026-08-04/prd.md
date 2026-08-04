---
title: "PRD: unitor-win64-driver"
status: draft
created: 2026-08-04
updated: 2026-08-04
project: unitor-win64-driver
organization: Ten Square Software
source_brief: brief-unitor-win64-driver-2026-08-04
authenticode_policy: strongly-recommended-v1
---

# PRD: unitor-win64-driver

*Working title confirmed — product identity under **Ten Square Software**.*

## 0. Document Purpose

This PRD turns the ready Product Brief `brief-unitor-win64-driver-2026-08-04` into testable requirements for Architecture, UX naming details, epics, and implementation. It defines **what** V1 must deliver and how acceptance is judged. It does **not** reopen locked product decisions (platform, usermode stack, VirtualMIDI as V1 backend, MIT license, MT4-validated scope, SysEx required). Mechanism depth (WinUSB binding details, DeviceProfile shape, Exact port-string spelling for multi-MT4, measurement harness design) belongs in Architecture / UX or in `addendum.md`.

Audience: Guillaume (product owner / builder), future Architecture and epic authors, and community contributors who need a clear acceptance bar.

Traceability: every major requirement maps to the brief’s locked decisions and success bar. Brief path: `_bmad-output/planning-artifacts/briefs/brief-unitor-win64-driver-2026-08-04/`.

## 1. Vision

**unitor-win64-driver** restores usable MIDI on Windows 10/11 (64-bit) for Emagic Unitor-family USB interfaces, starting with the **MT4**. Today the device enumerates over USB, but DAWs and SysEx editors see no useful ports: Windows’ generic class stack does not implement Emagic’s proprietary cable mapping, and no official 64-bit vendor driver exists.

V1 ships a community-facing, MIT-licensed **usermode bridge**: WinUSB transport, an original Emagic protocol reimplementation (informed by public Linux references — no GPL sources vendored), and virtual MIDI ports via **VirtualMIDI** (Tobias Erichsen). Users install once, plug in the MT4, and find stable **MT4 Port N** endpoints suitable for studio performance MIDI **and** real SysEx editor/librarian traffic — including first-party validation of **Matrix-Control** on Windows.

Differentiation is honesty and operability for orphaned hardware: measurable timing discipline, multi-hour stability, auto-start, multi-client use, multi-MT4 design, friendly install, and clear licensing — not invented feature moats.

## 2. Target User

### 2.1 Jobs To Be Done

- **Functional:** Make my MT4 show reliable 2 IN / 4 OUT ports in my DAW and SysEx tools on modern Windows.
- **Functional:** Send and receive large SysEx dumps (editors/librarians) without dropping or corrupting messages.
- **Functional:** Keep working through a long session (~4 hours) and after unplug/replug without rebooting Windows.
- **Emotional:** Trust that a forgotten interface is usable again without Driver Hell or shady unsigned stacks presented as “the only way.”
- **Social / community:** Benefit from (and later contribute to) a serious OSS hardware-support project under Ten Square Software.
- **Contextual (first-party):** Validate Matrix-Control on Windows against a real MT4 over the same bridge community users will run.

### 2.2 Non-Users (v1)

- Users whose only need is Patch mode, LTC/VITC, Fast Mode/AMT, or Emagic-style cascaded multi-interface stacks.
- Users who require a guaranteed AMT8 / Unitor8 product in V1 without physical validation hardware.
- Users who insist on “Windows MIDI Services only” on Windows 10.
- Users seeking a custom kernel MIDI driver or MIDI 2.0 claims for this era hardware.

### 2.3 Key User Journeys

**UJ-1. Alex installs once and plays MIDI the same evening.**
- **Persona + context:** Alex, small-studio Windows user, owns an MT4 unused since moving to 64-bit Windows.
- **Entry state:** Fresh Windows 10 or 11 x64 PC; MT4 available; no prior Unitor driver success.
- **Path:** Runs the project installer → follows VirtualMIDI prerequisite messaging → elevates admin once if required → plugs MT4 → bridge auto-starts → opens Ableton Live 12 → selects stable MT4 Port N I/O → notes/CC/clock flow.
- **Climax:** Ports appear with clear names and MIDI works without manual bridge launch each session.
- **Resolution:** Daily use needs no Administrator; docs state what works.
- **Edge case:** If VirtualMIDI is missing, installer/docs block with an obvious fix path — not a silent empty port list.

**UJ-2. Sam dumps SysEx through Matrix-Control.**
- **Persona + context:** Sam (or Guillaume validating Matrix-Control) needs editor/librarian traffic, not only performance MIDI.
- **Entry state:** Bridge running; MT4 connected; Matrix-Control installed.
- **Path:** Selects MT4 ports in Matrix-Control → initiates dump/restore or editor session → observes complete SysEx exchange → may also have a DAW or ShowMIDI open (multi-client).
- **Climax:** Large/bursty SysEx completes without requiring a bridge restart for normal librarian use.
- **Resolution:** Confidence that Windows SysEx workflows are first-class in V1.
- **Edge case:** Mid-dump unplug → ports recover after replug + rescan/supervised restart; Windows reboot is a failure.

**UJ-3. Jordan runs two MT4 units without port confusion.**
- **Persona + context:** Jordan has two MT4s on one PC for expanded I/O.
- **Entry state:** Two MT4 devices connected; bridge supports multi-instance by design.
- **Path:** Both units expose separate 2 IN / 4 OUT port sets → names are stable and distinguishable per unit → Ableton Live 12 and Reason Studios 12 recall the intended ports across launches.
- **Climax:** No cross-wiring between units after relaunch/replug.
- **Resolution:** Multi-MT4 is a supported V1 design; if only one unit was validated at ship, docs say so honestly.
- **Edge case:** One unit unplugged — remaining unit’s names stay stable; no cascade topology expected.

**UJ-4. Riley hot-plugs mid-session.**
- **Persona + context:** Riley unplugs the MT4 to move a rack, then replugs.
- **Entry state:** Session in progress (DAW and/or ShowMIDI).
- **Path:** Unplug → replug → bridge restores usable ports without Windows reboot → Riley rescans ports in the host or accepts a supervised bridge restart if required.
- **Climax:** MIDI is usable again without rebooting Windows.
- **Resolution:** Hot-plug honesty documented (rescan/restart OK; reboot = fail).

## 3. Glossary

- **Bridge** — The V1 usermode Windows application/service that talks to the MT4 over WinUSB, implements Emagic cable mapping, and exposes Virtual Ports.
- **MT4** — Emagic MT4 USB MIDI interface, USB identity `VID 086A` / `PID 0003`, physical **2 IN / 4 OUT**.
- **Virtual Port** — A named virtual MIDI endpoint exposed to Windows MIDI clients (DAWs, utilities, editors).
- **Port Name** — Stable, macOS-like display name of form **MT4 Port N** (exact multi-unit disambiguation spelling deferred to Architecture/UX; product rule: distinguishable per unit).
- **DeviceProfile** — Declarative per-PID hardware profile (masks, interface number, capability flags). Architecture detail; V1 ships a validated MT4 profile and must not structurally block cousins.
- **SysEx Session** — Editor/librarian traffic including large or bursty System Exclusive transfers (e.g. Matrix-Control).
- **Matrix-Control** — Ten Square first-party SysEx validation target; **not** a runtime dependency of the Bridge.
- **Validation Matrix** — Locked V1 host set: **Ableton Live 12**, **Reason Studios 12**, **Matrix-Control**, **ShowMIDI**, on **Windows 10 x64** (mandatory) and **Windows 11 x64**.
- **MIDI Path** — End-to-end path used for latency/jitter measurement through the Bridge and Virtual Ports — **not** ASIO audio buffer size.
- **VirtualMIDI** — Tobias Erichsen’s proprietary virtual MIDI SDK/driver stack used as the V1 Virtual Port backend.
- **Windows MIDI Services** — Microsoft MIDI stack; allowed as a future second backend (Win11-oriented); **not** the V1-only target.
- **Auto-Start** — Bridge starts with Windows and/or on MT4 USB arrival so the user need not launch it manually before every session.
- **Hot-Plug Recovery** — After unplug/replug, usable Virtual Ports return without a Windows reboot; host rescan or supervised Bridge restart is acceptable.
- **Public Installer** — End-user installer intended for community redistribution (subject to VirtualMIDI author clearance).
- **Studio-Done Gate** — Acceptance milestone for timing: provisional latency/jitter targets may ship as planning anchors, but V1 is not “studio-done” until a published MIDI Path harness has measured results and thresholds are confirmed or explicitly revised.

## 4. Features

### 4.1 Device Binding and Bridge Runtime

**Description:** The user binds the MT4 to WinUSB through a guided install path and runs a C++ usermode Bridge that keeps a live session with the device. Realizes UJ-1, UJ-4.

**Functional Requirements:**

#### FR-1: WinUSB binding for MT4

The installer or documented install path binds MT4 (`VID 086A` / `PID 0003`) to Microsoft WinUSB for end users without requiring Zadig as the primary path. Realizes UJ-1.

**Consequences (testable):**
- After successful install on a clean Win10 x64 and Win11 x64 machine, Device Manager shows the MT4 bound as intended by the install docs (WinUSB association).
- Zadig may remain a developer fallback; user docs do not present it as the primary community path.

#### FR-2: Usermode Bridge session

The Bridge opens and maintains a WinUSB session with a connected MT4 and performs Emagic cable multiplex/demultiplex in usermode. Realizes UJ-1.

**Consequences (testable):**
- With Bridge running and MT4 connected, Validation Matrix hosts can open Virtual Ports and exchange MIDI.
- No custom kernel MIDI driver is required for V1 operation.

#### FR-3: Auto-Start

The Bridge Auto-Starts with Windows and/or on MT4 USB arrival. Realizes UJ-1.

**Consequences (testable):**
- After reboot with MT4 connected (or after plugging MT4 post-login), Virtual Ports become available without a manual Bridge launch for the normal happy path.
- Daily operation does not require Administrator elevation.

### 4.2 Virtual Ports and Naming

**Description:** Each MT4 exposes physical-shaped Virtual Ports with stable macOS-like names. Realizes UJ-1, UJ-3.

**Functional Requirements:**

#### FR-4: Port topology per MT4

Each connected MT4 exposes **2 input** and **4 output** Virtual Ports corresponding to physical I/O — not a flood of per-channel endpoints. Realizes UJ-1.

**Consequences (testable):**
- Ableton Live 12, Reason Studios 12, ShowMIDI, and Matrix-Control each see 2 IN + 4 OUT endpoints per MT4 instance.

#### FR-5: Stable Port Names

Port Names follow macOS-like **MT4 Port N**, remain stable across launches and replugs, and are unambiguously distinguishable when two MT4s are present. Realizes UJ-3.

**Consequences (testable):**
- Replug / relaunch does not randomly rename ports for a given unit.
- With two MT4s connected, a user can tell which port set belongs to which unit without guesswork.
- `[ASSUMPTION: IN and OUT appear as separate selectable endpoints as Windows UI requires.]`
- Exact multi-unit string scheme is Architecture/UX — product rule above is normative.

### 4.3 MIDI Message Coverage (including SysEx and Clock)

**Description:** V1 carries studio channel MIDI, MIDI clock, and SysEx large enough for real editors. Realizes UJ-1, UJ-2.

**Functional Requirements:**

#### FR-6: Channel and system MIDI

The Bridge transports notes, CC, and common channel/system messages between MT4 physical ports and Virtual Ports. Realizes UJ-1.

**Consequences (testable):**
- Note and CC round-trips succeed on each of the 2 IN / 4 OUT ports in the Validation Matrix DAWs.

#### FR-7: MIDI clock

MIDI clock/transport-related timing messages required for sequencing use are carried in V1. Realizes UJ-1.

**Consequences (testable):**
- A Validation Matrix DAW can slave or observe MIDI clock through an MT4 Virtual Port without Bridge-induced dropouts under the session-stability scenario (see NFR-R1).

#### FR-8: SysEx as a V1 requirement

SysEx is a required V1 capability, sized for real editor/librarian use including Matrix-Control. Realizes UJ-2.

**Consequences (testable):**
- Matrix-Control can complete representative dump/restore or editor exchanges over the Bridge on Win10 x64 and Win11 x64.
- Bursty/large SysEx does not require a Bridge restart for normal librarian completion.
- Matrix-Control is not bundled as a runtime dependency of the Bridge.

### 4.4 Multi-Client and Multi-Instance

**Description:** Concurrent clients and multiple independent MT4s are in V1 design. Realizes UJ-2, UJ-3.

**Functional Requirements:**

#### FR-9: Multi-client access

A DAW and a MIDI utility can use Virtual Ports concurrently without exclusive-lock dead ends. Realizes UJ-2.

**Consequences (testable):**
- Ableton Live 12 (or Reason Studios 12) and ShowMIDI open the same relevant Virtual Ports concurrently and both observe MIDI activity as designed by VirtualMIDI multi-client behavior.
- `[ASSUMPTION: VirtualMIDI multi-client semantics meet this requirement; Architecture confirms and documents any host-specific caveats.]`

#### FR-10: Multi-MT4 instances

Two independent MT4 units on one PC are a supported V1 design (separate sessions and port sets). Realizes UJ-3.

**Consequences (testable):**
- Design and implementation support two concurrent MT4 sessions without requiring Emagic cascade topology.
- If only one physical MT4 is available at ship time, release notes/docs state multi-instance validation status honestly.

**Out of Scope:** Emagic-style cascaded/stacked multi-interface topologies.

### 4.5 Hot-Plug Recovery

**Description:** Unplug/replug restores usability without rebooting Windows. Realizes UJ-4.

**Functional Requirements:**

#### FR-11: Hot-Plug without Windows reboot

After MT4 unplug and replug, usable Virtual Ports return without a Windows reboot. Realizes UJ-4.

**Consequences (testable):**
- Host port rescan and/or supervised Bridge restart may be required and must be documented.
- Requiring a Windows reboot to restore MIDI is a V1 failure.

### 4.6 Installer, Docs, Licensing, and Trust

**Description:** Community-grade packaging and honest messaging. Realizes UJ-1.

**Functional Requirements:**

#### FR-12: User-friendly Public Installer bar

The Public Installer feels closer to a polished macOS installer than a developer toolchain: short steps, visible progress, clear success, explicit VirtualMIDI prerequisite handling, minimal jargon. Realizes UJ-1.

**Consequences (testable):**
- One-time Administrator elevation at install is acceptable; daily use is not.
- Install covers WinUSB association + Bridge + Auto-Start wiring + VirtualMIDI prerequisite messaging.
- Shipping a redistributable Public Installer that embeds/redistributes VirtualMIDI requires prior author clearance (release gate).

#### FR-13: User documentation

User docs cover VirtualMIDI prerequisites, install, Auto-Start, first MIDI test, first SysEx test, troubleshooting, and an explicit works / does-not-work list. Realizes UJ-1, UJ-2.

**Consequences (testable):**
- A new user can complete UJ-1 and UJ-2 using only shipped docs (plus named external prerequisites).

#### FR-14: Technical documentation and license honesty

Technical docs enable contributors without shipping GPL Linux sources. Licensing messaging states: MIT for this repository; VirtualMIDI is proprietary and separate; Windows MIDI Services is a future backend, not the V1 claim. Realizes community credibility goals.

**Consequences (testable):**
- README/docs contain the three-way license clarity above.
- No GPL Linux sources are vendored in the repository.

#### FR-15: Authenticode policy

Authenticode signing is strongly recommended for public builds but is **not** a hard gate if the certificate lags. Realizes trust goals without blocking first public build.

**Consequences (testable):**
- If a public build ships unsigned, docs explain SmartScreen behavior and mitigation steps.
- Signed builds, when available, use the Ten Square Software / chosen certificate path documented for the release.

### 4.7 Extensibility Without Scope Creep

**Description:** Architecture must absorb future DeviceProfiles without rewriting the Bridge core. Realizes secondary-user promise.

**Functional Requirements:**

#### FR-16: Multi-DeviceProfile readiness

V1 includes a multi-DeviceProfile architecture and validates the MT4 profile; AMT8/Unitor8 are not promised without hardware. Realizes secondary-user path.

**Consequences (testable):**
- Architecture review can point to a DeviceProfile boundary for per-PID masks/capabilities.
- V1 acceptance does not require shipping working AMT8/Unitor8 profiles.

## 5. Non-Goals (Explicit)

- Patch mode, LTC/VITC, Fast Mode / AMT in V1.
- Emagic-style cascaded / stacked multi-interface topologies in V1.
- Guaranteed AMT8 / Unitor8 / Unitor8 mk2 support without real test hardware.
- “Windows MIDI Services only” as the V1 target (Win10 is mandatory).
- Custom kernel MIDI driver in V1.
- MIDI 2.0 as a V1 product claim for this hardware era.
- Vendoring GPL Linux sources into the tree.
- Treating Matrix-Control as a runtime dependency of the Bridge.
- Replacing the Architecture document — this PRD fixes WHAT / acceptance only.

## 6. MVP Scope

### 6.1 In Scope

- Platforms: Windows 10 and 11, 64-bit (Win10 mandatory in Validation Matrix).
- Hardware: MT4 (`086A:0003`), 2 IN / 4 OUT, multi-instance design for two units.
- Stack orientation (product-level): WinUSB + C++ usermode Bridge + VirtualMIDI Virtual Ports.
- MIDI: channel messages + MIDI clock + SysEx (required).
- Auto-Start, Hot-Plug Recovery, multi-client (DAW + ShowMIDI).
- Public Installer UX bar + user/technical docs + honest licensing.
- First-party SysEx validation via Matrix-Control.
- Code quality gate when C++ exists: `conventions.md` + `scripts/quality/lint-touched.py`.
- Public facade: Ten Square Software.
- Provisional timing targets + Studio-Done Gate (see §7 / §10).

### 6.2 Out of Scope for MVP

- Advanced Unitor features (Patch, LTC/VITC, Fast Mode/AMT) — deferred.
- Cascaded multi-interface stacks — deferred / likely never as a soft promise.
- Cousin DeviceProfiles as guaranteed products — post-MVP hardware-gated workstreams.
- Windows MIDI Services as V1 backend — post-MVP optional second backend (Win11).
- Kernel custom driver — rejected for V1.
- MIDI 2.0 claims — deferred indefinitely for this hardware generation.

## 7. Success Metrics

**Primary**

- **SM-1 Studio MIDI operability:** On Win10 x64 and Win11 x64, Ableton Live 12 and Reason Studios 12 can select MT4 Virtual Ports and exchange notes/CC/clock for a normal session. Validates FR-4, FR-6, FR-7.
- **SM-2 SysEx operability:** Matrix-Control completes representative SysEx editor/librarian exchanges over the Bridge on both OS targets. Validates FR-8.
- **SM-3 Session stability:** Continuous studio/editor use for about **4 hours** (including SysEx Session activity) without requiring a Bridge restart for normal use. Validates FR-6–FR-8, NFR-R1.
- **SM-4 Hot-Plug Recovery:** Unplug/replug restores usable ports without Windows reboot (rescan/supervised restart OK). Validates FR-11.
- **SM-5 Install & Auto-Start:** New user completes install + Auto-Start path and reaches first MIDI without daily admin. Validates FR-1, FR-3, FR-12, FR-13.
- **SM-6 Community release honesty:** Public materials state MIT vs VirtualMIDI clearly; VirtualMIDI author clearance obtained before redistributable Public Installer; Authenticode strongly recommended / SmartScreen documented if unsigned. Validates FR-12–FR-15.

**Secondary**

- **SM-7 Multi-client:** Ableton Live 12 (or Reason Studios 12) + ShowMIDI concurrent use works per FR-9.
- **SM-8 Multi-MT4 design:** Two-unit support exists in design; physical dual-unit proof when hardware available; otherwise honest docs. Validates FR-10, FR-5.
- **SM-9 Studio-Done Gate (timing):** Published MIDI Path measurement method exists; provisional targets confirmed or revised before calling timing “done.” Validates NFR-P1, NFR-P2.

**Counter-metrics (do not optimize)**

- **SM-C1 Feature breadth over reliability:** Do not count cousin-device checkboxes or advanced Unitor modes as success if they jeopardize MT4 SysEx/stability.
- **SM-C2 ASIO buffer as MIDI proof:** Do not treat audio buffer size as evidence of MIDI Path latency/jitter.
- **SM-C3 “Ports sometimes visible” MVP:** Partial enumeration without stable SysEx + Auto-Start + docs is not success.

## 8. Cross-Cutting NFRs

### Performance (provisional → Studio-Done Gate)

- **NFR-P1 Latency (provisional):** Bridge-added end-to-end latency on the MIDI Path aims for **low single-digit milliseconds at p99** beyond the host USB path. `[ASSUMPTION: planning anchor from brief; replace after harness measurement.]`
- **NFR-P2 Jitter (provisional):** Jitter on the MIDI Path aims for **sub-millisecond to low-millisecond** behavior suitable for studio clock/sequencing in the Validation Matrix DAWs. `[ASSUMPTION: planning anchor from brief; replace after harness measurement.]`
- **NFR-P3 Measurement method:** Latency/jitter claims must use a reproducible **MIDI Path** method (loopback and/or host-observable MIDI timing), with published host/buffer settings — **not** ASIO buffer size. V1 timing is not Studio-Done until measurement exists and thresholds are confirmed or explicitly revised in this PRD / release notes.

### Reliability

- **NFR-R1:** ~4-hour continuous operation including SysEx Sessions without mandatory Bridge restart for normal use.
- **NFR-R2:** Hot-Plug Recovery without Windows reboot (FR-11).
- **NFR-R3:** SysEx bursts buffered sufficiently for Matrix-Control librarian dumps (failure = incomplete/corrupt dump under normal test vectors).

### Platform & deployment

- **NFR-D1:** Win10 x64 and Win11 x64 supported; Win10 mandatory in Validation Matrix.
- **NFR-D2:** Admin elevation allowed at install only; not for daily use.
- **NFR-D3:** Dual-machine reality: primary editing may be macOS; build/USB/DAW/SysEx validation runs on Windows x64 (Win10 required in matrix). CI at minimum covers Windows build; hardware/DAW/SysEx remain an explicit validation loop.

### Security / trust

- **NFR-S1:** Authenticode strongly recommended; unsigned public build allowed with SmartScreen documentation if certificate lags.
- **NFR-S2:** No custom kernel driver attack surface in V1.

### Quality / maintainability

- **NFR-Q1:** C++ changes pass `scripts/quality/lint-touched.py` per `conventions.md`.
- **NFR-Q2:** MIT original reimplementation; no vendored GPL Linux sources.
- **NFR-Q3:** Virtual MIDI backend remains abstractable for a future Windows MIDI Services backend without rewriting cable-mapping core (Architecture).

## 9. Risks and External Dependencies

| Risk / dependency | Impact | Mitigation |
| --- | --- | --- |
| VirtualMIDI redistribution / MSI terms | Blocks Public Installer | Early author contact; eval path vs redistributable path documented; clearance is a release gate |
| Usermode latency/jitter | Studio credibility | Provisional NFRs + Studio-Done Gate + MIDI Path harness |
| Large/bursty SysEx | Matrix-Control / editors fail | Explicit SysEx acceptance tests; buffering |
| Scarce Emagic protocol docs | Implementation risk | Linux reference (no copy) + USB captures if needed |
| SmartScreen / unsigned builds | Users abandon download | Authenticode strongly recommended; document if deferred |
| Dual-machine / hardware access | Gaps in CI and dual-MT4 proof | Win10 x64 validation loop; honest multi-MT4 validation status |
| Multi-MT4 naming | Wrong DAW port recalls | Stable per-unit naming rule; Architecture/UX spelling |
| Cousin-device pressure | Scope creep | Non-goals + DeviceProfile + hardware-gated stories only |
| VirtualMIDI multi-client caveats | FR-9 fails | Confirm in Architecture; document host limits |

## 10. V1 Validation Matrix

| Layer | Target | OS | Notes |
| --- | --- | --- | --- |
| DAW | **Ableton Live 12** | Win10 x64, Win11 x64 | Performance MIDI + clock |
| DAW | **Reason Studios 12** | Win10 x64, Win11 x64 | Second DAW host |
| SysEx editor | **Matrix-Control** | Win10 x64, Win11 x64 | First-party SysEx path; not a Bridge dependency |
| MIDI utility | **ShowMIDI** | Win10 x64, Win11 x64 | Multi-client concurrent with a DAW |
| Hardware | ≥1 **MT4** (`086A:0003`) | — | Second MT4 when available for FR-10 proof |
| Timing harness | MIDI Path method (TBD in Architecture) | Win10 x64 (min) | Required for Studio-Done Gate |

Pass rules:
- Each DAW row: open ports, notes/CC, clock smoke on both OS targets.
- Matrix-Control: representative SysEx exchange on both OS targets.
- ShowMIDI + one DAW: concurrent observation without exclusive-lock failure.
- Stability sample: ~4h including SysEx activity on at least Win10 x64.
- Hot-plug: one documented recovery drill without Windows reboot.

## 11. Constraints and Guardrails (Locked Product Decisions)

Inherited from brief — **do not reopen** unless a blocking risk is documented:

| Topic | Decision |
| --- | --- |
| Platforms | Windows 10 and 11, 64-bit — Win10 required |
| Solution type | Usermode (WinUSB + C++ Bridge) — no custom kernel driver in V1 |
| MIDI backend V1 | VirtualMIDI SDK — Windows MIDI Services = v2 / second backend |
| License | MIT (original reimplementation) |
| Hardware V1 | MT4 validated; multi-DeviceProfile + multi-instance from day one |
| MIDI content | Channel + clock + SysEx (required) |
| Port naming | Stable macOS-like MT4 Port N; distinguishable multi-MT4 |
| Quality | conventions.md + lint-touched.py when C++ exists |
| Public facade | Ten Square Software |
| Authenticode | Strongly recommended; not a hard gate if certificate lags |

## 12. Open Questions

1. Exact VirtualMIDI evaluation vs redistribution / installer bundling terms after author contact (**blocker for Public Installer**).
2. Final numeric latency/jitter thresholds after MIDI Path harness measurement (replace provisional NFR-P1/P2); harness design details in Architecture.
3. Authenticode certificate path/cost (personal vs org Ten Square Software) and timing vs first public build.
4. Availability of original Emagic protocol documentation vs reference+capture fallback only.
5. CI/CD detail across macOS-primary development and Windows 10 x64 build/USB/DAW/SysEx validation (build CI minimum already required).
6. Exact multi-MT4 Port Name disambiguation spelling (product rule locked; Architecture/UX).
7. Confirm VirtualMIDI multi-client behavior meets FR-9 with Ableton Live 12 / Reason Studios 12 + ShowMIDI; document caveats.
8. Representative Matrix-Control SysEx test vectors (which dumps/editor operations define “pass”).

## 13. Assumptions Index

- `[ASSUMPTION]` NFR-P1/P2 provisional timing anchors (low single-digit ms bridge-added p99 latency; sub-ms to low-ms jitter) until harness locks numbers (Studio-Done Gate).
- `[ASSUMPTION]` IN vs OUT appear as separate selectable endpoints as Windows UI requires.
- `[ASSUMPTION]` “macOS-class installer” means few steps, clear progress, obvious success, minimal jargon — tooling is Architecture.
- `[ASSUMPTION]` VirtualMIDI multi-client semantics can satisfy FR-9; Architecture confirms.
- `[ASSUMPTION]` Reason product marketing name “Reason Studios 12” refers to the Reason 12 DAW line used for validation (confirm exact SKU string in docs if needed).
- `[ASSUMPTION]` ShowMIDI remains available and suitable as the V1 multi-client utility; if unavailable, substitute is a PRD change.

## 14. Traceability to Brief

| Brief element | PRD location |
| --- | --- |
| Problem / solution / locked decisions | §1, §11 |
| Users / success bar | §2, §7, §8 |
| In/out MVP | §5, §6 |
| SysEx + Matrix-Control | FR-8, SM-2, §10 |
| Latency deferred to PRD with anchors | NFR-P1–P3, Studio-Done Gate, OQ-2 |
| ~4h stability, hot-plug, multi-client, multi-MT4 | FR-9–FR-11, NFR-R1–R2, SM-3–SM-4, SM-7–SM-8 |
| Installer + docs + license honesty | FR-12–FR-14, SM-5–SM-6 |
| VirtualMIDI clearance gate | FR-12, SM-6, OQ-1 |
| Authenticode policy | FR-15, NFR-S1 |
| Open questions carried forward | §12 |
| Technical tables / rejected alternatives | `addendum.md` |
