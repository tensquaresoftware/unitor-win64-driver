---
organization: Ten Square Software
project: unitor-win64-driver
title: Unitor MT4 Bridge — Windows 10 user guide
author: Guillaume DUPONT
created: 2026-08-21
updated: 2026-08-21
version: "1.1"
product_version: "0.1.0"
---

# Unitor MT4 Bridge — Windows 10

**Unitor MT4 Bridge** lets you use an **Emagic MT4** MIDI interface on **Windows 64-bit**. Emagic no longer ships an official driver for modern Windows: the last Emagic Windows driver only supports **32-bit** systems (Windows 98 / 2000 / XP).

This open-source project (Ten Square Software) is a community **hobby** effort: free sources on GitHub, **no** paid code-signing certificate, and **no** promise that installation alone always succeeds on every brand-new PC.

## What this version covers

- In this **first version (v1)**, only the **MT4** is guaranteed to work: it is the model the developer has tested and documented.
- In a later **v2**, support for **AMT8**, **Unitor8**, and **Unitor8 mk2** may follow — depending on community interest and user demand. Those models are **not** promised in v1.

## Two editions of the Bridge

| Edition | Windows | How MIDI reaches your software | Installation |
|---|---|---|---|
| **This guide** | **Windows 10** | **virtualMIDI** (by Tobias Erichsen) — install it yourself | More technical |
| Other guide | **Windows 11** | **Windows MIDI Services** (built in) | Relatively simple |

This Windows 10 guide is for people who stay on Windows 10 (or who deliberately choose virtualMIDI). It is **not** the simplest edition: on Windows 11, prefer the **Windows MIDI Services** edition if you can.

virtualMIDI is **never** redistributed with the Bridge (rights & licence). You download and install it yourself from Tobias Erichsen’s website (see below).

Want to install and use the Bridge on Windows 11? Open the [Windows 11 guide](unitor-mt4-bridge-win11-wms-user-guide.md). French: [`unitor-mt4-bridge-win10-virtualmidi-guide-utilisateur.md`](unitor-mt4-bridge-win10-virtualmidi-guide-utilisateur.md). Choice page: [`README.md`](README.md).

## Contents

