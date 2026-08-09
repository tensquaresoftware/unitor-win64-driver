---
baseline_commit: e0ca8e0
---

# Story 3.3: Multi-client DAW plus ShowMIDI

Status: review

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a studio user,
I want a DAW and ShowMIDI to open the same relevant Virtual Ports at once,
so that I can monitor MIDI while sequencing without exclusive-lock dead ends.

## Acceptance Criteria

1. **Given** Virtual Ports are live  
   **When** Ableton Live 12 or Reason Studios 12 and ShowMIDI open the same relevant ports concurrently  
   **Then** both observe MIDI activity per VirtualMIDI multi-client semantics — FR-9 / CAP-9 / AD-8

2. **And** the Bridge does not add an exclusive-open policy on top of VirtualMIDI — AD-8

3. **And** user/tech docs mention the VirtualMIDI ceiling of up to **8** clients per port — AD-8

4. **And** any host-specific quirks discovered are documented when observed (they do not change the rule) — AD-8

**Traces:** FR-9, CAP-9, AD-8, SM-7

## Tasks / Subtasks

- [x] Task 1: Lock multi-client contract + fences (AC: 1, 2, 3, 4)
  - [x] Author operator smoke guide `docs/tests/smoke-epic3-multiclient-mt4.md` (kebab-case) with English Pass/Fail matrix for SM-7
  - [x] State V1 multi-client contract in that guide (see Dev Notes → Recommended shape)
  - [x] Explicit fences: dual-MT4 ordinal persistence → **3.4**; polished `docs/user/` multi-client chapter → **4.2**; Public Installer → **4.1**; latency harness → Epic **5**; hot-plug recreate stays **3.2** (do not reopen)
  - [x] Document VirtualMIDI **≤8 clients/port** ceiling (Tobias Erichsen author docs / AD-8) in the smoke guide (tech docs for this story). Polished end-user prose can wait for **4.2**, but AC3 must not ship blank — the smoke guide (and any short English contributor note you add under `docs/dev/`) counts as the tech-doc home for V1
  - [x] Cite SSOT: epics Story 3.3; PRD FR-9 / SM-7 / UJ-2; AD-8 (OQ-7 closed); SPEC CAP-9
  - [x] Note open adjacent items only: **AQ-3** (SDK pin) and **AQ-4** (Win11 / Windows MIDI Services coexistence) — document quirks if seen; do not block SM-7 Pass on pinning AQ-3

- [x] Task 2: Prove Bridge has no exclusive-open policy (AC: 2)
  - [x] Audit `src/Midi/VirtualMidiBackend.*` + `TeVirtualMidiApi.h` + `VirtualMidiWinSupport.h`: create flags remain `PARSE_* | INSTANTIATE_*` only — no Bridge-side client-count gate, no “first opener wins”, no exclusive-open flag layered on `virtualMIDICreatePortEx2`
  - [x] Confirm App (`MidiSessionCli`) and Device (`DeviceSession`) never refuse a second host; they only create/destroy the port set via AD-9
  - [x] Prefer a thin offline Catch2 contract (or comment + smoke citation) that locks the expected create-flag mask / “no exclusive policy” invariant — hardware SM-7 remains the product gate
  - [x] **If** lab fails exclusive-open: investigate VirtualMIDI install version / host open mode / Windows MIDI Services quirks (AQ-4) — **do not** “fix” by inventing Bridge exclusive locking
  - [x] Do **not** change AD-9 lifecycle, hot-plug loop, or dual-unit naming under this story ID

