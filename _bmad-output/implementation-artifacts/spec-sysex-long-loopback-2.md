---
title: 'MT4 — long SysEx DIN loopback on Windows Bridge (palier 3, Mac parity)'
type: 'chore'
created: '2026-08-07'
status: 'in-progress'
baseline_commit: '0c8234f'
review_loop_iteration: 0
context:
  - '{project-root}/conventions.md'
  - '{project-root}/_bmad-output/implementation-artifacts/spec-sysex-long-loopback.md'
  - '{project-root}/docs/tests/lab-reports/macos-sysex-paliers-2026-08-07.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** After Windows Bridge paliers 1–2 at 100 %, we still lack proof that large SysEx survives a full Bridge USB↔DIN round-trip like the macOS Apple control (1024 / 4096 / fixture ~14708 B). Today Bridge caps assemble hold at 1024 and host→device encode scratch at 4096, so Mac-parity sizes cannot Pass honestly.

**Approach:** Raise both Bridge ceilings just enough for the existing long fixture, keep DIN Out1→In1 option A (Matrix off those jacks), and prove byte-identical loopback at 100 % across ≥2 fresh Bridge Starts with the same payloads as macOS.

## Boundaries & Constraints

**Always:**
- Single goal: Windows Bridge palier-3 long SysEx integrity via physical DIN loopback (exact equality).
- Topology: DIN Out1 → In1; Matrix disconnected from those jacks.
- Payloads: synthetic 1024 + 4096 + fixture `tests/fixtures/sysex/long-loopback-14708.syx` (~14708 B); do not invent another fixture.
- Pass = received frame identical to sent (length + every byte); reassemble until F7; `--pass-percent 100`.
- ≥20 reps per payload; interval ≥50 ms; reply timeout large enough for ~1–5 s/frame; ≥2 fresh Bridge Starts (`--fresh-starts 2`); no opaque warm-up.
- Raise `kMaxSysexHoldBytes` and `kEncodeBufferCapacity` to **16384** (clears 14708 with headroom; oversize reject path stays observable and relative to the new cap).
- Preserve: no `SendToHost` from WinUSB completion thread; always-pending USB IN ring + immediate resubmit.
- Logs under `tests/lab-logs/sysex-long-loopback/` (+ bridge start logs). Hardware control SSOT remains macOS report — do not blame DIN/MT4 if Windows fails under the same topo.
- C++ only if a proven hole remains after the ceiling raise; `lint-touched.py` clean; English-only source.

**Ask First:**
- Switching gate to Python ACK/dump responder (option B).
- Claiming Pass while Matrix is still on those DIN jacks.
- Raising hold/encode beyond 16384, or lowering the 100 % bar.
- Skipping paliers 1–2 Windows prerequisites on this machine.

**Never:**
- Under-1024-only gate as the Pass definition for this story (Guillaume chose Mac-parity stress including the 14 KiB fixture).
- Matrix-Control / real Matrix forms as this gate; Windows MIDI Services / MidiView; AMT8 / Unitor8.
- Silent hang on oversize; French in source; commit without Guillaume’s explicit ask.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| synth_1024 / synth_4096 | Send synthetic F0…F7 via Bridge | Exact same frame on IN | TIMEOUT / mismatch → Fail (instrument len/offset/Bridge counters) |
| fixture_14708 | Send long fixture | Exact same frame on IN | Same |
| Oversize above new hold | SysEx ≥16384 held bytes | English oversize reject / pump fail; no partial Pass | Observable reject count |
| Fresh Start 2 | New Bridge process | First trial of each size still 100 % | No warm-up |

</frozen-after-approval>

## Code Map

