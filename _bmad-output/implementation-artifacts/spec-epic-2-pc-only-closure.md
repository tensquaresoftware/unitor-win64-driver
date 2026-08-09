---
title: 'Epic 2 PC-only closure (clock lab + Matrix script day-gate)'
type: 'chore'
created: '2026-08-10'
status: 'done'
baseline_commit: '569a2a2'
review_loop_iteration: 0
context:
  - '{project-root}/_bmad-output/implementation-artifacts/epic-2-context.md'
  - '{project-root}/_bmad-output/implementation-artifacts/2-1-midi-clock-and-transport-realtime.md'
  - '{project-root}/_bmad-output/implementation-artifacts/2-4-matrix-control-minimum-sysex-pass-vectors.md'
  - '{project-root}/scripts/lab/mtc-loopback-lab.py'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** Epic 2 is still open because story 2.1 lacks a Win10 hardware clock/transport Pass and story 2.4 still waits on Matrix-Control GUI rows, while Scarlett/DAW gear is unavailable tonight. The same PC-only DIN-loopback barème already accepted for story 2.2 (Python harness Pass) should close the remaining epic work honestly.

**Approach:** Add a MIDI clock + transport loopback lab mirrored from the MTC harness; get a real Pass on Out2→In2; re-confirm Matrix mid/bank day-gate (or cite still-valid recent green stamps); mark 2.1 + 2.4 + `epic-2` done; defer Scarlett/DAW clock UAT and Matrix-Control GUI UAT.

## Boundaries & Constraints

**Always:**
- Prefer zero C++ tonight; lab Python + docs + story/sprint status only unless a tiny Bridge defect is proven by the clock lab Fail.
- Clock Pass = Python DIN loopback on virtual MT4 ports with physical Out2→In2 (same topology class as MTC lab). Blank or synthetic-only ≠ Pass.
- Matrix 2.4 close tonight uses script mid/bank day-gate Pass (same honesty barème as 2.2 harness closeout), not Matrix-Control GUI.
- Update smoke docs / English checklist / deferred-work to state PC-only harness closure and what remains deferred.
- Commit only if Guillaume explicitly asks.

**Ask First:**
- Clock lab fails without an obvious small Bridge fix → HALT with evidence; do not invent Pass.
- Closing 2.4 would require claiming Matrix-Control GUI Pass without GUI → do not; keep GUI deferred and close only on script barème as frozen here.
- Any C++ change beyond a tiny proven Bridge hole → HALT before coding.

**Never:**
- Touch concurrent IN demux hardening.
- Start Epic 3 depth work.
- Launch the 8 h overnight soak (Guillaume runs it later).
- Depend on Scarlett / Ableton / Matrix-Control GUI for tonight’s Pass.
- Fake green status or widen into soak / demux / Epic 3.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Clock happy path | Bridge up, Out2→In2 DIN, free ports; send F8 stream + FA/FB/FC | Exit 0; journal Pass (≥98% clocks received; Start/Continue/Stop each ≥1; no Bridge fail needles) | Exit 2 + English fail reason in log |
| Ports busy / missing | DAW or Matrix-Control holds MT4 ports | Lab fails fast listing ports / open error | Do not claim Pass; operator frees ports |
| Bridge pump defect | WriteBulk / SendToHost / pump fail needles in Bridge log | Lab Fail even if some MIDI echoed | Diagnose minimally; fix only if tiny + obvious |
| Matrix day-gate green | Mid 5×10 + bank 20×100 @ 100% (or cite still-valid post-midburst stamps) | Document stamps; close 2.4 on script barème | If red, HALT 2.4/epic close; do not invent Pass |
| Epic status | 2.1 and 2.4 both Pass under tonight’s barème | `epic-2: done` in sprint-status | Leave epic in-progress if either story not closed |

</frozen-after-approval>

## Code Map

- `scripts/lab/mtc-loopback-lab.py` -- template: `--with-bridge`, port fuzzy match, BridgeSession, fail needles, lab-logs stamp
- `scripts/lab/midi-clock-loopback-lab.py` -- **create**: F8/FA/FB/FC DIN loopback Pass harness
- `scripts/lab/sysex-matrix-mid-loop.py` -- mid day-gate re-proof / cite
- `scripts/lab/sysex-matrix-bank-loop.py` -- bank day-gate re-proof / cite
- `scripts/lab/overnight-matrix-stress.py` -- document 8 h command only; do not run
- `.gitignore` -- add `tests/lab-logs/midi-clock-loopback/`
- `docs/tests/smoke-epic2-mt4.md` -- §3 clock: record PC-only harness Pass + topo
- `docs/tests/checklists/smoke-epic2-clock-mt4.md` -- English checklist result + harness note
- `_bmad-output/implementation-artifacts/2-1-midi-clock-and-transport-realtime.md` -- status done + lab bilan
- `_bmad-output/implementation-artifacts/2-4-matrix-control-minimum-sysex-pass-vectors.md` -- status done on script barème; GUI deferred
- `_bmad-output/implementation-artifacts/sprint-status.yaml` -- 2-1, 2-4, epic-2 → done
- `_bmad-output/implementation-artifacts/deferred-work.md` -- Scarlett/DAW clock UAT; Matrix-Control GUI UAT; leave demux alone

## Tasks & Acceptance