- [x] Task 3: Operator SM-7 smoke — DAW + ShowMIDI concurrent (AC: 1, 4)
  - [x] Smoke matrix rows (Win10 x64 mandatory; Win11 when available):
    1. Bridge live (`--auto-session` or `--start-session`) → Virtual Ports visible (`MT4 In N` / `MT4 Out N` for K=1 — use **actual** `DeviceSessionManager` display names, not the AD-5 shorthand alone)
    2. Open **Ableton Live 12** or **Reason Studios 12** on the same relevant ports
    3. Open **ShowMIDI** on the same relevant ports (concurrent — both stay open)
    4. Generate MIDI (notes/CC on at least one IN and/or OUT path under test) → **both** hosts observe activity
    5. Negative: exclusive-lock / “port in use” dead end from Bridge policy = **Fail**
    6. Record any host quirk (rescan needed, IN/OUT pairing oddity, Win11/WMS) in Notes — quirk doc does **not** change AD-8
  - [x] Honesty bar: blank lab rows ≠ Pass; substituting another utility for ShowMIDI is a **PRD change**, not a silent story rewrite
  - [x] Cross-link from `docs/tests/smoke-epic3-hotplug-mt4.md` and `docs/tests/smoke-epic3-autostart-mt4.md` fences (replace “→ 3.3” with pointer to the new smoke guide when Pass path exists)
  - [x] Lab note: Epic 2 ops sometimes avoided ShowMIDI for logging friction; MidiView historically caused BSOD on this machine — **SM-7 still requires ShowMIDI** per Validation Matrix; if ShowMIDI is unavailable, stop and escalate (do not invent a substitute)

- [x] Task 4: Regression + quality (AC: all)
  - [x] `--auto-session` hot-plug recreate from **3.2** still works (hosts may already be open — intentional)
  - [x] `Bridge --test-mapper` and `Bridge --test-port-names` still exit 0
  - [x] `ctest` / `BridgeTests` Pass when C++ changed
  - [x] If C++ changed: `python scripts/quality/lint-touched.py` exits 0; compile under `builds/`; no French in sources; Protocol/Profile free of VirtualMIDI/WinUSB
  - [x] Confirm no dual-MT4 work, no Session-0 service, no second port authority, no Windows MIDI Services backend switch

## Dev Notes

### Soft dependencies

| Dependency | Status | How this story treats it |
|---|---|---|
| Epic 1 VirtualMIDI backend + stable names | **done** | Hard — multi-client rides on existing `CreatePortSet` faces |
| Epic 2 transport / SysEx / longevity | **done** | Soft — do not reopen clock/MTC/SysEx; SM-7 ≠ SM-3 soak |
| Story **3.1** Auto-Start | **done** | Soft — product host may already be running |
| Story **3.2** hot-plug | **done** | Soft — concurrent hosts may stay open across unplug/replug; do not change recreate loop unless lab proves Bridge exclusive policy |
| Story **3.4** dual-MT4 | backlog | Keep K=1 naming; do not implement ordinal persistence |
| Story **4.2** user docs | backlog | Tech/smoke docs satisfy AC3 for V1; polished `docs/user/` later |
| OQ-7 | **closed by AD-8** | Assumption confirmed architecturally; this story is the SM-7 proof |
| AQ-3 / AQ-4 | open (deferred) | Document quirks; do not block on SDK pin |

### Scope fence

This story lands **FR-9 / CAP-9 / SM-7 multi-client proof**: DAW + ShowMIDI share ports; Bridge must not add exclusive-open; document the 8-client ceiling. It is **not** dual-MT4 naming, hot-plug redesign, or end-user docs polish.

| In scope | Out of scope (later / never) |
|---|---|
| SM-7 operator smoke (DAW + ShowMIDI concurrent) | Dual-MT4 ordinal persistence → **3.4** |
| Audit / lock “no exclusive-open” on VirtualMIDI create path | Inventing Bridge exclusive locking “to be safe” |
| Document ≤8 clients/port (tech/smoke) | Polished `docs/user/` chapter → **4.2** |
| Document host quirks when observed | Public Installer / VirtualMIDI redistributable → **4.1** / OQ-1 |
| Thin offline flag/contract test if useful | Windows MIDI Services backend switch |
| | Shared `MidiBackend` across concurrent DeviceSessions (deferred multi-unit) |
| | AMT8 / Unitor8 product claims |
| | Replacing ShowMIDI without a PRD change |

### Epic context

Epic 3 (“Daily Studio Resilience”) makes daily studio use survivable: no hand-launch (3.1), hot-plug without reboot (3.2), **multi-client (3.3)**, two-unit naming (3.4). Story **3.3** is the concurrent-host slice — proof that VirtualMIDI multi-client semantics work for Validation Matrix DAW + ShowMIDI, with the Bridge staying out of the way.

### Architecture compliance (must follow)

