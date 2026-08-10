---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 4.1 — Public Installer (AD-12 UX bar)
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
---

# Smoke guide — Epic 4.1 Public Installer (MT4 on Windows)

Operator guide for **Story 4.1**: Public Installer packaging that binds WinUSB when trust allows, installs the Bridge, wires Auto-Start, and checks virtualMIDI.

**Course correction (2026-08-10):** hobby / hobby install — Setup-alone WinUSB on clean PC **Fail** without trusted catalog is **expected** (no certificate in this line). Guided WinUSB (Zadig) is the supported clean-PC path. Community VirtualMIDI-linked Releases are **out of scope** (Epic 6 = WMS). See root README + user guides.

**Honesty bar:** a blank cell is **not** Pass. Win10 x64 is **mandatory** to close the lab claim; Win11 x64 when available. Physical MT4 is required for bind rows. Do **not** claim polished commercial same-evening Setup-alone success on clean PC.

## Lab session — 2026-08-10 (clean Win10 x64)

| Field | Value |
|---|---|
| Machine | Clean Win10 x64 (restore point taken before smoke) |
| Artifact | `builds/installer/UnitorMt4Bridge-Setup.exe` (Release Bridge; AppVersion 0.1.0; Explorer File version was `0.0.0.0` on the build under test — fixed in packaging after the session) |
| Operator | Guillaume |
| Sequence | virtualMIDI-absent block → loopMIDI install → Setup with MT4 → WinUSB fail → reboot + retry (same Fail) → lab stopped; restore point recommended |
| WinUSB | `pnputil` exit `-536870353` (`0xE000022F` — third-party INF lacks digital signature information) |
| UX note | Finished-page / error text truncated mid-sentence (contributor-long prose); shortened post-session in `installer/public-installer.iss` — **not** re-validated on hardware in this session |

French walkthrough used: [`guide-operateur-smoke-4-1-pc-propre-win10.md`](guide-operateur-smoke-4-1-pc-propre-win10.md).

## Product intent

The packaging path should stay short and clear. On clean PCs without a paid catalog, **expect** WinUSB association inside Setup to Fail — then musicians follow **guided WinUSB** (Zadig) in the user guide. Zadig is no longer “contributor-only taboo”; it is the hobby install USB path.

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

### OQ-1 (out of community scope)

Do **not** embed virtualMIDI MSI. Do **not** claim community redistribution of VirtualMIDI-linked binaries. Community ready-to-run binaries wait on Epic 6 (Windows MIDI Services / Win11).

## Scope fences

| Topic | Owner |
|---|---|
| Polished end-user docs (first MIDI / SysEx / troubleshooting) | **4.2** — [`docs/user/README.md`](../user/README.md); close via [`smoke-epic4-user-docs-mt4.md`](smoke-epic4-user-docs-mt4.md) |
| Three-way MIT vs virtualMIDI vs Windows MIDI Services honesty | **4.3** — [`docs/dev/license-and-backends.md`](../dev/license-and-backends.md); close via [`smoke-epic4-license-honesty-mt4.md`](smoke-epic4-license-honesty-mt4.md) |
| Authenticode / public catalog signing / SmartScreen honesty | **4.4** — [`docs/dev/authenticode-and-smartscreen.md`](../dev/authenticode-and-smartscreen.md); close via [`smoke-epic4-authenticode-smartscreen-mt4.md`](smoke-epic4-authenticode-smartscreen-mt4.md) |
| Tobias MSI **embed** / VirtualMIDI-linked community binaries | **OQ-1 out of community scope** — Epic 6 WMS path |
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
| 1 | Few steps / minimal jargon walkthrough (AD-12-1, 8) | Pass | | Few-step wizard OK until bind. Error-path UX weak (truncated finished text) — packaging shortened after session, not re-smoked |
| 2 | Visible progress during install (AD-12-2) | Pass | | Progress / status visible through copy and WinUSB attempt |
| 3 | Clear success screen **only** when all gates passed (AD-12-3) | Pass | | No false success: « Installation incomplete » + rollback after WinUSB fail |
| 4 | virtualMIDI missing → block + English fix path; not success (AD-12-4 / AC2) | Pass | | Blocked before install with English loopMIDI/rtpMIDI fix path |
| 5 | After install with virtualMIDI present: Device Manager shows WinUSB on MT4 / GUID path opens (AD-12-5) | Fail | | Clean PC rejected unsigned INF (`0xE000022F`). Known fence → Authenticode/catalog (4.4) / lab `sign-lab-package.ps1`. Not a Zadig Pass |
| 6 | Auto-Start registered; logon or plug-after-login yields ports without manual launch (AD-12-6) | N/A | | Install rolled back before Auto-Start; not reached |
| 7 | One UAC/admin at install; daily Bridge / register path does not prompt (AD-12-7) | Pass | | Single UAC for Setup observed. Daily path N/A (no successful install left) |
| 8 | Uninstall unregisters Auto-Start; logon no longer starts Bridge | N/A | | Nothing left installed after rollback / incomplete |
| 9 | Regression: lab `Bridge --start-session` / `--test-mapper` / `--test-port-names` still work from install dir or `builds/` as documented | N/A | | Install dir not retained after failed community Setup; offline `builds/release` not re-run in this session |

**Session verdict:** community Setup on clean Win10 **fail-closes correctly** when WinUSB catalog trust is missing; full AD-12 success path (rows 5–6, 8–9) remains **blocked** until a trusted INF catalog / public signing path exists. Do **not** claim SM-5 install closed.

## Build / run (reference)

```powershell
# From repo root on Windows, after Bridge is built (prefer a Release layout):
.\scripts\packaging\build-public-installer.ps1
# Optional:
#   -BridgeDir builds\release\Release
#   -AppVersion 0.2.0   # override only; default resolves from CMake project(VERSION) / bridge-version.txt
# Auto-detect prefers Release over Debug; invalid -BridgeDir fails closed (no silent fallback).
# AppVersion defaults from the same SSOT as Bridge --version (not a hard-coded second number).

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
- Claiming polished commercial Setup-alone clean-PC WinUSB success without a shipped certificate
- Embedding Tobias virtualMIDI MSI / shipping VirtualMIDI-linked community Releases (OQ-1 out of community scope)
- Treating row 5 Fail (`0xE000022F`) as a temporary “until we buy a cert” gap — it is the **known hobby contract**
- Session-0 Windows Service
- Zadig as primary community UX
- MIDI Path latency proof (Epic **5**)

## Related docs

- French step-by-step operator walkthrough (clean Win10 PC): [`guide-operateur-smoke-4-1-pc-propre-win10.md`](guide-operateur-smoke-4-1-pc-propre-win10.md)
- End-user docs: [`docs/user/README.md`](../user/README.md)
- User-docs smoke (Story 4.2): [`smoke-epic4-user-docs-mt4.md`](smoke-epic4-user-docs-mt4.md)
- License / backend honesty smoke (Story 4.3): [`smoke-epic4-license-honesty-mt4.md`](smoke-epic4-license-honesty-mt4.md)
- Authenticode / SmartScreen smoke (Story 4.4): [`smoke-epic4-authenticode-smartscreen-mt4.md`](smoke-epic4-authenticode-smartscreen-mt4.md)
- WinUSB bind (contributor / Zadig fallback): [`docs/dev/winusb-bind.md`](../dev/winusb-bind.md)
- Auto-Start runtime smoke: [`smoke-epic3-autostart-mt4.md`](smoke-epic3-autostart-mt4.md)
- Installer sources: `installer/public-installer.iss`
