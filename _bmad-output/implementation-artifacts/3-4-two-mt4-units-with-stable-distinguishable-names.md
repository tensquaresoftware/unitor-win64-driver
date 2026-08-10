---
baseline_commit: 42e4314
---

# Story 3.4: Two MT4 units with stable distinguishable names

Status: done

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a studio user with two MT4 interfaces,
I want each unit to get its own session and clearly named port set,
so that DAWs recall the right ports and units never cross-wire after relaunch/replug.

## Acceptance Criteria

1. **Given** two MT4 devices enumerated on one PC (or a simulated dual-instance test if only one physical unit is available — then docs must state validation status honestly)  
   **When** both units are connected and the Bridge runs  
   **Then** each unit has an independent DeviceSession (own WinUSB handle, mapper state, Virtual Port set) — FR-10 / AD-4

2. **And** unit ordinal `K==1` uses shipped directional names `MT4 In N` / `MT4 Out N`; unit `K≥2` uses `MT4 #K In N` / `MT4 #K Out N` exactly — FR-5 / AD-5 cable identity (`Port N` in planning docs = product-cable index `N`; **do not** revert to identical IN/OUT labels or invent a third scheme)

3. **And** `DeviceSessionManager` (plus its identity registry) is the sole owner of ordinal `K` (USB serial preferred; topology-path map fallback) — AD-6

4. **And** ordinal `K` does not reshuffle for a known unit when another unit is (un)plugged

5. **And** Emagic cascade/stacked topologies are not implemented or claimed — non-goal

6. **And** AQ-1 (serial vs topology primary) is deferred to first dual-unit enumeration notes — do not block the story design; record what the lab actually saw

**Traces:** FR-5, FR-10, CAP-5, CAP-10, AD-4, AD-5, AD-6, SM-8; deferred AQ-1

## Tasks / Subtasks

- [x] Task 1: Lock dual-MT4 contract + fences (AC: 1–6)
  - [x] Author operator smoke guide `docs/tests/smoke-epic3-dual-mt4-mt4.md` (kebab-case) with English Pass/Fail matrix for SM-8 / FR-10
  - [x] State V1 dual-unit contract in that guide (see Dev Notes → Recommended shape)
  - [x] Explicit fences: polished `docs/user/` multi-MT4 honesty chapter → **4.2**; Public Installer → **4.1**; multi-client proof stays **3.3**; hot-plug recreate loop stays **3.2** (extend to N units, do not invent a second lifecycle); latency harness → Epic **5**
  - [x] Honesty bar: if only one physical MT4 is available, simulated dual-instance / offline registry proof may Pass design claims — docs **must** state physical dual-unit validation status honestly (FR-10 / SM-8). Blank lab rows ≠ Pass for physical dual claim
  - [x] Cite SSOT: epics Story 3.4; PRD FR-5 / FR-10 / UJ-3 / SM-8; AD-4 / AD-5 / AD-6; SPEC CAP-5 / CAP-10
  - [x] Cross-link fences from `docs/tests/smoke-epic3-autostart-mt4.md`, `docs/tests/smoke-epic3-hotplug-mt4.md`, `docs/tests/smoke-epic3-multiclient-mt4.md` (replace “→ 3.4” with pointer to the new smoke guide)
  - [x] Note AQ-1 as lab observation only — prefer serial when present; topology path must still work as fallback

- [x] Task 2: Enumerate + open by identity (AC: 1)
  - [x] Replace “exactly one GUID/HWID match or refuse” as the **only** success mode with: enumerate all present MT4 WinUSB interfaces (project GUID + `ifnum`), each with a durable device path
  - [x] Extend `WinUsbOpenOptions` (or equivalent) so `WinUsbTransport::Open` can bind a **specific** device path / instance; keep GUID-first product path; Zadig/HWID fallback must also refuse ambiguous multi-match **unless** a concrete instance is selected
  - [x] Extend presence helper (`Mt4WinUsbPresence` / related) from boolean Present to count/list of live interfaces (paths + identity keys) for auto-session / hot-plug
  - [x] Fail closed with English diagnostics when open is still ambiguous (no selected path and `matchCount != 1`)
  - [x] Do **not** share one WinUSB handle across units (AD-4)

