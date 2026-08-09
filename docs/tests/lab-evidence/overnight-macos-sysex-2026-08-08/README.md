# Evidence — macOS overnight SysEx (2026-08-08/09)

**Machine :** MacBook Pro M5, macOS Tahoe  
**Stack :** Emagic MT4, **Apple driver** (no Bridge, no WinUSB, no VirtualMIDI)  
**Run :** `scripts/lab/overnight-macos-sysex-stress.py --hours 8` under `caffeinate -dims`  
**Summary log :** [`overnight-20260808T214135Z.log`](overnight-20260808T214135Z.log)

This capsule is the **hardware control** for the Windows Bridge overnight holes: same mid + bank Matrix stress, plus long SysEx on a separate DIN loop, for ~8 h. Bulk per-cycle trees stay under local `tests/lab-logs/overnight-macos/` (gitignored).

---

## Topology

| Jacks | Cable | Lab ports | Suites |
|---|---|---|---|
| Out 1 ↔ In 1 | Matrix-1000 | `MT4 Port 1` | Mid + bank |
| Out 2 → In 2 | Red DIN loop | `MT4 Port 2` | Long SysEx (1024 / 4096 / fixture ~14 708 B) |

Preflight smoke (mid ×5, bank ×20, long ×5) was green before the 8 h window.

---

## Headline results

| Suite | OK | FAIL |
|---|---:|---:|
| Mid (patch/master) | 239 | **0** |
| Bank (100× dump patch) | 238 | **0** |
| Long (DIN loop Port 2) | 238 | **0** |

```text
DONE cycles=239 mid_ok=239 mid_fail=0 bank_ok=238 bank_fail=0 long_ok=238 long_fail=0 stopped=False
```

Window: `2026-08-08T21:41:35Z` → `2026-08-09T05:42:03Z` (~8 h).  
One extra mid vs bank/long: cycle 239 hit the deadline after mid only — expected for a wall-clock stop.

**Verdict :** MT4 + Matrix (short SysEx) + DIN long loopback are **100 % reliable** under the Apple driver for this overnight. Windows Bridge TIMEOUT / missing-dump failures are **not** explained by this hardware/cabling on macOS.

---

## Related

- Short paliers (same Mac, same day-before): `docs/tests/lab-reports/macos-sysex-paliers-2026-08-07.md`
- Windows overnight contrast: [`../overnight-matrix-windows-2026-08-07/`](../overnight-matrix-windows-2026-08-07/)
- Harness: `scripts/lab/overnight-macos-sysex-stress.py` (`deaf2ff`)
