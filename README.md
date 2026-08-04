# unitor-win64-driver

Usermode Windows 64-bit bridge that makes Emagic Unitor-family USB MIDI interfaces work again on modern Windows — starting with the **MT4**.

There is no official 64-bit driver. The last vendor package targeted Windows XP (32-bit). Under Windows 10/11 the device enumerates over USB, but the stock class driver does not understand Emagic’s proprietary cable mapping, so DAWs never see usable MIDI ports.

This project aims to fix that **without writing a custom kernel driver**: WinUSB in user mode, an Emagic protocol layer, and virtual MIDI ports exposed to your DAW.

> **Status:** greenfield / early planning. No release binary yet. Architecture and MIDI backend choices are still being locked.

## Why this exists

Many musicians still own Emagic **Unitor8**, **AMT8**, or **MT4** interfaces and have been stuck for years after moving to 64-bit Windows. Related discussions:

- [Mod Wiggler — Emagic Unitor / AMT / MT4 drivers](https://www.modwiggler.com/forum/viewtopic.php?t=142226)
- [Gearspace — Emagic AMT8 drivers](https://gearspace.com/board/music-computers/141084-emagic-amt-8-drivers-2.html)
- [Cockos forum archive](https://forum.cockos.com/archive/index.php/t-191736.html)

## Supported hardware (planned)

| Model | USB VID | USB PID | Physical I/O (MVP intent) | Status |
| --- | --- | --- | --- | --- |
| **MT4** | `086A` | `0003` | 2 IN / 4 OUT | **V1 target** (hardware available) |
| AMT8 | `086A` | `0002` | 8 IN / 8 OUT (expected) | Post-MVP / needs test hardware |
| Unitor8 | `086A` | `0001` | 8 IN / 8 OUT (expected) | Post-MVP / needs test hardware |

USB composite identity for the MT4: `VID_086A&PID_0003`. Bus-powered (no external PSU).

The Linux kernel treats all three models with the same quirk (`QUIRK_MIDI_EMAGIC`) and the same endpoint info structure — only the cable bitmasks differ (`sound/usb/quirks-table.h`). This project will model that as declarative **device profiles** keyed by PID, with a single shared cable-mapping implementation.

## How it works (intended architecture)

Not a kernel-mode PortCls / WDM MIDI driver. The planned pipeline:

1. **USB transport** — bind the interface to Microsoft’s signed **WinUSB** (`winusb.sys`), via a minimal INF and/or [Zadig](https://zadig.akeo.ie/) during development.
2. **Protocol** — a C++ usermode service/app talks to the device through `winusb.dll` (bulk/interrupt) and reimplements Emagic cable multiplex / demultiplex (informed by the public Linux implementation in `sound/usb/midi.c`, not by shipping GPL sources as-is).
3. **DAW-facing MIDI** — expose virtual MIDI ports via either:
   - [teVirtualMIDI / VirtualMIDI SDK](https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html) (proprietary — redistribution terms must be cleared), or
   - **Windows MIDI Services** (native on recent Windows 11 builds — preferred for a clean open-source redistribution story if OS requirements are acceptable).

Architectural inspiration for the WinUSB + virtual MIDI pattern: [Prodikeys64](https://github.com/CrazyRedMachine/Prodikeys64).

### Why not a custom kernel driver?

A KMDF / PortCls stack needs the WDK, Microsoft attestation signing for Secure Boot machines, and specialized driver expertise. That is disproportionate for this problem and a major barrier to community installs. WinUSB keeps the signed kernel piece on Microsoft’s side.

## Planned deliverables

- INF associating Emagic VID/PID(s) with WinUSB (needed for simple end-user installs; Zadig may suffice for developers)
- Usermode C++ service/application (the real product)
- Optional installer packaging WinUSB binding + service + MIDI backend dependency
- Optional Authenticode signing of the usermode binary (recommended for SmartScreen trust; not a kernel signature)

## MVP scope (V1)

**In**

- MT4 only (`PID 0003`)
- Basic MIDI I/O: 2 inputs / 4 outputs
- Architecture ready for additional `DeviceProfile` entries (AMT8 / Unitor8 later)

**Out (for V1)**

- Patch mode, LTC/VITC, Fast Mode / AMT features from the Unitor8/AMT8 manuals
- Cascaded / stacked multi-interface setups (fragile even under Linux/ALSA)
- Guaranteed AMT8 / Unitor8 support without real hardware validation

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
docs/dev/          Development briefs and process docs
scripts/quality/   Diff-scoped Clean Code quality gate
scripts/dev/       Developer helpers (e.g. BMad Agents title resolver)
src/               C++ sources (PascalCase filenames; kebab-case folders)
_bmad/             BMad Method install (chat FR, generated docs EN)
```

Build trees are expected under `builds/`. Coding standards and the quality gate live in [`conventions.md`](conventions.md) and [`contributing.md`](contributing.md).

## Development environment

| Role | Machine |
| --- | --- |
| Design / editing | macOS (Cursor) |
| Build, USB hardware tests, DAW checks | Windows 10/11 **64-bit** |

Language target: **C++17** usermode. Exact stack (pure C++ vs shared tooling such as JUCE) will be decided in architecture.

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

This project’s **own** source is released under the **[MIT License](LICENSE)**.

**Recommendation rationale (short):** MIT keeps redistribution and contribution friction low for a community hardware-support tool. Prefer an **original** protocol implementation informed by public Linux sources — do **not** copy GPL Linux kernel files into this tree, or the project would need GPL instead.

**Third-party caveats (important):**

- Linux kernel sources remain GPL — use them as reference, not as vendored code.
- If VirtualMIDI SDK is selected, its **proprietary** terms apply separately and may restrict redistribution even though this repo is MIT.
- WinUSB / Windows MIDI Services remain under Microsoft’s licensing for the OS components you use.

## Contributing

See [`contributing.md`](contributing.md). Issues and commit messages are in English. The product is still early — design discussions and hardware test reports (especially AMT8 / Unitor8) are particularly welcome once development starts.

## Acknowledgments

- Linux `snd-usb-audio` Emagic quirk authors and maintainers
- Community threads that kept the problem visible for years
- [Prodikeys64](https://github.com/CrazyRedMachine/Prodikeys64) for the practical WinUSB + virtual MIDI pattern on Windows 64-bit
- Tobias Erichsen’s VirtualMIDI ecosystem (if/when used under its own license)
