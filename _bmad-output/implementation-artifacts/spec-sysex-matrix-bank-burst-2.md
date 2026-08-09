---
title: 'MT4 — Windows Bridge Matrix bank burst SysEx (palier 2)'
type: 'chore'
created: '2026-08-07'
status: 'done'
baseline_commit: 'c535069'
review_loop_iteration: 0
context:
  - '{project-root}/conventions.md'
  - '{project-root}/_bmad-output/implementation-artifacts/spec-sysex-matrix-bank-burst.md'
  - '{project-root}/docs/tests/lab-reports/macos-sysex-paliers-2026-08-07.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** macOS Apple already passes bank-scale Matrix dump bursts at 100 %; Windows Bridge still lacks a stamped 100 % gate for the same 100× 275 B device→host traffic.

**Approach:** Run the existing bank harness with `--with-bridge` on a warm Matrix, document Pass/Fail evidence, amend checklist/spec notes so Windows is the active gate (not “later”), and only extend Bridge C++ if the lab proves a new hole.

## Boundaries & Constraints

**Always:**
- Single goal: Windows Bridge palier-2 bank burst = 100 sequential patch dumps, device→host, `--pass-percent 100`, ≥2 fresh Bridge Starts.
- Request: `F0 10 06 04 01 <n> F7` slot sweep (default 0…99); Pass frame = exactly 275 B, prefix `F0 10 06 01`, trailing `F7`; pacing ≥10 ms; per-frame timeout ~3 s.
- Reuse `scripts/lab/sysex-matrix-bank-loop.py` (do not rewrite). Logs under `tests/lab-logs/sysex-matrix-bank/` (+ `bridge-<stamp>-startN.log`).
- Keep palier-1 Bridge guards (post-Start calm, OUT silence during dump expect, leading-F0 repair, Matrix size reject+1 retry). Never `SendToHost` from WinUSB completion thread.
- On Fail with DIN LEDs active: blame Bridge stack; log index/slot/len/head/tail/`dt_ms` + Bridge counters; one root-cause track → rebuild → re-lab ≥2 Starts.

**Ask First:**
- Switching the gate to a real Oberheim Dump All export.
- Lowering the 100 % bar, skipping a fresh Start, or Pass with Inquiry warm-up.
- Removing or weakening any palier-1 guard “to simplify”.

**Never:**
- Palier 3 mega-SysEx / DIN echo without Matrix.
- Matrix-Control UI / heartbeat as gate or fix; AMT8 / Unitor8; Windows MIDI Services; MidiView/ShowMIDI (retired; use MIDI-OX).
- Accusing Matrix, DIN cable, or MT4 hardware (Apple control already 100 % same path).
- Artificial slowdown beyond stock ≥10 ms pacing; French in source.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| bank_burst Bridge | `--with-bridge`, Matrix warm, DIN 1↔1, 2 Starts × 100 dumps | exit 0, `overall_pass=true`, every frame 275 B | TIMEOUT / wrong size → Fail exit 2; stamp logs |
| Fresh Start 2 | New Bridge Start, no Inquiry warm-up | First frame of burst still Pass | BRIDGE_FAIL_NEEDLES → Fail that Start |
| Fragmented recv | VirtualMIDI/rtmidi splits SysEx | Reassemble until F7 then validate | Never Pass on open buffer |
| Instrumented Fail | DIN LEDs active, wrong len/timeout | One C++ fix if hole proven; re-lab ≥2 Starts | No stacked speculative patches |

</frozen-after-approval>

## Code Map

- `scripts/lab/sysex-matrix-bank-loop.py` -- bank-burst harness; `--with-bridge` lifecycle already implemented
- `builds/debug/Debug/Bridge.exe` -- Debug Bridge launched by harness (`--start-session --dev-zadig`)
- `docs/tests/checklists/smoke-mt4-sysex-matrix-bank.md` -- operator checklist; Windows section → stamp evidence
- `_bmad-output/implementation-artifacts/spec-sysex-matrix-bank-burst.md` -- macOS-done sibling; Design Notes said Windows later
- `src/Device/DeviceSession*.cpp` / `DeviceSessionSupport.*` / `WinUsbBulkInAsync.cpp` -- palier-1 guards; touch only if lab proves new hole
- `tests/lab-logs/sysex-matrix-bank/` -- Windows evidence (create on first run)
- `docs/tests/lab-reports/macos-sysex-paliers-2026-08-07.md` -- hardware control SSOT (do not reopen)

## Tasks & Acceptance

