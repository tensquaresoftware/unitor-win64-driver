---
title: 'MT4 — Matrix bank burst SysEx lab (palier 2, 100× patch dump)'
type: 'chore'
created: '2026-08-07'
status: 'done'
baseline_commit: '5913443'
review_loop_iteration: 0
context:
  - '{project-root}/conventions.md'
  - '{project-root}/_bmad-output/implementation-artifacts/spec-sysex-matrix-mid-macos.md'
  - '{project-root}/docs/tests/checklists/smoke-epic2-matrix-control-mt4.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** After mid-size patch/master SysEx is proven on Apple (and partially on Windows Bridge), we still lack an automated 100 % gate for bank-scale device→host traffic (~100× 275 B patch frames).

**Approach:** Ship a Python bank-burst harness that issues 100 sequential Matrix patch dump requests (slot sweep), reassembles each reply to F7, and records a strict 100 % pass across ≥2 fresh sessions — first as macOS Apple-driver control, with Windows `--with-bridge` flags ready for later.

## Boundaries & Constraints

**Always:**
- Single goal: palier-2 bank burst = N sequential patch dumps (default N=100), device→host.
- Request shape: `F0 10 06 04 01 <n> F7` with `<n>` from `--start-slot` for N slots (default 0…99).
- Pass per frame: exactly 275 B, prefix `F0 10 06 01`, trailing `F7`; reassemble until F7.
- Pacing ≥10 ms between trial starts (`--interval` default 0.01); per-frame reply timeout ~3 s (not one Identity-style timeout for the whole burst).
- ≥2 fresh sessions (MIDI-only process reopen on macOS; Bridge Starts when `--with-bridge`).
- `--pass-percent 100`; logs under `tests/lab-logs/sysex-matrix-bank/` (Windows) or `…-macos/` (Apple).
- macOS lab: no `--with-bridge`; use listed Apple port names (e.g. `MT4 Port 1`).
- Script-only unless a later Windows Bridge lab proves a hole.

**Ask First:**
- Switching the gate to a real Oberheim Dump All export.
- Lowering the 100 % bar or skipping a fresh session.
- Blaming Matrix when DIN LEDs show activity on Fail.

**Never:**
- Palier 3 mega-SysEx / echo responder in this story.
- Matrix-Control UI as primary gate; AMT8 / Unitor8; Windows MIDI Services.
- Opaque Inquiry warm-up; artificial slowdown beyond stock ≥10 ms pacing.
- French in source code.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| bank_burst slot sweep | N dump requests, ≥10 ms pacing | N/N replies 275 B exact | TIMEOUT / wrong size → Fail; log index, slot, len, head, tail, dt_ms |
| Fresh session 2 | New process / Bridge Start | First frame of burst still 100 % | No Inquiry warm-up |
| Fragmented recv | rtmidi may split SysEx | Buffer until F7 then validate | Never Pass on open buffer |

</frozen-after-approval>

## Code Map

- `scripts/lab/sysex-matrix-mid-loop.py` -- patterns: assembler, ports, fresh sessions, Bridge lifecycle
- `scripts/lab/sysex-matrix-bank-loop.py` -- new bank-burst harness
- `scripts/lab/requirements-device-inquiry.txt` -- mido / python-rtmidi
- `docs/tests/checklists/smoke-epic2-matrix-control-mt4.md` -- vector #6 bank stress shape
- `tests/lab-logs/sysex-matrix-bank-macos/` -- Apple-driver evidence

## Tasks & Acceptance

**Execution:**
- [x] `scripts/lab/sysex-matrix-bank-loop.py` -- bank burst harness (slot sweep, F7 reassembly, fresh sessions, optional --with-bridge) -- palier-2 gate
- [x] `tests/lab-logs/sysex-matrix-bank-macos/README.md` -- macOS operator how-to -- evidence path
- [x] `docs/tests/checklists/smoke-mt4-sysex-matrix-bank.md` -- EN checklist -- human retest
- [x] Lab run macOS 2×100 on MT4 Port 1 -- document stamps / rates -- hardware control

**Acceptance Criteria:**
- Given Apple MT4 Port 1 and Matrix on DIN Out1↔In1, when the harness runs 100 sequential patch dumps at ≥10 ms pacing with `--pass-percent 100`, then every reply is exactly 275 B with locked prefix/suffix inside the per-frame timeout.
- Given ≥2 fresh Mac sessions, when each runs the full burst without Inquiry warm-up, then both sessions meet 100 % and the journal has `overall_pass=true`.
- Given any single TIMEOUT or wrong-size frame, when the run finishes, then overall Fail with index/slot/len/head/tail/dt_ms logged.

## Spec Change Log

## Design Notes

macOS Apple control stamp `20260807T171139Z` — `overall_pass=true`, exit 0.
Ports: OUT/IN `MT4 Port 1`.
Session 1: `bank_burst_patch` sent=100 ok=100 rate=100% (~13.5 s); first frame Pass dt_ms≈129.
Session 2 (fresh process): same 100/100; first frame Pass.

```text
python3 scripts/lab/sysex-matrix-bank-loop.py \
  --out-port "MT4 Port 1" --in-port "MT4 Port 1" \
  --count 100 --interval 0.01 --reply-timeout 3 \
  --pass-percent 100 --fresh-sessions 2 \
  --log-dir tests/lab-logs/sysex-matrix-bank-macos
```

Windows Bridge gate: see `_bmad-output/implementation-artifacts/spec-sysex-matrix-bank-burst-2.md`
(`python scripts/lab/sysex-matrix-bank-loop.py --with-bridge --pass-percent 100`
— defaults include `--fresh-starts 2`).
Palier 3 deferred pending hardware decision for device→host mega-SysEx.

## Verification

**Commands:**
- `python3 scripts/lab/sysex-matrix-bank-loop.py --list-ports` -- expected: MT4 Port 1 listed
- Full macOS one-shot above -- expected: exit 0, `overall_pass=true`

**Manual checks (if no CLI):**
- Matrix LEDs blink across the burst; no DAW holding ports.
