---
title: 'MT4 — long SysEx DIN loopback lab (palier 3, option A)'
type: 'chore'
created: '2026-08-07'
status: 'done'
baseline_commit: 'd6c8341'
review_loop_iteration: 0
context:
  - '{project-root}/conventions.md'
  - '{project-root}/_bmad-output/implementation-artifacts/spec-sysex-matrix-bank-burst.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** After mid-size and bank Matrix traffic are proven on Apple MT4, we still lack a hardware control that large SysEx frames survive a full MT4 USB↔DIN round-trip without depending on Matrix-1000.

**Approach:** DIN Out1→In1 physical loopback (no Matrix): send known long F0…F7 frames (synthetic 1024/4096 + fixture ~14708 B) and require exact byte match on IN at 100 % across ≥2 fresh sessions.

## Boundaries & Constraints

**Always:**
- Single goal: palier-3 long SysEx integrity via MT4 loopback (exact equality).
- Topology: physical DIN Out1 → In1; Matrix disconnected from those jacks.
- Payloads: synthetic sizes (default 1024, 4096) + `tests/fixtures/sysex/long-loopback-14708.syx` unless `--skip-fixture`.
- Pass = received frame identical to sent (length + every byte); reassemble until F7.
- ≥20 reps per payload; interval ≥50 ms; per-trial timeout ~5 s; `--pass-percent 100`.
- ≥2 fresh Mac sessions (process reopen); no `--with-bridge` on macOS.
- Logs under `tests/lab-logs/sysex-long-loopback-macos/`.

**Ask First:**
- Switching gate to Python ACK/dump responder (option B).
- Claiming Pass when Matrix is still in the DIN path.

**Never:**
- Matrix-Control UI as gate; Bridge C++ changes in this lab; AMT8 / Unitor8.
- Opaque warm-up; French in source.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| synth_1024 / synth_4096 | Send synthetic F0…F7 | Exact same frame on IN | TIMEOUT / mismatch → Fail |
| fixture_14708 | Send fixture | Exact same frame on IN | Same |
| Fresh session 2 | New process | First trial of each size still 100 % | No warm-up |

</frozen-after-approval>

## Code Map

- `scripts/lab/sysex-long-loopback.py` -- long loopback harness
- `tests/fixtures/sysex/long-loopback-14708.syx` -- ~14.7 KiB valid SysEx fixture
- `scripts/lab/sysex-matrix-mid-loop.py` / `sysex-matrix-bank-loop.py` -- patterns reused

## Tasks & Acceptance

**Execution:**
- [x] Fixture + harness + macOS README/checklist/spec
- [x] Lab run 2 fresh sessions on MT4 Port 1

**Acceptance Criteria:**
- Given DIN Out1→In1 loopback and Apple MT4 Port 1, when the harness sends each payload ≥20× at 100 % bar across ≥2 fresh sessions, then every reply equals the sent frame and `overall_pass=true`.

## Spec Change Log

## Design Notes

macOS Apple control (clean run after DIN loopback confirmed):
stamp under `tests/lab-logs/sysex-long-loopback-macos/` with `overall_pass=true`.
Both sessions: synth_1024 / synth_4096 / fixture_14708 = 20/20 each.
First run had early synth_1024 TIMEOUTs (`last=none`) before loopback was live; not counted as hardware fail once re-run clean.

```text
python3 scripts/lab/sysex-long-loopback.py \
  --out-port "MT4 Port 1" --in-port "MT4 Port 1" \
  --count 20 --interval 0.05 --reply-timeout 8 \
  --sizes 1024,4096 --pass-percent 100 --fresh-sessions 2 \
  --log-dir tests/lab-logs/sysex-long-loopback-macos
```

Rough round-trip on this Mac: ~0.35 s (1 KiB), ~1.3 s (4 KiB), ~4.7 s (14.7 KiB).

## Verification

**Commands:**
- Full macOS one-shot above -- expected: exit 0, `overall_pass=true`
