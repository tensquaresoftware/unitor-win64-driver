---
organization: Ten Square Software
project: unitor-win64-driver
title: Unitor MT4 Bridge — Windows 10 / virtualMIDI user guide
author: Guillaume DUPONT
created: 2026-08-21
updated: 2026-08-21
version: "1.0"
product_version: "0.1.0"
---

This guide is for **motivated Windows 10** users (and Win11 users who choose the virtualMIDI Setup) with `UnitorMt4Bridge-Setup-win10-virtualmidi-…`.

You **self-install** Tobias Erichsen’s **virtualMIDI** driver. This project **never** embeds or redistributes `teVirtualMIDI.dll`, the virtualMIDI MSI, or the SDK (OQ-1).

Prefer a simpler Win11 path without virtualMIDI? Use the [Win11 / Windows MIDI Services guide](unitor-mt4-bridge-win11-wms-user-guide.md). French: [`unitor-mt4-bridge-win10-virtualmidi-guide-utilisateur.md`](unitor-mt4-bridge-win10-virtualmidi-guide-utilisateur.md). Router: [`README.md`](README.md).

Hobby / open-source (Ten Square Software): **no** code-signing certificate; clean-PC WinUSB often needs a **guided** step when there is no **trusted catalog**.

## Contents

1. [Prerequisites](#prerequisites)
2. [Install virtualMIDI yourself](#install-virtualmidi-yourself)
3. [Installation (Bridge Setup)](#installation-bridge-setup)
4. [Auto-Start](#auto-start)
5. [First MIDI test](#first-midi-test)
6. [First SysEx test](#first-sysex-test)
7. [Troubleshooting](#troubleshooting)
8. [Unplug and replug the MT4](#unplug-and-replug-the-mt4)
9. [What works / what does not](#what-works--what-does-not)
10. [Two MT4 interfaces](#two-mt4-interfaces)

---

# Prerequisites

| Need | Detail |
|---|---|
| Computer | **Windows 10** or **Windows 11**, **64-bit** (this Setup targets the virtualMIDI path) |
| Hardware | An **Emagic MT4** |
| virtualMIDI | Already installed by **you** — see below |

## Useful software later on

| Purpose | Example |
|---|---|
| First MIDI test | Ableton Live 12 or MIDI-OX |
| First SysEx test | Matrix-Control, or MIDI-OX |

# Install virtualMIDI yourself

Today’s Win10 community Setup talks to DAWs through **virtualMIDI**. Install it from Tobias Erichsen **before** Bridge Setup, for example:

- **[loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html)** (most common), or
- **[rtpMIDI](https://www.tobias-erichsen.de/software/rtpmidi.html)**

Then confirm `teVirtualMIDI.dll` exists in `C:\Windows\System32\`.

**Why self-install?** The virtualMIDI SDK is proprietary. This MIT project does **not** have redistribution clearance (OQ-1) and will **never** ship the DLL or MSI inside Setup.

Without the DLL, Setup **fails closed** — an empty MIDI port list is not a successful install.

Operator helper (optional, advanced): `installer/check-virtualmidi.ps1` in the repo.

# Installation (Bridge Setup)

1. Plug in the **MT4** before or while running Setup — the wizard does **not** pause to ask you to plug it in.
2. Download `UnitorMt4Bridge-Setup-win10-virtualmidi-{version}.exe` from this project’s **Releases**.
3. Accept the usual folder under Program Files (`Ten Square Software\Unitor MT4 Bridge`).
4. Allow Administrator **once**.
5. Wait for success — or incomplete if a gate failed.

Setup succeeds when:

- `teVirtualMIDI.dll` is present,
- WinUSB association is OK,
- Auto-Start is registered.

**Same AppId note:** this Setup shares the same Windows product id as the Win11 WMS Setup. Installing one **replaces** the other under Program Files and rewires Auto-Start to that flavor’s MIDI backend.

## Windows SmartScreen (unsigned or unrecognized Setup)

Windows may show **Microsoft Defender SmartScreen** (“Windows protected your PC”). That can happen when a community build is **unsigned**, or signed but **not yet reputation-trusted**. A warning does **not** automatically mean malware.

**Only continue if you downloaded Setup from this project’s Releases.**

To check whether **this** Setup file is signed: right-click → **Properties** → **Digital Signatures**. If that tab is missing, the file is typically **unsigned**. This project **does not ship** an Authenticode certificate (OQ-3).

When your PC’s policy allows it:

1. Choose **More info**.
2. Choose **Run anyway**.

Alternate: right-click Setup → **Properties** → enable **Unblock** if shown, then Apply / OK, and run Setup again.

Do **not** turn SmartScreen off globally or run copies from untrusted mirrors. Policy: [`docs/dev/authenticode-and-smartscreen.md`](../dev/authenticode-and-smartscreen.md).

## WinUSB failed on a clean PC

Common cause: Windows rejects an unsigned driver package / missing **trusted catalog**. Guided **[Zadig](https://zadig.akeo.ie/)** association for **MI_02** (`USB\VID_086A&PID_0003&MI_02`) is the supported hobby fix. Contributor detail: [`docs/dev/winusb-bind.md`](../dev/winusb-bind.md).

# Auto-Start

Bridge starts with your user session (not a Windows Service). Look for `MT4 In` / `MT4 Out` ports after sign-in or plug-in. Use **Unregister Auto-Start** to disable. Uninstall clears Auto-Start for the user who uninstalls only.

# First MIDI test

Goal: send and receive notes (and optionally CC) between the MT4 and your software.

| Direction | Names |
|---|---|
| Inputs | `MT4 In 1`, `MT4 In 2` |
| Outputs | `MT4 Out 1` … `MT4 Out 4` |

1. Confirm [Installation](#installation-bridge-setup) and [Auto-Start](#auto-start) (or launch **Unitor MT4 Bridge** once).
2. Plug in the MT4.
3. Open Ableton Live 12 or MIDI-OX.
4. Enable at least **`MT4 In 1`** and **`MT4 Out 1`**.
5. Send notes or CC and check the other direction.

Send ordinary MIDI first for **Computer Mode** — SysEx alone does not wake it.

## Several applications at once

virtualMIDI typically allows about eight clients per port. Turn off exclusive MIDI mode if a host complains.

# First SysEx test

1. Bridge running; `MT4 In` / `MT4 Out` visible.
2. Send a short CC for Computer Mode.
3. In Matrix-Control (or MIDI-OX), select the matching virtual ports and complete a dump/restore (or short SysEx exchange).

The exchange should finish without restarting the Bridge for a normal librarian session.

# Troubleshooting

## virtualMIDI missing

Install loopMIDI or rtpMIDI, confirm `teVirtualMIDI.dll` in System32, re-run Setup. This project will **never** embed the DLL.

## SmartScreen / WinUSB / no ports / no SysEx

Same spine as the Win11 guide: SmartScreen honesty, Zadig for clean-PC WinUSB (no **trusted catalog**), rescan MIDI, Computer Mode before SysEx. Technical PowerShell checks (service status, DLL probe scripts) are acceptable on this motivated path — they are **not** the Win11 comfort-path primary recovery.

## Ports missing after unplug / replug

Wait, rescan MIDI, or quit/relaunch Bridge. A Windows reboot is **not** the normal hot-plug fix.

# Unplug and replug the MT4

Unplug → ports for that unit disappear. Replug → Bridge recreates them. Rescan or supervised Bridge restart = OK. Requiring reboot for ordinary hot-plug = fail.

# What works / what does not

## What works

- Parallel community Win10 path with **user-installed** virtualMIDI
- Same WinUSB / Auto-Start / MIDI / SysEx / hot-plug spine as Win11 once prerequisites are met
- Lab use of virtualMIDI on Win10/Win11

## What this does not do

- Ship or embed virtualMIDI MSI/SDK/DLL
- Claim Win10 as the **comfort** community promise (that is Win11 + WMS)
- Setup-alone WinUSB on every clean PC without guided steps / **trusted catalog**
- Authenticode certificate / silent SmartScreen
- Patch mode, LTC/VITC, Fast Mode / AMT, cascaded topologies, guaranteed AMT8/Unitor8, custom kernel MIDI driver

See [license-and-backends.md](../dev/license-and-backends.md).

# Two MT4 interfaces

First unit: `MT4 In N` / `MT4 Out N`. Later units: `MT4 #2 …`. Single-unit daily use is the proven path documented here.
