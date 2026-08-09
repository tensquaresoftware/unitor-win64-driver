---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 3.2 — Hot-plug recovery without Windows reboot (MT4)
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
---

# Smoke — Epic 3.2 Hot-plug recovery (MT4 under Windows)

Operator-facing Pass/Fail guide for **Story 3.2**: after a live session with Virtual Ports, unplug then replug the MT4 and regain usable ports **without rebooting Windows**.

**Honesty bar:** blank lab rows ≠ Pass. Mid-dump unplug during SysEx may need a host MIDI rescan or a supervised Bridge restart (PRD UJ-2 edge) — document what happened; do **not** claim Matrix-Control GUI UAT closed here. Requiring a Windows reboot to regain ports is a **Fail**.

## V1 hot-plug recovery contract

| Topic | Contract |
|---|---|
| Product host | `Bridge.exe --auto-session` (user-session process only; **not** a Windows Service — AD-20) |
| Detect loss | Pump failure / unexpected `!IsRunning()` while cancel is **not** requested |
| Teardown | `DeviceSession::Stop()` destroys Virtual Ports via `MidiBackend` (AD-9); English console: `MT4 disconnected; waiting for replug...` |
| Wait / rescan | After disconnect: poll until WinUSB GUID is **Absent** (clears stale Present), then until **Present** again — every **2 s**, progress every **30 s**, fail closed after **900 s** with English diagnostics — do not hang forever silently |
| Recreate | **New** `DeviceSession::Start` under AD-6 identity (V1 single unit: `MT4 Port N`); App must **not** call `CreatePortSet` / `DestroyPortSet` |
| AQ-2 (UX preference) | V1 default = **silent in-process recreate** with English console diagnostics; no tray/GUI acknowledge dialog required |
| Host visibility | Ableton / Reason / ShowMIDI may need a MIDI device **rescan**; supervised Bridge process restart is an **allowed** escape (AD-10), not the only path |
| Lab one-shot | `Bridge.exe --start-session` / `--run-midi` still **exit** on mid-session USB loss so existing lab spawners keep working |
| Escape hatch | Documented supervised Bridge restart OK if in-process recreate is insufficient; reboot required = **V1 failure** |
| Clean stop | Prefer **Ctrl+C** (not the console close button). Known deferred risk: `CTRL_CLOSE` may leave orphan ports (`deferred-work.md`) |

### Explicit fences (later stories)

| Concern | Story |
|---|---|
| Multi-client DAW + ShowMIDI policy | **3.3** |
| Dual-MT4 ordinal naming / persistence | **3.4** |
| Polished `docs/user/` hot-plug chapter | **4.2** |
| Public Installer | **4.1** |
| MIDI Path latency harness | Epic **5** |

### SSOT citations

- Epics Story 3.2
- PRD FR-11 / NFR-R2 / SM-4 / UJ-4
- Architecture AD-9, AD-10, AD-20
- SPEC CAP-11
- Deferred AQ-2 (UX preference only; lifecycle ownership stays AD-9)

## Prerequisites

- Story **3.1** Auto-Start path works on this Windows session (`docs/tests/smoke-epic3-autostart-mt4.md`)
- Epic 1–2 Bridge functionality already working (notes/CC + WinUSB bind)
- VirtualMIDI installed
- Built `Bridge.exe` under `builds/` (e.g. `builds/debug`)
- A Validation Matrix host open during the drill: ShowMIDI and/or a DAW that lists the Virtual Ports

## How to mark results

- **Pass** / **Fail** / **N/A** (+ short reason)
- Blank cell = not run (does **not** count as Pass)
- Win10 x64 is **mandatory** for closing the lab claim; Win11 x64 when available

## Pass/Fail matrix

| # | Check | Win10 x64 | Win11 x64 | Notes |
|---|---|---|---|---|
| 1 | Live `--auto-session` + ShowMIDI and/or Validation Matrix DAW open → Virtual Ports usable (`MT4 Port N`) | | | |
| 2 | Unplug MT4 → ports tear down (no orphan names on happy path / Ctrl+C-class stop); Bridge **process stays alive** for `--auto-session` (console shows wait-for-replug). If you used supervised Bridge restart instead, record that path in Notes | | | |
| 3 | Replug MT4 → usable Virtual Ports return **without Windows reboot** | | | |
| 4 | Host MIDI rescan (or documented supervised Bridge restart) restores host visibility if needed | | | |
| 5 | Recovery used a **new** session recreate under AD-5/AD-6 names (`MT4 Port N` for single unit) — console shows started banner again after replug | | | |
| 6 | **Negative:** requiring Windows reboot to regain ports = **Fail** | | | |

## Operator commands (reference)

```text
builds\debug\Bridge.exe --auto-session
builds\debug\Bridge.exe --start-session
builds\debug\Bridge.exe --run-midi
builds\debug\Bridge.exe --test-mapper
builds\debug\Bridge.exe --test-port-names
```

**Product drill:** start with `--auto-session`, open ShowMIDI/DAW, unplug, wait for English wait banner, replug, confirm ports return, rescan host if needed.

**Lab one-shot:** `--start-session` / `--run-midi` exit on unplug (scripts that expect process exit). Do not use those flags alone to claim FR-11 recovery.

**Supervised restart escape (AD-10):** if in-process recreate fails to restore host visibility, stop with Ctrl+C, relaunch `Bridge.exe --auto-session`, then rescan the host. Still **no Windows reboot**.

## Out of scope for this smoke

- Multi-client exclusive-open policy → **3.3**
- Dual-MT4 hot-plug naming stability → **3.4**
- Public Installer / polished end-user docs → **4.x**
- Claiming Matrix-Control GUI UAT closed
- Epic 2 ~4 h longevity (supervised restart is OK for hot-plug, **not** an excuse for longevity Fail)

## Related docs

- Auto-Start (3.1): [`docs/tests/smoke-epic3-autostart-mt4.md`](smoke-epic3-autostart-mt4.md)
- Longevity ownership: [`docs/tests/checklists/smoke-epic2-longevity-mt4.md`](checklists/smoke-epic2-longevity-mt4.md)
- Epic 2 transport smoke: [`docs/tests/smoke-epic2-mt4.md`](smoke-epic2-mt4.md)
- WinUSB bind (one-time admin): [`docs/dev/winusb-bind.md`](../dev/winusb-bind.md)
