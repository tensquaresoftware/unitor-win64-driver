# unitor-win64-driver

**Hobby / open-source** usermode bridge that makes Emagic Unitor-family USB MIDI interfaces work again on modern Windows — starting with the **MT4**.

Built first for personal studio use (Matrix-Control and daily MIDI). Shared freely on GitHub so other musicians can benefit. **Ten Square Software** is the public facade; there is **no** code-signing certificate in this hobby project, and **no** promise of a polished commercial one-click install on every clean PC.

There is no official 64-bit Emagic driver. The last vendor package targeted Windows XP (32-bit). Under Windows 10/11 the device enumerates over USB, but the stock class driver does not understand Emagic’s proprietary cable mapping, so DAWs never see usable MIDI ports.

This project fixes that **without a custom kernel driver**: Microsoft **WinUSB** in user mode, an original Emagic protocol layer (MIT), and virtual MIDI ports exposed to your DAW via **Unitor MT4 Bridge**.

## Project posture (read this first)

| We aim for | We do **not** promise |
| --- | --- |
| Free MIT sources on GitHub | A code-signing / driver-catalog certificate for this hobby release |
| Honest docs for musicians with little IT background | A polished commercial “double-click Setup on a clean PC and play the same evening” experience **without** extra USB steps |
| A realistic **hobby install** path (SmartScreen explained + **guided WinUSB** when needed) | That Setup-alone will bind WinUSB on a brand-new PC (lab Fail `0xE000022F` without a trusted INF catalog) |
| A future **public ready-to-run binary** on **Windows 11** via **Windows MIDI Services** | Redistributing Bridge/Setup binaries that use the proprietary **virtualMIDI** SDK as a community release |
| Keeping the lab Bridge useful on Win10 + virtualMIDI for development and Epic 5 measurements | Making the community roadmap depend on third-party virtualMIDI redistribution clearance |

Course correction detail: [`_bmad-output/planning-artifacts/sprint-change-proposal-2026-08-10.md`](_bmad-output/planning-artifacts/sprint-change-proposal-2026-08-10.md).

## Status

| Area | Today |
| --- | --- |
| **MT4 Bridge (lab)** | Ports, SysEx, Auto-Start, hot-plug, multi-client — implemented and used in lab |
| **Docs** | End-user manuals EN + FR under [`docs/user/`](docs/user/README.md) (aligned to the hobby install contract) |
| **Public Installer packaging** | Exists (`UnitorMt4Bridge-Setup.exe`) with fail-closed gates; **clean-PC WinUSB via Setup-alone still fails** without a trusted catalog — see [`docs/tests/smoke-epic4-public-installer-mt4.md`](docs/tests/smoke-epic4-public-installer-mt4.md) |
| **Public GitHub Releases binary** | **Not** the community vehicle yet while the Bridge depends on virtualMIDI |
| **Code signing** | **No certificate purchase** — unsigned builds + SmartScreen docs when binaries ship later |
| **Next engineering** | **Epic 5** — MIDI Path latency/jitter proof on the current lab stack |
| **Then** | **Epic 6** — Windows MIDI Services backend, **Win11-only** community target, then honest public binaries |

## Start here