- [x] Task 3: Stable ordinal `K` registry (AC: 2, 3, 4, 6)
  - [x] Implement durable unit-identity → `K` mapping owned only by `DeviceSessionManager` / a Device-layer registry it owns (AD-6). Prefer USB serial string when present; else USB topology path (`bus/port` chain or equivalent durable instance key)
  - [x] Persist the map across Bridge relaunches (local user-scoped file under the Bridge process identity — no second naming authority in App/Midi)
  - [x] Assignment rules: known identity keeps its `K`; new identity gets the next free `K` (`1` then `2`…); unplug of unit A must **not** renumber unit B
  - [x] `buildPortNameSet` must take / use the assigned `K` (retire hard-wired `unitOrdinalK_ = 1` as the only path)
  - [x] Keep shipped formatter SSOT `formatPortDisplayName` directional spelling (`MT4 In/Out` / `MT4 #K In/Out`) — already matches chassis silkscreen
  - [x] MidiBackend continues to receive **ready-made** `PortNameSet` strings — never re-derives `K` from USB
  - [x] Record first dual-unit enumeration notes for AQ-1 (serial usable? empty? topology primary?) without blocking design
  - [x] Offline Catch2 (or `--test-port-names` expansion) locking: K=1 / K=2 name strings; persistence “known unit keeps K when peer leaves”; no MidiBackend inventing names

- [x] Task 4: Concurrent DeviceSessions on the product host (AC: 1, 2, 4)
  - [x] Host **N** live units under `--auto-session` (and keep `--start-session` usable for lab): each unit = own `DeviceSession` + own `VirtualMidiBackend` + own `PortNameSet(K)` (AD-4 / AD-9)
  - [x] Do **not** share one `MidiBackend` / one `CreatePortSet` across two sessions (deferred-work ambiguity → resolve as **one backend instance per session**)
  - [x] Extend hot-plug loop: per-unit Absent → Stop/DestroyPortSet that unit only; Present → new session under same AD-6 `K`; remaining units keep ports and `K`
  - [x] Diagnostics print each unit’s `K`, identity key class (serial vs topology), and expected port names
  - [x] Single-unit path must keep working identically for operators with one MT4 (`K=1` names unchanged)

- [x] Task 5: Operator SM-8 / dual-unit smoke + honesty (AC: 1, 2, 4, 5, 6) — lab when hardware allows
  - [x] Smoke matrix rows (Win10 x64 mandatory for physical claim; Win11 when available):
    1. Two MT4s connected (or simulated dual path documented) → Bridge `--auto-session`
    2. Hosts list two distinguishable port sets: unit1 `MT4 In/Out N`; unit2 `MT4 #2 In/Out N` (use **live** display names)
    3. Optional light MIDI activity on one cable per unit proves sessions are independent (no cross-wire)
    4. Unplug unit 2 only → unit 1 names/`K` unchanged; replug unit 2 → same `#2` names return (AD-6)
    5. Relaunch Bridge with both connected → same `K` assignment for known identities
    6. Negative: claiming cascade / stacked Emagic topology = Fail / out of scope
    7. Honesty: if only one physical unit → document “physical dual not proven”; do not mark physical dual Pass
  - [x] AQ-1 notes row: which identity key was primary in lab

- [x] Task 6: Regression + quality (AC: all)
  - [x] Single-unit Auto-Start (3.1) and hot-plug (3.2) still work
  - [x] Multi-client (3.3) still applies per port set (no exclusive-open regression)
  - [x] `Bridge --test-mapper` and `Bridge --test-port-names` still exit 0 (expand port-name tests for K≥2 + registry contracts as needed)
  - [x] `ctest` / `BridgeTests` Pass when C++ changed
  - [x] `python scripts/quality/lint-touched.py` exits 0 on touched C++; compile under `builds/`; no French in sources; Protocol/Profile free of VirtualMIDI/WinUSB
  - [x] Update `_bmad-output/implementation-artifacts/deferred-work.md`: retire multi-unit open ambiguity / shared-backend ownership notes that this story closes
  - [x] Confirm no Session-0 service, no second port authority, no Windows MIDI Services backend, no AMT8/Unitor8 product claims, no Emagic cascade

### Review Findings

