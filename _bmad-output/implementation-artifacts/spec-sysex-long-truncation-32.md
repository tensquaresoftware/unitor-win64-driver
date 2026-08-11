---
title: 'Fix intermittent −32-byte long SysEx on DIN Out2→In2 loopback'
type: 'bugfix'
created: '2026-08-11'
status: 'done'
baseline_commit: 'f5da003'
review_loop_iteration: 0
context:
  - '{project-root}/conventions.md'
  - '{project-root}/docs/tests/lab-prompts/lab-palier-3-sysex-long-loopback.md'
  - '{project-root}/tests/lab-logs/overnight-long-loopback/overnight-20260811T094155Z.log'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** On Windows Bridge long SysEx DIN loopback (MT4 Out 2 → In 2), frames sometimes return exactly 32 bytes short (4096→4064, 14708→14676) with a moving first-diff offset, so the 100 % gate fails even when Matrix In1/Out1 soaks stay green.

**Approach:** Reproduce on solo loopback with minimal Bridge diagnostics that prove where the 32-byte Emagic quantum disappears, apply the smallest transport/session fix that closes the hole, then lock it with a short targeted lab (not an overnight) before any combined soak.

## Boundaries & Constraints

**Always:**
- Single goal: eliminate the intermittent −32 B long-SysEx integrity hole on physical Out2→In2 loopback under the existing Bridge.
- Treat Bridge `SendToHost … bytes=` as ground truth for framed length (lab already matches it); localize loss on device→host before virtualMIDI.
- Solo loopback first; combined Matrix+loopback only after solo preflight is 100 %.
- Pass = byte-identical F0…F7 vs sent; `--pass-percent 100`.
- Keep always-pending bulk IN ring + no `SendToHost` from the WinUSB completion thread.
- C++ changes pass `scripts/quality/lint-touched.py` on the touched diff; English-only source; commits only on Guillaume’s request.

**Ask First:**
- Turning on `bulkInTrySkipLostSeq` / any deliberate mid-SysEx seq skip (known to manufacture N×32 holes).
- Raising `kBulkInAsyncSlotCount` past the WaitForMultipleObjects 64-handle budget, or rewriting the async ring architecture.
- Declaring the DIN/MT4 hardware guilty while Windows still fails and macOS overnight control remains clean on the same class of payloads.
- Shipping overnight/combined 4h+ as the iteration gate instead of a short solo lab.

**Never:**
- Kernel driver work; Epic 2 debt cleanup; Matrix mid/bank feature changes; inventing a new long fixture; blaming mido/lab length when Bridge already logged the short `bytes=`.
- Broad refactors “for later” (AMT8/Unitor8, new abstractions).

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Happy long loopback | Synth 4096 + fixture ~14708 on Out2→In2, Bridge running | Identical F0…F7 lengths and bytes at 100 % for the short gate | N/A |
| Historic −32 hole | Same payloads; intermittent under sustained IN during long OUT | Must not emit complete short F0…F7 missing exactly 32 B | If transport must fail, fail loud (pump/log) — never silent short Pass |
| Queue overflow | Deliver queue at cap during long OUT | Newest rejected + pump failure (existing fail-closed) | Lab sees bridge fail needle; not a silent −32 Pass |
| Matrix undisturbed | Mid/bank on In1/Out1 after solo green | No regression required in this spec’s short gate; combined soak is post-gate only | Do not block −32 fix on combined overnight |

</frozen-after-approval>

## Code Map

- `src/Usb/WinUsbTransport.h` -- `kBulkInAsyncSlotCount=63` (WFMO max: slots + stop ≤ 64); comments on lab `275−N×32` / −32 B arming starvation
- `src/Usb/WinUsbBulkInAsyncRing.h` -- ordered harvest; `bulkInTrySkipLostSeq` present but unused (must stay off unless Ask First)
- `src/Usb/WinUsbBulkInCompletion.cpp` -- completion → ordered deliver → packet handler
- `src/Usb/WinUsbEmagicHostMidi.cpp` -- host→device OUT chunk cap 32 B; between-chunk IN drain hook
- `src/Device/DeviceSessionDeviceHost.cpp` -- `enqueueBulkInPacket`, deferred `SendToHost` during OUT
- `src/Device/DeviceSessionBulkInDeliver.cpp` -- between-chunk drain; `deliver_hw` / long SysEx OUT logs
- `src/Device/DeviceSessionHostOutbound.cpp` -- long WriteBulk path holding `usbIoMutex_` while IN queues
- `src/Device/DeviceSessionSupport.cpp` -- `logLongSysexSendToHost` (already shows short `bytes=`)
- `src/Protocol/EmagicCableMapper.cpp` -- demux / sticky F5 (secondary suspect only)
- `src/Midi/MidiMessageFramer.*` -- SysEx hold across URBs (passes through USB gaps as short complete F0…F7)
- `scripts/lab/sysex-long-loopback.py` -- exact-match harness; Bridge fail needles
- `scripts/lab/overnight-long-loopback-stress.py` -- solo soak launcher (post-fix only)
- Evidence: `tests/lab-logs/overnight-long-loopback/` and `tests/lab-logs/overnight-combined/` (2026-08-11) — Bridge `bytes=4064/14676` with `missing_bytes=32`

## Tasks & Acceptance