1. [Check the prerequisites](#check-the-prerequisites)
2. [Install virtualMIDI](#install-virtualmidi)
3. [Install the Bridge](#install-the-bridge)
4. [Pass the Windows SmartScreen warning](#pass-the-windows-smartscreen-warning)
5. [Use automatic start](#use-automatic-start)
6. [Read the MT4 front-panel lights](#read-the-mt4-front-panel-lights)
7. [Try your first MIDI notes](#try-your-first-midi-notes)
8. [Try your first SysEx transfer](#try-your-first-sysex-transfer)
9. [Fix common problems](#fix-common-problems)
10. [Unplug and replug the MT4](#unplug-and-replug-the-mt4)
11. [Know what works and what does not](#know-what-works-and-what-does-not)
12. [Use two MT4 interfaces](#use-two-mt4-interfaces)
13. [Glossary](#glossary)

---

# Check the prerequisites

| Need | Detail |
|---|---|
| Computer | **Windows 10**, **64-bit** (this edition of the Bridge also runs on Windows 11 if you choose the virtualMIDI installer) |
| Hardware | An **Emagic MT4** MIDI interface |
| Extra MIDI driver | **virtualMIDI**, installed **by you** — see below |

## Software you may use later

For everyday checks, open **your usual music software** (your DAW) — for example Ableton Live, Cubase, Reaper, Bitwig, etc. Any editor that can send and receive MIDI notes, controllers (CC), or even SysEx is fine for a first try in your home studio.

# Install virtualMIDI

In this Windows 10 edition, the Bridge talks to your DAW through **virtualMIDI**. Install it from Tobias Erichsen’s website **before** the Bridge installer, for example by downloading:

- **[loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html)** (most common), or
- **[rtpMIDI](https://www.tobias-erichsen.de/software/rtpmidi.html)**

Then confirm that the file `teVirtualMIDI.dll` exists in `C:\Windows\System32\`.

**Why install this software yourself?** virtualMIDI is proprietary. This MIT repository does **not** have redistribution clearance and will **never** ship the DLL or installer package inside the Bridge download.

Without the DLL, the Bridge installer **fails clearly**. If your DAW then shows no `MT4 In` / `MT4 Out` ports, that is **not** a successful install: the MIDI ports were not created (often because virtualMIDI is missing). Fix the prerequisite instead of treating an empty port list as OK.

# Install the Bridge

Download name looks like: `unitor-mt4-bridge-{version}-win10-virtualmidi-setup.exe` (from this project’s GitHub **Releases**).

1. Plug in the **MT4** before or while running the installer — the wizard does **not** pause to ask you to plug it in.
2. Run the installer you downloaded.
3. Accept the usual folder under Program Files (`Ten Square Software\Unitor MT4 Bridge`).
4. Allow Administrator **once**.
5. Wait for success — or incomplete if a check failed.

Success = DLL present + WinUSB OK + automatic start registered. If after install your DAW shows no `MT4 In` / `MT4 Out` ports, that is **not** success: the Bridge or the MIDI link is not really in place. Fix the problem instead of treating an empty port list as OK.

**One installer at a time:** the Windows 11 and Windows 10 installers share the **same Windows product identity**. Installing one **replaces** the other under Program Files and rewires automatic start to that edition.

**WinUSB** is already in Windows. On a brand-new PC, Windows often refuses to associate the MT4 with that driver on its own, because there is no signed **trusted catalog** — this project does not provide one.

In that case, use the free tool **[Zadig](https://zadig.akeo.ie/)**:

1. Plug in the MT4 and launch Zadig.
2. In the list, select the MT4 composite MIDI interface named **MI_02** (USB id `VID_086A&PID_0003&MI_02`) — not some other random USB line.
3. Choose the **WinUSB** driver, then confirm the install.
4. Re-run the Bridge installer if needed.

# Pass the Windows SmartScreen warning

Windows may show **Microsoft Defender SmartScreen** (“Windows protected your PC”). That can happen when a file is **unsigned**, or signed but **not yet trusted by reputation**. A warning does **not** automatically mean malware.

**Only continue if you downloaded the installer from this project’s Releases.**

To check whether **this** file is signed: right-click → **Properties** → **Digital Signatures**. If that tab is missing, the file is typically **unsigned**. For this community project, the developer chose **not to buy** an Authenticode certificate, whose yearly cost is very high.

When your PC’s policy allows it:

1. Choose **More info**.
2. Choose **Run anyway**.

Alternate: right-click the installer → **Properties** → enable **Unblock** if shown, then Apply / OK, and run it again.

Do **not** turn SmartScreen off globally or run copies from untrusted mirrors.

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

| Direction | Names |
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

## Use several applications at once

With **virtualMIDI**, the same port can usually be opened by **about eight applications** at once (DAW + editor + MIDI monitor, and so on).

If a program refuses to open a port that is already in use, or shows an error such as “exclusive”, “exclusive mode”, or “device in use”:

1. Open the **MIDI preferences** (or **MIDI devices**) of **each** application involved.
2. Look for an option often named **Exclusive**, **Exclusive Mode**, or **Exclusive access** on the `MT4 In` / `MT4 Out` input or output.
3. **Clear** that checkbox, confirm, then reopen the ports or restart the application.

The exact menu depends on the software (Ableton Live, Cubase, Reaper, Bitwig, etc.), but the idea is always the same: do not reserve the port for a single application only.

# Try your first SysEx transfer

1. Bridge running; `MT4 In` / `MT4 Out` visible.
2. Send a short ordinary MIDI message for Computer Mode.
3. In **your usual DAW** or synthesizer editor, select the matching ports and complete a short SysEx exchange (for example a patch dump / restore if you use a MIDI editor such as [Matrix-Control](https://github.com/tensquaresoftware/matrix-control), by Ten Square Software).

# Fix common problems

## virtualMIDI missing

Install loopMIDI or rtpMIDI, confirm `teVirtualMIDI.dll` in System32, re-run the Bridge installer. This project will **never** embed the DLL owned by Tobias Erichsen.

## SmartScreen, WinUSB, no ports, no SysEx

Use the same checks as in the Windows 11 guide, stated simply:

1. SmartScreen warning → **Run anyway** only if the file comes from this project’s Releases.
2. USB association → Zadig on **MI_02** if Windows refuses the package for lack of a **trusted catalog**.
3. Ports missing → refresh the MIDI list in the DAW, confirm the Bridge is running.
4. SysEx with no reply → wake **Computer Mode** first with ordinary MIDI.

On Windows 10, if you are comfortable with the computer, you can also check a few technical details (PowerShell, Windows services). That is **not** required for most musicians, and it is **not** the main recovery recommended on Windows 11 (where a simple PC reboot is often enough).

## Ports missing after unplug / replug

Wait, rescan MIDI, or quit/relaunch the Bridge. A Windows reboot is **not** the normal unplug/replug fix.

# Unplug and replug the MT4

Unplug → ports for that unit disappear. Replug → Bridge recreates them. Rescan or supervised Bridge restart = OK. Requiring reboot for ordinary unplug/replug = fail.

# Know what works and what does not

## What works

- This Windows 10 edition with **user-installed** virtualMIDI
- Same WinUSB / automatic start / MIDI / SysEx / unplug-replug flow as Windows 11 once prerequisites are met

## What this edition does not do

- Ship or embed virtualMIDI packages or DLLs
- Present Windows 10 as the simplest edition (prefer Windows 11 + Windows MIDI Services if you can)
- Installer-alone WinUSB on every clean PC without guided steps / **trusted catalog**
- Authenticode certificate / silent SmartScreen
- Patch mode, LTC/VITC, Fast Mode / AMT, cascaded topologies, guaranteed AMT8 / Unitor8 / Unitor8 mk2 in v1, custom kernel MIDI driver

See [license-and-backends.md](../dev/license-and-backends.md).

# Use two MT4 interfaces

First unit: `MT4 In N` / `MT4 Out N`. Later units: `MT4 #2 …`. Single-unit daily use is the proven path documented here.

# Glossary

| Term | Full name | Plain meaning |
|---|---|---|
| **MT4** | Emagic MT4 | Four-port MIDI interface this v1 Bridge targets |
| **Bridge** | Unitor MT4 Bridge | The Windows program that connects the MT4 to your DAW |
| **DAW** | Digital Audio Workstation | Your usual music software (Ableton Live, Cubase, Reaper, Bitwig, …) |
| **virtualMIDI** | Tobias Erichsen MIDI driver | Creates virtual ports for the DAW — **you** install it; this project never ships it |
| **teVirtualMIDI.dll** | virtualMIDI system library | File under `System32` that proves the driver is installed |
| **loopMIDI** / **rtpMIDI** | Tobias Erichsen apps | Common ways to get virtualMIDI installed |
| **WinUSB** | Windows USB user-mode driver | How Windows talks to the MT4 over USB once associated |
| **Zadig** | Third-party association helper | Tool used when the installer cannot associate WinUSB on a clean PC |
| **SmartScreen** | Microsoft Defender SmartScreen | Windows warning on unsigned or little-known downloads — use **Run anyway** only for this project’s Releases |
| **Trusted catalog** | Signed driver catalog Windows trusts | Missing here (no paid certificate) — guided Zadig is the hobby workaround |
| **SysEx** | System Exclusive | MIDI messages used for synth dumps / editor sessions |
| **Computer Mode** | Emagic MT4 behaviour | MT4 “awake for computer” state — wake it with ordinary MIDI before SysEx |
| **Windows MIDI Services** | Microsoft MIDI stack on Windows 11 | Used by the **other** edition — not required on this Windows 10 edition |
| **MI_02** | USB composite interface | The correct line to pick in Zadig for the MIDI part of the MT4 |

---

Yet another project successfully completed with [BMad](https://github.com/bmad-code-org/bmad-method)!