- [x] [Review][Patch] Document `--dev-zadig` as single-unit lab only; reopen Zadig multi-match honesty in deferred-work (product GUID multi-unit stays closed by 3.4) [`docs/tests/smoke-epic3-dual-mt4-mt4.md`] [`_bmad-output/implementation-artifacts/deferred-work.md`]
- [x] [Review][Patch] Quarantine unreadable identity registry (rename aside) and start with an empty map plus clear English warning instead of refusing Bridge start [`src/App/MidiSessionMultiHost.cpp`] [`src/Device/UnitIdentityRegistry.cpp`]
- [x] [Review][Patch] Hot-plug Start or interface-list failure aborts the whole Bridge (peers die) with no PnP settle retry [`src/App/MidiSessionMultiHostHotPlug.cpp`]
- [x] [Review][Patch] Serial↔topology identity flip can assign a new `K` to the same physical unit [`src/Usb/WinUsbEnumerate.cpp`] [`src/Device/UnitIdentityRegistry.cpp`]
- [x] [Review][Patch] Stable-looking serial whose UTF-8 conversion is empty is accepted as Serial instead of falling through to topology [`src/Usb/WinUsbEnumerate.cpp`]
- [x] [Review][Patch] Empty `selectedDevicePath` after UTF-8→wide conversion falls through to unique-match / Zadig open [`src/Usb/WinUsbOpenIdentity.cpp`] [`src/Usb/WinUsbTransportInit.cpp`]
- [x] [Review][Patch] Duplicate identity keys are handled silently (`emplace` keeps one; missing start skips peers) instead of fail-closed English diagnostics [`src/App/MidiSessionMultiHost.cpp`]
- [x] [Review][Patch] Registry `saveToFile` truncates in place (crash mid-write can brick next launch); `stoul` accepts partial ordinal tails [`src/Device/UnitIdentityRegistryIo.cpp`]
- [x] [Review][Patch] Empty `devicePathUtf8` after wide→UTF-8 loss still enters the present list [`src/App/Mt4WinUsbPresence.cpp`]
- [x] [Review][Defer] Offline hot-plug coverage still only asserts wait-constant aliases while product loop no longer calls that wait helper — deferred, pre-existing thin offline pattern (hardware SM-4 / dual smoke remain the gate) [`tests/unit/HotPlugContractTests.cpp`]
- [x] [Review][Defer] Topology identity keys use full USB instance ID and change when the unit moves hub/port — deferred, known serial-less fallback limit (AQ-1 lab notes) [`src/Usb/WinUsbEnumerate.cpp`]
- [x] [Review][Defer] Two Bridge processes can race the same registry file — deferred, V1 assumes a single Bridge host [`src/Device/UnitIdentityRegistryIo.cpp`]

## Dev Notes

### Soft dependencies

| Dependency | Status | How this story treats it |
|---|---|---|
| Story **1.5** VirtualMIDI + unit-1 names | **done** | Hard — extend `DeviceSessionManager` / formatter; do not reinvent naming |
| Story **1.6** notes/CC | **done** | Soft — per-session mapper already works; prove independence, don’t reopen MIDI framing |
| Epic 2 transport / SysEx | **done** | Soft — do not reopen clock/MTC/SysEx acceptance |
| Story **3.1** Auto-Start | **done** | Soft — product host remains `--auto-session`; must host N sessions |
| Story **3.2** hot-plug | **done** | Hard pattern — extend recreate to per-unit; keep AD-9 destroy-on-teardown |
| Story **3.3** multi-client | **done** | Soft — each unit’s ports remain multi-client capable; no exclusive-open |
| Story **4.2** user docs honesty | backlog | Smoke/tech docs carry V1 honesty; polished `docs/user/` later |
| AQ-1 | open (deferred) | Lab notes only; design ships serial-prefer + topology fallback |

### Scope fence

This story lands **FR-10 / FR-5 multi-unit**: two independent sessions, distinguishable stable names, AD-6 ordinal persistence. It is **not** cascade topology, end-user docs polish, installer work, or a MIDI Path latency gate.