**Execution:**
- [x] `src/Usb/WinUsbBulkInCompletion.cpp` + `src/Device/DeviceSessionDeviceHost.cpp` + `src/Device/DeviceSessionBulkInDeliver.cpp` -- add minimal lab-facing counters/logs: per-URB size while any SysEx hold is open, enqueue vs drain counts across one long OUT, and any reject/drop path -- prove or disprove a single missing 32 B Emagic IN quantum before coding a fix
- [x] Reproduce with `scripts/lab/sysex-long-loopback.py` solo Out2/In2 (4096 + fixture, high reps, `--pass-percent 100`, `--with-bridge`) until one −32 mismatch is captured with the new traces -- establish locus (ring/completion vs demux vs deferred send) *(2026-08-11T102745Z: start1 100%; start2 fixture 19/20 with `bytes=14676` + `gap_armed_starved=yes` / `gap_min_armed=0` — confirms HC arming starvation locus; first re-harvest fix insufficient alone)*
- [x] Apply the smallest fix at the proven locus (prefer fail-closed or preventing the gap; do not enable seq-skip without Ask First) -- close the −32 hole without widening scope *(v1/v2 −32 arming; v3 sticky F5 no longer steals product `0x01`/`0x02` mid-SysEx across URB — pending lab; observed −1 B was fixture `0x63` so may need another pass)*
- [x] Keep or trim diagnostics to a maintainable high-signal subset after the fix -- avoid permanent log spam *(steady path: `armed_urbs` / `gap_min_armed` only; enq/drain/f5/reject only on anomaly)*
- [x] Re-run the same short solo lab to 100 %; optionally a few combined cycles only after solo green -- regression gate for this bug *(2026-08-11T213557Z: `overall_pass: true`; both starts 20/20×3; no −32/−1; no `gap_armed_starved`; `gap_min_armed`≥46/55; `gap_f5_*=0`)*
- [x] `scripts/quality/lint-touched.py` on touched C++ -- quality door

**Acceptance Criteria:**
- Given Bridge + DIN Out2→In2 and the existing 4096 + ~14708 payloads, when the short solo lab runs at `--pass-percent 100`, then every trial is byte-identical (no `missing_bytes=32` / short `SendToHost bytes=`).
- Given a −32 failure is reproduced under instrumentation, when traces are read, then they identify whether a 32 B IN quantum was missing before demux, dropped after demux, or deferred-send overflow — and the chosen fix matches that locus.
- Given the deliver queue hits its cap, when overflow occurs, then the pump still fails closed (no silent Pass with a short F0…F7).
- Given Guillaume has not asked to commit, when the work finishes, then changes remain uncommitted.

## Design Notes

Evidence already rules out lab/mido length invention and host→device encode length: WriteBulk logs full `midi_bytes` while `SendToHost` logs the short frame with intact F0/F7. Deliver-queue overflow rejects newest and fails the pump (loud) — inconsistent with `bridge_fail` count 0 on these runs. Framer has no 32 B quantum; Emagic full-speed bulk is 32 B (`kEmagicOutChunkCap` / historic `275−N×32`). Prefer proving a missing IN URB/quantum under long OUT + DIN loopback pressure (`deliver_hw` spikes) before touching demux.

Do **not** “fix” by enabling `bulkInTrySkipLostSeq`: that path was deliberately left unused because skipping a lost seq mid-SysEx manufactures exactly this class of short complete frames.

## Verification

**Commands:**
- `python scripts/lab/sysex-long-loopback.py --with-bridge --dev-zadig --out-port "MT4 Out 2" --in-port "MT4 In 2" --pass-percent 100` -- expected: exit 0 / `overall_pass: true`, no `missing_bytes=32`
- `python scripts/quality/lint-touched.py` -- expected: clean on touched C++

**Manual checks (if no CLI):**
- On any residual FAIL, Bridge log must not show `SendToHost … bytes=` short by 32 with matching lab `missing_bytes=32` unless the pump also failed closed and the lab recorded a bridge failure (not a silent integrity Pass).

## Suggested Review Order

**USB IN arming (−32 B)**

- Entry: deepen always-pending IN ring to WFMO max (63 + stop).
  [`WinUsbTransport.h:28`](../../src/Usb/WinUsbTransport.h#L28)

- Re-harvest before each ordered pop so completed slots re-arm during deliver storms.
  [`WinUsbBulkInCompletion.cpp:150`](../../src/Usb/WinUsbBulkInCompletion.cpp#L150)

- Re-harvest again after the packet handler closes the unlocked enqueue window.
  [`WinUsbBulkInCompletion.cpp:208`](../../src/Usb/WinUsbBulkInCompletion.cpp#L208)

- Armed count uses WAIT_TIMEOUT only (WAIT_FAILED must not look “armed”).
  [`WinUsbBulkInCompletion.cpp:54`](../../src/Usb/WinUsbBulkInCompletion.cpp#L54)

**Sticky F5 (−1 B landmine)**

- Cross-URB sticky F5 while SysEx open keeps product `0x01`/`0x02` as data.
  [`EmagicCableMapper.cpp:213`](../../src/Protocol/EmagicCableMapper.cpp#L213)

- Same-URB mid-SysEx `F5 xx` still consumes a real cable retag.
  [`EmagicCableMapper.cpp:262`](../../src/Protocol/EmagicCableMapper.cpp#L262)

**Lab probes (trimmed)**

- Arm gap probe only around a long host→device SysEx, then clear after flush.
  [`DeviceSessionHostOutbound.cpp:234`](../../src/Device/DeviceSessionHostOutbound.cpp#L234)

- Steady logs keep `gap_min_armed`; extra fields only on anomaly.
  [`DeviceSessionSupport.cpp:78`](../../src/Device/DeviceSessionSupport.cpp#L78)

**Tests**

- Sticky mid-SysEx must not steal In2 (`0x02`) or In1 (`0x01`) data bytes.
  [`EmagicCableMapperTests.cpp:89`](../../tests/unit/EmagicCableMapperTests.cpp#L89)
