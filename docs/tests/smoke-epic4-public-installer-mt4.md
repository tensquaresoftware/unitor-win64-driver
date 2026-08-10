---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 4.1 — Public Installer (AD-12 UX bar)
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
---

# Smoke guide — Epic 4.1 Public Installer (MT4 on Windows)

Operator guide for **Story 4.1**: a short community Public Installer that binds WinUSB, installs the Bridge, wires Auto-Start, and checks virtualMIDI — so first MIDI the same evening does not feel like a developer toolchain.

**Honesty bar:** a blank cell is **not** Pass. Win10 x64 is **mandatory** to close the lab claim; Win11 x64 when available. Physical MT4 is required for bind rows. Polished end-user docs now live under [`docs/user/README.md`](../user/README.md); SM-5 still needs that story’s user-docs smoke Pass — [`smoke-epic4-user-docs-mt4.md`](smoke-epic4-user-docs-mt4.md). Authenticode / SmartScreen honesty ships under Story **4.4** — [`smoke-epic4-authenticode-smartscreen-mt4.md`](smoke-epic4-authenticode-smartscreen-mt4.md) (do **not** claim full SM-6 while OQ-1 or blank 4.1 hardware rows remain).

## Product intent

The community path should feel closer to a polished macOS installer than a developer kit. **Zadig is not** the primary user path (contributor fallback only — see [`docs/dev/winusb-bind.md`](../dev/winusb-bind.md)).

## Installer technology (locked under AD-12)

| Choice | Decision |
|---|---|
| Technology | **Inno Setup 6** (pin: **6.7.3**; build with `ISCC.exe`) |
| Rationale | Fast progress + clear success UI, straightforward custom `pnputil` / prerequisite checks, single community EXE; avoids centering on MSIX while INF / Driver Store association is required; WiX remains viable if an MSI-first ARP story is needed later — **not** reopened as a product decision here |
| Output | `builds/installer/UnitorMt4Bridge-Setup.exe` (via `scripts/packaging/build-public-installer.ps1`) |
| Sources | `installer/public-installer.iss` + helpers under `installer/` |

## Contract (AD-12 checklist)

| # | AD-12 item | Contract |
|---|---|---|
| 1 | Few steps | One elevated wizard; virtualMIDI gate → copy Bridge → WinUSB bind → unelevated Auto-Start register → success |
| 2 | Visible progress | Inno modern wizard progress during file copy and bind |
| 3 | Clear success screen | Success text **only** when virtualMIDI present **and** WinUSB association reported OK **and** Auto-Start registration reported OK |
| 4 | virtualMIDI prerequisite explicit | Block before success if `teVirtualMIDI.dll` missing from System32; English fix path (loopMIDI / rtpMIDI). Empty port list ≠ success |
| 5 | WinUSB association | Elevated `pnputil /add-driver … /install` on project INF; HWID `USB\VID_086A&PID_0003&MI_02`; GUID `{aa209017-cf8a-49ad-a0e7-701187ff7e05}` |
| 6 | Auto-Start wired | `"<install>\Bridge.exe" --register-auto-start` in **interactive user** context (`runasoriginaluser` / equivalent) — not the elevated admin profile |
| 7 | One-time admin | UAC once for Program Files + bind; daily Bridge / Auto-Start must not require admin (`asInvoker`) |
| 8 | Minimal jargon | Ten Square Software facade; short English UI strings |

### OQ-1 (release gate only)

Tobias virtualMIDI **MSI embed/redistribution** is a **release gate** for a redistributable installer that ships virtualMIDI. Eval prerequisite messaging (loopMIDI / rtpMIDI) ships **now**. Do not silently embed virtualMIDI MSI without clearance.

## Scope fences

| Topic | Owner |
|---|---|
| Polished end-user docs (first MIDI / SysEx / troubleshooting) | **4.2** — [`docs/user/README.md`](../user/README.md); close via [`smoke-epic4-user-docs-mt4.md`](smoke-epic4-user-docs-mt4.md) |
| Three-way MIT vs virtualMIDI vs Windows MIDI Services honesty | **4.3** — [`docs/dev/license-and-backends.md`](../dev/license-and-backends.md); close via [`smoke-epic4-license-honesty-mt4.md`](smoke-epic4-license-honesty-mt4.md) |
| Authenticode / public catalog signing / SmartScreen honesty | **4.4** — [`docs/dev/authenticode-and-smartscreen.md`](../dev/authenticode-and-smartscreen.md); close via [`smoke-epic4-authenticode-smartscreen-mt4.md`](smoke-epic4-authenticode-smartscreen-mt4.md) |
| Tobias MSI **embed** redistributable | **OQ-1** release gate only |
| WinUSB bind materials / GUID open policy | **1.3** (reuse; do not rewrite transport) |
| Auto-Start runtime CLI | **3.1** (wire; no second mechanism / no Session-0 service) |
| MIDI Path harness | Epic **5** |

## SSOT citations

- Epics Story 4.1
- PRD FR-12 / SM-5 (install portion)
- Architecture AD-12 / AD-20 / AD-19 (facade branding)
- SPEC CAP-12

## Prerequisites

- Clean Win10 x64 (mandatory) or Win11 x64 machine
- Physical MT4 for bind / Device Manager rows
- virtualMIDI present for success-path rows; deliberate uninstall/disable for the missing-driver negative row
- Built `Bridge.exe` under `builds/` (e.g. `builds/debug`)
- Inno Setup 6 installed to compile the setup EXE (Windows validation machine)

