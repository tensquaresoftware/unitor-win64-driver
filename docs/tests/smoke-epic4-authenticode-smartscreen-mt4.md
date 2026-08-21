---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 4.4 — Authenticode policy and SmartScreen honesty
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-21
---

# Smoke guide — Epic 4.4 Authenticode / SmartScreen honesty (MT4)

Operator guide for **Story 4.4**: prove a downloader can tell (a) whether Setup is signed, (b) what to do on SmartScreen, and (c) that this hobby project **does not ship** a certificate — unsigned + docs is the plan.

**Course correction (2026-08-10):** OQ-3 **no certificate purchase / out of scope hobby**. Re-check wording against current [`authenticode-and-smartscreen.md`](../dev/authenticode-and-smartscreen.md) + user guides (no “strongly recommended / buy later” as the live plan).

**Honesty bar:** a blank cell is **not** Pass. Prefer **docs-first** verification (no purchased certificate required — and none planned).

## Product intent

A musician downloading Setup from the official project channel is not silently abandoned by Windows trust warnings, and contributors never confuse lab self-signing with public Authenticode.

## Scope fences

| Topic | Owner |
|---|---|
| Public Installer AD-12 UX / wizard redesign | **4.1** (string/messaging only if bind-fail honesty needs a clearer unsigned/SmartScreen pointer; no wizard redesign) |
| End-user UJ-1 / UJ-2 chapter structure under `docs/user/` | **4.2** (add SmartScreen section / troubleshooting; do **not** reopen single-file manual shape) |
| Three-way MIT ≠ virtualMIDI ≠ Windows MIDI Services | **4.3** (cross-link only; do not rewrite license page) |
| Tobias MSI / VirtualMIDI-linked community binaries | **OQ-1 out of community scope** |
| OQ-3 certificate purchase | **No certificate purchase / out of scope hobby** (Correct Course 2026-08-10) |
| MIDI Path latency claims / harness | Epic **5** |
| Kernel / WHQL / Partner Center attestation signing | **never** (usermode Bridge + WinUSB INF catalog only) |

## SSOT citations

- Epics Story **4.4**
- PRD FR-15 / NFR-S1 / SM-6 / OQ-3
- Architecture AD-19
- SPEC Authenticode constraint (no CAP for FR-15 — do **not** invent CAP-15 for signing; CAP-15 is DeviceProfile)

## Prerequisites

- Repo checkout (macOS or Windows)
- Ability to read `README.md`, `docs/user/`, `docs/dev/authenticode-and-smartscreen.md`, `docs/tests/`
- No purchased code-signing certificate required for docs rows

## How to score

- **Pass** / **Fail** / **N/A** (+ short reason)
- Blank = not run (**does not** count as Pass)
- Docs-only rows may Pass from reading shipped markdown when wording matches the contract on **any checkout** (macOS or Windows). For those rows, mark **N/A** on unused OS columns, or Pass both columns only when the same wording was checked once and the claim is OS-independent — note the host in Notes
- Optional SmartScreen UI capture on an unsigned Setup is nice-to-have, **not** a hard Pass gate for FR-15 docs rows

## Pass / Fail matrix (FR-15 / NFR-S1 / AD-19)

| # | Verification | Win10 x64 | Win11 x64 | Notes |
|---|---|---|---|---|
| 1 | User docs (EN) explain SmartScreen behavior + mitigation if public Setup is unsigned / unrecognized (AD-19) | | | Docs-only: dual path EN guides SmartScreen sections — re-check after 6.2 (clear Pass only when re-read) |
| 2 | FR user peer covers the same SmartScreen facts (not a stale EN-only island) | | | Docs-only: dual path FR guides — re-check after 6.2 |
| 3 | Public/contributor surface states certificate purchase **no certificate purchase** / unsigned + SmartScreen docs is the hobby plan (FR-15 / NFR-S1 course-corrected) | N/A | N/A | Re-verify after Correct Course 2026-08-10 — README + authenticode-and-smartscreen.md |
| 4 | Optional “if a certificate ever appears” path (if documented) does not contradict no certificate in this line as the default | N/A | N/A | Re-verify after Correct Course 2026-08-10 |
| 5 | OQ-3 is explicitly **no certificate purchase / out of scope hobby** (not “deferred purchase decision”) | N/A | N/A | Re-verify after Correct Course 2026-08-10 |
| 6 | Lab `sign-lab-package.ps1` is clearly labeled **not** public Authenticode; two domains (binary Authenticode vs INF catalog) are distinguished | N/A | N/A | Docs-only any-checkout 2026-08-10: authenticode-and-smartscreen.md + lab script header — **Pass** |
| 7 | Discoverability: community reader can reach SmartScreen / signing honesty from README and/or `docs/user/` without opening `_bmad-output/` | N/A | N/A | Docs-only any-checkout 2026-08-10: README Status + Deliverables + `docs/user/README.md` pointer — **Pass** |
| 8 | Scope fence: no AD-12 wizard redesign; no 4.3 three-way rewrite; no OQ-1 MSI embed; no kernel/WHQL attestation; no claim that signed == never SmartScreen; no secrets in repo | N/A | N/A | Docs + optional gated PowerShell change set 2026-08-10; fences restated here — **Pass** |

## Related docs

- Deep policy page: [`docs/dev/authenticode-and-smartscreen.md`](../dev/authenticode-and-smartscreen.md)
- User guides: [`docs/user/README.md`](../user/README.md)
- Public Installer smoke: [`smoke-epic4-public-installer-mt4.md`](smoke-epic4-public-installer-mt4.md)
- User docs smoke: [`smoke-epic4-user-docs-mt4.md`](smoke-epic4-user-docs-mt4.md)
- License honesty smoke: [`smoke-epic4-license-honesty-mt4.md`](smoke-epic4-license-honesty-mt4.md)
- WinUSB bind: [`docs/dev/winusb-bind.md`](../dev/winusb-bind.md)

## Out of scope for this smoke

- Claiming a paid certificate is still the community plan (**OQ-3 no certificate purchase**)
- Embedding Tobias virtualMIDI MSI / VirtualMIDI-linked community Releases (**OQ-1 out of community scope**)
- Redesigning Public Installer UX (**4.1**)
- MIDI Path / latency Studio-Done claims (Epic **5**)
- Kernel / WHQL / attestation signing
- Requiring a purchased certificate for docs-row Pass
- Committing PFX / private keys / CI secrets
