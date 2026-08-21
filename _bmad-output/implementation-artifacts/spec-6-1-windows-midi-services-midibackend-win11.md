---
title: '6.1 Windows MIDI Services MidiBackend (Win11)'
type: 'feature'
created: '2026-08-20'
status: 'done'
baseline_commit: '85f5bbc3daacd0d43fc318be6c908d6facbc491c'
review_loop_iteration: 0
context:
  - '{project-root}/_bmad-output/implementation-artifacts/epic-6-context.md'
  - '{project-root}/docs/dev/license-and-backends.md'
  - '{project-root}/conventions.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** The Bridge still creates DAW-facing MT4 ports only through proprietary virtualMIDI, so community MIT binaries cannot ship without SDK redistribution clearance.

**Approach:** Add a Win11 Windows MIDI Services `MidiBackend` behind the existing seam so a live DeviceSession creates and destroys the same directional MT4 port names without requiring virtualMIDI on that path; keep Protocol/Profile free of WMS headers.

## Boundaries & Constraints

**Always:**
- Implement WMS behind `MidiBackend` (`CreatePortSet` / `DestroyPortSet` / `SendToHost` / `SetHostToDeviceSink`).
- Use ready-made names from `DeviceSessionManager::formatPortDisplayName` (`MT4 In N` / `MT4 Out N`, or `MT4 #K …` when K≥2); backend must not invent unit ordinal K.
- Topology remains 2 IN + 4 OUT per live unit; only a live DeviceSession owns create/destroy.
- Fail closed with an obvious error if WMS is selected but unavailable — never present an empty port list as success.
- Protocol and Profile stay free of WMS, WinUSB, and virtualMIDI headers (AD-2).
- C++17 usermode; artifacts under `builds/`; English in code/docs; no French in sources.

**Ask First:**
- Choosing a different WMS API family than the Microsoft MIDI App SDK virtual-device path (`MidiVirtualDeviceManager` / `Microsoft.Windows.Devices.Midi2`).
- Removing or permanently unlinking `VirtualMidiBackend` from the Bridge (belongs to Story 6.2 binary policy).
- Changing display-name spelling away from the coded `In`/`Out` SSOT.
- Treating Windows 10 as a supported community WMS claim.

**Never:**
- Story 6.2 public binary packaging / install-doc rewrite / Authenticode.
- Story 6.3 full Validation Matrix soak / multi-client ceiling rewrite as a ship gate for 6.1.
- Custom kernel MIDI driver; virtualMIDI MSI embed; rewriting Emagic mapper for WMS.
- Soft-echo as a substitute for USB/DIN hardware proof.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Happy path Win11 | WMS transport available; live DeviceSession starts | Six named endpoints appear (2 IN + 4 OUT); teardown destroys them | N/A |
| WMS missing | WMS backend selected; transport unavailable | Session start fails; no empty “success” ports | Clear error naming WMS prerequisite |
| virtualMIDI override | Explicit lab override selects virtualMIDI | Existing VirtualMidiBackend path still works when DLL present | Unchanged fail-closed messaging |
| Host→device | DAW sends on `MT4 Out N` | Sink delivers bytes to DeviceSession for that out index | Drop/log only if session already tearing down |
| Device→host | USB path yields MIDI for in index | `SendToHost` delivers to matching `MT4 In N` | Propagate backend send failure to session error path |
| Hot unplug | DeviceSession stops | `DestroyPortSet` idempotent; ports gone from host enumeration | No second lifecycle owner |

</frozen-after-approval>

## Code Map

- `src/Midi/MidiBackend.h` -- abstract seam; keep contract stable (L23–48).
- `src/Midi/VirtualMidiBackend.*` + `VirtualMidiBackendPorts.cpp` -- reference lifecycle/I/O; keep for lab override in 6.1.
- **Create** `src/Midi/WmsMidiBackend.h` / `WmsMidiBackend.cpp` (+ small Win11 helper only if needed) -- `MidiVirtualDeviceManager` create/destroy; map each directional name to one WMS virtual endpoint; MIDI 1.0 byte path via service translation.
- `src/App/MidiSessionMultiHost.h` / `.cpp` -- widen `unique_ptr<VirtualMidiBackend>` → `unique_ptr<MidiBackend>`; construct WMS by default on Win11 when transport available (`startLiveUnit` ~L80).
- `src/App/MidiSessionCli.*` / `Main.cpp` -- optional `--midi-backend=wms|virtualmidi` (or env) for lab coexistence; default WMS on Win11.
- `src/Device/DeviceSession*.cpp` / `DeviceSessionManager.cpp` -- reuse as-is for Start/Stop and naming SSOT (`formatPortDisplayName` L4–20).
- `CMakeLists.txt` -- add WMS sources; wire Microsoft MIDI App SDK / CppWinRT packages for Win32 Bridge only; keep Protocol/Profile/BridgeTests free of WMS.
- **Read-only:** `src/Protocol/**`, `src/Profile/**` -- no WMS/WinUSB headers.
- `docs/dev/license-and-backends.md` -- honesty context only; do not rewrite for 6.2 in this story.