| Decision | Rule for this story |
|---|---|
| **AD-8** | VirtualMIDI allows multiple apps on the same virtual port (author docs: up to **8** clients/port). Concurrent DAW + ShowMIDI on the same relevant ports is required. Bridge must **not** add exclusive-open. Document the ceiling. Host quirks documented when observed — they do **not** change the rule. |
| **AD-7** | V1 backend remains runtime `teVirtualMIDI.dll` via `VirtualMidiBackend` (LoadLibrary SYSTEM32). Do not fork `aaron1a12/virtual-midi`. |
| **AD-9** | Only a live `DeviceSession` creates/destroys ports via `MidiBackend`. Multi-client is host-side opens of already-created ports — App must still never call Create/DestroyPortSet. |
| **AD-5 / AD-6** | Use ready-made names from `DeviceSessionManager` (V1 K=1: `MT4 In N` / `MT4 Out N`). Do not reshuffle names; dual-unit proof is **3.4**. |
| **AD-20** | Bridge stays a user-session process (already true). |
| Structural Seed | DAW + ShowMIDI + Matrix-Control all talk to VirtualMIDI ↔ Bridge; capability row: VirtualMIDI backend → `Midi/VirtualMidiBackend` under AD-7/8/9. |

### Current code baseline (UPDATE files)

**`src/Midi/VirtualMidiWinSupport.h` — today**
- IN flags: `PARSE_TX | INSTANTIATE_TX`
- OUT flags: `PARSE_RX | INSTANTIATE_RX`
- **No** exclusive / single-client flag bit exists in project-owned API surface

**`src/Midi/TeVirtualMidiApi.h` — today**
- Flag bits: ParseRx=1, ParseTx=2, InstantiateRx=4, InstantiateTx=8 (hand-rolled; **AQ-3** unpinned)
- Exports used: `virtualMIDICreatePortEx2`, `virtualMIDIClosePort`, `virtualMIDISendData`

**`src/Midi/VirtualMidiBackend.cpp` — today**
- Directional TX-only / RX-only faces (no shared bidirectional handle / local echo)
- Runtime LoadLibraryEx SYSTEM32 for `teVirtualMIDI.dll`
- Fail-closed English fix path if driver/DLL missing
- **No** client-count tracking; **no** open-refusal path for second host

**`src/Device/DeviceSession.*` — today**
- Sole Create/DestroyPortSet caller (AD-9)
- Unrelated to how many Windows apps open the virtual endpoints

**`src/App/MidiSessionCli.cpp` — today**
- Owns backend + session; does not gate hosts
- Hot-plug recreate (3.2) already assumes hosts may be open during drills

**What this story changes**
- Mostly **proof + docs**: SM-7 smoke guide, 8-client ceiling documentation, fence updates, optional thin offline contract on create flags
- C++ changes **only if** audit/lab finds a Bridge-imposed exclusive policy (unexpected) or a documented quirk needs a non-exclusive fix

**What must be preserved**
- Directional face create model and AD-9 lifecycle
- GUID-first WinUSB open; product Auto-Start / hot-plug loops
- VirtualMIDI fail-closed English fix path
- Port display names from `DeviceSessionManager` (`MT4 In N` / `MT4 Out N`)
- Binary output under `builds/` only
- Protocol/Profile isolation from VirtualMIDI/WinUSB

**Likely NEW**
- `docs/tests/smoke-epic3-multiclient-mt4.md`
- Optional: thin Catch2 contract under `tests/unit/` for create-flag mask / “no exclusive policy” comment invariant
- Optional: short English contributor note under `docs/dev/` if smoke alone feels thin for AC3 (prefer one home; avoid duplicate SSOT drift)

**Likely UPDATE**
- Cross-link fences in `docs/tests/smoke-epic3-hotplug-mt4.md`, `docs/tests/smoke-epic3-autostart-mt4.md`, longevity ownership tables if they still say “→ 3.3” without a target
- `VirtualMidiBackend` / flag headers — **only** if audit finds exclusive policy or lab requires a non-exclusive correction

**Likely leave alone**
- `DeviceSession` Start/Stop order, hot-plug recreate loop, Auto-Start registration
- `DeviceSessionManager` naming / dual-unit ordinal (`→ 3.4`)
- Emagic mapper, framers, WinUSB transport
- CMake LoadLibrary model (no linked teVirtualMIDI.lib)
- Public Installer / WiX / Inno

