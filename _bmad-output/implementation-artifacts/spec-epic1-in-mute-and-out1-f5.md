---
title: 'Epic 1 — device→host IN mute + first Out 1 F5'
type: 'bugfix'
created: '2026-08-05'
status: 'done'
baseline_commit: 'f3ba1ddf17fda2f8ecfdda5a8ab4f48d4e3664a5'
review_loop_iteration: 0
context:
  - '{project-root}/conventions.md'
  - '{project-root}/docs/tests/smoke-epic1-mt4.md'
  - '{project-root}/_bmad-output/implementation-artifacts/deferred-work.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** On Boot Camp with a live MT4 session, DIN In 1/2 LEDs show notes/CC but Ableton sees no traffic on virtual `MT4 Port 1/2` IN (P0). Separately, the first notes to Out 1 after init can light all four Out LEDs because the Emagic mapper omits the initial `F5` when sticky OUT cable starts at 0 (P1).

**Approach:** Make the device→host path observable and correct (ASCII counters + complete MIDI framing before VirtualMIDI TX), then force a first-use Out cable switch so Out 1 never fans on first encode. Keep both fixes in one lab retest; do not start Epic 2.

## Boundaries & Constraints

**Always:**
- C++17 usermode; English sources; ASCII-simple user console text (no typographic dashes)
- Builds under `builds/`; end with `python scripts/quality/lint-touched.py` on the C++ diff
- Preserve In 1 vs In 2 cable separation; merged VirtualMIDI `MT4 Port N` names stay
- Session `--dev-zadig` only for MIDI smoke (no `--probe-usb` required for retest)
- Commits only if Guillaume asks; commit messages in English

**Ask First:**
- If lab counters prove **zero** successful bulk IN bytes while DIN In LEDs blink → stop before changing WinUSB pipe claim / interface selection
- Any change that splits merged VirtualMIDI ports back into distinct IN/OUT display names

**Never:**
- Epic 2 (clock / MTC / SysEx product work), custom kernel driver, Public Installer
- French in source; inventing Broadcast Virtual Ports; widening to AMT8/Unitor8 beyond declarative profile

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| P0 note In 1 | DIN note on In 1 during `--start-session --dev-zadig` | Complete note appears on virtual `MT4 Port 1` IN (MIDI-OX or Live) | Failed `SendToHost` stays diagnosable (English); do not silent-drop framed messages |
| P0 note In 2 | DIN note on In 2 | Same on `MT4 Port 2` IN only (no mix with Port 1) | Non-product cables ignored |
| P0 CC | DIN CC on In 1/2 | CC visible on matching virtual IN | Same as notes |
| Partial USB slice | Note bytes split across bulk IN reads | Hold until complete channel message, then `SendToHost` | Do not kill session on incomplete slice; do not emit partial note/CC to host |
| P1 first Out 1 | Fresh mapper/session, first host→device notes to Out 1 | Wire includes `F5 01` before MIDI; only Out 1 LED activity | Encode failure → existing pump/English path |
| P1 sticky Out 1 | Second encode still Out 1 | No extra `F5` | N/A |
| P1 after other Out | Encode Out 2 then Out 1 | Out 1 encode includes `F5 01` | N/A |

</frozen-after-approval>

## Code Map

- `src/Device/DeviceSession.cpp` -- readerLoop / processBulkRead / forwardDeviceMidi; device→host counters + framer hook before SendToHost
- `src/Protocol/EmagicCableMapper.h` / `.cpp` -- DecodeFromDevice demux; `currentOutCable_` sentinel for P1
- `src/Midi/VirtualMidiBackend.cpp` -- SendToHost / merged TX+RX ports (touch only if counters prove TX failure)
- `src/App/MapperSmoke.cpp` -- encode expectations: first cable-0 must emit `F5 01`
- `docs/tests/smoke-epic1-mt4.md` -- update after lab confirmation (post-implement, with Guillaume)
- `_bmad-output/implementation-artifacts/deferred-work.md` -- close/adjust P0/P1 entries after lab

## Tasks & Acceptance

