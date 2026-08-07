---
title: 'MT4 Bridge — Device Inquiry every-other drop'
type: 'bugfix'
created: '2026-08-07'
status: 'in-progress'
baseline_commit: 'db66f66'
review_loop_iteration: 0
context:
  - '{project-root}/conventions.md'
  - '{project-root}/_bmad/custom/clarity-bar-fr.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** On the MT4 Bridge path (WinUSB + Emagic + teVirtualMIDI), Universal Device Inquiry (`F0 7E 7F 06 01 F7`) sent on `MT4 Output Y` gets an Identity Reply (`06 02`) on `MT4 Input X` only about every other time (~50 %), independent of 5 s vs 10 s spacing. The same Matrix-1000 via Scarlett answers 100 % even at 1 inquiry/s, so Matrix-Control presence flips ERROR on the Bridge path while the instrument itself is fine.

**Approach:** Instrument host→device and device→host per inquiry to locate the loss, apply a minimal Bridge fix for the proven root cause, and retest with MIDI-OX (≥20 inquiries @ ~5 s, ≥95 % replies, target 100 %) then calm Matrix-Control presence — without treating Matrix-Control heartbeat as the primary fix.

## Boundaries & Constraints

**Always:**
- Keep SysEx opaque transparent carry (no Oberheim/allowlist rewrite of Inquiry payloads).
- Preserve directional ports `MT4 Input N` / `MT4 Output Y` (no merged-port echo).
- Preserve Emagic F5 cable model and Computer Mode kick; do not break notes/CC/clock/MTC.
- Lab gate: Bridge + MIDI-OX ≥20 Device Inquiries ~5 s apart → ≥95 % Identity Reply on matching Input; then Matrix-Control presence calm 2–5 min without cyclic ERROR.
- Run `python scripts/quality/lint-touched.py` clean on the C++ diff; builds under `builds/`.
- Document a short MIDI-OX (+ optional Matrix-Control) retest protocol in English under docs/tests or the spec Verification section.

**Ask First:**
- Changing Matrix-Control heartbeat / presence timeout as the main fix (only if Bridge cannot reach ≥95 % quickly).
- Broad WinUSB timeout retunes without evidence they cause the alternation.
- Overlapping/async multi-URB IN redesign beyond a minimal proven gap fix.
- Pad-to-`wMaxPacketSize` encode change if lab proves single-`0xFF` pad is innocent — still Ask First if it risks regressing notes/SysEx smokes without a clear lab A/B.

**Never:**
- Windows MIDI Services migration; MidiView (BSOD); AMT8 / Unitor8 work.
- Inventing Emagic escaping for raw MIDI `0xF5` / `0xFF` (still deferred).
- Claiming pass from unit smokes alone without MIDI-OX lab (or documented equivalent contributor lab).

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Happy Inquiry | Host sends `F0 7E 7F 06 01 F7` on `MT4 Output 1`; Matrix on DIN Out1↔In1 | Identity Reply `F0 7E … 06 02 … F7` on `MT4 Input 1` within ~200 ms | N/A |
| Strict alternation regression | ≥20 inquiries @ ~5 s on Output 1 only | ≥95 % replies (no enduring 50 % pattern) | If still ~50 %, keep instrumentation counters and HALT for Ask First |
| Spacing independence | Same at ~10 s | Same success rate as ~5 s | Do not “fix” by slowing MC only |
| No local echo | Inquiry on Output 1 | Input 1 must not show the host’s `06 01` as if it were a reply | Drop/ignore local TX on IN faces (already directional) |
| Encode / IN reply | Repeated Out1 Inquiry after kick; device may reply in one or more bulk packets | Complete Emagic OUT frame accepted; full Identity Reply reaches SendToHost (no every-other loss) | Encode/Write fail → English pump fail; oversize/nested SysEx → existing framer reject |

</frozen-after-approval>

## Code Map

