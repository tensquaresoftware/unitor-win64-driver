---
organization: Ten Square Software
project: unitor-win64-driver
title: Unitor MT4 Bridge — Windows 11 user guide
author: Guillaume DUPONT
created: 2026-08-21
updated: 2026-08-21
version: "1.1"
product_version: "0.1.0"
---

# Unitor MT4 Bridge — Windows 11

**Unitor MT4 Bridge** lets you use an **Emagic MT4** MIDI interface on **Windows 64-bit**. Emagic no longer ships an official driver for modern Windows: the last Emagic Windows driver only supports **32-bit** systems (Windows 98 / 2000 / XP).

This open-source project (Ten Square Software) is a community **hobby** effort: free sources on GitHub, **no** paid code-signing certificate, and **no** promise that installation alone always succeeds on every brand-new PC.

## What this version covers

- In this **first version (v1)**, only the **MT4** is guaranteed to work: it is the model the developer has tested and documented.
- In a later **v2**, support for **AMT8**, **Unitor8**, and **Unitor8 mk2** may follow — depending on community interest and user demand. Those models are **not** promised in v1.

## Two editions of the Bridge

| Edition | Windows | How MIDI reaches your software | Installation |
|---|---|---|---|
| **This guide** | **Windows 11** | **Windows MIDI Services** (built in) | Relatively simple |
| Other guide | **Windows 10** | **virtualMIDI** (by Tobias Erichsen) — install it yourself | More technical |

This Windows 11 edition is usually the simplest if your PC supports it. virtualMIDI is **never** redistributed with the Bridge (rights & licence). The Windows 10 guide explains why and how (see that guide for the download).

Want to install and use the Bridge on Windows 10? Open the [Windows 10 guide](unitor-mt4-bridge-win10-virtualmidi-user-guide.md). French: [`unitor-mt4-bridge-win11-wms-guide-utilisateur.md`](unitor-mt4-bridge-win11-wms-guide-utilisateur.md). Choice page: [`README.md`](README.md).

## Contents