| In scope | Out of scope (later / never) |
|---|---|
| Enumerate + open N MT4 instances by identity | Emagic cascade / stacked multi-interface topologies |
| Durable `K` registry (serial → topology fallback) | Embedding USB serial **in** the display name string |
| N× DeviceSession + N× VirtualMidiBackend | Sharing one MidiBackend across sessions |
| Names `MT4 In/Out N` and `MT4 #K In/Out N` | Reverting to abstract identical `MT4 Port N` on both faces, or `Input`/`Output` words |
| Dual-unit / simulated dual smoke + honesty docs | Polished `docs/user/` → **4.2** |
| Extend 3.2 hot-plug to per-unit without reshuffling `K` | Second port authority / keep-alive ports after Stop (rejected by AD-9) |
| AQ-1 enumeration notes | Blocking on AQ-1; inventing tray GUI for AQ-2 |
| | Public Installer / OQ-1; Windows MIDI Services backend; AMT8/Unitor8 claims |
| | Claiming physical dual Pass with one hardware unit |

### Epic context

Epic 3 (“Daily Studio Resilience”) makes daily studio use survivable: no hand-launch (3.1), hot-plug without reboot (3.2), multi-client (3.3), **two-unit naming (3.4)**. Story **3.4** is the multi-MT4 slice — Jordan’s UJ-3: two units, distinguishable stable port sets, no cross-wire after relaunch/replug.

### Naming SSOT (critical — prevent “fix AD-5” regressions)

Planning docs (AD-5 / epic AC) use cable-identity shorthand:

- `K==1`: `MT4 Port N`
- `K≥2`: `MT4 #K Port N`

**Shipped product code** (Story 1.5 intentional silkscreen alignment) uses directional faces because Windows lists IN and OUT separately and identical labels collide:

- `K==1`: `MT4 In N` / `MT4 Out N`
- `K≥2`: `MT4 #K In N` / `MT4 #K Out N`

**Rule for this story:** keep the **directional** SSOT already in `formatPortDisplayName`. Treat epic/AD-5 `Port N` as the cable index identity, not a rename ticket. Smoke steps must print **live** names operators will see. Do not introduce a third scheme (`MT4 Input`, serial suffixes in the label, etc.).

### Architecture compliance (must follow)

| Decision | Rule for this story |
|---|---|
| **AD-4** | One `DeviceSession` per MT4; two units = two sessions (own WinUSB, mapper, Virtual Port set). No cascade. |
| **AD-5** | `K` assigned only by DeviceSessionManager; backends get ready-made names; 2 IN + 4 OUT per unit. |
| **AD-6** | Sole owner of identity→`K`; USB serial preferred; topology-path map fallback; `K` must not reshuffle when another unit is (un)plugged. |
| **AD-7** | V1 backend remains runtime `teVirtualMIDI.dll` via `VirtualMidiBackend` (LoadLibrary SYSTEM32). |
| **AD-8** | Do not add exclusive-open while multi-unit lands; multi-client still VirtualMIDI’s job. |
| **AD-9** | Only a live `DeviceSession` creates/destroys **that unit’s** ports via `MidiBackend`. Unplug → destroy; replug → new session under same AD-6 identity/`K`. |
| **AD-10 / AD-20** | Auto-Start / hot-plug stay user-session Bridge (no Session-0 service). |
| Structural Seed | Dual MT4 under one Bridge; Multi-MT4 sessions live in `Device/`. |

### Current code baseline (UPDATE files)

**`src/Device/DeviceSessionManager.*` — today**
- `formatPortDisplayName` already emits K≥2 directional strings
- `unitOrdinalK_ = 1` hard-wired; comment points at Story 3.4
- `buildPortNameSet` uses only that single `K`
- No serial / topology registry / persistence

**`src/Usb/WinUsbOpenDetail.cpp` — today**
- GUID path: `matchCount != 1` → `"Multiple WinUSB interfaces match… refusing ambiguous open"`
- Zadig/HWID path similarly refuses or is ambiguous on multi-match
- No public enumerate-all / open-by-path product API

**`src/Usb/WinUsbTransport.h` — today**
- `WinUsbOpenOptions` only has `allowZadigFallback`
- `Open(profile, error, options)` assumes unique match

**`src/App/MidiSessionCli.cpp` — today**
- `prepareMt4PortNames` builds one `PortNameSet` via default manager
- `runMt4MidiSession` / `runMt4AutoSession`: **one** `VirtualMidiBackend` + **one** `DeviceSession`
- Hot-plug loop recovers a single session