- `src/Midi/VirtualMidiBackend.cpp` -- OUT callback → host→device
- `src/Device/DeviceSessionHostOutbound.cpp` -- encode + `WriteBulk` (OUT loss site)
- `src/Protocol/EmagicCableMapper.cpp` / `.h` -- F5 sticky cable, single `0xFF` pad, demux truncate / `seenF5_`
- `src/Device/DeviceSessionDeviceHost.cpp` -- single-buffer `ReadBulk` → demux → framer → SendToHost
- `src/Usb/WinUsbTransport.cpp` / `.h` -- sync pipes; `BulkInReadCapacity()` = `wMaxPacketSize`
- `src/Device/DeviceHostCounters.*` + `src/App/MidiSessionCli.cpp` -- host-out / reply counters for lab
- `src/Protocol/EmagicMapperSysexSmoke.cpp` + unit SysEx/framer tests -- Inquiry coverage
- `_bmad-output/implementation-artifacts/2-3-transparent-sysex-transport-with-burst-buffering.md` -- opaque SysEx invariants
- `_bmad-output/implementation-artifacts/spec-mt4-separate-virtual-ports.md` -- directional ports shipped
- `docs/tests/checklists/smoke-epic2-matrix-control-mt4.md` -- cross-link Inquiry retest

## Tasks & Acceptance

**Execution:**
- [x] Counters + logs on host-outbound / device-host / CLI -- count OUT writes vs Identity Reply IN per send; show loss before USB OUT, after OUT with no IN, or after demux/framer -- localize the 50 % drop
- [ ] Lab (Zadig + `--dev-zadig`, MIDI-OX) -- ≥10 inquiries @ ~5 s with instrumentation; classify bucket before patching -- no timeout roulette
- [x] Minimal Bridge patch (`DeviceSession` reader/outbound + WinUSB short IN drain) -- after host→device Write on the reader path (and after each IN packet), briefly poll bulk IN so Emagic half-duplex bursts are not dropped between sync Reads -- remove alternation hypothesized from init-lab NAK pattern
- [x] SysEx smoke + unit tests as needed -- Inquiry/Identity helper Catch2 tests -- prevent silent regress of matchers
- [x] Short English MIDI-OX (+ optional Matrix-Control) retest notes under `docs/tests/checklists/smoke-mt4-device-inquiry-bridge.md` -- reproducible gate

**Acceptance Criteria:**
- Given Bridge + Matrix on DIN Out1↔In1 and MIDI-OX on `MT4 Output 1` / `MT4 Input 1`, when ≥20 Device Inquiries @ ~5 s, then ≥95 % Identity Reply on Input 1 (target 100 %) and every-other silence is gone. **(lab pending — contributor Boot Camp)**
- Given Matrix-Control presence on Input 1 / Output 1 only, when idle 2–5 minutes, then no cyclic `deviceMidiUnresponsive` / ERROR from missing replies. **(lab pending)**
- Given a lost Inquiry during diagnosis, when logs are inspected, then the failing stage is visible (no WriteBulk / Write OK but no bulk IN / demux or SendToHost drop). **(instrumentation shipped: `inquiry_out` / `identity_reply_in` / `host_out_ok`)**
- Given the C++ diff, when `python scripts/quality/lint-touched.py` runs, then it exits clean. **(done)**

## Design Notes

Lab ~50 % is timing-independent with MIDI-OX alone → prefer state/alternation over Matrix rate limits.

Order (instrument first; one fix with evidence):
1. **OUT pad** — single trailing `0xFF` vs fill-to-`wMaxPacketSize` may desync Emagic OUT every other short write.
2. **Bulk IN gap** — one sync Read; a two-packet burst can lose every other packet between complete and next submit.
3. **F5 sticky after cable-0 kick** — Out1 may omit F5; weaker alone for perfect 50 %, A/B if pad/IN look clean.
4. Do not lead with WinUSB timeout tweaks or Matrix-Control heartbeat changes.

## Verification

**Commands:**
- `python scripts/quality/lint-touched.py` -- expected: clean on touched C++
- Emagic SysEx / framer unit or `--test-mapper` smokes -- expected: pass after encode/demux changes

**Manual checks (contributor lab):**
1. Zadig + `--start-session --dev-zadig`; MIDI-OX ≥20× `F0 7E 7F 06 01 F7` @ ~5 s on Output 1; ≥95 % `06 02` on Input 1.
2. Optional: Matrix-Control presence Input 1 / Output 1, calm 2–5 min — no cyclic ERROR.
3. Scarlett control already 100 % — do not blame the Matrix-1000.