| Who you are | Where to go |
| --- | --- |
| MT4 owner / musician | [`docs/user/README.md`](docs/user/README.md) → [English guide](docs/user/unitor-mt4-bridge-user-guide.md) or [guide français](docs/user/unitor-mt4-bridge-guide-utilisateur.md) |
| Contributor / builder | [`contributing.md`](contributing.md) + [`docs/dev/contributor-dual-machine-loop.md`](docs/dev/contributor-dual-machine-loop.md) |
| License / backends | [License](#license) + [`docs/dev/license-and-backends.md`](docs/dev/license-and-backends.md) |

**Clean PC tip:** WinUSB itself is already in Windows. What fails without a trusted catalog is *associating this non-class-compliant Emagic device* to WinUSB. Expect a **guided** association step (e.g. **Zadig** or documented equivalent) — not a silent Setup-only success.

## Why this exists

Many musicians still own Emagic **Unitor8**, **AMT8**, or **MT4** interfaces and have been stuck for years after moving to 64-bit Windows. Related discussions:

- [Mod Wiggler — Emagic Unitor / AMT / MT4 drivers](https://www.modwiggler.com/forum/viewtopic.php?t=142226)
- [Gearspace — Emagic AMT8 drivers](https://gearspace.com/board/music-computers/141084-emagic-amt-8-drivers-2.html)
- [Cockos forum archive](https://forum.cockos.com/archive/index.php/t-191736.html)

## Supported hardware

| Model | USB VID | USB PID | Physical I/O (MVP intent) | Status |
| --- | --- | --- | --- | --- |
| **MT4** | `086A` | `0003` | 2 IN / 4 OUT | **Current target** (hardware available) |
| AMT8 | `086A` | `0002` | 8 IN / 8 OUT (expected) | Later / needs test hardware |
| Unitor8 | `086A` | `0001` | 8 IN / 8 OUT (expected) | Later / needs test hardware |

USB composite identity for the MT4: `VID_086A&PID_0003`. Bus-powered (no external PSU).

The Linux kernel treats all three models with the same quirk (`QUIRK_MIDI_EMAGIC`) and the same endpoint info structure — only the cable bitmasks differ. This project models that as declarative **device profiles** keyed by PID, with a single shared cable-mapping implementation.

## How it works

Not a kernel-mode PortCls / WDM MIDI driver. The pipeline:

1. **USB transport** — bind the interface to Microsoft’s signed **WinUSB** (`winusb.sys`) via project INF / installer when trust allows; otherwise **guided** association (Zadig is the practical clean-PC path without a paid catalog).
2. **Protocol** — a C++ usermode Bridge talks to the device through `winusb.dll` and reimplements Emagic cable multiplex / demultiplex (informed by the public Linux implementation in `sound/usb/midi.c`, **without** vendoring GPL sources).
3. **DAW-facing MIDI (interim lab)** — virtual ports via [teVirtualMIDI / virtualMIDI](https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html) (driver usually installed via loopMIDI / rtpMIDI).
4. **DAW-facing MIDI (next community)** — **Windows MIDI Services** on **Windows 11 only** (Epic 6), so public binaries need not depend on the proprietary virtualMIDI SDK.

Architectural inspiration for the WinUSB + virtual MIDI pattern: [Prodikeys64](https://github.com/CrazyRedMachine/Prodikeys64).

### Why not a custom kernel driver?

A KMDF / PortCls stack needs the WDK, Microsoft attestation signing for Secure Boot machines, and specialized driver expertise. That is disproportionate for a hobby community project. WinUSB keeps the signed kernel piece on Microsoft’s side — the remaining hard part is **trusting the INF that points your MT4 at WinUSB**, which normally wants a certificate this project line does not ship.

## What ships in this repo

- MIT sources for the usermode C++ Bridge
- Public Installer sources / packaging scripts (Inno Setup + INF) — useful for lab and for a future WMS-based community binary
- End-user docs: [`docs/user/README.md`](docs/user/README.md)
- Contributor docs: WinUSB bind, license honesty, Authenticode/SmartScreen policy (no certificate in this line), dual-machine loop

**Not** currently offered as a community download: a ready-made `Bridge.exe` / Setup that links virtualMIDI for strangers to install from Releases (proprietary SDK — out of community redistribution scope).

## Scope

**In (current lab / MT4)**

- MT4 only (`PID 0003`)
- Basic MIDI I/O as stable `MT4` port names (see user guide)
- Architecture ready for additional `DeviceProfile` entries (AMT8 / Unitor8 later)
- Epic 5 MIDI Path measurements on the interim stack

**Out / later**

- Patch mode, LTC/VITC, Fast Mode / AMT features from the Unitor8/AMT8 manuals
- Cascaded / stacked multi-interface setups
- Guaranteed AMT8 / Unitor8 without real hardware
- Paid code signing
- Marketing this as a polished commercial installer product

Capabilities overview: [What works / what does not](docs/user/unitor-mt4-bridge-user-guide.md#what-works--what-does-not).

User-facing Unitor8 / AMT8 manual (functional reference):  
https://www.deepsonic.ch/deep/docs_manuals/emagic_unitor8_mkII_amt8_manual.pdf

## Protocol & reverse-engineering notes

Emagic’s USB MIDI framing is **not** fully USB-MIDI class-compliant. Windows enumerates the device, but the generic audio/MIDI class stack does not map Emagic cables correctly.

Public references:

- Linux quirk + cable masks: [`quirks-table.h`](https://github.com/torvalds/linux/blob/master/sound/usb/quirks-table.h)
- Emagic message handling: [`midi.c`](https://github.com/torvalds/linux/blob/master/sound/usb/midi.c) (search for `emagic`)
- Microsoft WinUSB docs: [Introduction](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/introduction-to-winusb-for-developers), [considerations](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-considerations), [installation](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-installation)

Cable masks observed in the Linux quirk table:

| Model | `in_cables` | `out_cables` | USB interface (`ifnum`) |
| --- | --- | --- | --- |
| MT4 | `0x8003` | `0x800f` | `2` |
| AMT8 | `0x80ff` | `0x80ff` | `2` |
| Unitor8 | `0x80ff` | `0x80ff` | `2` |

If original Emagic protocol documents cannot be recovered from community archives, validation may also rely on USB captures (e.g. Wireshark + USBPcap).

## Project layout

```
docs/user/         End-user manuals (EN + FR)
docs/dev/          Contributor / process docs
docs/tests/        Operator smoke guides
scripts/quality/   Diff-scoped quality gate
scripts/dev/       Developer helpers
installer/         Installer (Inno Setup) + INF
src/               C++ sources (PascalCase filenames; kebab-case folders)
_bmad/             BMad Method install (chat FR, generated docs EN)
_bmad-output/      Planning + implementation artifacts (including Correct Course)
```

Build trees are expected under `builds/`. Coding standards: [`conventions.md`](conventions.md) and [`contributing.md`](contributing.md).

## Development environment

| Role | Machine |
| --- | --- |
| Design / editing | macOS (Cursor) |
| Build, USB hardware tests, DAW checks (interim) | Windows 10/11 **64-bit** (Win10 still used for lab / Epic 5) |
| Community backend / Epic 6 | Windows **11** lab (planned) |

Language target: **C++17** usermode.

How contributors split macOS edit vs Windows validate: [`docs/dev/contributor-dual-machine-loop.md`](docs/dev/contributor-dual-machine-loop.md).

### Quality gate

```bash
python -m pip install -r scripts/quality/requirements.txt
python scripts/quality/lint-touched.py
```

## Name & trademarks

The repository is named **unitor-win64-driver** on purpose:

- **Unitor** refers to the product *family* sharing one protocol architecture, without putting the **Emagic** trademark in the project name (Emagic was acquired by Apple in 2002).
- **win64** states the platform clearly.
- **driver** is the word users search for, even though the implementation is WinUSB + usermode bridge + virtual MIDI — not a classic kernel MIDI driver.

Emagic, Unitor, AMT, MT4, and related names remain trademarks of their respective owners. They appear here only for descriptive / interoperability purposes.

## License

Three separate claims — do not collapse them:

| Claim | Meaning |
| --- | --- |
| **MIT (this repo)** | Bridge sources, installer scripts, and project docs under [`LICENSE`](LICENSE) — copyright Guillaume DUPONT / **Ten Square Software** |
| **virtualMIDI (proprietary)** | Tobias Erichsen’s driver / SDK — **not** covered by this MIT license. **Interim lab / personal** backend only. Eval typically means a pre-installed [loopMIDI / rtpMIDI](https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html) (or any install that provides `teVirtualMIDI.dll`). **Do not** redistribute Bridge/Setup binaries that use the SDK as a community release. No virtualMIDI MSI embed. |
| **Windows MIDI Services (next community)** | Planned **Win11-only** community backend (Epic 6) behind the same `MidiBackend` abstraction — the intended path for public ready-to-run binaries without depending on the proprietary virtualMIDI SDK |

**Also:**

- Linux `sound/usb/midi.c` and `quirks-table.h` (`QUIRK_MIDI_EMAGIC`) are **reference only**. **No GPL Linux sources are vendored** in this tree.
- [aaron1a12/virtual-midi](https://github.com/aaron1a12/virtual-midi) is an **integration existence proof** only (GPL + vendored SDK) — **not** a fork base.
- WinUSB and Windows MIDI Services remain under Microsoft’s licensing for the OS components you use.

Deep page: [`docs/dev/license-and-backends.md`](docs/dev/license-and-backends.md). Signing policy (no certificate in this line): [`docs/dev/authenticode-and-smartscreen.md`](docs/dev/authenticode-and-smartscreen.md).

## Contributing

See [`contributing.md`](contributing.md) and the [dual-machine loop](docs/dev/contributor-dual-machine-loop.md). Issues and commit messages are in English. Hardware test reports (especially AMT8 / Unitor8) are welcome. Expect honest issues about install friction — that is preferred over marketing claims.

## Acknowledgments

- Linux `snd-usb-audio` Emagic quirk authors and maintainers
- Community threads that kept the problem visible for years
- [Prodikeys64](https://github.com/CrazyRedMachine/Prodikeys64) for the practical WinUSB + virtual MIDI pattern on Windows 64-bit
- Tobias Erichsen’s virtualMIDI ecosystem (used under its own license for interim lab work)
- Microsoft Windows MIDI Services (next community backend direction)
- [aaron1a12/virtual-midi](https://github.com/aaron1a12/virtual-midi) as integration existence proof only — not a fork base
