---
title: 'MT4 separate virtual Input/Output ports (no local echo)'
type: 'feature'
created: '2026-08-06'
status: 'done'
baseline_commit: '6869d0f8c9f67df988f78d59bd7b1fd3f0b883ec'
review_loop_iteration: 0
context:
  - '{project-root}/conventions.md'
  - '{project-root}/_bmad-output/implementation-artifacts/deferred-work.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** The Bridge creates teVirtualMIDI ports with identical IN/OUT display names (`MT4 Port N`), so the backend merges them into one bidirectional handle. Apps then see local echo (what they send on OUT reappears on IN), unlike a hardware interface.

**Approach:** Create distinct teVirtualMIDI endpoints with directional names — `MT4 Input N` (host ← DIN) and `MT4 Output Y` (host → DIN) — using separate TX-only and RX-only faces so host→device traffic does not loop back onto the matching virtual IN.

## Boundaries & Constraints

**Always:**
- Preserve MT4 product wiring: 2 IN cables (DIN In 1/2) and 4 OUT cables (DIN Out 1–4); keep In1 vs In2 separation.
- Unit 1 names: `MT4 Input 1`…`MT4 Input 2`, `MT4 Output 1`…`MT4 Output 4`. Multi-unit: `MT4 #K Input N` / `MT4 #K Output Y`.
- Six distinct teVirtualMIDI creates (2 IN-only + 4 OUT-only); no shared handle between an IN face and an OUT face.
- Update operator-facing smoke / bind docs so MIDI From = `MT4 Input X`, MIDI To = `MT4 Output Y`.
- English diagnostics and source comments; commits only on explicit request.

**Ask First:**
- If separate faces still echo OUT→IN in lab (or teVirtualMIDI license/API blocks non-loopback), document the constraint and halt — do not pivot to Windows MIDI Services in this change.
- If Windows enumerates unexpected name suffixes that break DAW/Matrix-Control selection, halt before inventing a second naming scheme.

**Never:**
- WinUSB / session timeout refactors as a “fix” for this issue.
- Windows MIDI Services migration or dropping Win10.
- Matrix-Control code changes (Device Inquiry filter already shipped there).
- MidiView/ShowMIDI labs (retired; MidiView known BSOD — use MIDI-OX).
- Rewriting historical epic story markdown as part of this ship; live docs + code SSOT only.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Happy create | Valid PortNameSet (2 IN / 4 OUT directional names) | Six teVirtualMIDI ports; IN faces TX-only; OUT faces RX-only + callback | N/A |
| Name uniqueness | Distinct Input/Output strings | No merge; `inPorts_[i]` ≠ `outPorts_[j]` handles | Fail closed if create returns alias/error |
| Host→device | App sends Device Inquiry (or notes) to `MT4 Output Y` | Bytes reach DIN Out Y only; do **not** appear on any `MT4 Input X` | Log / session failure path unchanged |
| Device→host | DIN In X activity | Delivered only on `MT4 Input X` via `SendToHost` | Existing pump failure recording |
| Multi-unit K≥2 | Second MT4 session naming | `MT4 #K Input/Output …` spelling | Same fail-closed create path |
| Partial create | Mid-set `virtualMIDICreatePortEx2` fails | Tear down any opened handles; English error | Fail closed (existing pattern) |

</frozen-after-approval>

## Code Map

- `src/Device/DeviceSessionManager.cpp` / `.h` — SSOT for directional display names (`formatPortDisplayName` + `MidiPortDirection` / `buildPortNameSet`)
- `src/Midi/VirtualMidiBackend.cpp` / `.h` — `CreatePortSet` → `createDirectionalPortSet` (separate TX-only IN / RX-only OUT faces)
- `src/Midi/VirtualMidiWinSupport.cpp` / `.h` — UTF-8/name validation + uniqueness; `kVirtualMidiInPortFlags` / `kVirtualMidiOutPortFlags`
- `src/Midi/TeVirtualMidiApi.h` — `kTeVmFlagsParseTx|InstantiateTx` (IN) vs `ParseRx|InstantiateRx` (OUT)
- `src/Midi/MidiBackend.h` — `PortNameSet` already directional (`inNames` / `outNames`)
- `src/Device/DeviceSession*.cpp` — index→cable maps; unchanged if 1-based product order stays
- `src/App/Main.cpp` — `--test-port-names` expectations for Input/Output spelling
- `src/App/MidiSessionCli.cpp` — prints expected port names at session start
- `docs/dev/winusb-bind.md`, `docs/tests/smoke-epic1-mt4.md`, `docs/tests/smoke-epic2-mt4.md` — operator wiring for Input/Output
- `_bmad-output/implementation-artifacts/deferred-work.md` — merged collision workaround superseded

## Tasks & Acceptance