**`src/App/Mt4WinUsbPresence.*` — today**
- Boolean presence on first matching interface — no count/list

**`src/Device/DeviceSession.*` — today**
- Start opens transport from profile only (no instance binding field)
- Per-session mapper/ports already correct once Open targets the right unit

**What this story changes**
- Multi-instance enumeration + open-by-identity
- Durable `K` assignment + persistence
- App host composition for N concurrent sessions
- Per-unit hot-plug without reshuffling peers
- SM-8 smoke + honesty documentation
- Offline contracts for naming/registry

**What must be preserved**
- Directional name spelling and unit-1 operator experience
- AD-9 lifecycle (destroy on teardown; no orphan “keep alive” ports)
- GUID-first WinUSB bind; VirtualMIDI fail-closed English fix path
- Auto-Start registration from 3.1 (behavior inside CLI expands; registration model stays)
- Protocol/Profile isolation from VirtualMIDI/WinUSB
- Binary output under `builds/` only
- Multi-client non-exclusive create flags (3.3)

**Likely NEW**
- `src/Device/UnitIdentityRegistry.h/.cpp` (or equivalent name under `Device/`) — durable identity→`K` map
- Persistence file helper (user-local path; English errors; fail closed / recreate-safe)
- `docs/tests/smoke-epic3-dual-mt4-mt4.md`
- Catch2 registry / naming contracts under `tests/unit/`
- Optional simulated dual-instance lab path (two fake identities) when only one physical MT4 exists

**Likely UPDATE**
- `DeviceSessionManager.*` — assign `K` per identity; `buildPortNameSet(profile, K, …)` or manager-held current binding
- `WinUsbOpenDetail.*` / `WinUsbOpenSupport.*` / `WinUsbTransport.*` — enumerate + open selected path
- `DeviceSession.*` / `DeviceSessionStartRequest` — carry selected instance / open options
- `MidiSessionCli.*` / `Mt4WinUsbPresence.*` / diagnostics — N-unit host + per-unit hot-plug
- `Main.cpp` `--test-port-names` — K≥2 + registry stability cases
- Epic 3 smoke fence cross-links; `deferred-work.md` retireals
- `CMakeLists.txt` if new sources/tests

**Likely leave alone**
- `DeviceProfile` cable masks / cousin stubs
- `EmagicCableMapper` framing (already per-session)
- VirtualMIDI create-flag model (3.3 contract)
- Auto-Start Task Scheduler / Run-key registration mechanism (3.1)
- Public Installer / WiX / Inno

### Recommended implementation shape (not a second product decision)

1. **Enumerate** all present MT4 WinUSB interfaces (GUID + ifnum filter) → list of `{devicePath, identityKey, identityKind}`.
2. **Resolve `K`** via registry: lookup by serial if non-empty; else topology/instance path; allocate stable `K`; persist.
3. **For each present unit:** build `PortNameSet(K)` → construct `VirtualMidiBackend` → `DeviceSession::Start` with open-by-path options.
4. **Hot-plug:** diff previous vs current interface set; stop missing units; start new/returning units with same `K`; never renumber survivors.
5. **Single-unit regression:** one device still gets `K=1` and familiar `MT4 In/Out N` names.
6. **Honesty:** physical dual matrix when two boxes available; otherwise simulated dual + explicit “physical dual not proven” in smoke Notes.
7. **AQ-1 note:** one short English lab paragraph — which key class won on real hardware.

**Anti-patterns (do not)**
- Sorting plug-order every launch and rewriting `K` (breaks DAW recall)
- Letting `VirtualMidiBackend` format `MT4 #K` from USB descriptors
- One shared WinUSB handle or one shared `CreatePortSet` for two MT4s
- Keeping unit-2 ports alive after session Stop “for convenience”
- Claiming Emagic cascade / stacked chassis support
- Renaming ports back to non-directional `MT4 Port N` on both faces (IN/OUT collision)
- Treating blank dual-unit smoke rows as physical Pass
- Session-0 Windows Service; Windows MIDI Services backend switch
- Expanding scope to AMT8/Unitor8 product claims

### Previous story intelligence (3.3 → 3.4)