1. [Check the prerequisites](#check-the-prerequisites)
2. [Install the Bridge](#install-the-bridge)
3. [Pass the Windows SmartScreen warning](#pass-the-windows-smartscreen-warning)
4. [Use automatic start](#use-automatic-start)
5. [Read the MT4 front-panel lights](#read-the-mt4-front-panel-lights)
6. [Try your first MIDI notes](#try-your-first-midi-notes)
7. [Try your first SysEx transfer](#try-your-first-sysex-transfer)
8. [Fix common problems](#fix-common-problems)
9. [Recover when MIDI is stuck](#recover-when-midi-is-stuck)
10. [Unplug and replug the MT4](#unplug-and-replug-the-mt4)
11. [Know what works and what does not](#know-what-works-and-what-does-not)
12. [Use two MT4 interfaces](#use-two-mt4-interfaces)
13. [Glossary](#glossary)

---

# Check the prerequisites

| Need | Detail |
|---|---|
| Computer | **Windows 11**, **64-bit**, with **Windows MIDI Services** available |
| Hardware | An **Emagic MT4** MIDI interface |
| Extra MIDI driver | **Not required** in this edition — you do **not** install virtualMIDI |

If Windows MIDI Services is missing, the Bridge **refuses** to pretend everything is fine when no MIDI ports appear. Update Windows, or use the Windows 10 edition if that is your machine.

## Software you may use later

For everyday checks, open **your usual music software** (your DAW) — for example Ableton Live, Cubase, Reaper, Bitwig, etc. Any editor that can send and receive MIDI notes, controllers (CC), or even SysEx is fine for a first try in your home studio.

# Install the Bridge

Download name looks like: `unitor-mt4-bridge-{version}-win11-wms-setup.exe` (from this project’s GitHub **Releases** page — not a random mirror).

1. Plug in the **MT4** (power + USB) **before** or while running the installer — the wizard does **not** pause to ask you to plug it in.
2. Run the installer you downloaded.
3. Accept the suggested folder, usually:

   `C:\Program Files\Ten Square Software\Unitor MT4 Bridge\`

4. Allow the Administrator prompt **once** (Program Files and USB association).
5. Wait for a clear success message — or an incomplete install if a check failed (that is **not** a successful install).

Install succeeds when Windows 11 with Windows MIDI Services is available, USB association (WinUSB) is OK, and automatic start is registered for your Windows user account. If after install your DAW shows no `MT4 In` / `MT4 Out` ports, that is **not** a successful install: the Bridge or the MIDI link is not really in place. Fix the problem instead of treating an empty port list as OK.

**One installer at a time:** the Windows 11 and Windows 10 installers share the **same Windows product identity**. Installing one **replaces** the other under Program Files and rewires automatic start to that edition. Do not expect both editions side-by-side on one PC.

**WinUSB** is already in Windows. On a brand-new PC, Windows often refuses to associate the MT4 with that driver on its own, because there is no signed **trusted catalog** — this project does not provide one.

If the installer’s WinUSB step **succeeds**, you are done for USB. If that step **fails** (common on a clean PC), follow [Associate the MT4 with WinUSB](#associate-the-mt4-with-winusb) — guided association with **Zadig** is the supported fix.

# Pass the Windows SmartScreen warning

Windows may show **Microsoft Defender SmartScreen** (“Windows protected your PC”). That can happen when a file is **unsigned**, or signed but **not yet trusted by reputation**. A warning does **not** automatically mean malware.

**Only continue if you downloaded the installer from this project’s Releases.**

To check whether **this** file is signed: right-click → **Properties** → **Digital Signatures**. If that tab is missing, the file is typically **unsigned**. For this community project, the developer chose **not to buy** an Authenticode certificate, whose yearly cost is very high.

When your PC’s policy allows it:

1. Choose **More info**.
2. Choose **Run anyway**.

Alternate: right-click the installer → **Properties** → enable **Unblock** if shown, then Apply / OK, and run it again.

Do **not** turn SmartScreen off globally or run copies from untrusted mirrors. On managed PCs, policy may block **Run anyway** — try a personal machine or ask your admin.

# Use automatic start

After installation, **Unitor MT4 Bridge** starts on its own when you sign in to Windows. It is **not** a Windows service running in the background for every account: it is a program tied to **your user session**.

Once you are signed in (or after you plug in the MT4), Windows should show ports **`MT4 In 1`**, **`MT4 In 2`**, **`MT4 Out 1`** … **`MT4 Out 4`**. You can also start **Unitor MT4 Bridge** by hand from the Start menu.

To **turn off** automatic start: open the Bridge folder in the Start menu and choose **Unregister Auto-Start**. Uninstalling also removes automatic start, but only for the Windows user who runs the uninstall.

# Read the MT4 front-panel lights

On the front of the MT4, the lights help you see what is going on:

| Light | Colour | What it means in practice |
|---|---|---|
| **MIDI In** | Red | Blinks when the MT4 **receives** MIDI (from the computer or a plugged-in device) |
| **MIDI Out** | Green | Blinks when the MT4 **sends** MIDI |
| **Patch** | Red | Related to the MT4 Patch mode (outside everyday use of this v1 Bridge) |
| **USB** | Orange | Usually means the MT4 is powered over USB and the link with the computer / Bridge is active |

If the orange **USB** light stays off while the cable is plugged in, check power, the USB cable, then that the Bridge is running. The **MIDI In** / **MIDI Out** lights are useful during your note or SysEx tests.

# Try your first MIDI notes

| Direction | Names in Windows |
|---|---|
| Inputs | `MT4 In 1`, `MT4 In 2` |
| Outputs | `MT4 Out 1` … `MT4 Out 4` |

1. Confirm [Install the Bridge](#install-the-bridge) and [Use automatic start](#use-automatic-start) (or launch **Unitor MT4 Bridge** once).
2. Plug in the MT4.
3. Open **your usual DAW**.
4. Enable at least **`MT4 In 1`** and **`MT4 Out 1`**.
5. Send notes (or controllers) and check the other direction.

### Wake Computer Mode

The MT4 has a state called **Computer Mode**: it must “wake up” before it talks cleanly to the computer. To wake it, first send some **ordinary MIDI** — for example a few notes, or a short controller (CC) move — from your DAW to an **`MT4 Out`** port.

Only then start a SysEx exchange. **SysEx alone does not wake** Computer Mode: if you begin with a dump right away, the MT4 may stay silent.

# Try your first SysEx transfer

1. Bridge running; `MT4 In` / `MT4 Out` visible.
2. Send a short ordinary MIDI message for Computer Mode.
3. In **your usual DAW** or synthesizer editor, select the matching ports and complete a short SysEx exchange (for example a patch dump / restore if you use a MIDI editor such as [Matrix-Control](https://github.com/tensquaresoftware/matrix-control), by Ten Square Software).

# Fix common problems

## Windows MIDI Services is missing

If the installer refuses to start, or ports stay empty after what looked like a good day:

1. Close your DAW and quit **Unitor MT4 Bridge**.
2. **Reboot** the PC.
3. After sign-in, open **Settings → Windows Update** and install pending updates (Windows MIDI Services often arrives with OS updates).
4. If Microsoft shows an optional **Windows MIDI Services** / MIDI enablement step on your build, follow that screen — **no** PowerShell Admin is required for this musician edition.
5. Run the Windows 11 installer again, or launch the Bridge once and rescan MIDI in the DAW.

Still on Windows 10 hardware? Use the [Windows 10 guide](unitor-mt4-bridge-win10-virtualmidi-user-guide.md) instead.

## Associate the MT4 with WinUSB

The installer reports that it could not associate the MT4 with WinUSB; install is often rolled back.

1. Plug in the MT4 and launch **[Zadig](https://zadig.akeo.ie/)**.
2. In the list, select the MT4 composite MIDI interface named **MI_02** (USB id `VID_086A&PID_0003&MI_02`) — not some other random USB line.
3. Choose the **WinUSB** driver, then confirm the install.
4. Re-run the Bridge installer if it rolled back, then launch **Unitor MT4 Bridge**.

On a clean PC, Windows often refuses the package for lack of a **trusted catalog** — Zadig is the supported workaround.

## No ports visible

Use the same simple checks:

1. Bridge running (automatic start or manual launch).
2. MT4 plugged in.
3. Windows MIDI Services available.
4. MIDI device rescan in your DAW.

## No SysEx reply

Send controllers or notes first for **Computer Mode**, then retry SysEx.

# Recover when MIDI is stuck

If MIDI looks stuck after you stop the Bridge or after a messy quit, use this **simple** recovery — **no** PowerShell, **no** Administrator terminal:

1. Close your DAW (and other MIDI apps).
2. Quit **Unitor MT4 Bridge** if it is still running.
3. **Reboot** the PC.
4. After sign-in, launch **Unitor MT4 Bridge** **once** (or rely on automatic start), then open the DAW.

# Unplug and replug the MT4

1. Unplug: that interface’s ports disappear.
2. Plug back in: the Bridge recreates the ports.
3. If needed, rescan MIDI in your software, or quit and relaunch the Bridge, then rescan.

**OK recovery:** host MIDI rescan and/or a supervised Bridge restart.  
**Not the normal unplug/replug fix:** a full Windows reboot (reboot is reserved for the stuck-MIDI case above).

# Know what works and what does not

## What works in this Windows 11 edition

- Install **without** virtualMIDI
- WinUSB via the installer when Windows accepts the package; otherwise guided Zadig
- 2 inputs / 4 outputs as `MT4 In` / `MT4 Out`
- Notes, controllers, clock / transport, MTC, SysEx at editor scale
- Automatic start without daily Administrator
- Unplug/replug with rescan / Bridge relaunch
- Second MT4 naming when you have one (see below)

## What this edition does not do

- Installer-alone WinUSB success on every clean PC without guided steps / **trusted catalog**
- Embedding or shipping virtualMIDI
- A paid Authenticode certificate / silent SmartScreen
- Promising Windows 10 on this Windows MIDI Services edition (use the Windows 10 guide)
- Patch mode, LTC/VITC, Fast Mode / AMT features, cascaded Emagic topologies
- Guaranteed AMT8 / Unitor8 / Unitor8 mk2 in v1
- A custom kernel MIDI driver

Deeper licence notes for contributors: [license-and-backends.md](../dev/license-and-backends.md).

# Use two MT4 interfaces

| Interface | Port names |
|---|---|
| First | `MT4 In N` / `MT4 Out N` |
| Second and later | `MT4 #2 In N` / `MT4 #2 Out N`, and so on |

**Honesty:** daily use for a **single** physical MT4 is the proven path documented here. Dual naming exists; do not assume a closed dual-unit lab claim from this guide alone.

# Glossary

| Term | Full name | Plain meaning |
|---|---|---|
| **MT4** | Emagic MT4 | Four-port MIDI interface this v1 Bridge targets |
| **Bridge** | Unitor MT4 Bridge | The Windows program that connects the MT4 to your DAW |
| **DAW** | Digital Audio Workstation | Your usual music software (Ableton Live, Cubase, Reaper, Bitwig, …) |
| **Windows MIDI Services** | Microsoft MIDI stack on Windows 11 | Built-in way this edition creates the `MT4 In` / `MT4 Out` ports — **no** virtualMIDI install |
| **WinUSB** | Windows USB user-mode driver | How Windows talks to the MT4 over USB once associated |
| **Zadig** | Third-party association helper | Tool used when the installer cannot associate WinUSB on a clean PC |
| **SmartScreen** | Microsoft Defender SmartScreen | Windows warning on unsigned or little-known downloads — use **Run anyway** only for this project’s Releases |
| **Trusted catalog** | Signed driver catalog Windows trusts | Missing here (no paid certificate) — guided Zadig is the hobby workaround |
| **SysEx** | System Exclusive | MIDI messages used for synth dumps / editor sessions |
| **Computer Mode** | Emagic MT4 behaviour | MT4 “awake for computer” state — wake it with ordinary MIDI before SysEx |
| **Automatic start** | Auto-Start | Bridge launches with your Windows user session |
| **virtualMIDI** | Tobias Erichsen driver | Used on the **Windows 10** edition only — not part of this Windows 11 edition |
| **MI_02** | USB composite interface | The correct line to pick in Zadig for the MIDI part of the MT4 |

---

Yet another project successfully completed with [BMad](https://github.com/bmad-code-org/bmad-method)!