## How to score

- **Pass** / **Fail** / **N/A** (+ short reason)
- Blank = not run (**does not** count as Pass)
- Win10 x64 column mandatory to close the claim

## Pass / Fail matrix (AD-12 1:1 + extras)

| # | Verification | Win10 x64 | Win11 x64 | Notes |
|---|---|---|---|---|
| 1 | Few steps / minimal jargon walkthrough (AD-12-1, 8) | | | |
| 2 | Visible progress during install (AD-12-2) | | | |
| 3 | Clear success screen **only** when all gates passed (AD-12-3) | | | |
| 4 | virtualMIDI missing → block + English fix path; not success (AD-12-4 / AC2) | | | Deliberate remove/disable `teVirtualMIDI.dll` / driver |
| 5 | After install with virtualMIDI present: Device Manager shows WinUSB on MT4 / GUID path opens (AD-12-5) | | | Physical MT4 required |
| 6 | Auto-Start registered; logon or plug-after-login yields ports without manual launch (AD-12-6) | | | Reuse Epic 3 Auto-Start expectations — [`smoke-epic3-autostart-mt4.md`](smoke-epic3-autostart-mt4.md) |
| 7 | One UAC/admin at install; daily Bridge / register path does not prompt (AD-12-7) | | | |
| 8 | Uninstall unregisters Auto-Start; logon no longer starts Bridge | | | |
| 9 | Regression: lab `Bridge --start-session` / `--test-mapper` / `--test-port-names` still work from install dir or `builds/` as documented | | | |

## Build / run (reference)

```powershell
# From repo root on Windows, after Bridge is built (prefer a Release layout):
.\scripts\packaging\build-public-installer.ps1
# Optional:
#   -BridgeDir builds\release\Release
#   -AppVersion 0.1.0
# Auto-detect prefers Release over Debug; invalid -BridgeDir fails closed (no silent fallback).

# Offline contract check (no Inno / no hardware):
python scripts\packaging\verify-installer-contract.py
```

Install path (stable absolute):

```text
C:\Program Files\Ten Square Software\Unitor MT4 Bridge\Bridge.exe
```

Auto-Start registration (must be interactive user, not elevated admin profile):

```text
"<install>\Bridge.exe" --register-auto-start
```

## Uninstall / upgrade hygiene

| Topic | Behavior |
|---|---|
| Order | `--unregister-auto-start` (interactive user) → remove Bridge files → leave Driver Store residual as OS retains (honest: residual INF entries may remain until admin `pnputil /delete-driver`) |
| Unit identity | **Preserve** `%LOCALAPPDATA%\unitor-win64-driver\` by default (dual-MT4 ordinal `K` must not reshuffle casually). No “remove user data” checkbox in V1 — data is left in place |
| Upgrade | Same Inno `AppId` replaces in place; post-install re-registers Auto-Start once (Scheduler **or** Run, not both — Story 3.1). Must not leave two Bridge Auto-Start backends active |
| ARP | Add/Remove Programs entry: **Unitor MT4 Bridge** / **Ten Square Software** |

## Lab mitigation (unsigned INF)

Clean machines may reject an unsigned INF / missing `.cat`. Lab-only: [`installer/sign-lab-package.ps1`](../../installer/sign-lab-package.ps1). Public Authenticode / catalog policy: [`docs/dev/authenticode-and-smartscreen.md`](../dev/authenticode-and-smartscreen.md) — operator smoke [`smoke-epic4-authenticode-smartscreen-mt4.md`](smoke-epic4-authenticode-smartscreen-mt4.md). Still exercise the association path now; fail closed with English diagnostics (no success screen on bind failure).

## Out of scope for this smoke

- Claiming SM-5 fully closed (needs [`smoke-epic4-user-docs-mt4.md`](smoke-epic4-user-docs-mt4.md) Pass on top of this installer matrix)
- Claiming **full** SM-6 community-release honesty while **OQ-1** or blank hardware rows remain (Authenticode/SmartScreen **slice** closes via [`smoke-epic4-authenticode-smartscreen-mt4.md`](smoke-epic4-authenticode-smartscreen-mt4.md))
- Embedding Tobias virtualMIDI MSI without OQ-1
- Session-0 Windows Service
- Zadig as primary community UX
- MIDI Path latency proof (Epic **5**)

## Related docs

- End-user docs: [`docs/user/README.md`](../user/README.md)
- User-docs smoke (Story 4.2): [`smoke-epic4-user-docs-mt4.md`](smoke-epic4-user-docs-mt4.md)
- License / backend honesty smoke (Story 4.3): [`smoke-epic4-license-honesty-mt4.md`](smoke-epic4-license-honesty-mt4.md)
- Authenticode / SmartScreen smoke (Story 4.4): [`smoke-epic4-authenticode-smartscreen-mt4.md`](smoke-epic4-authenticode-smartscreen-mt4.md)
- WinUSB bind (contributor / Zadig fallback): [`docs/dev/winusb-bind.md`](../dev/winusb-bind.md)
- Auto-Start runtime smoke: [`smoke-epic3-autostart-mt4.md`](smoke-epic3-autostart-mt4.md)
- Installer sources: `installer/public-installer.iss`
