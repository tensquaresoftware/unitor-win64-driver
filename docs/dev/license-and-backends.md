---
organization: Ten Square Software
project: unitor-win64-driver
title: License and MIDI backends — three-way honesty
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-21
---

# License and MIDI backends

This page is the deep companion to the root [`README.md`](../../README.md) **License** section. It keeps three separate claims honest for community evaluators and contributors.

**Course correction (2026-08-10):** hobby / free GitHub posture — see [`sprint-change-proposal-2026-08-10.md`](../../_bmad-output/planning-artifacts/sprint-change-proposal-2026-08-10.md).

**Story 6.2 expansion:** dual community Releases — **Win11 + Windows MIDI Services** (comfort) and **Win10 + user-installed virtualMIDI** (parallel motivated path). Win10 is an assumed parallel offer, not the comfort promise.

## Three-way split

| Layer | What it is | What it is not |
|---|---|---|
| **MIT (this repository)** | Unitor MT4 Bridge sources, installer scripts, and project docs listed under [`LICENSE`](../../LICENSE) — copyright Guillaume DUPONT / **Ten Square Software** | A license for Tobias Erichsen’s virtualMIDI SDK or driver |
| **virtualMIDI (proprietary, separate)** | Community **Win10** path when the user **self-installs** the driver ([teVirtualMIDI / virtualMIDI](https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html)); also **lab / personal** on Win10/Win11. Any install that provides `teVirtualMIDI.dll` qualifies — **loopMIDI** / **rtpMIDI** are the usual examples | Covered by this repo’s MIT; freeware SDK; cleared for this project to **redistribute** Bridge/Setup binaries that embed the SDK/DLL/MSI |
| **Windows MIDI Services (community Win11)** | Shipping **Win11** `MidiBackend` behind the same abstraction — comfort path for public ready-to-run binaries without a virtualMIDI prerequisite | Available as the community comfort backend on Windows 10; a reason to embed virtualMIDI |

### virtualMIDI redistribution honesty (OQ-1 — unchanged)

The virtualMIDI SDK is **not** freeware. This hobby project does **not** embed or redistribute `teVirtualMIDI.dll`, a virtualMIDI MSI, or the SDK.

Therefore:

- **Community Win10 Setup:** requires the user to install virtualMIDI themselves and present `teVirtualMIDI.dll` before/during Setup; Bridge may load the DLL at runtime when present.
- **Community Win11 Setup:** does **not** require virtualMIDI; uses Windows MIDI Services.
- **Lab / personal:** virtualMIDI remains valid on Win10/Win11 via loopMIDI / rtpMIDI (or equivalent).

Do not read “MIT repo” as permission to redistribute virtualMIDI.

Vendor terms (paraphrase; always check the live page):  
https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html

### Windows MIDI Services

Microsoft’s Windows MIDI Services path is the **Win11 community comfort** backend. Fail closed when the transport is unavailable — never present empty port lists as success. Musician docs: [`docs/user/unitor-mt4-bridge-win11-wms-user-guide.md`](../user/unitor-mt4-bridge-win11-wms-user-guide.md).

## Integration proof (not a fork base)

[aaron1a12/virtual-midi](https://github.com/aaron1a12/virtual-midi) is cited as **existence proof** that virtualMIDI SDK integration can work on Windows. That repository is GPL-3.0 and vendors SDK materials. **Do not fork it** as this project’s base. This tree keeps an original MIT Bridge and loads `teVirtualMIDI.dll` at runtime when present (Win10 community + lab).

## Linux Emagic quirk sources — reference only

Linux `sound/usb/midi.c` and `quirks-table.h` (`QUIRK_MIDI_EMAGIC`) informed the Emagic cable mapping design. They are **read-only reference**. **No GPL Linux sources are vendored** in this repository (no copied `midi.c` / `quirks-table.h` trees). Do **not** copy those GPL files into this tree, or the project would need GPL instead.

## Catch2 (build-time test harness)

Catch2 (via CMake FetchContent) is a **build-time test harness** dependency. It is not shipped to end users with the Public Installer. FetchContent trees under `builds/**/_deps` are local build artifacts, not redistributables.

## Public facade

Public surfaces use **Ten Square Software** (README, LICENSE, installer branding, user docs).

## Related

- Install prerequisites / OS router: [`docs/user/README.md`](../user/README.md)
- Release automation: [`release-guide.md`](release-guide.md)
- Contributor dual-machine loop: [`contributor-dual-machine-loop.md`](contributor-dual-machine-loop.md)
- Operator smoke for this honesty bar: [`docs/tests/smoke-epic4-license-honesty-mt4.md`](../tests/smoke-epic4-license-honesty-mt4.md)
- Authenticode / SmartScreen policy (no certificate purchase in this hobby project): [`authenticode-and-smartscreen.md`](authenticode-and-smartscreen.md) — operator smoke [`docs/tests/smoke-epic4-authenticode-smartscreen-mt4.md`](../tests/smoke-epic4-authenticode-smartscreen-mt4.md)