- 3.3 explicitly fenced dual-MT4 ordinal persistence to **3.4** and called out shared-`MidiBackend`-across-sessions as a multi-unit concern — **resolve here** as one backend per session.
- Patterns to copy: French operator smoke under `docs/tests/` with English Pass/Fail matrix, honesty bar (blank ≠ Pass), `lint-touched.py`, builds under `builds/`, AD-9 “App starts a session; never a second port authority”.
- Naming: smoke must use **live** `MT4 In/Out` / `MT4 #K In/Out` strings, not AD-5 shorthand alone.
- Review lessons: offline contracts are useful but **hardware (or honest simulated) smoke is the product gate**; document Ctrl+C preference (CTRL_CLOSE orphan risk remains deferred).
- 3.2 hot-plug loop is the recreate template — generalize to N units without breaking single-unit operators.
- Deferred-work still lists multi-unit open ambiguity and shared-backend ownership — close those bullets when this story lands.

### Git intelligence summary

- `42e4314` — Validation Matrix utility locked to MIDI-OX after 3.3 lab Pass; keep MIDI-OX as matrix utility; dual-MT4 does not reopen ShowMIDI/MidiView.
- `6b95279` — multi-client proof (docs + create-flag contract); leave exclusive-open fence alone.
- `e0ca8e0` / `0e238ab` — hot-plug recovery in `MidiSessionCli`; extend, don’t rewrite lifecycle ownership.
- Pattern: Epic 3 resilience slices = product-path C++ where needed + operator smoke + thin offline contracts; dual-MT4 is the first Epic 3 story that **must** change open/session composition, not docs-only.

### Latest tech information (WinUSB multi-instance)

- Enumerate device interfaces with `SetupDiGetClassDevs` + `SetupDiEnumDeviceInterfaces` + `SetupDiGetDeviceInterfaceDetail` for the project DeviceInterfaceGUID; each interface yields a unique device path for `CreateFile` / WinUSB init.
- Distinguish identical VID/PID units via USB serial string descriptor when the device provides one; otherwise use a durable instance/topology key (device instance ID / bus-port chain). Do not parse undocumented path fragments as the only identity if a proper instance ID is available.
- WinUSB handles are per-open; two units need two opens. Keep GUID-first product path; selected-path open is the multi-unit disambiguator.
- VirtualMIDI still creates independent named ports per session; name uniqueness across the whole Bridge process matters — `MT4 #2 …` disambiguates from unit 1.
- AQ-3 (SDK pin) / AQ-4 (Win11 WMS coexistence) remain deferred; note quirks if dual-unit lab hits them.

### Testing requirements

| Gate | Expectation |
|---|---|
| Contributor smoke | `docs/tests/smoke-epic3-dual-mt4-mt4.md` Pass/Fail; Win10 mandatory for physical dual claim |
| Independence | Two sessions; distinguishable names; no cross-wire on light MIDI check |
| Stability | Unplug peer / relaunch does not reshuffle known unit `K` |
| Honesty | Single-hardware lab must state physical dual not proven |
| Offline | Registry + `formatPortDisplayName` contracts; expand `--test-port-names` |
| Regression | 3.1 Auto-Start; 3.2 hot-plug; 3.3 multi-client; mapper/port-name tests; `lint-touched.py` |

### Project Structure Notes

- C++ under `src/` = **PascalCase**; docs/scripts = **kebab-case**.
- Registry belongs under `src/Device/` (naming/`K` authority), not under `src/Midi/` or `src/Protocol/`.
- No French in sources or Bridge user-visible strings.
- Builds only under `builds/` (reject repo-root `build/`).
- Do not invent `src/Service/` Session-0 host.

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Epic 3 / Story 3.4]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — FR-5, FR-10, UJ-3, SM-8]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-4, AD-5, AD-6, AD-9, AQ-1]
- [Source: `_bmad-output/specs/spec-unitor-win64-driver/SPEC.md` — CAP-5, CAP-10]
- [Source: `_bmad-output/implementation-artifacts/3-3-multi-client-daw-plus-showmidi.md` — fence + patterns]
- [Source: `_bmad-output/implementation-artifacts/3-2-hot-plug-recovery-without-windows-reboot.md` — recreate loop]
- [Source: `_bmad-output/implementation-artifacts/1-5-virtualmidi-backend-and-stable-mt4-port-names.md` — K=1 hard-wire deferral]
- [Source: `_bmad-output/implementation-artifacts/deferred-work.md` — multi-unit open / shared MidiBackend]
- [Source: `src/Device/DeviceSessionManager.h` — directional naming SSOT]
- [Source: `src/Usb/WinUsbOpenDetail.cpp` — ambiguous multi-match refuse]
- [Source: `src/App/MidiSessionCli.cpp` — single-session host]
- [Source: `docs/tests/smoke-epic3-multiclient-mt4.md` — fence → 3.4]

