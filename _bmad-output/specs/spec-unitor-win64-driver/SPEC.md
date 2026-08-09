---
id: SPEC-unitor-win64-driver
companions:
  - glossary.md
  - validation-matrix.md
  - ../../planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md
  - ../../planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/matrix-control-sysex-extract.md
  - ../../../conventions.md
sources:
  - ../../planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md
  - ../../planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/addendum.md
  - ../../planning-artifacts/briefs/brief-unitor-win64-driver-2026-08-04/brief.md
  - ../../planning-artifacts/briefs/brief-unitor-win64-driver-2026-08-04/addendum.md
---

> **Canonical contract.** This SPEC and the files in `companions:` are the complete, preservation-validated contract for what to build, test, and validate. Source documents listed in frontmatter are for traceability only — consult them only if you need narrative rationale or prose color this contract intentionally omits. Mechanism depth (AD-1…AD-20) lives in the adopted Architecture Spine — do not re-decide those IDs here.

# SPEC: unitor-win64-driver V1

## Why

**Pain to solve + vision to realize.** Emagic Unitor-family USB interfaces (starting with the **MT4**, `086A:0003`) enumerate on modern Windows but expose no usable MIDI ports: the generic class stack does not implement Emagic cable mapping, and no official 64-bit vendor driver exists. Studio users and SysEx editors (including Ten Square’s **Matrix-Control**) need a trustworthy MIT-licensed **usermode bridge** — WinUSB transport, original Emagic protocol reimplementation, VirtualMIDI SDK ports — under the **Ten Square Software** facade, with stability, SysEx honesty, and packaging quality of a serious OSS hardware-support project rather than a ports-only MVP.

## Capabilities

- **CAP-1**
  - **intent:** End user can bind the MT4 to WinUSB through a guided install path without Zadig as the primary community path.
  - **success:** On clean Win10 x64 and Win11 x64, Device Manager shows MT4 associated with WinUSB per install docs; Zadig remains contributor fallback only.

- **CAP-2**
  - **intent:** User can run a C++17 usermode Bridge that maintains a live WinUSB session, performs Emagic cable multiplex/demultiplex, and creates/destroys Virtual Ports via the VirtualMIDI SDK (driver present).
  - **success:** With Bridge running and MT4 connected, Validation Matrix hosts open Virtual Ports and exchange MIDI; no custom kernel MIDI driver required.

- **CAP-3**
  - **intent:** Bridge Auto-Starts with Windows and/or on MT4 USB arrival so daily use needs no manual launch and no Administrator elevation.
  - **success:** After reboot with MT4 connected (or plug after login), Virtual Ports become available without manual Bridge launch on the happy path.

- **CAP-4**
  - **intent:** Each connected MT4 exposes physical-shaped Virtual Ports: **2 input** and **4 output**.
  - **success:** Ableton Live 12, Reason Studios 12, MIDI-OX, and Matrix-Control each see 2 IN + 4 OUT endpoints per MT4 instance.

- **CAP-5**
  - **intent:** Port Names stay stable across launches/replugs and remain distinguishable when multiple MT4s are present, using the locked Architecture spelling.
  - **success:** Unit 1 uses `MT4 Port N` (`N` 1..4); unit `K≥2` uses `MT4 #K Port N`; replug/relaunch does not randomly rename a known unit; two units do not cross-wire after relaunch.

- **CAP-6**
  - **intent:** Bridge transports notes, CC, and common channel/system MIDI between MT4 physical ports and Virtual Ports.
  - **success:** Note and CC round-trips succeed on each of the 2 IN / 4 OUT ports in Validation Matrix DAWs.

- **CAP-7**
  - **intent:** Bridge carries MIDI clock, Start/Stop/Continue, and MTC (quarter-frame and full-frame used for sync) for sequencing/sync use.
  - **success:** A Validation Matrix DAW can slave or observe clock (`0xF8`), Start/Stop/Continue, and MTC through an MT4 Virtual Port without Bridge-induced dropouts under the ~4h stability scenario.

- **CAP-8**
  - **intent:** Bridge carries SysEx transparently (including Oberheim Matrix frames from Matrix-Control) at editor/librarian scale — Matrix-Control is a validation target, not a Bridge runtime dependency.
  - **success:** Matrix-Control completes the minimum SysEx pass vectors in `validation-matrix.md` on Win10 x64 and Win11 x64 without Bridge restart for normal librarian completion; mixed-wire tolerance holds.

