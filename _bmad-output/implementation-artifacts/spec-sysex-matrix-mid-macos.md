---
title: 'MT4 Apple driver — mid-size Matrix SysEx lab (macOS hardware control)'
type: 'chore'
created: '2026-08-07'
status: 'done'
baseline_commit: 'ad2ca25b539f92e6d4b0795d33faeb6d6374261e'
review_loop_iteration: 0
context:
  - '{project-root}/conventions.md'
  - '{project-root}/_bmad/custom/clarity-bar-fr.md'
  - '{project-root}/_bmad-output/implementation-artifacts/spec-sysex-matrix-mid-roundtrip.md'
  - '{project-root}/docs/tests/checklists/smoke-mt4-sysex-matrix-mid.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** Windows Bridge still often loses the first large Matrix SysEx dump after a fresh Start; we need a macOS control run through the official Apple MT4 driver to tell hardware/Matrix/cable from the Windows stack.

**Approach:** Reuse the existing mid-size SysEx Python harness without Bridge, discover real Apple MIDI port names on this Mac, run the same four scenarios (≥10 each, 100 % bar) across ≥2 fresh Mac sessions, and log a clear hardware clean / not-clean verdict.

## Boundaries & Constraints

**Always:**
- Single goal: macOS Apple-driver lab evidence for editor-sized Matrix SysEx (patch 275 B / master 351 B), both directions.
- Reuse `scripts/lab/sysex-matrix-mid-loop.py`; deps via `scripts/lab/requirements-device-inquiry.txt` (mido + python-rtmidi).
- Never use `--with-bridge`, Bridge.exe, WinUSB, or VirtualMIDI in this lab.
- Fixtures: `tests/fixtures/sysex/Patch.syx` (275 B, `F0 10 06 01`…`F7`), `Master.syx` (351 B, `F0 10 06 03`…`F7`).
- Dump requests: patch `F0 10 06 04 01 00 F7`; master `F0 10 06 04 03 00 F7`.
- Device→host Pass = exact length + locked prefix + trailing `F7`; reassemble until `F7` (never Pass on a fragment).
- Host→device Pass = full fixture send with no MIDI API error (no Bridge log scan on macOS).
- Discover ports with `--list-ports` (or equivalent); use listed Apple/Emagic/MT4 names equivalent to Out1/In1 — never invent names.
- ≥10 trials per critical scenario; `--pass-percent 100`; dump reply timeout ~3 s; interval ~1 s.
- ≥2 fresh Mac sessions (USB re-plug and/or close/reopen ports / restart Python after a delay) stressing first dump after cold start; no opaque Device Inquiry warm-up.
- Logs under `tests/lab-logs/sysex-matrix-mid-macos/` + short README (EN or FR, kebab-case); document chosen port names in log/README.
- Diffs minimal; commits only if Guillaume asks; no remote push unless asked.
- Chat verdict in clear French: hardware clean yes / no / mitigated + per-scenario rates + first-dump-after-fresh behavior + log paths.

**Ask First:**
- Declaring Matrix/cable/power root cause when Apple ports are missing or DIN LEDs show no activity.
- Lowering the 100 % bar or skipping a fresh-session dump pair.
- Changing Bridge C++ or expanding into AMT8 / Unitor8 / Windows MIDI Services / Matrix-Control UI as gate.

**Never:**
- Fix or retune the Windows Bridge in this chat.
- All-bank dump, synthetic very-long SysEx, Matrix-Control UI gate.
- Fake echo app or warm-up Inquiry to hide first-shot loss.
- French in source code; PascalCase folders; inventing port names.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| List ports | `--list-ports` on Mac with Apple MT4 | Print real OUT/IN names | Halt if no Emagic/MT4 Port 1 pair |
| push_patch | Send `Patch.syx` on Apple Out1 equiv. | Send OK (API) | Fail trial; keep rates |
| push_master | Send `Master.syx` | Send OK | Same |
| dump_patch | Dump request → wait ≤3 s | Exactly 275 B, prefix `F0 10 06 01`, ends `F7` | TIMEOUT / wrong size → Fail; log len, head, tail, dt_ms |
| dump_master | Dump request | Exactly 351 B, prefix `F0 10 06 03`, ends `F7` | Same |
| Fresh session 1+2 | ≥2 cold Mac sessions; dumps each | First dump after reopen measured; overall 100 % or clear Fail | No Inquiry warm-up |
| Fragmented recv | rtmidi may split SysEx | Buffer until `F7` then validate | Never Pass on open buffer |

</frozen-after-approval>

## Code Map

