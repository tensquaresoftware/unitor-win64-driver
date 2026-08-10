---
organization: Ten Square Software
project: unitor-win64-driver
title: License and MIDI backends — three-way honesty
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
---

# License and MIDI backends

This page is the deep companion to the root [`README.md`](../../README.md) **License** section. It keeps three separate claims honest for community evaluators and contributors.

**Course correction (2026-08-10):** hobby / free GitHub posture — see [`sprint-change-proposal-2026-08-10.md`](../../_bmad-output/planning-artifacts/sprint-change-proposal-2026-08-10.md).

## Three-way split

| Layer | What it is | What it is not |
|---|---|---|
| **MIT (this repository)** | Unitor MT4 Bridge sources, installer scripts, and project docs listed under [`LICENSE`](../../LICENSE) — copyright Guillaume DUPONT / **Ten Square Software** | A license for Tobias Erichsen’s virtualMIDI SDK or driver |
| **virtualMIDI (proprietary, separate)** | **Interim lab / personal** DAW-facing MIDI backend ([teVirtualMIDI / virtualMIDI](https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html)); any install that provides `teVirtualMIDI.dll` qualifies — **loopMIDI** / **rtpMIDI** are the usual eval examples | Covered by this repo’s MIT; freeware SDK; cleared for community binary redistribution |
| **Windows MIDI Services (next community)** | Planned **Win11-only** `MidiBackend` (Epic 6) behind the same abstraction — intended path for public ready-to-run binaries | Already shipping; available as the community backend on Windows 10 |

### virtualMIDI redistribution honesty (OQ-1 — out of community scope)

The virtualMIDI SDK is **not** freeware. Software that links the SDK — including redistributing **Bridge** / **Setup** binaries built against it — must not be distributed without prior clearance from the author.

**Product decision (Correct Course 2026-08-10):** this hobby project does **not** ship community Releases of VirtualMIDI-linked Bridge/Setup binaries. Community ready-to-run binaries are planned for the **Windows MIDI Services** backend (Epic 6, Win11-only) instead.

Therefore:

- **Lab / personal use:** virtualMIDI may remain installed via loopMIDI / rtpMIDI (or equivalent) so the current Bridge can run on Win10/Win11 lab machines through Epic 5.
- **Community redistributable binaries:** wait for the **Windows MIDI Services** backend (Epic 6, Win11-only).
- **MSI embed** of virtualMIDI in the Public Installer stays **out of scope** under this hobby posture.

Do not read “MIT repo” as permission to redistribute virtualMIDI.

Vendor terms (paraphrase; always check the live page):  
https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html

### Windows MIDI Services

Microsoft’s Windows MIDI Services path is the **next community** Win11-only backend (Epic 6). Until then, end-user docs may still describe the interim virtualMIDI lab path honestly. See [What works / what does not](../user/unitor-mt4-bridge-user-guide.md#what-works--what-does-not) and the root README.

## Integration proof (not a fork base)

[aaron1a12/virtual-midi](https://github.com/aaron1a12/virtual-midi) is cited as **existence proof** that virtualMIDI SDK integration can work on Windows. That repository is GPL-3.0 and vendors SDK materials. **Do not fork it** as this project’s base. This tree keeps an original MIT Bridge and loads `teVirtualMIDI.dll` at runtime when present (interim backend).

## Linux Emagic quirk sources — reference only

Linux `sound/usb/midi.c` and `quirks-table.h` (`QUIRK_MIDI_EMAGIC`) informed the Emagic cable mapping design. They are **read-only reference**. **No GPL Linux sources are vendored** in this repository (no copied `midi.c` / `quirks-table.h` trees). Do **not** copy those GPL files into this tree, or the project would need GPL instead.

## Catch2 (build-time test harness)

Catch2 (via CMake FetchContent) is a **build-time test harness** dependency. It is not shipped to end users with the Public Installer. FetchContent trees under `builds/**/_deps` are local build artifacts, not redistributables.

## Public facade

Public surfaces use **Ten Square Software** (README, LICENSE, installer branding, user docs).

## Related

- Install prerequisites for musicians: [`docs/user/README.md`](../user/README.md)
- Contributor dual-machine loop: [`contributor-dual-machine-loop.md`](contributor-dual-machine-loop.md)
- Operator smoke for this honesty bar: [`docs/tests/smoke-epic4-license-honesty-mt4.md`](../tests/smoke-epic4-license-honesty-mt4.md)
- Authenticode / SmartScreen policy (no certificate purchase in this hobby project): [`authenticode-and-smartscreen.md`](authenticode-and-smartscreen.md) — operator smoke [`docs/tests/smoke-epic4-authenticode-smartscreen-mt4.md`](../tests/smoke-epic4-authenticode-smartscreen-mt4.md)