### Recommended implementation shape (not a second product decision)

1. **Assume green path:** VirtualMIDI already multi-clients; Bridge already non-exclusive → story is smoke + docs + audit.
2. **Audit:** Confirm create flags and call sites; add a small offline assertion if it prevents a future exclusive “safety” PR.
3. **Smoke:** Start Bridge → open DAW on e.g. `MT4 In 1` / `MT4 Out 1` → open ShowMIDI on the same → send/observe MIDI both ways as applicable → Pass/Fail matrix.
4. **Docs:** State ≤8 clients/port and “Bridge does not exclusive-lock”; record quirks in the smoke Notes column or a short Observed quirks section.
5. **Fail path:** If a host exclusive-opens, document which host and whether VirtualMIDI/WMS is involved (AQ-4). Escalate product/PRD only if ShowMIDI cannot participate — do not silently swap utilities.

**Anti-patterns (do not)**
- Adding Bridge logic that tracks MIDI clients or rejects a second `midiInOpen`/`midiOutOpen`
- “Fixing” multi-client by keeping ports alive after session Stop (rejected by AD-9)
- Claiming dual-MT4 concurrent sessions under this story ID
- Treating blank smoke rows as Pass
- Substituting MidiView / MIDI-OX / loopMIDI UI for ShowMIDI without PRD change (loopMIDI does not list Bridge ports anyway)
- Forking third-party VirtualMIDI samples into the tree
- Switching V1 backend to Windows MIDI Services

### Previous story intelligence (3.2 → 3.3)

- 3.2 explicitly fenced multi-client exclusive-open policy to **3.3** — hosts may already be open during hot-plug drills.
- Patterns to copy: English diagnostics, kebab-case smoke under `docs/tests/`, honesty bar (blank ≠ Pass), `lint-touched.py`, builds under `builds/`, AD-9 “App starts a session; never a second port authority”.
- Review lessons to carry: offline unit tests may be thin contracts — **hardware smoke is the real gate** for SM-7; document Ctrl+C preference (CTRL_CLOSE orphan risk remains deferred).
- Naming: product code uses `MT4 In N` / `MT4 Out N` (directional). Epic/AD shorthand “`MT4 Port N`” means the unit’s cable-N identity — smoke steps must use **live display names** operators will see.
- Deferred shared-`MidiBackend`-across-sessions is a **multi-unit** concern (closer to 3.4), not DAW+ShowMIDI.

### Git intelligence summary

- `e0ca8e0` / `0e238ab` — hot-plug recovery (`MidiSessionCli`, `Mt4PresenceWait`, diagnostics); leave recreate loop alone unless exclusive-open evidence appears.
- `e26be0e` — Epic 2 PC-only closure; do not reopen transport acceptance here.
- Pattern: flag-gated lab paths + operator smoke guides close Epic 3 resilience slices; product path remains `--auto-session`.

### Latest tech information (VirtualMIDI multi-client)

- Tobias Erichsen virtualMIDI driver: multi-client means each created virtual MIDI port can be opened by multiple applications concurrently; author-stated ceiling **currently 8 concurrent applications per port** (see tobias-erichsen.de virtualMIDI / update posts). Document that ceiling; do not invent a higher Bridge-side limit.
- Driver ships with loopMIDI / rtpMIDI installers; Bridge loads `teVirtualMIDI.dll` from System32 at runtime (AD-7).
- Win11 + Windows MIDI Services coexistence with dynamic VirtualMIDI create/destroy remains **AQ-4** — note quirks during Win11 matrix rows; do not switch backend.
- AQ-3: hand-rolled flag constants in `TeVirtualMidiApi.h` remain unpinned — out of scope to fully resolve unless lab proves wrong flags break multi-client (then pin with evidence).

### Testing requirements

| Gate | Expectation |
|---|---|
| Contributor smoke | `docs/tests/smoke-epic3-multiclient-mt4.md` Pass/Fail; Win10 mandatory for lab claim |
| Concurrent hosts | Ableton Live 12 **or** Reason Studios 12 **+** ShowMIDI on same relevant ports; both observe MIDI |
| Negative | Bridge-imposed exclusive-lock / “port busy” dead end = Fail |
| Docs | ≤8 clients/port stated in tech/smoke docs |
| Regression | 3.1 Auto-Start; 3.2 hot-plug; mapper/port-name tests; `lint-touched.py` if C++ touched |
| Offline | Optional flag-mask contract; does not replace SM-7 hardware |