- **CAP-9**
  - **intent:** A DAW and a MIDI utility can use the same relevant Virtual Ports concurrently without Bridge-imposed exclusive locks.
  - **success:** Ableton Live 12 or Reason Studios 12 plus MIDI-OX open the same relevant ports concurrently and both observe MIDI activity (VirtualMIDI multi-client; document 8-client ceiling).

- **CAP-10**
  - **intent:** Two independent MT4 units on one PC are a supported design (separate sessions and port sets — not Emagic cascade).
  - **success:** Implementation supports two concurrent MT4 sessions; if only one physical unit is validated at ship, docs state multi-instance validation status honestly.

- **CAP-11**
  - **intent:** After unplug/replug, usable Virtual Ports return without a Windows reboot.
  - **success:** Documented host rescan and/or supervised Bridge restart restores MIDI; requiring a Windows reboot is a V1 failure.

- **CAP-12**
  - **intent:** Public Installer meets a short community UX bar and fails closed when VirtualMIDI is missing.
  - **success:** Install covers WinUSB association + Bridge + Auto-Start wiring + explicit VirtualMIDI prerequisite messaging; one-time admin OK; daily admin not required; missing VirtualMIDI shows an obvious fix path (Architecture AD-12 checklist).

- **CAP-13**
  - **intent:** Shipped user docs enable a new user to complete first MIDI and first SysEx without tribal knowledge.
  - **success:** Docs cover VirtualMIDI prerequisites, install, Auto-Start, first MIDI test, first SysEx test, troubleshooting, and an explicit works / does-not-work list sufficient for UJ-1 and UJ-2.

- **CAP-14**
  - **intent:** Technical docs and public messaging keep license and backend claims honest for contributors and users.
  - **success:** README/docs state MIT (this repo) ≠ VirtualMIDI (proprietary) ≠ Windows MIDI Services (future Win11-only backend); no GPL Linux sources vendored; `aaron1a12/virtual-midi` cited as integration proof only, not a fork base.

- **CAP-15**
  - **intent:** Architecture absorbs future DeviceProfiles without rewriting the Bridge core; V1 validates MT4 only.
  - **success:** A declarative DeviceProfile boundary exists for per-PID masks/capabilities; V1 acceptance does not require shipping working AMT8/Unitor8 products.

- **CAP-16**
  - **intent:** Timing claims are measurable on the MIDI Path so Studio-Done Gate can confirm or revise provisional anchors.
  - **success:** In-repo MIDI Path harness exists (`tools/midi-path-harness/` per AD-11); results publish under `docs/dev/measurements/`; ASIO buffer size is never used as MIDI proof; provisional anchors remain until confirmed (healthy ≤4–5 ms p99 bridge-added; jitter ≤1–2 ms p99; do-not-ship-worse ~8–10 ms p99).

- **CAP-17**
  - **intent:** Bridge sustains normal studio/editor sessions including SysEx without mandatory restart.
  - **success:** ~4 hours continuous use including SysEx Sessions without mandatory Bridge restart; Matrix-Control librarian vectors complete without incomplete/corrupt dumps under normal test conditions.

## Constraints

- Platforms: Windows **10 and 11**, 64-bit; **Win10 mandatory** in the Validation Matrix.
- Solution type: **C++17 usermode** Bridge (WinUSB + Emagic cable map); **no custom kernel MIDI driver** and **no home-grown kernel VirtualMIDI Plan B** in V1.
- MIDI backend V1: **VirtualMIDI SDK** (programmatic create/destroy); driver must be present; Windows MIDI Services allowed later as a **second backend on Win11 only**, behind the same `MidiBackend` abstraction.
- License: **MIT** original reimplementation; do not vendor GPL Linux sources; do not fork `aaron1a12/virtual-midi` as project base.
- Hardware V1: validated product is **MT4** (`VID 086A` / `PID 0003`); multi-DeviceProfile + multi-instance from day one.
- Port naming spelling locked (AD-5/AD-6): unit 1 `MT4 Port N`; unit `K≥2` `MT4 #K Port N`; `DeviceSessionManager` sole owner of ordinal `K`.
- Bridge runtime: **user-session** process (not Session-0 Windows Service) — AD-20.
- Build artifacts under **`builds/`** (never `build/` at repo root).
- C++ under `src/` uses **PascalCase** filenames; repo folders kebab-case; no French in source; touched C++ passes `scripts/quality/lint-touched.py` per `conventions.md` §3.
- Authenticode: **strongly recommended** for public builds; **not** a hard V1 gate — unsigned public build allowed only with SmartScreen documentation.
- VirtualMIDI author clearance / redistributable MSI: **release gate for Public Installer only** — not a Spec or Architecture blocker (owner: Guillaume; outreach sent, no reply yet).
- Public facade: **Ten Square Software**.
- Mechanism invariants AD-1…AD-20 in the adopted Architecture Spine are binding; Spec consumers must not reopen them without a documented course correction.
- Validation Matrix host/OS set is locked in `validation-matrix.md` (PRD inheritance); bumping DAW majors requires a PRD change.

