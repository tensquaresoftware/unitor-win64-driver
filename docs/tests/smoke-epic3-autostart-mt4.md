---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 3.1 — Auto-Start without daily Administrator (MT4)
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
---

# Smoke — Epic 3.1 Auto-Start (MT4 under Windows)

Operator-facing Pass/Fail guide for **Story 3.1**: Bridge Auto-Start on a **user Windows session** so Virtual Ports appear after logon and/or first USB arrival without a manual Bridge launch and without daily Administrator elevation.

**Honesty bar:** blank lab rows ≠ Pass. This guide closes the **Auto-Start runtime** portion of SM-5 / FR-3 / CAP-3. Full install+docs packaging remains Epic **4**. Mid-session unplug/replug recovery is Story **3.2** — out of scope here.

## V1 Auto-Start contract

| Topic | Contract |
|---|---|
| Host process | Bridge is an **interactive user-session** process only |
| Forbidden | Session-0 Windows Service / SCM service project (AD-20) |
| Registration mechanism | Try per-user **Task Scheduler** task at logon first: `TASK_LOGON_INTERACTIVE_TOKEN` (“Run only when user is logged on”), **RunLevel Limited** (not highest privileges), action = absolute path to the registering `Bridge.exe` + `--auto-session` |
| V1 primary when Scheduler refuses | **HKCU Run is an acceptable V1 primary**, not a degraded niche: if Task Scheduler returns access denied (or other failure), Bridge registers `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` (same absolute exe + `--auto-session`). Stdout reports which mechanism was used. Either mechanism Passes register when non-admin + no UAC |
| Task name | `UnitorMt4BridgeAutoStart` |
| CLI | `Bridge.exe --register-auto-start` / `--unregister-auto-start` / `--auto-session` |
| Privilege bar | WinUSB INF bind / lab signing remain **one-time Administrator** (install). Daily register / run / unregister must work as the interactive user **without elevation** (NFR-D2) |
| Auto-session behaviour | If MT4 already present → start `DeviceSession` (Virtual Ports via existing session path). If absent → wait/rescan for WinUSB DeviceInterfaceGUID `{aa209017-cf8a-49ad-a0e7-701187ff7e05}` |
| Wait / rescan bound | Poll every **2 s**, progress log every **30 s**, fail closed after **900 s** (15 min) with English diagnostics — do not hang forever silently |
| VirtualMIDI missing | Fail closed with English fix path (empty port list ≠ success) |
| Clean stop | Prefer **Ctrl+C** (not the console close button). Known deferred risk: `CTRL_CLOSE` may leave orphan ports (`deferred-work.md`) |
| Lab path preserved | `Bridge.exe --start-session` / `--run-midi` (+ optional `--dev-zadig`) unchanged for contributor labs |

### Explicit fences (later stories)

| Concern | Story |
|---|---|
| Mid-session unplug/replug recovery | **3.2** |
| Multi-client DAW + ShowMIDI policy | **3.3** |
| Dual-MT4 ordinal naming | **3.4** |
| Public Installer UX packaging of Auto-Start | **4.1** |
| End-user docs polish | **4.2** |
| MIDI Path latency harness | Epic **5** |

### SSOT citations

- Epics Story 3.1
- PRD FR-3 / NFR-D2 / SM-5 (Auto-Start portion)
- Architecture AD-10, AD-12 items (6)(7), AD-20
- SPEC CAP-3

## Prerequisites

- Epic 1–2 Bridge functionality already working on this Windows session (notes/CC + WinUSB bind)
- VirtualMIDI installed for Pass rows that require ports; row 6 deliberately removes/disables it
- Built `Bridge.exe` under `builds/` (e.g. `builds/debug`)
- Run register/unregister/auto-session as a **standard** (non-admin) interactive user for the daily-ops claim

## How to mark results

- **Pass** / **Fail** / **N/A** (+ short reason)
- Blank cell = not run (does **not** count as Pass)
- Win10 x64 is **mandatory** for closing the lab claim; Win11 x64 when available

## Pass/Fail matrix

| # | Check | Win10 x64 | Win11 x64 | Notes |
|---|---|---|---|---|
| 1 | Register Auto-Start as non-admin: `Bridge.exe --register-auto-start` exits 0; English stdout shows **either** Task Scheduler task name **or** HKCU Run value name, plus absolute exe + `--auto-session`; **no UAC**. Run-only after Scheduler refusal is Pass | | | |
| 2 | Logoff/logon **or** reboot with MT4 already connected → Virtual Ports appear **without** manual `Bridge` launch | | | |
| 3 | After login with MT4 disconnected, plug MT4 → ports appear without manual launch (wait/rescan path within 900 s) | | | |
| 4 | Process is user-session (Task Manager → Details: `Bridge.exe` under the logged-on user; **no** Session-0 service) | | | |
| 5 | No UAC elevation prompt on daily Auto-Start launch | | | |
| 6 | With VirtualMIDI removed/disabled: fail closed + English fix path (no silent empty success) | | | |
| 7 | `Bridge.exe --unregister-auto-start` then logon → Bridge **does not** start | | | |

## Operator commands (reference)

```text
builds\debug\Bridge.exe --register-auto-start
builds\debug\Bridge.exe --unregister-auto-start
builds\debug\Bridge.exe --auto-session
builds\debug\Bridge.exe --start-session
builds\debug\Bridge.exe --test-mapper
builds\debug\Bridge.exe --test-port-names
```

## Out of scope for this smoke

- Claiming full SM-5 (installer + polished user docs) closed
- Hot-plug recovery after ports were already live → **3.2**
- Public Installer MSI wiring → **4.1**

## Related docs

- WinUSB bind (one-time admin): [`docs/dev/winusb-bind.md`](../dev/winusb-bind.md)
- Epic 2 transport smoke: [`docs/tests/smoke-epic2-mt4.md`](smoke-epic2-mt4.md)