**Execution:**
- [x] `src/Device/DeviceSessionManager.cpp` / `.h` -- Emit directional `MT4 Input N` / `MT4 Output Y` (and `#K` forms); stop using one undirected `MT4 Port N` for both faces -- New UX contract; unique names enable separate creates
- [x] `src/Midi/VirtualMidiBackend.cpp` / `.h` + `VirtualMidiWinSupport.cpp` / `.h` -- Create six directional ports (IN=TX-only, OUT=RX-only+callback); stop sharing one handle across IN+OUT; simplify or bypass merge-when-names-equal -- Removes local-echo merged model
- [x] `src/App/Main.cpp` (+ CLI diagnostics if needed) -- Update `--test-port-names` / PortNameSet expectations to Input/Output strings -- Keeps naming gate green
- [x] `docs/dev/winusb-bind.md`, `docs/tests/smoke-epic1-mt4.md`, `docs/tests/smoke-epic2-mt4.md` -- Document MIDI From=`MT4 Input X`, MIDI To=`MT4 Output Y` -- Operator / Ableton / Matrix-Control wiring
- [x] `_bmad-output/implementation-artifacts/deferred-work.md` -- Append note that merged `MT4 Port N` collision workaround is superseded by directional ports -- Traceability
- [x] Unit / CLI checks covering I/O matrix name+create edge cases (extend existing port-name tests; add focused tests where practical without hardware) -- Guards regressions

**Acceptance Criteria:**
- Given Bridge session start with teVirtualMIDI available, when ports are created, then Windows lists `MT4 Input 1–2` and `MT4 Output 1–4` as distinct endpoints (no undirected `MT4 Port N` for this session).
- Given an app sends MIDI (including Device Inquiry SysEx) to `MT4 Output Y`, when observing `MT4 Input X` (MIDI-OX or Bridge logs), then that traffic does **not** appear on any Input face.
- Given DIN activity on MT4 In 1 vs In 2, when Bridge is running, then traffic appears only on the matching `MT4 Input` endpoint.
- Given Matrix-Control (build with Device Inquiry filter) + this Bridge model, when presence is watched 2–5 minutes at idle with light edits, then presence stays stable without relying on WinUSB timeout changes.
- Given `--test-port-names` (or equivalent), when run, then directional unit-1 and multi-unit `#K` spellings pass.

## Spec Change Log

## Design Notes

Today AD-5 reused one label on IN and OUT; teVirtualMIDI unique-name rules forced `createMergedPortSet` to OR TX+RX flags on a single handle — that is the echo source, not WinUSB.

Directional names make merge a no-op (six plans). Prefer creating directional ports explicitly rather than depending on “merge finds no collisions.” Keep index→cable maps; only naming + create/destroy change.

Golden names (unit 1): `MT4 Input 1`, `MT4 Input 2`, `MT4 Output 1`, `MT4 Output 2`, `MT4 Output 3`, `MT4 Output 4`.

## Verification

**Commands:**
- `cmake --build builds --target Bridge` (or project-equivalent Windows build) -- expected: build succeeds
- `Bridge.exe --test-port-names` (or documented CLI) -- expected: directional name assertions pass
- Relevant unit tests under `tests/unit` for any new/changed coverage -- expected: pass
- `python scripts/quality/lint-touched.py` -- expected: clean on touched C++ diff

**Manual checks (if no CLI):**
- Lab: send Device Inquiry to `MT4 Output N`; confirm it does not appear on `MT4 Input X` (MIDI-OX). Presence soak 2–5 min with filtered Matrix-Control. Do not use MidiView or ShowMIDI (retired — use MIDI-OX).

## Suggested Review Order

**Naming contract**

- Directional `MT4 Input` / `MT4 Output` (and `#K`) SSOT
  [`DeviceSessionManager.cpp:5`](../../src/Device/DeviceSessionManager.cpp#L5)

**teVirtualMIDI create path**

- Six separate faces: TX-only IN, RX-only OUT + callback
  [`VirtualMidiBackend.cpp:274`](../../src/Midi/VirtualMidiBackend.cpp#L274)

- Fail closed if driver ever returns a shared IN/OUT handle
  [`VirtualMidiBackend.cpp:252`](../../src/Midi/VirtualMidiBackend.cpp#L252)

- Flag constants + unique-name validation
  [`VirtualMidiWinSupport.h:22`](../../src/Midi/VirtualMidiWinSupport.h#L22)

**Operator docs**

- MIDI From = Input, MIDI To = Output; warn about old `MT4 Port N` zombies
  [`winusb-bind.md:85`](../../docs/dev/winusb-bind.md#L85)

- Epic 1: names intentionally distinct (no merged same-label)
  [`smoke-epic1-mt4.md:285`](../../docs/tests/smoke-epic1-mt4.md#L285)

**Tests**

- `--test-port-names` expects Input/Output + no IN/OUT alias collision
  [`Main.cpp:94`](../../src/App/Main.cpp#L94)
