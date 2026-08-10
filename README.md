# unitor-win64-driver

Usermode Windows 64-bit bridge that makes Emagic Unitor-family USB MIDI interfaces work again on modern Windows — starting with the **MT4**.

There is no official 64-bit driver. The last vendor package targeted Windows XP (32-bit). Under Windows 10/11 the device enumerates over USB, but the stock class driver does not understand Emagic’s proprietary cable mapping, so DAWs never see usable MIDI ports.

This project fixes that **without writing a custom kernel driver**: WinUSB in user mode, an Emagic protocol layer, and virtual MIDI ports (virtualMIDI) exposed to your DAW via **Unitor MT4 Bridge** (Ten Square Software).

> **Status:** V1 Bridge capabilities (ports, SysEx, Auto-Start, hot-plug, multi-client) are implemented for **MT4**. End-user docs ship under [`docs/user/README.md`](docs/user/README.md). Public Installer work continues (Story **4.1** — hardware smoke rows may still be blank). License / backend honesty for community readers is documented (Story **4.3** — see [License](#license)). Authenticode is **strongly recommended** but **not** a hard V1 gate; SmartScreen honesty for unsigned / low-reputation Setup ships in the [user guide](docs/user/unitor-mt4-bridge-user-guide.md#windows-smartscreen-unsigned-or-unrecognized-setup) and [`docs/dev/authenticode-and-smartscreen.md`](docs/dev/authenticode-and-smartscreen.md). Latency “studio-done” numbers are Epic 5.

## Start here (community users)

If you own an **MT4** and want first MIDI / first SysEx on Windows 10/11 x64:

**→ [`docs/user/README.md`](docs/user/README.md)** — then the [English user guide](docs/user/unitor-mt4-bridge-user-guide.md) or the [manuel français](docs/user/unitor-mt4-bridge-manuel-utilisateur.md) (install, Auto-Start, first MIDI, first SysEx, troubleshooting).

## Why this exists

Many musicians still own Emagic **Unitor8**, **AMT8**, or **MT4** interfaces and have been stuck for years after moving to 64-bit Windows. Related discussions:

- [Mod Wiggler — Emagic Unitor / AMT / MT4 drivers](https://www.modwiggler.com/forum/viewtopic.php?t=142226)
- [Gearspace — Emagic AMT8 drivers](https://gearspace.com/board/music-computers/141084-emagic-amt-8-drivers-2.html)
- [Cockos forum archive](https://forum.cockos.com/archive/index.php/t-191736.html)

## Supported hardware

| Model | USB VID | USB PID | Physical I/O (MVP intent) | Status |
| --- | --- | --- | --- | --- |
| **MT4** | `086A` | `0003` | 2 IN / 4 OUT | **V1 target** (hardware available) |
| AMT8 | `086A` | `0002` | 8 IN / 8 OUT (expected) | Post-MVP / needs test hardware |
| Unitor8 | `086A` | `0001` | 8 IN / 8 OUT (expected) | Post-MVP / needs test hardware |

USB composite identity for the MT4: `VID_086A&PID_0003`. Bus-powered (no external PSU).

The Linux kernel treats all three models with the same quirk (`QUIRK_MIDI_EMAGIC`) and the same endpoint info structure — only the cable bitmasks differ (`sound/usb/quirks-table.h`). This project models that as declarative **device profiles** keyed by PID, with a single shared cable-mapping implementation.

## How it works

Not a kernel-mode PortCls / WDM MIDI driver. The pipeline:

1. **USB transport** — bind the interface to Microsoft’s signed **WinUSB** (`winusb.sys`) via the Public Installer / project INF (Zadig is contributor fallback only).
2. **Protocol** — a C++ usermode Bridge talks to the device through `winusb.dll` and reimplements Emagic cable multiplex / demultiplex (informed by the public Linux implementation in `sound/usb/midi.c`, not by shipping GPL sources as-is).
3. **DAW-facing MIDI** — virtual MIDI ports via [teVirtualMIDI / virtualMIDI](https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html) (eval via loopMIDI / rtpMIDI). Windows MIDI Services remains a possible future Win11-only backend — not V1.

Architectural inspiration for the WinUSB + virtual MIDI pattern: [Prodikeys64](https://github.com/CrazyRedMachine/Prodikeys64).

### Why not a custom kernel driver?

A KMDF / PortCls stack needs the WDK, Microsoft attestation signing for Secure Boot machines, and specialized driver expertise. That is disproportionate for this problem and a major barrier to community installs. WinUSB keeps the signed kernel piece on Microsoft’s side.

## Deliverables

- Public Installer (`UnitorMt4Bridge-Setup.exe`) — WinUSB association, Bridge, Auto-Start, virtualMIDI presence gate
- Usermode C++ Bridge (`Bridge.exe`)
- End-user docs: [`docs/user/README.md`](docs/user/README.md)
- Optional Authenticode signing of the usermode binary / Setup — **strongly recommended**, not a hard V1 gate; policy + SmartScreen honesty: [`docs/dev/authenticode-and-smartscreen.md`](docs/dev/authenticode-and-smartscreen.md)

## MVP scope (V1)

**In**

- MT4 only (`PID 0003`)
- Basic MIDI I/O: 2 inputs / 4 outputs as `MT4 In N` / `MT4 Out N`
- Architecture ready for additional `DeviceProfile` entries (AMT8 / Unitor8 later)

**Out (for V1)**

- Patch mode, LTC/VITC, Fast Mode / AMT features from the Unitor8/AMT8 manuals
- Cascaded / stacked multi-interface setups (fragile even under Linux/ALSA)
- Guaranteed AMT8 / Unitor8 support without real hardware validation

Capabilities overview: [What works / what does not](docs/user/unitor-mt4-bridge-user-guide.md#what-works--what-does-not) in the user guide.

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

## Project layout (current)

```
docs/user/         End-user manual (single guide — start here for community installs)
docs/dev/          Contributor / process docs (WinUSB bind, license honesty, Authenticode / SmartScreen, dual-machine loop)
docs/tests/        Operator smoke guides
scripts/quality/   Diff-scoped Clean Code quality gate
scripts/dev/       Developer helpers
installer/         Public Installer (Inno Setup) + INF
src/               C++ sources (PascalCase filenames; kebab-case folders)
_bmad/             BMad Method install (chat FR, generated docs EN)
```

Build trees are expected under `builds/`. Coding standards and the quality gate live in [`conventions.md`](conventions.md) and [`contributing.md`](contributing.md).

## Development environment

| Role | Machine |
| --- | --- |
| Design / editing | macOS (Cursor) |
| Build, USB hardware tests, DAW checks | Windows 10/11 **64-bit** (Win10 mandatory in the matrix) |

Language target: **C++17** usermode.

How contributors split macOS edit vs Windows validate (artifacts under `builds/`, CI vs lab Pass): [`docs/dev/contributor-dual-machine-loop.md`](docs/dev/contributor-dual-machine-loop.md).

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
| **virtualMIDI (proprietary)** | Tobias Erichsen’s driver / SDK — **not** covered by this MIT license. Eval = pre-installed [loopMIDI / rtpMIDI](https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html) (or any install that provides `teVirtualMIDI.dll`). Software that links the SDK (including distributing Bridge / Setup binaries built against it) must **not** be redistributed without prior clearance. Separately, the Public Installer does **not** embed a virtualMIDI MSI / merge module until that clearance (**OQ-1** = open redistributable-embed question) |
| **Windows MIDI Services (future)** | Optional later Win11-only backend behind the same `MidiBackend` abstraction (pluggable MIDI backend interface) — **not** V1. V1 ships virtualMIDI only |

**Also:**

- Linux `sound/usb/midi.c` and `quirks-table.h` (`QUIRK_MIDI_EMAGIC`) are **reference only**. **No GPL Linux sources are vendored** in this tree. Prefer an **original** protocol implementation informed by those public sources — do **not** copy GPL Linux kernel files into this tree, or the project would need GPL instead.
- [aaron1a12/virtual-midi](https://github.com/aaron1a12/virtual-midi) is an **integration existence proof** only (GPL + vendored SDK) — **not** a fork base for this project.
- WinUSB and Windows MIDI Services remain under Microsoft’s licensing for the OS components you use.

Deep page (OQ-1 detail, Catch2 note, facade): [`docs/dev/license-and-backends.md`](docs/dev/license-and-backends.md). Operator smoke: [`docs/tests/smoke-epic4-license-honesty-mt4.md`](docs/tests/smoke-epic4-license-honesty-mt4.md).

## Contributing

See [`contributing.md`](contributing.md) and the [dual-machine loop](docs/dev/contributor-dual-machine-loop.md). Issues and commit messages are in English. Hardware test reports (especially AMT8 / Unitor8) are welcome.

## Acknowledgments

- Linux `snd-usb-audio` Emagic quirk authors and maintainers
- Community threads that kept the problem visible for years
- [Prodikeys64](https://github.com/CrazyRedMachine/Prodikeys64) for the practical WinUSB + virtual MIDI pattern on Windows 64-bit
- Tobias Erichsen’s virtualMIDI ecosystem (used under its own license)
- [aaron1a12/virtual-midi](https://github.com/aaron1a12/virtual-midi) as integration existence proof only — not a fork base
