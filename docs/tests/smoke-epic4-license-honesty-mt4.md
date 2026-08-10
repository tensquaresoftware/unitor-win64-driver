---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 4.3 — Technical docs and three-way license honesty
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
---

# Smoke guide — Epic 4.3 license / backend honesty (MT4)

Operator guide for **Story 4.3**: prove that a community reader or contributor can tell what this repo licenses (MIT), what virtualMIDI requires separately (proprietary), and that Windows MIDI Services is **not** the V1 backend — without tribal knowledge from `_bmad-output/` planning artifacts.

**Honesty bar:** a blank cell is **not** Pass. Prefer **docs-only** verification (no physical MT4 required for Pass). Win10 x64 remains listed when any hardware-adjacent claim is checked. This story may claim **FR-14 / CAP-14** closed when its ACs Pass. Phrase **SM-6** as **partially** met (license/backend honesty only) — full community-release honesty still needs Story **4.4** SmartScreen / Authenticode messaging.

## Product intent

A contributor or community evaluator can trust public messaging about licenses and MIDI backends without reading internal BMad planning docs.

## Scope fences

| Topic | Owner |
|---|---|
| Public Installer AD-12 UX / packaging | **4.1** (do **not** redesign the wizard; keep OQ-1 MSI-embed honesty as-is) |
| End-user UJ-1 / UJ-2 manuals under `docs/user/` | **4.2** (do **not** reopen chapter structure; light cross-links OK) |
| Authenticode / SmartScreen honesty for unsigned public builds | **4.4** |
| Tobias MSI **embed** redistributable clearance | **OQ-1** release gate only (never claim cleared under this story) |
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
| 1 | README (or linked `docs/dev/license-and-backends.md`) states MIT (this repo) ≠ virtualMIDI (proprietary) ≠ Windows MIDI Services (future Win11-only, not V1) in plain language | N/A | N/A | Docs-only any-checkout 2026-08-10 (host not OS-specific): README § License three-way table + [`docs/dev/license-and-backends.md`](../dev/license-and-backends.md) — **Pass** |
| 2 | `aaron1a12/virtual-midi` cited as integration proof only / not a fork base | N/A | N/A | Docs-only any-checkout 2026-08-10: README Acknowledgments + license-and-backends.md — **Pass** |
| 3 | Explicit “no GPL Linux sources vendored” (or equivalent) on a public/contributor surface; spot-check **source tree**: no vendored `midi.c` / `quirks-table.h` / aaron1a12 fork (exclude `builds/**/_deps`) | N/A | N/A | Content + source-tree check 2026-08-10: no `sound/usb/midi.c`, no `quirks-table.h`, no source `third_party/`, no aaron1a12 subtree — **Pass** |
| 4 | Contributor dual-machine loop documented (macOS edit / Windows x64 validate; Win10 mandatory) | N/A | N/A | Docs-only any-checkout 2026-08-10: [`docs/dev/contributor-dual-machine-loop.md`](../dev/contributor-dual-machine-loop.md) + `contributing.md` — **Pass** |
| 5 | Public facade **Ten Square Software** visible on README + LICENSE (and not contradicted by installer / user docs) | N/A | N/A | Docs-only any-checkout 2026-08-10: LICENSE copyright; README Bridge blurb; installer / `docs/user/` already Ten Square — **Pass** |
| 6 | OQ-1 honesty preserved: no claim that virtualMIDI MSI embed is cleared / redistributable in the Public Installer | N/A | N/A | Docs-only 2026-08-10: license-and-backends.md + **read-only** check of existing installer virtualMIDI / MSI-gate strings (no installer edit in this story) — **Pass** |
| 7 | Discoverability: community reader can reach the three-way explanation from README without opening `_bmad-output/` | N/A | N/A | Docs-only any-checkout 2026-08-10: README § License → deep page link — **Pass** |
| 8 | Scope fence: no SmartScreen/Authenticode chapter claimed under this story ID (→ 4.4); no 4.2 manual rewrite; no installer UX redesign | N/A | N/A | Docs-only change set; fences restated in this guide — **Pass** |

## Related docs

- Deep three-way page: [`docs/dev/license-and-backends.md`](../dev/license-and-backends.md)
- Contributor dual-machine loop: [`docs/dev/contributor-dual-machine-loop.md`](../dev/contributor-dual-machine-loop.md)
- Public Installer smoke: [`smoke-epic4-public-installer-mt4.md`](smoke-epic4-public-installer-mt4.md)
- User docs smoke: [`smoke-epic4-user-docs-mt4.md`](smoke-epic4-user-docs-mt4.md)
- End-user manuals: [`docs/user/README.md`](../user/README.md)

## Out of scope for this smoke

- Claiming **SM-6** fully closed without Story **4.4**
- Claiming Tobias virtualMIDI MSI embed cleared (**OQ-1**)
- SmartScreen / Authenticode policy prose (**4.4**)
- Rewriting `docs/user/` UJ manuals (**4.2**)
- Redesigning Public Installer UX (**4.1**)
- MIDI Path / latency Studio-Done claims (Epic **5**)
- Vendoring SDK binaries, GPL Linux trees, or forking aaron1a12