**Execution:**
- [x] `src/Device/DeviceSession.cpp` (+ small helper header under `src/Device/` or `src/Protocol/` if needed) -- add device→host MIDI channel-message framer (note/CC and other short channel voice) so each `SendToHost` gets one complete command; hold partials across bulk reads per IN cable -- teVirtualMIDI requires complete commands; prevents mute-or-kill on sliced USB
- [x] `src/Device/DeviceSession.cpp` -- add ASCII-simple session counters (or infrequent summary) for bulk IN bytes accepted, demux product sinks, and SendToHost success/fail -- proves empty USB vs demux drop vs TX fail on next lab pass
- [x] `src/Protocol/EmagicCableMapper.h` -- initialize `currentOutCable_` to an invalid sentinel (not a product OUT index) so first Out 1 encode always emits `F5` -- fixes lab P1 fan-out
- [x] `src/App/MapperSmoke.cpp` -- update first cable-0 encode tests to expect `F5 01` + MIDI + pad; keep sticky same-cable omit and re-switch coverage -- locks P1 fix
- [x] Rebuild Debug under `builds/` and run mapper smoke / lint-touched on C++ diff -- local gate before hardware retest

**Acceptance Criteria:**
- Given a live `--start-session --dev-zadig` session with DIN activity on In 1 and In 2, when notes/CC are played, then matching virtual `MT4 Port 1/2` IN show traffic in MIDI-OX or Ableton without cross-mixing ports
- Given a fresh session, when the first notes are sent to Out 1, then only Out 1 green LED activity occurs (no four-LED fan)
- Given mapper unit smoke, when encoding cable 0 on a fresh mapper, then the frame starts with `F5 01`
- Given the Bridge console during IN activity, when bulk IN delivers data, then ASCII counters move in a way that distinguishes USB / demux / SendToHost

## Design Notes

- Investigation: path + merged VirtualMIDI TX/RX flags look structurally sound; silent mute with a live session points to empty/dropped device→host data more than a TX-flag bug. Framer is still required once USB delivers slices (deferred-work + teVirtualMIDI contract).
- P1: prefer mapper sentinel over session-level “force F5 after init” to keep Protocol sticky state local.
- Do not invent full SysEx reassembly here (Epic 2); channel voice for notes/CC smoke is enough.
- After Guillaume’s lab pass: update smoke guide §5/§6 and deferred-work P0/P1 lines — not a blocker for code complete.

## Verification

**Commands:**
- `cmake --build builds/debug` (or project’s usual Debug config under `builds/`) -- expected: Bridge + tests link OK
- `python scripts/quality/lint-touched.py` -- expected: exit 0 on touched C++ diff
- Mapper smoke path already wired in Bridge (`--test-mapper` or equivalent) -- expected: pass with new cable-0 F5 expectation

**Manual checks (if no CLI):**
- PowerShell: start session with `--start-session --dev-zadig` only; DIN → MIDI-OX/Live on `MT4 Port 1/2` IN; first Out 1 notes → single Out LED; Ctrl+C teardown still clean

## Suggested Review Order

**Device→host framing**

- Per-IN-cable Push then one complete command to VirtualMIDI
  [`DeviceSessionDeviceHost.cpp:53`](../../src/Device/DeviceSessionDeviceHost.cpp#L53)

- Channel-voice / running-status / SysEx hold for teVirtualMIDI
  [`MidiMessageFramer.cpp:168`](../../src/Protocol/MidiMessageFramer.cpp#L168)

**Lab counters**

- First bulk line printed after SendToHost on that stack
  [`DeviceSessionDeviceHost.cpp:146`](../../src/Device/DeviceSessionDeviceHost.cpp#L146)

- Final snapshot on Stop for Ctrl+C smoke
  [`DeviceSession.cpp:301`](../../src/Device/DeviceSession.cpp#L301)

**Out 1 F5**

- Sentinel so first cable 0 encode always switches
  [`EmagicCableMapper.h:78`](../../src/Protocol/EmagicCableMapper.h#L78)

**Safety / tests**

- Restored multi-condition IsRunning for CLI watchdog
  [`DeviceSession.cpp:332`](../../src/Device/DeviceSession.cpp#L332)

- Framer unit coverage including running status
  [`FramerSmoke.cpp:85`](../../src/App/FramerSmoke.cpp#L85)
