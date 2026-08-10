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

## Three-way split

| Layer | What it is | What it is not |
|---|---|---|
| **MIT (this repository)** | Unitor MT4 Bridge sources, installer scripts, and project docs listed under [`LICENSE`](../../LICENSE) — copyright Guillaume DUPONT / **Ten Square Software** | A license for Tobias Erichsen’s virtualMIDI SDK or driver |
| **virtualMIDI (proprietary, separate)** | V1 DAW-facing MIDI backend ([teVirtualMIDI / virtualMIDI](https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html)); any install that provides `teVirtualMIDI.dll` qualifies — **loopMIDI** / **rtpMIDI** are the usual eval examples | Covered by this repo’s MIT; freeware SDK; redistributable MSI already cleared for the Public Installer |
| **Windows MIDI Services (future)** | Optional later **Win11-only** `MidiBackend` behind the same abstraction | V1; available as the shipping backend on Windows 10 |

### virtualMIDI redistribution honesty (OQ-1)

The virtualMIDI SDK is **not** freeware. Software that links the SDK — including redistributing **Bridge** / **Setup** binaries built against it — **must not be distributed** without prior clearance from Tobias Erichsen. That clearance is separate from (and broader than) MSI embed.

The Public Installer therefore:

- Requires virtualMIDI to be **already present** (`teVirtualMIDI.dll` via loopMIDI / rtpMIDI eval, paid SDK install, or equivalent)
- Does **not** embed or ship a virtualMIDI MSI / merge module until clearance (**OQ-1** = open redistributable-embed question) is obtained

Do not read “MIT repo” as permission to redistribute virtualMIDI, nor treat “no MSI in the installer yet” as permission to ship Bridge binaries without Tobias clearance.

Vendor terms (paraphrase; always check the live page):  
https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html

### Windows MIDI Services

Microsoft’s Windows MIDI Services path is a **future optional Win11-only** backend (same scope as the three-way table above). V1 ships and claims **virtualMIDI** only. End-user wording: [What works / what does not](../user/unitor-mt4-bridge-user-guide.md#what-works--what-does-not).

## Integration proof (not a fork base)

[aaron1a12/virtual-midi](https://github.com/aaron1a12/virtual-midi) is cited as **existence proof** that virtualMIDI SDK integration can work on Windows. That repository is GPL-3.0 and vendors SDK materials. **Do not fork it** as this project’s base. This tree keeps an original MIT Bridge and loads `teVirtualMIDI.dll` at runtime when present.

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
- Authenticode / SmartScreen policy (lab vs public, OQ-3 deferral): [`authenticode-and-smartscreen.md`](authenticode-and-smartscreen.md) — operator smoke [`docs/tests/smoke-epic4-authenticode-smartscreen-mt4.md`](../tests/smoke-epic4-authenticode-smartscreen-mt4.md)