- `src/Protocol/MidiMessageFramer.h` / `.cpp` -- `kMaxSysexHoldBytes` + oversize reject
- `src/Device/DeviceSessionHostOutbound.cpp` -- `kEncodeBufferCapacity` (host→device encode scratch)
- `src/Device/DeviceSessionDeviceHost.cpp` -- English oversize pump fail text
- `src/App/FramerSysexSmoke.cpp` / `tests/unit/MidiMessageFramerSysexTests.cpp` -- oversize relative to cap
- `scripts/lab/sysex-long-loopback.py` -- harness (`--with-bridge`, defaults 1024/4096 + fixture)
- `tests/fixtures/sysex/long-loopback-14708.syx` -- Mac-proven long fixture
- `docs/tests/checklists/smoke-mt4-sysex-long-loopback.md` -- smoke checklist
- `docs/tests/lab-prompts/lab-palier-3-sysex-long-loopback.md` -- operator prompt
- `_bmad-output/implementation-artifacts/spec-sysex-long-loopback.md` -- macOS done control (context only)

## Tasks & Acceptance

**Execution:**
- [ ] `src/Protocol/MidiMessageFramer.h` -- set `kMaxSysexHoldBytes = 16384` -- clear fixture on device→host assemble
- [ ] `src/Device/DeviceSessionHostOutbound.cpp` -- set `kEncodeBufferCapacity = 16384` -- clear 4096/14708 on host→device encode
- [ ] Rebuild Debug Bridge; run framer unit + FramerSysex smoke -- oversize still fails cleanly above new cap
- [ ] `docs/tests/checklists/smoke-mt4-sysex-long-loopback.md` + `docs/tests/lab-prompts/lab-palier-3-sysex-long-loopback.md` -- Windows Mac-parity gate (full sizes); point to this spec
- [ ] Amend Design Notes on parent `spec-sysex-long-loopback.md` with Windows pointer only -- do not reopen macOS frozen intent
- [ ] Lab: `python scripts/lab/sysex-long-loopback.py --with-bridge --pass-percent 100` -- ≥2 fresh Starts, all payloads 100 %, document stamp
- [ ] On KO under raised ceilings: instrument (len, mismatch offset, Bridge counters) then minimal C++ fix -- one track → rebuild → re-lab
- [ ] `python scripts/quality/lint-touched.py` -- clean on touched C++

**Acceptance Criteria:**
- Given DIN Out1→In1, Matrix off those jacks, Zadig + Bridge Debug, when the harness runs synthetic 1024 + 4096 + fixture ~14708 at `--pass-percent 100` with `--fresh-starts 2`, then every trial is byte-identical and the journal shows `overall_pass=true` with exit 0.
- Given a SysEx that exceeds the new hold, when framer smoke/unit runs, then oversize remains an English-observable Fail (no silent partial Pass).
- Given any Fail on ≤14708 under the same topo as the macOS control, when diagnosing, then treat it as a Bridge defect path — not “Mac was better so hardware”.

## Spec Change Log

## Design Notes

macOS Apple already proved 1024/4096/14708 at 100 % (DIN Out1→In1) — see
`docs/tests/lab-reports/macos-sysex-paliers-2026-08-07.md`.

Both ceilings must move together: raise hold alone still cannot encode outbound 4096/14708
while `kEncodeBufferCapacity` stays 4096.

```text
python scripts/lab/sysex-long-loopback.py --with-bridge --pass-percent 100
```

Defaults: `MT4 Output 1` / `MT4 Input 1`, `--sizes 1024,4096`, fixture on, `--fresh-starts 2`,
`--count 20`, `--interval 0.05`, `--reply-timeout 8`.
Early `TIMEOUT last=none` usually means loopback not live yet — confirm cable before counting Fail.

## Verification

**Commands:**
- Rebuild Debug Bridge -- expected: success
- Framer unit + FramerSysex smoke -- expected: pass; oversize still observable above 16384
- `python scripts/lab/sysex-long-loopback.py --with-bridge --pass-percent 100` -- expected: exit 0, `overall_pass=true`
- `python scripts/quality/lint-touched.py` -- expected: clean