## Tasks & Acceptance

**Execution:**
- [x] `src/Midi/WmsMidiBackend.h` + `WmsMidiBackend.cpp` -- implement MidiBackend via WMS virtual-device APIs; one endpoint per ready-made In/Out name; Create/Destroy + SendToHost + sink.
- [x] `src/App/MidiSessionMultiHost.h` + `MidiSessionMultiHost.cpp` -- own `unique_ptr<MidiBackend>`; default-construct WMS on Win11 when available.
- [x] `src/App/MidiSessionCli.*` (+ `Main.cpp` if needed) -- lab override to virtualMIDI without requiring it for the WMS default path.
- [x] `CMakeLists.txt` -- compile/link WMS backend on Windows; fail configure/build clearly if required SDK pieces are missing.
- [x] `src/Midi/*` unit/smoke where feasible without hardware -- cover Create/Destroy fail-closed when transport unavailable (mock or guarded test).
- [x] Lab notes under Verification -- checklist for Matrix-1000 + DIN loopback after build (not full Story 6.3 matrix).

**Acceptance Criteria:**
- Given Win11 with Windows MIDI Services available and a live DeviceSession, when the WMS MidiBackend starts, then that unit’s virtual ports are created through WMS using `MT4 In/Out N` (or `#K`) names and destroyed on session teardown.
- Given the WMS community path is selected, when the Bridge runs, then virtualMIDI is not required for port create/destroy or I/O on that path.
- Given WMS sources are wired, when Protocol and Profile translation units build, then they include no WMS/WinUSB/virtualMIDI headers.
- Given WMS is selected but transport is unavailable, when session start runs, then the Bridge fails closed with an obvious error and does not advertise empty ports as success.

## Spec Change Log

- 2026-08-20: Implemented WmsMidiBackend (App SDK RC4 virtual devices), MidiBackend selection/factory, CMake NuGet+CppWinRT fetch, fail-closed unit tests, lab verification notes.
- 2026-08-21: Code review patches — host convert logging, ProductInstanceId suffix, SysEx interrupt, assemble unlock, CreatePortSet catch-all, UMP type fail-closed, CMake marker version, default-WMS test pin, lab teardown caveat + Design Notes honesty.

## Design Notes

Prefer Microsoft MIDI App SDK virtual devices (`MidiVirtualDeviceManager::CreateVirtualDevice` / `MidiVirtualDeviceCreationConfig`) over inventing a second host API. Map **one WMS virtual endpoint per directional display name** so DAWs keep seeing separate `MT4 In 1`… and `MT4 Out 1`… faces like today’s virtualMIDI layout.

Default: WMS for all Windows builds (CLI/env override to virtualMIDI). Transport availability is checked at `CreatePortSet` (fail closed with a clear error) rather than at resolve time. Keep `VirtualMidiBackend` selectable for lab coexistence during 6.1; do not delete it here. Soft-echo remains VirtualMIDI-oriented lab tooling — do not treat it as WMS hardware proof.

`HostOutboundQueue` caps (`kMaxMessages` 256, `kMaxQueuedBytes` 131072) are a Story 6.1 WMS prerequisite so concurrent long DIN/SysEx does not trip the host→device queue early.

## Verification

**Commands:**
- Configure + build Bridge with project presets under `builds/` -- expected: Bridge links with WMS backend sources on Win11.
- `python scripts/quality/lint-touched.py` on touched C++ under `src/` -- expected: gate clean per `conventions.md` §3.
- Existing BridgeTests that assert `MT4 In/Out` naming -- expected: still pass.
- `BridgeTests` case `rejectMissingWmsTransport fail-closed messaging` -- expected: PASS.

**Manual checks (Win11 lab, after build):**
- Start Bridge with MT4 attached, WMS default: confirm `MT4 In 1..2` and `MT4 Out 1..4` appear to the host; stop Bridge: ports disappear.
- Confirm Bridge runs without `teVirtualMIDI.dll` on the WMS path.
- Lab override smoke: `--midi-backend=virtualmidi` still creates the same names when teVirtualMIDI is present.
- Prep follow-on: Matrix-1000 on one MT4 DIN pair + physical MIDI loopback cable on another — notes/CC smoke only; full soak stays Story 6.3.