## Non-goals

- Patch mode, LTC/VITC, Fast Mode / AMT in V1.
- Emagic-style cascaded / stacked multi-interface topologies in V1.
- Guaranteed AMT8 / Unitor8 / Unitor8 mk2 product support without real test hardware.
- “Windows MIDI Services only” as the V1 target.
- Custom kernel MIDI driver in V1.
- MIDI 2.0 as a V1 product claim for this hardware era.
- Treating Matrix-Control as a Bridge runtime dependency.
- Replacing or rewriting the Architecture Spine (status:final) from Spec.
- Inventing final latency/jitter thresholds before harness measurement (Studio-Done Gate).
- Blocking Spec/epics on Tobias / MSI redistribution terms (installer release gate only).

## Success signal

On Win10 x64 and Win11 x64, a user installs once, plugs an MT4, and finds stable **MT4 Port N** endpoints that Ableton Live 12 and Reason Studios 12 use for notes/CC/clock/Start-Stop-Continue/MTC; Matrix-Control completes the minimum SysEx vectors; MIDI-OX can observe concurrently with a DAW; ~4h sessions and hot-plug recovery work without Windows reboot; public materials state MIT vs VirtualMIDI honestly — and timing is only called Studio-Done after the MIDI Path harness confirms or revises provisional anchors.

## Assumptions

- Provisional latency/jitter anchors (≤4–5 ms p99 bridge-added; ≤1–2 ms p99 jitter; do-not-ship-worse ~8–10 ms p99) until Studio-Done Gate.
- IN and OUT appear as separate selectable endpoints as Windows UI requires; same `MT4 … Port N` label on both sides for cable `N`.
- “macOS-class installer” means the AD-12 short checklist (few steps, progress, success, VirtualMIDI explicit, WinUSB bind, Auto-Start, one-time admin, minimal jargon).
- VirtualMIDI multi-client meets CAP-9 (author docs: up to 8 clients/port); host quirks documented when observed.
- **Reason Studios 12** means the Reason 12 DAW product line (exact SKU string confirmable in user-facing docs later).
- MIDI-OX remains available as the V1 multi-client utility; substitution is a PRD change.
- Matrix-Control SysEx vectors match current Oberheim Matrix-1000 traffic; Guillaume may refine sizes/timeouts without reopening SysEx-as-required.
- Typical MT4 USB serial string may suffice for stable unit ordinal `K`; else topology-path map is authoritative (AQ-1 / AD-6).
- First MIDI Path harness iteration uses Bridge-mediated Virtual Port software loop; hardware loopback when available (AD-11).
- CMake 3.20+ minimum unless pinned VS generator requires newer (AD-13).

## Open Questions

- **AQ-1:** Does typical MT4 USB serial suffice for stable unit ordinal `K`, or must topology-path map be primary? (Revisit: first dual-unit USB enumeration on Windows; details in Architecture Spine.)
- **AQ-2:** Preferred supervised hot-plug UX — silent session recreate vs require Bridge restart acknowledgment? (Lifecycle ownership already fixed in AD-9; UX only.)
- **AQ-3:** Pin exact VirtualMIDI SDK version + redistributable terms once Tobias replies or eval SDK is frozen for first public build.
- **AQ-4:** VirtualMIDI dynamic port create/destroy behavior on current Win11 (including coexistence with Windows MIDI Services)? Document under AD-8 after first Win11 matrix soak.
- **Release gate (not Spec blocker):** VirtualMIDI evaluation vs redistribution / MSI terms (OQ-1) — owner Guillaume; blocks redistributable Public Installer only.
- **Studio-Done Gate (not Spec invent):** Final latency/jitter thresholds after MIDI Path harness (OQ-2).
- **Deferred (Guillaume):** Authenticode certificate path personal vs org Ten Square Software before first tagged public community release (OQ-3).
