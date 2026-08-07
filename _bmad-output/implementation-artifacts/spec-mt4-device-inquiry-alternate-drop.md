---
title: 'MT4 Bridge — Device Inquiry every-other drop'
type: 'bugfix'
created: '2026-08-07'
status: 'done'
baseline_commit: 'db66f66'
head_at_followup: 'b201256'
review_loop_iteration: 0
context:
  - '{project-root}/conventions.md'
  - '{project-root}/_bmad/custom/clarity-bar-fr.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** On the MT4 Bridge path (WinUSB + Emagic + teVirtualMIDI), Universal Device Inquiry (`F0 7E 7F 06 01 F7`) on `MT4 Output Y` still loses Identity Reply on the **first Inquiry after a fresh session Start** (TIMEOUT / no Identity SendToHost), while inquiries #2+ succeed (~95 % overall after async IN). The Matrix-1000 answers on every attempt (MT4 LEDs); Scarlett stays ~100 %. Hardware is not the culprit.

**Approach:** Close the first-shot hole with a minimal Bridge fix (Start vs async IN arming / post-init residue / F5), instrument just enough to prove which race, then lab-gate **100 %** Identity on ≥20 inquiries @ ~5 s across multiple fresh Starts — without Matrix-Control heartbeat as the primary fix and without a silent warm-up Inquiry unless Ask First.

## Boundaries & Constraints