**Execution:**
- [x] `_bmad-output/implementation-artifacts/spec-sysex-matrix-bank-burst.md` -- append Design Note pointer that Windows Bridge gate lives in this `-2` spec -- continuity
- [x] `docs/tests/checklists/smoke-mt4-sysex-matrix-bank.md` -- mark Windows Bridge as active gate; record stamp / rates after lab -- operator SSOT
- [x] Lab: `python scripts/lab/sysex-matrix-bank-loop.py --with-bridge --pass-percent 100` -- ≥2 Starts, Matrix already warm -- primary gate
- [x] On Fail: instrument one Bridge root cause (keep palier-1 guards), rebuild Debug Bridge, re-lab ≥2 Starts -- N/A (lab Pass first try; no C++ change)
- [x] If C++ changed: `python scripts/quality/lint-touched.py` clean -- N/A (no C++ change)

**Acceptance Criteria:**
- Given Matrix warm on DIN Out1↔In1 and ports free, when the harness runs `--with-bridge --pass-percent 100` with defaults (100 dumps, ≥10 ms, 2 fresh Starts), then exit 0 and journal `overall_pass=true` with every reply exactly 275 B `F0 10 06 01…F7`.
- Given any TIMEOUT or wrong-size frame, when the run ends, then Fail with index/slot/len/head/tail/`dt_ms` (and Bridge logs) and checklist records an honest Fail — never Pass with warm-up.
- Given a Bridge C++ change, when re-lab completes, then ≥2 Starts still meet 100 % and palier-1 guards remain in place.

## Spec Change Log

## Design Notes

macOS Apple control already closed: stamp `20260807T171139Z`, 2×100, `overall_pass=true` (see sibling done spec + palier report).

Windows Bridge lab closed first try: stamp `20260807T191915Z`, ports `MT4 Output 1`/`MT4 Input 1`,
Start1 `bank_burst_patch` 100/100 (~14.26 s), Start2 100/100 (~14.39 s), `overall_pass=true`, exit 0.
No Bridge C++ change required (palier-1 guards sufficient for sustained dump burst).
Both Starts: `inquiry_out=0` / `identity_reply_in=0` (no Inquiry warm-up); leading-F0 repair
logged once per cold Start before the first dump completed.

```text
python scripts/lab/sysex-matrix-bank-loop.py --with-bridge --pass-percent 100
```

Defaults: `MT4 Output 1` / `MT4 Input 1`, `--fresh-starts 2`, `--count 100`, `--interval 0.01`, `--reply-timeout 3`.

Palier-1 mid-size on this machine: `sysex-matrix-mid-20260807T190336Z` `overall_pass=true` after `c535069`.

## Verification

**Commands:**
- `python scripts/lab/sysex-matrix-bank-loop.py --with-bridge --pass-percent 100` -- expected: exit 0, `overall_pass=true`
- `python scripts/quality/lint-touched.py` -- expected: clean (only if C++ touched)

**Manual checks (if no CLI):**
- Matrix LEDs blink across the burst; no DAW/MIDI-OX holding MT4 ports; cold Matrix mid-run is not a valid Pass.

## Suggested Review Order

**Evidence**

- Lab journal closes the Windows bank gate at 100 % across two Starts
  [`sysex-matrix-bank-20260807T191915Z.log:443`](../../tests/lab-logs/sysex-matrix-bank/sysex-matrix-bank-20260807T191915Z.log#L443)

- Start 1 summary (100/100)
  [`sysex-matrix-bank-20260807T191915Z.log:222`](../../tests/lab-logs/sysex-matrix-bank/sysex-matrix-bank-20260807T191915Z.log#L222)

- Start 2 summary (100/100)
  [`sysex-matrix-bank-20260807T191915Z.log:437`](../../tests/lab-logs/sysex-matrix-bank/sysex-matrix-bank-20260807T191915Z.log#L437)

**Operator docs**

- Checklist Lab notes with stamp, Inquiry counters, F0-repair note
  [`smoke-mt4-sysex-matrix-bank.md:85`](../../docs/tests/checklists/smoke-mt4-sysex-matrix-bank.md#L85)

- Windows procedure is the active gate (not “later”)
  [`smoke-mt4-sysex-matrix-bank.md:46`](../../docs/tests/checklists/smoke-mt4-sysex-matrix-bank.md#L46)

**Spec continuity**

- Design Notes capture Pass + retained palier-1 guards
  [`spec-sysex-matrix-bank-burst-2.md:83`](./spec-sysex-matrix-bank-burst-2.md#L83)

- Sibling macOS done spec points here with fresh-starts default
  [`spec-sysex-matrix-bank-burst.md:93`](./spec-sysex-matrix-bank-burst.md#L93)

**Peripherals**

- Harness docstring marks Windows as palier-2 gate
  [`sysex-matrix-bank-loop.py:12`](../../scripts/lab/sysex-matrix-bank-loop.py#L12)

- Lab prompt points operators at this `-2` spec
  [`lab-palier-2-sysex-matrix-bank.md:16`](../../docs/tests/lab-prompts/lab-palier-2-sysex-matrix-bank.md#L16)