- `scripts/lab/sysex-matrix-mid-loop.py` -- existing push/dump harness; MIDI-only path (no `--with-bridge`); extend minimally for macOS log dir / fresh sessions / `overall_pass`
- `scripts/lab/requirements-device-inquiry.txt` -- mido / python-rtmidi
- `tests/fixtures/sysex/Patch.syx` / `Master.syx` -- locked fixtures
- `tests/lab-logs/sysex-matrix-mid/README.md` -- Windows Bridge log pattern to mirror for macOS
- `docs/tests/checklists/smoke-mt4-sysex-matrix-mid.md` -- Windows checklist (read-only reference)
- `_bmad-output/implementation-artifacts/spec-sysex-matrix-mid-roundtrip.md` -- Windows lab SSOT (do not re-prove here)

## Tasks & Acceptance

**Execution:**
- [x] `scripts/lab/sysex-matrix-mid-loop.py` -- minimal macOS-friendly tweaks (`--log-dir` and/or default docs; MIDI-only `overall_pass`; optional fresh-session loop without Bridge) -- keep Windows `--with-bridge` path intact
- [x] `tests/lab-logs/sysex-matrix-mid-macos/README.md` -- short how-to: deps, `--list-ports`, example Apple port flags, fresh-session procedure, Pass bar -- operator path on Mac
- [x] Lab run on this Mac -- list ports, choose Out1/In1 names, install deps, run pushes+dumps ≥10×, ≥2 fresh sessions, capture logs -- produce hardware verdict
- [x] Chat deliverable -- French clarity-bar summary: verdict, rates, first-dump behavior, log paths -- decision input for Windows vs hardware

**Acceptance Criteria:**
- Given Apple MT4 ports visible and Matrix on DIN Out1↔In1, when `--list-ports` runs, then the log/README records the exact OUT/IN names used (no invented names).
- Given those ports, when the harness runs push_patch, push_master, dump_patch, and dump_master (≥10 each) with `--pass-percent 100` and no `--with-bridge`, then exit 0 only if every scenario meets 100 %, with `overall_pass=true` (or equivalent) in the journal.
- Given ≥2 fresh Mac sessions, when dump_patch/dump_master run without Inquiry warm-up, then the first dump after each cold open is recorded Pass or Fail with len/head/tail/dt_ms on Fail.
- Given any single TIMEOUT or wrong-size dump, when the run finishes, then overall Fail and the chat states hardware clean as no or mitigated — not yes.
- Given this work completes, when reviewing scope, then Bridge C++ and AMT8/Unitor8/Windows MIDI Services/Matrix-Control UI remain untouched as gates.

## Spec Change Log

## Design Notes

Ports used (Apple driver): OUT/IN `MT4 Port 1`.

Lab stamp `20260807T165511Z` — `overall_pass=true`, exit 0.
Session 1 first dump_patch: Pass, 275 B, dt_ms≈128 (no TIMEOUT).
Session 2 first dump_patch (fresh process): Pass, 275 B, dt_ms≈127.

```text
python3 scripts/lab/sysex-matrix-mid-loop.py \
  --out-port "MT4 Port 1" --in-port "MT4 Port 1" \
  --pass-percent 100 --count 10 --reply-timeout 3 --interval 1 \
  --fresh-sessions 2 --session-gap 2 \
  --log-dir tests/lab-logs/sysex-matrix-mid-macos
```

Do not pass `--with-bridge` on macOS.

## Verification

**Commands:**
- `python3 scripts/lab/sysex-matrix-mid-loop.py --list-ports` -- expected: Apple/Emagic/MT4 Port 1 names printed
- Full lab command above (twice across fresh sessions) -- expected: exit 0 and `overall_pass=true` for hardware-clean yes; else Fail with diagnostics
- `git diff -- scripts/lab/sysex-matrix-mid-loop.py` -- expected: minimal, Windows `--with-bridge` unchanged in behavior

**Manual checks (if no CLI):**
- Audio MIDI Setup / port list shows MT4 before run; Matrix LEDs blink on dump traffic; no DAW holding ports.

## Suggested Review Order

**MIDI-only fresh sessions (macOS)**

- Parent loop: new process per session, dumps-only after session 1
  [`sysex-matrix-mid-loop.py:815`](../../scripts/lab/sysex-matrix-mid-loop.py#L815)

- Flag guards: no pushes-only / append-log with multi-session
  [`sysex-matrix-mid-loop.py:710`](../../scripts/lab/sysex-matrix-mid-loop.py#L710)

- `--log-dir` resolution + relative `--log` under that dir
  [`sysex-matrix-mid-loop.py:85`](../../scripts/lab/sysex-matrix-mid-loop.py#L85)

**Operator docs + evidence**

- Apple ports, one-shot command, cold-open caveats
  [`README.md:37`](../../tests/lab-logs/sysex-matrix-mid-macos/README.md#L37)

- Lab journal with `overall_pass=true` (session 1+2)
  [`sysex-matrix-mid-20260807T165511Z.log:1`](../../tests/lab-logs/sysex-matrix-mid-macos/sysex-matrix-mid-20260807T165511Z.log#L1)