### Project context reference

No `project-context.md` is present in-repo. Follow `conventions.md` §3 quality gate, AD-4/5/6 multi-unit rules, AD-9 lifecycle, and the Structural Seed paths above.

## Dev Agent Record

### Agent Model Used

Cursor Grok 4.5

### Debug Log References

- lint-touched.py failed initially on function-length / file-size / params; split multi-unit host + WinUSB enumerate/identity helpers until Quality gate OK.

### Completion Notes List

- Dual-MT4 product host: enumerate all present WinUSB interfaces, open each by selected path, one DeviceSession + VirtualMidiBackend + PortNameSet(K) per unit.
- UnitIdentityRegistry persists identity→K under `%LOCALAPPDATA%\unitor-win64-driver\unit-identity-registry.txt` (serial preferred, topology fallback); known K never reshuffles when peers leave.
- Hot-plug reconciles per unit; peers keep ports/K. Single-unit operators still see `MT4 In/Out N`.
- Smoke guide documents honesty bar: physical dual not proven with one lab MT4; offline naming/registry contracts Pass.
- Deferred-work multi-unit open ambiguity + shared MidiBackend ownership retired.
- Validated: `ctest` BridgeTests Pass; `--test-port-names` / `--test-mapper` exit 0; `lint-touched.py` OK.

### File List

- `CMakeLists.txt`
- `docs/tests/smoke-epic3-dual-mt4-mt4.md`
- `docs/tests/smoke-epic3-autostart-mt4.md`
- `docs/tests/smoke-epic3-hotplug-mt4.md`
- `docs/tests/smoke-epic3-multiclient-mt4.md`
- `src/App/Main.cpp`
- `src/App/MidiSessionCli.cpp`
- `src/App/MidiSessionMultiHost.cpp`
- `src/App/MidiSessionMultiHost.h`
- `src/App/MidiSessionMultiHostDetail.h`
- `src/App/MidiSessionMultiHostHotPlug.cpp`
- `src/App/Mt4WinUsbPresence.cpp`
- `src/App/Mt4WinUsbPresence.h`
- `src/Device/DeviceSessionManager.cpp`
- `src/Device/DeviceSessionManager.h`
- `src/Device/UnitIdentityRegistry.cpp`
- `src/Device/UnitIdentityRegistry.h`
- `src/Device/UnitIdentityRegistryIo.cpp`
- `src/Usb/WinUsbEnumerate.cpp`
- `src/Usb/WinUsbOpenDetail.cpp`
- `src/Usb/WinUsbOpenDetail.h`
- `src/Usb/WinUsbOpenIdentity.cpp`
- `src/Usb/WinUsbOpenSupport.cpp`
- `src/Usb/WinUsbOpenSupport.h`
- `src/Usb/WinUsbTransport.cpp`
- `src/Usb/WinUsbTransport.h`
- `src/Usb/WinUsbTransportInit.cpp`
- `tests/unit/UnitIdentityRegistryTests.cpp`
- `_bmad-output/implementation-artifacts/deferred-work.md`
- `_bmad-output/implementation-artifacts/sprint-status.yaml`
- `_bmad-output/implementation-artifacts/3-4-two-mt4-units-with-stable-distinguishable-names.md`

### Change Log

- 2026-08-10: Implemented dual-MT4 stable naming (enumerate/open-by-path, AD-6 registry, N-session host, SM-8 smoke + honesty, offline contracts).
- 2026-08-10: Code review patches — hot-plug peer isolation + Start retry throttle, serial↔topology K migration, registry quarantine/atomic save, Zadig lab honesty.

## Story completion status

- Status: **done**
- Note: Code-review patches applied (hot-plug peer isolation, identity migration, registry quarantine/atomic save, Zadig lab honesty). Physical dual hardware matrix still blank with honest “physical dual not proven” until two MT4s are available in lab.