**Execution:**
- [x] `scripts/lab/midi-clock-loopback-lab.py` -- create mirror of MTC lab for Timing Clock `0xF8` + Start `0xFA` + Continue `0xFB` + Stop `0xFC`; CLI ports/duration/count/`--with-bridge`/`--bridge-exe`/`--log-dir`/`--list-ports`; Pass thresholds in I/O matrix; log under `tests/lab-logs/midi-clock-loopback/` -- hardware closeout for 2.1 without Scarlett
- [x] `.gitignore` -- ignore `tests/lab-logs/midi-clock-loopback/` -- match other bulky labs
- [x] Lab run clock -- free MT4 ports; run new harness with Bridge debug; obtain real Pass; stamp log path in story/docs -- blank ≠ Pass — stamp `20260809T221926Z`
- [x] Lab Matrix day-gate -- if recent post-midburst mid/bank stamps still valid for current Bridge, document them; else re-run short mid `5×10` + bank `20×100` @ 100% and stamp -- script barème for 2.4 — mid `20260809T220849Z` + bank `20260809T221054Z`
- [x] `2-1-midi-clock-and-transport-realtime.md` + clock smoke/checklist -- mark done; record harness Pass (not Scarlett); note DAW UAT deferred -- honest closeout
- [x] `2-4-matrix-control-minimum-sysex-pass-vectors.md` + Matrix smoke notes as needed -- mark done on script day-gate; explicitly defer Matrix-Control GUI UAT -- same barème as 2.2
- [x] `sprint-status.yaml` -- set `2-1-…: done`, `2-4-…: done`, `epic-2: done` only if both Pass -- epic closure
- [x] `deferred-work.md` -- append Scarlett/DAW clock UAT and Matrix-Control GUI UAT if not already clear; do not reopen demux -- keep tonight’s fences
- [x] Session end note -- short French summary: closed vs deferred + exact 8 h soak command without launching it -- operator handoff (step-04 / final)

**Acceptance Criteria:**
- Given Bridge Debug and DIN Out2→In2 free, when `midi-clock-loopback-lab.py --with-bridge` runs, then exit 0 with Pass counts (≥98% F8; FA/FB/FC each ≥1; no Bridge fail needles) and a journal under `tests/lab-logs/midi-clock-loopback/`.
- Given Matrix mid/bank script proof (cited recent green stamps or fresh day-gate Pass), when story 2.4 is closed, then status is `done` with GUI Matrix-Control UAT explicitly deferred — not claimed Pass.
- Given both 2.1 and 2.4 Pass under tonight’s barème, when sprint-status is updated, then `epic-2: done`.
- Given session complete, when summarizing, then the 8 h soak command is quoted and was not run; demux concurrent and Epic 3 were not touched.

## Spec Change Log

## Design Notes

Tonight reuses the **story 2.2 honesty barème**: Python DIN-loopback / librarian script Pass closes the story for Bridge carry; Scarlett/DAW and Matrix-Control GUI remain real UAT deferred work, not silent green.

Clock lab defaults should match MTC topology (Out2/In2) so Matrix can stay on Out1/In1. Prefer optional short note-on sanity before scoring clocks (same spirit as MTC lab) without making notes part of Pass thresholds.

Canonical short Matrix day-gate if re-proof needed:

```text
python scripts/lab/sysex-matrix-mid-loop.py --with-bridge --out-port "MT4 Out 1" --in-port "MT4 In 1" --pass-percent 100 --count 10 --fresh-starts 5 --bridge-exe builds\debug\Debug\Bridge.exe --log-dir tests\lab-logs\sysex-matrix-mid\day-gate

python scripts/lab/sysex-matrix-bank-loop.py --with-bridge --out-port "MT4 Out 1" --in-port "MT4 In 1" --pass-percent 100 --count 100 --fresh-starts 20 --bridge-exe builds\debug\Debug\Bridge.exe --log-dir tests\lab-logs\sysex-matrix-bank\day-gate
```

Soak (Guillaume later, not this session):

```text
python scripts/lab/overnight-matrix-stress.py --hours 8
```

## Verification

**Commands:**
- `python scripts/lab/midi-clock-loopback-lab.py --with-bridge` -- expected: exit 0, journal Pass
- Matrix day-gate commands above (if re-run) -- expected: exit 0, `overall_pass=true`
- `python scripts/quality/lint-touched.py` -- expected: exit 0 **only if** any C++ was touched; ideally skipped (zero C++)

**Manual checks (if no CLI):**
- Story files 2.1 / 2.4 show `done` with lab stamps and deferred GUI/Scarlett honesty
- `sprint-status.yaml` shows `2-1…`, `2-4…`, `epic-2` all `done`
- No demux / Epic 3 / soak execution in the change set

## Suggested Review Order

**Clock harness**

- Entry: F8/FA/FB/FC DIN loopback Pass thresholds (stop≥2, ≥98% clocks)
  [`midi-clock-loopback-lab.py:291`](../../scripts/lab/midi-clock-loopback-lab.py#L291)

- Scoring + Bridge fail needles at exit
  [`midi-clock-loopback-lab.py:385`](../../scripts/lab/midi-clock-loopback-lab.py#L385)

**Story / epic closeout**

- Story 2.1 → done on harness stamp
  [`2-1-midi-clock-and-transport-realtime.md:7`](./2-1-midi-clock-and-transport-realtime.md#L7)

- Story 2.4 → done on mid/bank scripts; GUI vectors 5–6 left open
  [`2-4-matrix-control-minimum-sysex-pass-vectors.md:7`](./2-4-matrix-control-minimum-sysex-pass-vectors.md#L7)

- `epic-2: done` only after both Pass
  [`sprint-status.yaml:72`](./sprint-status.yaml#L72)

**Operator honesty**

- FR smoke §3.4 harness closeout note
  [`smoke-epic2-mt4.md:373`](../../docs/tests/smoke-epic2-mt4.md#L373)

- English checklist filled with harness Pass (not blank green)
  [`smoke-epic2-clock-mt4.md:82`](../../docs/tests/checklists/smoke-epic2-clock-mt4.md#L82)

- Scarlett clock UAT + Matrix GUI UAT deferred
  [`deferred-work.md:214`](./deferred-work.md#L214)
