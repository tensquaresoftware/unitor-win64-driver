# Evidence — Windows overnight Matrix (2026-08-07/08)

**Machine :** Windows Boot Camp lab PC  
**Stack :** Bridge debug + WinUSB + virtualMIDI, ports **MT4 Out 1 / MT4 In 1**  
**Run :** `overnight-matrix-stress.py` ~8 h (`hours=8`, mid×10 + bank×100 per cycle)  
**Summary log :** [`overnight-20260807T222620Z.log`](overnight-20260807T222620Z.log)

This capsule is the **durable memory** of the overnight symptom that triggered the Epic 1 cross-cutting code review. Bulk per-cycle trees remain under local `tests/lab-logs/overnight-matrix/` (gitignored).

---

## Headline results

| Suite | OK | FAIL | Approx. success |
|---|---:|---:|---:|
| Mid (patch/master calm) | 453 | 21 | ~95.6 % |
| Bank (100 dumps burst) | 398 | 75 | ~84.0 % |
| Cycles completed | 474 | — | stopped cleanly at end of window |

Typical failure shape in samples below:

- Lab: `TIMEOUT … last=none` (no SysEx reply frame)
- `bridge_fail_start1: count=0` (Bridge process stayed up)
- Often **99/100** on bank (single missing dump), not mid-message truncation as the primary printout

---

## Representative samples

| Sample | Why kept |
|---|---|
| [`samples/cycle-0011-bank/`](samples/cycle-0011-bank/) | Early bank fail: `TIMEOUT dump_patch` @ 43/100, **99 %**, Bridge fail count 0 |
| [`samples/cycle-0016-bank/`](samples/cycle-0016-bank/) | Second bank exemplar: same shape (`TIMEOUT` @ 12/100, **99 %**) |
| [`samples/cycle-0051-mid/`](samples/cycle-0051-mid/) | Mid fail on **dump_master** (351 B): 9/10, Bridge fail count 0 |
| [`samples/cycle-0061-mid/`](samples/cycle-0061-mid/) | Mid fail on **dump_patch** (275 B): 9/10, Bridge fail count 0 |

Each sample folder has the suite log (`sysex-matrix-*.log`) and the matching Bridge start log.

---

## Follow-up (review → fixes)

Cross-cutting review focused on joints WinUSB → DeviceSession → virtualMIDI (short Matrix SysEx first).

Landed on `main` (2026-08-08), among others:

- `0c85e20` — sticky Emagic F5 must not steal mid-SysEx data as a port index  
- `f9651a2` — fail closed on short Matrix dumps; USB pad must not delay 274/350 idle finalize; cleaner Stop  
- `7e197bd` — WinUSB async stop/sync hardening  
- `b75345f` — log virtualMIDI host→device drops while sink unset  

**Next lab check :** re-run bank (and optionally a short overnight) and compare against this capsule — expect either higher OK rate or **visible Bridge pump failures** instead of mute TIMEOUT / `bridge_fail=0`.

---

## Related docs

- Prompt / mission: `docs/dev/revue-code-transverse-epic-1-mt4-midi.md`  
- macOS control (hardware clean): `docs/tests/lab-reports/macos-sysex-paliers-2026-08-07.md`
