---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 4.2 — End-user documentation (first MIDI / SysEx)
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
---

# Smoke guide — Epic 4.2 user docs (MT4 on Windows)

Operator guide for **Story 4.2**: prove a new MT4 owner can complete first MIDI (**UJ-1**) and first SysEx (**UJ-2**) using **only** the shipped [`docs/user/unitor-mt4-bridge-user-guide.md`](../user/unitor-mt4-bridge-user-guide.md) plus **named** external prerequisites (virtualMIDI via loopMIDI/rtpMIDI; Ableton Live 12 or MIDI-OX; Matrix-Control or documented SysEx equivalent).

**Honesty bar:** a blank cell is **not** Pass. Win10 x64 is **mandatory** to close the lab claim; Win11 x64 when available. Physical MT4 is required for MIDI / SysEx rows. Do **not** claim SM-5 closed until this matrix Passes **and** Story **4.1** installer path is credible. Do **not** claim FR-14 under this story ID. Authenticode / SmartScreen honesty is Story **4.4** — [`smoke-epic4-authenticode-smartscreen-mt4.md`](smoke-epic4-authenticode-smartscreen-mt4.md).

## Product intent

A musician who never saw the French lab smokes under `docs/tests/` can reach first MIDI the same evening and first SysEx without contributor tribal knowledge.

## Scope fences

| Topic | Owner |
|---|---|
| Public Installer AD-12 UX / packaging | **4.1** (describe; do **not** redesign the wizard) |
| Three-way MIT vs virtualMIDI vs Windows MIDI Services honesty + contributor dual-machine loop | **4.3** — [`docs/dev/license-and-backends.md`](../dev/license-and-backends.md), [`docs/dev/contributor-dual-machine-loop.md`](../dev/contributor-dual-machine-loop.md); close via [`smoke-epic4-license-honesty-mt4.md`](smoke-epic4-license-honesty-mt4.md) |
| Authenticode / SmartScreen honesty for unsigned public builds | **4.4** — [`docs/dev/authenticode-and-smartscreen.md`](../dev/authenticode-and-smartscreen.md); close via [`smoke-epic4-authenticode-smartscreen-mt4.md`](smoke-epic4-authenticode-smartscreen-mt4.md) |
| Tobias MSI **embed** redistributable | **OQ-1** release gate only (docs name eval path: loopMIDI / rtpMIDI) |
| MIDI Path latency claims / harness | Epic **5** (never cite ASIO buffer size as MIDI proof) |
| Zadig primary path | **Forbidden** — contributor-only in [`docs/dev/winusb-bind.md`](../dev/winusb-bind.md) |

## SSOT citations

- Epics Story **4.2**
- PRD FR-13 / UJ-1 / UJ-2 / SM-5
- Architecture AD-19 / AD-10 / AD-8 / AD-12 / AD-5
- SPEC CAP-13

## Docs-alone rule

Verifier instructions come from [`docs/user/unitor-mt4-bridge-user-guide.md`](../user/unitor-mt4-bridge-user-guide.md) only (+ named externals). Do **not** require Epic 1–3 French operator smokes as the user instruction set. Those remain lab references.

## Prerequisites

- Clean or representative Win10 x64 (mandatory) or Win11 x64
- Physical MT4 for rows 2–3
- virtualMIDI via loopMIDI or rtpMIDI
- Public Installer available **or** Bridge already installed from Story 4.1 (row 1 may confirm “already installed”)
- Ableton Live 12 **or** MIDI-OX for UJ-1
- Matrix-Control **or** MIDI-OX SysEx equivalent for UJ-2

## How to score

- **Pass** / **Fail** / **N/A** (+ short reason)
- Blank = not run (**does not** count as Pass)
- Win10 x64 column mandatory to close the claim
- Content-only rows (4–8) may Pass from reading shipped markdown when wording matches the contract; MIDI/SysEx rows need hardware

## Pass / Fail matrix (UJ-1 + UJ-2 from user guide)

| # | Verification | Win10 x64 | Win11 x64 | Notes |
|---|---|---|---|---|
| 1 | Fresh reader follows the user guide only → completes install + Auto-Start expectations (or confirms already installed from 4.1) without lab smoke tribal knowledge | | | |
| 2 | First MIDI (UJ-1): notes/CC visible in Ableton Live 12 **or** MIDI-OX on named `MT4 In` / `MT4 Out` | | | Physical MT4 |
| 3 | First SysEx (UJ-2): Matrix-Control **or** documented equivalent completes dump/restore (or short SysEx exchange) without Bridge restart for normal completion | | | Physical MT4; Computer Mode CC wake |
| 4 | Troubleshooting section matches at least one deliberate negative (e.g. virtualMIDI missing messaging aligns with installer/docs) | Pass | | Content check 2026-08-10: Troubleshooting + Install + installer virtualMIDI fix path aligned |
| 5 | Works / does-not-work list present and consistent with PRD non-goals | Pass | | Content check 2026-08-10: `# What works / what does not` (works + V1 non-goals) |
| 6 | Hot-plug expectations stated (rescan / supervised restart OK; reboot = fail) | Pass | | `# Unplug and replug the MT4` states OK recovery vs Windows reboot = fail |
| 7 | Multi-MT4 honesty stated (proven dual-unit **or** explicit “single-unit proven” wording) | Pass | | `# Two MT4 interfaces` validation honesty: single MT4 proven path; dual naming implemented, dual lab not claimed closed here |
| 8 | Discoverability: root README and installer support/success pointer reach `docs/user/` | Pass | | README → `docs/user/README.md`; `AppSupportURL` / FinishedLabel / success MsgBox → GitHub `docs/user/README.md` |

## Shipped user guide

Landing: [`docs/user/README.md`](../user/README.md). English guide: [`docs/user/unitor-mt4-bridge-user-guide.md`](../user/unitor-mt4-bridge-user-guide.md). French peer: [`docs/user/unitor-mt4-bridge-guide-utilisateur.md`](../user/unitor-mt4-bridge-guide-utilisateur.md).

## Out of scope for this smoke

- Claiming SM-5 fully closed while rows 1–3 are blank
- Claiming **full** SM-6 while OQ-1 or blank **4.1** hardware rows remain (Authenticode/SmartScreen slice: [`smoke-epic4-authenticode-smartscreen-mt4.md`](smoke-epic4-authenticode-smartscreen-mt4.md))
- Claiming FR-14 / three-way license polish without [`smoke-epic4-license-honesty-mt4.md`](smoke-epic4-license-honesty-mt4.md) Pass (**4.3**)
- Embedding Tobias virtualMIDI MSI without OQ-1
- Zadig as primary community UX
- MIDI Path latency proof (Epic **5**)
- Treating French `docs/tests/` operator guides as community manuals (FR **user** manual under `docs/user/` is an intentional peer)

## Related docs

- Public Installer smoke: [`smoke-epic4-public-installer-mt4.md`](smoke-epic4-public-installer-mt4.md)
- License / backend honesty smoke: [`smoke-epic4-license-honesty-mt4.md`](smoke-epic4-license-honesty-mt4.md)
- Authenticode / SmartScreen smoke: [`smoke-epic4-authenticode-smartscreen-mt4.md`](smoke-epic4-authenticode-smartscreen-mt4.md)
- User docs landing: [`docs/user/README.md`](../user/README.md)
- User manual (EN): [`docs/user/unitor-mt4-bridge-user-guide.md`](../user/unitor-mt4-bridge-user-guide.md)
- Guide utilisateur (FR): [`docs/user/unitor-mt4-bridge-guide-utilisateur.md`](../user/unitor-mt4-bridge-guide-utilisateur.md)