### Project Structure Notes

- C++ under `src/` = **PascalCase**; docs/scripts = **kebab-case**.
- No French in sources or Bridge user-visible strings.
- Do not place multi-client policy under `src/Protocol/` or invent `src/Service/`.
- Builds only under `builds/` (reject repo-root `build/`).

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Epic 3 / Story 3.3]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — FR-9, SM-7, UJ-2, Validation Matrix ShowMIDI row, OQ-7]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-7, AD-8, AD-9]
- [Source: `_bmad-output/specs/spec-unitor-win64-driver/SPEC.md` — CAP-9]
- [Source: `_bmad-output/implementation-artifacts/3-2-hot-plug-recovery-without-windows-reboot.md` — fence + patterns]
- [Source: `src/Midi/VirtualMidiWinSupport.h` — create flags]
- [Source: `src/Midi/VirtualMidiBackend.cpp` — CreatePortSet directional faces]
- [Source: `src/Device/DeviceSessionManager.cpp` — `MT4 In N` / `MT4 Out N` display names]
- [Source: `docs/tests/smoke-epic3-hotplug-mt4.md` — fence → 3.3]
- [Source: https://www.tobias-erichsen.de/ — VirtualMIDI multi-client ≤8 apps/port]

### Project context reference

No `project-context.md` is present in-repo. Follow `conventions.md` §3 quality gate, AD-8 multi-client rule, AD-9 lifecycle, and the Structural Seed paths above.

## Dev Agent Record

### Agent Model Used

Cursor Grok 4.5 (bmad-dev-story)

### Debug Log References

- Audit: `VirtualMidiWinSupport.h` IN/OUT flags = PARSE/INSTANTIATE only; `VirtualMidiBackend::createDirectionalPortSet` passes those flags only; no client-count / exclusive-open gate in App or DeviceSession.
- Build: `builds/debug` Debug — Bridge + BridgeTests green; `ctest -C Debug` BridgeTests Pass; `--test-mapper` / `--test-port-names` exit 0; `lint-touched.py` OK on MultiClientContractTests.cpp.

### Completion Notes List

- Green path confirmed: Bridge already non-exclusive; story deliverable is SM-7 smoke + ≤8 clients/port docs + thin Catch2 create-flag contract.
- Authored `docs/tests/smoke-epic3-multiclient-mt4.md` with V1 contract, fences, SSOT citations, SM-7 Pass/Fail matrix, Observed quirks table, ShowMIDI-required honesty bar.
- Cross-linked fences from Autostart / Hot-plug smoke guides and Epic 2 longevity ownership table.
- Added `tests/unit/MultiClientContractTests.cpp` locking directional create-flag mask (no exclusive bits); wired into `BridgeTests` via CMakeLists.txt.
- No C++ product-path change required; AD-9 lifecycle / hot-plug / dual-unit naming untouched.
- Hardware SM-7 Pass/Fail rows remain for operator lab on Win10 x64 (blank ≠ Pass).

### File List

- `docs/tests/smoke-epic3-multiclient-mt4.md` (new)
- `docs/tests/smoke-epic3-autostart-mt4.md` (fence → 3.3 smoke guide)
- `docs/tests/smoke-epic3-hotplug-mt4.md` (fence → 3.3 smoke guide)
- `docs/tests/checklists/smoke-epic2-longevity-mt4.md` (fence → 3.3 smoke guide)
- `tests/unit/MultiClientContractTests.cpp` (new)
- `CMakeLists.txt` (BridgeTests + MultiClientContractTests)
- `_bmad-output/implementation-artifacts/3-3-multi-client-daw-plus-showmidi.md`
- `_bmad-output/implementation-artifacts/sprint-status.yaml`

### Change Log

- 2026-08-10: Implemented Story 3.3 multi-client proof (docs + offline create-flag contract); status → review.

## Story completion status

- Status: **review**
- Note: Implementation complete; hardware SM-7 Pass/Fail rows remain for operator lab on Win10 x64.
