# Evidence — Windows overnight combined hybrid (2026-08-11/12)

**Machine :** Windows Boot Camp lab PC  
**Stack :** Bridge debug (`a896f9a`) + WinUSB + virtualMIDI  
**Topology :** Matrix-1000 on **MT4 Out 1 / In 1** + red DIN loopback **Out 2 → In 2**  
**Run :** `scripts/lab/overnight-combined-stress.py --hours 8`  
**Window :** `2026-08-11T21:53:05Z` → `2026-08-12T05:53:51Z` (UTC), ~8 h, not interrupted  
**Summary log :** [`overnight-20260811T215305Z.log`](overnight-20260811T215305Z.log)

This capsule is the **durable memory** that the post-truncation Bridge (`a896f9a` — bulk IN re-harvest / ring 63 + sticky F5 mid-SysEx preserve) survives an overnight **hybrid** soak at 100 %. Bulk per-cycle trees remain under local `tests/lab-logs/overnight-combined/` (gitignored).

Contrast: the same hybrid preflight on the morning of 2026-08-11 (pre-fix) failed long SysEx with `missing_bytes=32` (`overnight-20260811T093346Z` under local lab-logs).

---

## Headline results

| Suite | OK | FAIL |
|---|---:|---:|
| Mid (Matrix In1/Out1) | 227 | **0** |
| Bank (Matrix In1/Out1, 100 dumps) | 227 | **0** |
| Long SysEx loopback (Out2→In2) | 227 | **0** |
| Cycles completed | 227 | stopped cleanly at end of window |

Journal line:

```text
DONE cycles=227 mid_ok=227 mid_fail=0 bank_ok=227 bank_fail=0 long_ok=227 long_fail=0 stopped=False
```

Child-log scan for this night’s stamps: no `TIMEOUT` / `missing_bytes`, no `overall_pass: false`, no `gap_armed_starved`, no non-zero `bridge_fail`.

---

## Representative samples (all Pass)

| Sample | Why kept |
|---|---|
| [`samples/cycle-0001-mid/`](samples/cycle-0001-mid/) | First mid of the night — green start of hybrid soak |
| [`samples/cycle-0001-bank/`](samples/cycle-0001-bank/) | First bank of the night — Matrix burst under same Bridge session pattern |
| [`samples/cycle-0001-long/`](samples/cycle-0001-long/) | First long loopback Out2→In2 after mid+bank in the same cycle |
| [`samples/cycle-0113-long/`](samples/cycle-0113-long/) | Mid-soak long (~4 h in) — still byte-identical |
| [`samples/cycle-0227-long/`](samples/cycle-0227-long/) | Last long of the window — still 100 % |

Each sample folder has the suite log and the matching Bridge start log (night stamps only; morning leftover logs from the same cycle folder names were not copied).

---

## Fix lineage (remembered here)

Landed on `main` before this soak:

- `a896f9a` — Fix intermittent long SysEx truncation on DIN Out2→In2 loopback (re-harvest during deliver, `kBulkInAsyncSlotCount=63`, sticky F5 must not steal product `0x01`/`0x02` mid-SysEx across URBs)
- Spec / gate: `_bmad-output/implementation-artifacts/spec-sysex-long-truncation-32.md` (solo preflight `overall_pass: true` at `20260811T213557Z`)

---

## Related docs

- Prior Windows Matrix overnight (failures, pre-fix): [`../overnight-matrix-windows-2026-08-07/`](../overnight-matrix-windows-2026-08-07/)  
- macOS overnight control (hardware clean): [`../overnight-macos-sysex-2026-08-08/`](../overnight-macos-sysex-2026-08-08/)  
- Lab prompt long loopback: `docs/tests/lab-prompts/lab-palier-3-sysex-long-loopback.md`