**Lab notes (Matrix-1000 + DIN loopback prep — not Story 6.3 soak):**
1. Install Windows MIDI Services + App SDK runtime on the Win11 lab PC; build Bridge (`cmake --preset debug` then `cmake --build --preset debug`).
2. Plug MT4 (WinUSB), run `Bridge.exe --start-session` with no `--midi-backend` override (WMS default).
3. In MIDI-OX / DAW, confirm six faces: `MT4 In 1`, `MT4 In 2`, `MT4 Out 1`…`MT4 Out 4`.
4. Notes/CC smoke: Matrix-1000 into one DIN IN → host sees traffic on the matching `MT4 In N`; host play on one `MT4 Out N` → DIN OUT audible/visible.
5. Physical MIDI loopback cable on a second DIN pair: Out→In notes/CC round-trip once; log pass/fail only (no overnight matrix).
6. Stop Bridge (Ctrl+C); confirm ports leave host enumeration when possible. **Known lab caveat:** WMS teardown abandons the midisrv session without `Close()` to avoid indefinite stalls on ghost endpoints — ports may remain visible until midisrv/host refresh; do not treat lingering faces alone as a Bridge hang. Retry without `teVirtualMIDI.dll` present to prove the WMS path.
7. Do **not** treat `--soft-echo` as WMS hardware proof.

### Review Findings

- [x] [Review][Decision] Destroy abandons WMS session without Close — resolved 2026-08-21: keep anti-stall abandon; document lab port-visibility caveat (→ Patch below).
- [x] [Review][Decision] Default WMS not gated to Win11 / transport-at-resolve — resolved 2026-08-21: keep always-WMS default + fail-closed at CreatePortSet; align CLI help (→ Patch below).
- [x] [Review][Decision] WinUSB/Zadig presence changes in 6.1 scope — resolved 2026-08-21: keep in 6.1 for lab; dismissed (no code change).
- [x] [Review][Decision] HostOutboundQueue capacity bump in 6.1 — resolved 2026-08-21: keep bump; document as WMS prerequisite (→ Patch below).

- [x] [Review][Patch] Document WMS teardown may leave host ports visible (lab Verification caveat) [`spec Verification`]
- [x] [Review][Patch] Align CLI help with always-WMS default (fail-closed at session start) [`src/App/Main.cpp`]
- [x] [Review][Patch] Document HostOutboundQueue capacity bump as WMS SysEx prerequisite [`spec Design Notes` + `HostOutboundQueue.h`]
- [x] [Review][Patch] Log host→device conversion failures instead of empty `catch (...)` [`src/Midi/WmsMidiBackendPorts.cpp:167`]
- [x] [Review][Patch] Preserve ProductInstanceId suffix when truncating to 32 chars [`src/Midi/WmsMidiWinSupport.cpp:264`]
- [x] [Review][Patch] Clear SysEx hold when a channel/system-common status arrives mid-SysEx [`src/Midi/Midi1StreamAssembler.cpp:55`]
- [x] [Review][Patch] Unlock `hostAssembleMutex` before `forwardHostToDevice` to avoid teardown deadlock [`src/Midi/WmsMidiBackend.cpp:355`]
- [x] [Review][Patch] Rollback CreatePortSet on non-`hresult_error` exceptions [`src/Midi/WmsMidiBackendPorts.cpp:398`]
- [x] [Review][Patch] Fail SendToHost on unknown UMP message type instead of assuming 1 word [`src/Midi/WmsMidiBackend.cpp:121`]
- [x] [Review][Patch] Invalidate CMake projection marker when SDK version changes [`cmake/FetchMicrosoftMidiAppSdk.cmake:85`]
- [x] [Review][Patch] Pin unit test: no-override default resolves to WMS [`tests/unit/MidiBackendSelectTests.cpp:30`]
- [x] [Review][Patch] Fix comment typo “Droping” → “Dropping” [`src/Midi/WmsMidiBackendPorts.cpp:72`]

- [x] [Review][Defer] SendToHost BufferFull sleep/retry up to ~200 ms [`src/Midi/WmsMidiBackend.cpp:174`] — deferred, pre-existing-by-design for this story (paced WMS send)
- [x] [Review][Defer] Non-SysEx fragmentation across Append calls [`src/Midi/Midi1StreamAssembler.cpp:71`] — deferred, pre-existing assumption documented in code
- [x] [Review][Defer] Host sink lifetime race after mutex copy [`src/Midi/WmsMidiBackend.cpp:301`] — deferred, pre-existing pattern shared with VirtualMidiBackend
- [x] [Review][Defer] NuGet download without checksum [`cmake/FetchMicrosoftMidiAppSdk.cmake:25`] — deferred, pre-existing fetch style; version pin exists
- [x] [Review][Defer] No automated CreatePortSet fail-closed beyond helper [`tests/unit/MidiBackendSelectTests.cpp:19`] — deferred, lab `--test-wms-ports` covers lifecycle; full WinRT mock out of 6.1
