---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 4.3 — Technical docs and three-way license honesty
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
---

# Smoke guide — Epic 4.3 license / backend honesty (MT4)

Operator guide for **Story 4.3**: prove a reader can tell MIT (this repo) ≠ virtualMIDI (proprietary **interim lab**) ≠ Windows MIDI Services (**next community**, Win11) — without tribal knowledge from `_bmad-output/`.

**Course correction (2026-08-10):** VirtualMIDI-linked community Releases are **out of scope**; WMS is the next community backend. Re-check docs wording against current [`license-and-backends.md`](../dev/license-and-backends.md) + README.

**Honesty bar:** a blank cell is **not** Pass. Prefer **docs-only** verification. Historical Pass rows below were taken under older wording; after Correct Course, treat criteria as the **current** three-way table (interim lab vs next community).

## Product intent

A contributor or community evaluator can trust public messaging about licenses and MIDI backends without reading internal BMad planning docs.

## Scope fences

| Topic | Owner |
|---|---|
| Public Installer AD-12 UX / packaging | **4.1** |
| Tobias MSI / VirtualMIDI-linked community binaries | **OQ-1 out of community scope** — never claim cleared; Epic 6 WMS |
| End-user UJ-1 / UJ-2 manuals under `docs/user/` | **4.2** (do **not** reopen chapter structure; light cross-links OK) |
| Authenticode / SmartScreen honesty for unsigned public builds | **4.4** — [`docs/dev/authenticode-and-smartscreen.md`](../dev/authenticode-and-smartscreen.md); close via [`smoke-epic4-authenticode-smartscreen-mt4.md`](smoke-epic4-authenticode-smartscreen-mt4.md) |
| Tobias MSI **embed** redistributable clearance | **OQ-1 out of community scope** |
| MIDI Path latency claims / harness | Epic **5** |
| Protocol reimplementation / Emagic mapper code | Epics **1–2** (already done; this story documents honesty only) |

## SSOT citations

- Epics Story **4.3**
- PRD FR-14 / NFR-D3 / NFR-Q2 / NFR-Q3 / SM-6
- Architecture AD-7 / AD-13 / AD-14 / AD-19
- SPEC CAP-14
- PRD addendum — virtualMIDI licensing

## Prerequisites

- Repo checkout (macOS or Windows)
- Ability to read `README.md`, `LICENSE`, `docs/dev/`, `docs/user/`, `contributing.md`
- Optional: tree grep for vendored GPL filenames (documented in matrix notes)

## How to score

- **Pass** / **Fail** / **N/A** (+ short reason)
- Blank = not run (**does not** count as Pass)
- Docs-only rows may Pass from reading shipped markdown when wording matches the contract on **any checkout** (macOS or Windows). For those rows, mark **N/A** on unused OS columns, or Pass both columns only when the same wording was checked once and the claim is OS-independent — note the host in Notes
- Physical MT4 is **not** required for FR-14 / CAP-14 rows in this guide
- Tree spot-checks mean **source tree** (tracked paths). Ignore FetchContent / harness trees under `builds/**/_deps` (and similar build outputs)

## Pass / Fail matrix (FR-14 / CAP-14)

| # | Verification | Win10 x64 | Win11 x64 | Notes |
|---|---|---|---|---|
| 1 | README (or linked `docs/dev/license-and-backends.md`) states MIT ≠ virtualMIDI (Win10 **self-install** community + lab / proprietary) ≠ Windows MIDI Services (**shipping** community Win11 comfort) in plain language | N/A | N/A | Re-verify after Story 6.2 — dual shipping paths; OQ-1 no embed |
| 2 | `aaron1a12/virtual-midi` cited as integration proof only / not a fork base | N/A | N/A | Docs-only any-checkout 2026-08-10: README Acknowledgments + license-and-backends.md — **Pass** |
| 3 | Explicit “no GPL Linux sources vendored” (or equivalent) on a public/contributor surface; spot-check **source tree**: no vendored `midi.c` / `quirks-table.h` / aaron1a12 fork (exclude `builds/**/_deps`) | N/A | N/A | Content + source-tree check 2026-08-10: no `sound/usb/midi.c`, no `quirks-table.h`, no source `third_party/`, no aaron1a12 subtree — **Pass** |
| 4 | Contributor dual-machine loop documented (macOS edit / Windows x64 validate; Win10 lab through Epic 5) | N/A | N/A | Docs-only any-checkout 2026-08-10: [`docs/dev/contributor-dual-machine-loop.md`](../dev/contributor-dual-machine-loop.md) + `contributing.md` — **Pass** |
| 5 | Public facade **Ten Square Software** visible on README + LICENSE (and not contradicted by installer / user docs) | N/A | N/A | Docs-only any-checkout 2026-08-10: LICENSE copyright; README Bridge blurb; installer / `docs/user/` already Ten Square — **Pass** |
| 6 | OQ-1 honesty: no claim that virtualMIDI MSI/DLL embed is cleared; Win10 community = user self-install only; Win11 community = WMS | N/A | N/A | Re-verify after Story 6.2 |
| 7 | Discoverability: community reader can reach the three-way explanation from README without opening `_bmad-output/` | N/A | N/A | Docs-only any-checkout 2026-08-10: README § License → deep page link — **Pass** |
| 8 | Scope fence: no SmartScreen/Authenticode chapter claimed under this story ID (→ 4.4); no installer UX redesign | N/A | N/A | Docs-only change set; SmartScreen lives under 4.4 — **Pass** |

## Related docs

- Deep three-way page: [`docs/dev/license-and-backends.md`](../dev/license-and-backends.md)
- Contributor dual-machine loop: [`docs/dev/contributor-dual-machine-loop.md`](../dev/contributor-dual-machine-loop.md)
- Public Installer smoke: [`smoke-epic4-public-installer-mt4.md`](smoke-epic4-public-installer-mt4.md)
- User docs smoke: [`smoke-epic4-user-docs-mt4.md`](smoke-epic4-user-docs-mt4.md)
- Authenticode / SmartScreen smoke: [`smoke-epic4-authenticode-smartscreen-mt4.md`](smoke-epic4-authenticode-smartscreen-mt4.md)
- End-user manuals: [`docs/user/README.md`](../user/README.md)

## Out of scope for this smoke

- Claiming Tobias virtualMIDI MSI / VirtualMIDI-linked community binaries cleared (**OQ-1 out of community scope**)
- Owning SmartScreen / Authenticode policy prose under this story ID (**4.4**)
- MIDI Path / latency Studio-Done claims (Epic **5**)
- Vendoring SDK binaries, GPL Linux trees, or forking aaron1a12