**Always:**
- Keep SysEx opaque transparent carry (no Oberheim/allowlist rewrite of Inquiry payloads).
- Preserve directional ports `MT4 Input N` / `MT4 Output Y` (no merged-port echo).
- Preserve Emagic F5 cable model and Computer Mode kick; do not break notes/CC/clock/MTC.
- Lab gate: Bridge harness ≥20 Device Inquiries ~5 s after **fresh Start** → **100 %** Identity Reply on matching Input (including Inquiry #1); reproduce on ≥3 fresh Starts.
- Run `python scripts/quality/lint-touched.py` clean on the C++ diff; builds under `builds/`.
- Document lab results (EN) in Design Notes + `docs/tests/checklists/smoke-mt4-device-inquiry-bridge.md`.

**Ask First:**
- Changing Matrix-Control heartbeat / presence timeout as the main fix.
- Broad WinUSB timeout retunes without first-shot evidence.
- Discrete warm-up Inquiry at Start as a substitute for fixing the race (acceptable only as documented fallback).
- Re-introducing pad-to-`wMaxPacketSize` (lab-failed; discarded).

**Never:**
- Windows MIDI Services migration; MidiView (BSOD); AMT8 / Unitor8 work.
- Inventing Emagic escaping for raw MIDI `0xF5` / `0xFF` (still deferred).
- Claiming pass from unit smokes alone without harness/MIDI-OX lab.
- Treating MT4 hardware or Matrix silence as the root cause when LEDs prove DIN activity on KO.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Happy Inquiry | Host sends `F0 7E 7F 06 01 F7` on `MT4 Output 1`; Matrix on DIN Out1↔In1 | Identity Reply `F0 7E … 06 02 … F7` on `MT4 Input 1` within ~200 ms | N/A |
| First Inquiry after fresh Start | First Inquiry as soon as ports are usable after Start | Identity Reply (no TIMEOUT); Bridge `inquiry_out` == `identity_reply_in` | If still KO: keep ring/sink timestamps and HALT for Ask First (warm-up only) |
| Steady session | ≥20 inquiries @ ~5 s on Output 1 only | **100 %** replies; no enduring 50 % pattern | Do not “fix” by slowing MC only |
| Spacing independence | Same at ~10 s | Same 100 % rate | N/A |
| No local echo | Inquiry on Output 1 | Input 1 must not show the host’s `06 01` as if it were a reply | Drop/ignore local TX on IN faces (already directional) |
| Encode / IN reply | Repeated Out1 Inquiry after kick; device may reply in one or more bulk packets | Complete Emagic OUT frame accepted; full Identity Reply reaches SendToHost | Encode/Write fail → English pump fail; oversize/nested SysEx → existing framer reject |

</frozen-after-approval>

## Code Map

- `src/Midi/VirtualMidiBackend.cpp` -- OUT callback → host→device
- `src/Device/DeviceSessionHostOutbound.cpp` -- encode + `WriteBulk` (OUT; pad short; gate on ring)
- `src/Protocol/EmagicCableMapper.cpp` / `.h` -- F5 sticky cable, single `0xFF` pad, demux truncate / `seenF5_`
- `src/Device/DeviceSession.cpp` -- Start / `startPump` (arm async IN **before** MIDI sink)
- `src/Device/DeviceSessionDeviceHost.cpp` -- async bulk IN wait → harvest ready slots → demux → SendToHost
- `src/Usb/WinUsbTransport.cpp` / `.h` + `WinUsbBulkInAsync.cpp` -- sync pipes + 7-slot async IN ring (`INPUT_URBS`)
- `src/Usb/WinUsbTransportInit.cpp` -- Emagic init / Computer Mode drains
- `src/Device/DeviceHostCounters.*` + `src/App/MidiSessionCli.cpp` -- host-out / reply counters for lab
- `src/Protocol/EmagicMapperSysexSmoke.cpp` + unit SysEx/framer tests -- Inquiry coverage
- `_bmad-output/implementation-artifacts/2-3-transparent-sysex-transport-with-burst-buffering.md` -- opaque SysEx invariants
- `_bmad-output/implementation-artifacts/spec-mt4-separate-virtual-ports.md` -- directional ports shipped
- `docs/tests/checklists/smoke-mt4-device-inquiry-bridge.md` -- Inquiry retest + harness
- `scripts/lab/device-inquiry-loop.py` -- preferred lab harness

## Tasks & Acceptance

**Execution:**
- [x] Counters + logs on host-outbound / device-host / CLI -- count OUT writes vs Identity Reply IN per send; show loss before USB OUT, after OUT with no IN, or after demux/framer -- localize the 50 % drop
- [x] Lab (Zadig + `--dev-zadig`, harness/MIDI-OX) -- ≥10 inquiries @ ~5 s with instrumentation; classify bucket before patching -- no timeout roulette
- [x] Revert OUT pad to single `0xFF` (Linux short URB); discard full-`wMaxPacketSize` pad after lab ~45 % fail
- [x] Async multi-buffer bulk IN ring (7 slots, Linux `INPUT_URBS`) -- always-pending WinUSB reads so Emagic bursts do not lose the leading `F0` packet
- [x] SysEx smoke + unit tests as needed -- Inquiry/Identity helper Catch2 tests -- prevent silent regress of matchers
- [x] Short English MIDI-OX (+ optional Matrix-Control) retest notes under `docs/tests/checklists/smoke-mt4-device-inquiry-bridge.md` -- reproducible gate
- [x] Lab retest after async IN -- harness ≥20 @ ~5 s → ≥95 % Identity (target 100 %)
  - 2026-08-07T121808Z: `sent=20 recv=19 rate=95.0% pass=true` (first TIMEOUT only; Bridge `inquiry_out=20` / `identity_reply_in=19`)
- [x] First-shot follow-up: arm async IN ring **before** host MIDI sink; brief post-kick IN drain; harvest already-completed IN slots before sleeping -- close Start race / residual gap
- [x] Minimal English console timestamps (ring armed → sink live → first Inquiry Write; encode length / F5) -- distinguish Start race vs F5 vs post-init
- [x] Lab: ≥3× fresh Start, each 20× @ ~5 s → **100 %** including Inquiry #1; document in Design Notes + checklist
- [x] `python scripts/quality/lint-touched.py` clean on the C++ follow-up diff

**Acceptance Criteria:**
- Given Bridge + Matrix on DIN Out1↔In1 and a **fresh** session Start, when the harness runs ≥20 Device Inquiries @ ~5 s on `MT4 Output 1` / `MT4 Input 1`, then **100 %** Identity Reply including Inquiry #1 (no TIMEOUT). **(lab 2026-08-07: 3×20 @ 100 %)**
- Given the same lab repeated on ≥3 independent fresh Starts, when each run finishes, then every run is 100 % (zero first-shot TIMEOUT). **(lab stamps `124924Z`, `125116Z`, `125305Z`)**
- Given Matrix-Control presence on Input 1 / Output 1 only, when idle 2–5 minutes, then no cyclic `deviceMidiUnresponsive` / ERROR from missing replies. **(lab pending; not the primary fix)**
- Given a lost Inquiry during diagnosis, when logs are inspected, then ring/sink/first-Inquiry timing and the failing stage are visible. **(instrumentation shipped)**
- Given the C++ diff, when `python scripts/quality/lint-touched.py` runs, then it exits clean. **(done)**

## Design Notes

Lab ~50 % is timing-independent with MIDI-OX alone → prefer state/alternation over Matrix rate limits.

**Lab facts (2026-08-07 — do not rediscuss):**
- MT4 Out1 green + In1 red LEDs blink on **every** Inquiry (including Bridge KO) → Matrix answers; loss is on the Bridge path.
- Bridge KO: WriteBulk OK, ~+448 IN bytes / ~14 demux spans, **no** Identity `SendToHost`; demux often starts at `7E` without `F0` (first USB IN packet missing).
- OK ≈ +480 bytes / ~15 spans. Thru MIDI-OX ruled out. Sync IN drain + USB lock/timing stacks did **not** clear ~50 %; some destabilized Matrix (power-cycle).
- Prefer harness: `python scripts/lab/device-inquiry-loop.py --with-bridge --count 20 --interval 5`.

**Order (instrument first; one fix with evidence):**
1. **OUT pad** — ~~fill-to-`wMaxPacketSize`~~ **lab fail (~45 %, 9/20)**; reverted to Linux single trailing `0xFF` + short URB (`snd_usbmidi_emagic_output`). Full-packet pad is **discarded**.
2. **Bulk IN gap** — sync one-packet `ReadBulk` leaves a hole; Linux keeps `INPUT_URBS` (7) always pending. **Shipped:** WinUSB async multi-buffer IN ring (7 slots). Lab 19/20 — every-other gone.
3. **First-shot after Start (this follow-up)** — code shows `startPump` enabled the MIDI sink and returned **before** `StartBulkInAsyncRing` finished on the reader thread; Computer Mode channel kick had no post-Write IN drain. Fix: arm ring on Start thread before sink; brief post-kick drain; harvest completed IN slots in a tight loop so burst depth stays near 7.
4. Do **not** lead with WinUSB timeout tweaks, Matrix-Control heartbeat, or warm-up Inquiry (Ask First fallback only).

**Decision (Guillaume, option 1):** single-`0xFF` OUT pad + async multi-IN; no heartbeat pivot.

**Async IN lab (2026-08-07T121808Z):** harness 20× @ 5 s → **19/20 = 95 %** (`pass=true`). First Inquiry only: TIMEOUT + Bridge +448 / 14 spans / no Identity; inquiries 2–20 all RECV ~47 ms. Every-other ~50 % pattern is gone. Residual: first Inquiry after Start → **100 %** bar for V1.

**Follow-up baseline:** original `baseline_commit` remains `db66f66`; HEAD at follow-up start was `b201256` (working tree already carried async IN + pad revert).

**First-shot fix lab (2026-08-07 — 3 fresh Starts):**
| Stamp | sent | recv | rate | Inquiry #1 | Bridge counters |
|-------|------|------|------|------------|-----------------|
| `124924Z` | 20 | 20 | 100 % | RECV ~46 ms | `inquiry_out=20` / `identity_reply_in=20` |
| `125116Z` | 20 | 20 | 100 % | RECV ~47 ms | `inquiry_out=20` / `identity_reply_in=20` |
| `125305Z` | 20 | 20 | 100 % | RECV ~46 ms | `inquiry_out=20` / `identity_reply_in=20` |

On each #1: `first_after_start=yes`, `ring_active=yes`, `f5_switch=no` (Out1 shares cable with Computer Mode kick), `ms_since_ring_arm` ≈ 1.3 s, OK bulk ≈ +480 / 15 spans. No warm-up Inquiry used. Logs under `tests/lab-logs/device-inquiry/`.

**Post-review patch retest:** `130312Z` — another fresh Start 20/20 @ 100 % (Inquiry #1 RECV ~47 ms) after harvest/Stop/sink-order hardening.

## Verification

**Commands:**
- `python scripts/quality/lint-touched.py` -- expected: clean on touched C++
- Emagic SysEx / framer unit or `--test-mapper` smokes -- expected: pass after encode/demux changes
- `python scripts/lab/device-inquiry-loop.py --with-bridge --count 20 --interval 5 --pass-percent 100` -- expected: `recv=20 rate=100%` after fresh Start (repeat ≥3×)

**Manual checks (contributor lab):**
1. Zadig + harness (preferred) or MIDI-OX ≥20× `F0 7E 7F 06 01 F7` @ ~5 s on Output 1 after fresh Start; **100 %** `06 02` on Input 1 including #1.
2. Optional: Matrix-Control presence Input 1 / Output 1, calm 2–5 min — no cyclic ERROR.
3. Scarlett control already 100 % — do not blame the Matrix-1000.

## Suggested Review Order

**Start order (first-shot race)**

- Arm the 7-slot IN ring before any host MIDI WriteBulk.
  [`DeviceSession.cpp:185`](../../src/Device/DeviceSession.cpp#L185)

- Start the reader Wait loop, then enable the host sink.
  [`DeviceSession.cpp:226`](../../src/Device/DeviceSession.cpp#L226)

- Fail Start if post-kick short IN drain cannot arm.
  [`DeviceSession.cpp:55`](../../src/Device/DeviceSession.cpp#L55)

**Always-pending IN (Linux INPUT_URBS)**

- Seven overlapped bulk IN slots always pending.
  [`WinUsbBulkInAsync.cpp:163`](../../src/Usb/WinUsbBulkInAsync.cpp#L163)

- Harvest already-completed slots before sleeping; Abort during Stop is not fatal.
  [`DeviceSessionDeviceHost.cpp:214`](../../src/Device/DeviceSessionDeviceHost.cpp#L214)

- Retry Abort+wait on Stop so OVERLAPPED is not freed early.
  [`WinUsbBulkInAsync.cpp:221`](../../src/Usb/WinUsbBulkInAsync.cpp#L221)

**OUT encode + lab counters**

- Single trailing `0xFF` pad (full-packet pad discarded after lab fail).
  [`EmagicCableMapper.cpp:92`](../../src/Protocol/EmagicCableMapper.cpp#L92)

- First Inquiry logs ring timing / F5 / first-after-start.
  [`DeviceSessionHostOutbound.cpp:103`](../../src/Device/DeviceSessionHostOutbound.cpp#L103)

**Peripherals**

- Preferred harness gate (100 % after fresh Start).
  [`device-inquiry-loop.py:1`](../../scripts/lab/device-inquiry-loop.py#L1)

- Operator checklist + lab stamps.
  [`smoke-mt4-device-inquiry-bridge.md:1`](../../docs/tests/checklists/smoke-mt4-device-inquiry-bridge.md#L1)
