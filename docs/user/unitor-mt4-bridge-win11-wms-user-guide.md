---
organization: Ten Square Software
project: unitor-win64-driver
title: Unitor MT4 Bridge — Windows 11 / Windows MIDI Services user guide
author: Guillaume DUPONT
created: 2026-08-21
updated: 2026-08-21
version: "1.0"
product_version: "0.1.0"
---

This guide is for **Windows 11** musicians using the **Windows MIDI Services** community Setup (`UnitorMt4Bridge-Setup-win11-wms-…`). You do **not** need virtualMIDI.

On **Windows 10**, use the [Win10 / virtualMIDI guide](unitor-mt4-bridge-win10-virtualmidi-user-guide.md) instead. French: [`unitor-mt4-bridge-win11-wms-guide-utilisateur.md`](unitor-mt4-bridge-win11-wms-guide-utilisateur.md). Start here if unsure: [`README.md`](README.md).

This is a **hobby / open-source** project (Ten Square Software): free sources on GitHub, **no** code-signing certificate, and **no** promise that Setup alone always succeeds on every brand-new PC.

## Contents

1. [Prerequisites](#prerequisites)
2. [Installation](#installation)
3. [Auto-Start](#auto-start)
4. [First MIDI test](#first-midi-test)
5. [First SysEx test](#first-sysex-test)
6. [Troubleshooting](#troubleshooting)
7. [Sticky MIDI after stop / restart](#sticky-midi-after-stop--restart)
8. [Unplug and replug the MT4](#unplug-and-replug-the-mt4)
9. [What works / what does not](#what-works--what-does-not)
10. [Two MT4 interfaces](#two-mt4-interfaces)

---

# Prerequisites

| Need | Detail |
|---|---|
| Computer | **Windows 11**, **64-bit**, with **Windows MIDI Services** available |
| Hardware | An **Emagic MT4** MIDI interface |
| virtualMIDI | **Not required** for this path |

If Windows MIDI Services is missing, the Bridge **fails closed** (no empty “success” ports). Install / enable WMS on Windows 11, or use the Win10 virtualMIDI path if that is your machine.

## Useful software later on

| Purpose | Example |
|---|---|
| First MIDI test | Ableton Live 12 or MIDI-OX |
| First SysEx test | Matrix-Control, or MIDI-OX |

# Installation

1. Plug in the **MT4** (power + USB) **before** or while running Setup — the wizard does **not** pause to ask you to plug it in.
2. Download `UnitorMt4Bridge-Setup-win11-wms-{version}.exe` from this project’s **Releases** (not a random mirror).
3. Accept the suggested folder, usually:

   `C:\Program Files\Ten Square Software\Unitor MT4 Bridge\`

4. Allow the Administrator prompt **once** (Program Files and USB association).
5. Wait for **Installation successful** — or **Installation incomplete** if a gate failed (that is not a successful community install).

Setup succeeds when:

- Windows 11 with Windows MIDI Services is available (`midisrv`),
- USB association (WinUSB) is OK,
- Auto-Start is registered for your Windows user account.

virtualMIDI is **not** checked on this flavor. An empty MIDI port list after install is **not** success.

**Same AppId note:** this Setup shares the same Windows product id as the Win10 Setup. Installing one **replaces** the other under Program Files and rewires Auto-Start to that flavor’s MIDI backend. Do not expect both flavors side-by-side on one PC.

**WinUSB** is already in Windows. What often fails on a clean PC is *associating this Emagic MT4* with WinUSB when the project INF has no **trusted catalog** — this hobby project **does not ship** a signing certificate.

**Hobby install honesty:**

- If Setup’s WinUSB step **succeeds**, you are done for USB.
- If Setup’s WinUSB step **fails** (common on a clean PC), follow [USB association (WinUSB) failed](#usb-association-winusb-failed) — **guided association with Zadig** is the supported fix.

## Windows SmartScreen (unsigned or unrecognized Setup)

Windows may show **Microsoft Defender SmartScreen** (“Windows protected your PC”). That can happen when a community build is **unsigned**, or signed but **not yet reputation-trusted**. A warning does **not** automatically mean malware.

**Only continue if you downloaded Setup from this project’s Releases.**

To check whether **this** Setup file is signed: right-click → **Properties** → **Digital Signatures**. If that tab is missing, the file is typically **unsigned**. This project **does not ship** an Authenticode certificate (OQ-3).

When your PC’s policy allows it:

1. Choose **More info**.
2. Choose **Run anyway**.

Alternate: right-click Setup → **Properties** → enable **Unblock** if shown, then Apply / OK, and run Setup again.

Do **not** turn SmartScreen off globally or run copies from untrusted mirrors. On managed PCs, policy may block **Run anyway** — try a personal machine or ask your admin.

Contributor policy: [`docs/dev/authenticode-and-smartscreen.md`](../dev/authenticode-and-smartscreen.md).

# Auto-Start

After installation, Unitor MT4 Bridge starts with your Windows session. MIDI ports appear without opening the program every day, and without an Administrator password for daily use.

The Bridge is a **normal user-session program**. It is **not** a Windows Service.

## How to tell everything is ready

1. Sign in to Windows (or plug in the MT4 if you are already signed in).
2. Open your DAW or MIDI-OX.
3. Look for ports named **`MT4 In 1`**, **`MT4 In 2`**, **`MT4 Out 1`** … **`MT4 Out 4`**.

You can also start **Unitor MT4 Bridge** from the Start menu.

## Turn off Auto-Start

In the Start menu, choose **Unregister Auto-Start**. Uninstalling removes Auto-Start **for the Windows user who runs the uninstall**. Other accounts may still have their own entry.

# First MIDI test

Goal: send and receive notes (and optionally CC) between the MT4 and your software.

| Direction | Names in Windows |
|---|---|
| Inputs | `MT4 In 1`, `MT4 In 2` |
| Outputs | `MT4 Out 1`, `MT4 Out 2`, `MT4 Out 3`, `MT4 Out 4` |

1. Confirm [Installation](#installation) and [Auto-Start](#auto-start) (or launch **Unitor MT4 Bridge** once).
2. Plug in the MT4.
3. Open Ableton Live 12 or MIDI-OX.
4. Enable at least **`MT4 In 1`** and **`MT4 Out 1`**.
5. Send notes or CC and check the other direction.

### Computer Mode

Send a little ordinary MIDI first (short CC or a few notes) to activate **Computer Mode**. **SysEx alone does not wake Computer Mode.**

# First SysEx test

1. Bridge running; `MT4 In` / `MT4 Out` visible.
2. Send a short CC for Computer Mode.
3. In Matrix-Control (or MIDI-OX), select the matching virtual ports and complete a dump/restore (or short SysEx exchange).

The exchange should finish without restarting the Bridge for a normal librarian session.

# Troubleshooting

## SmartScreen blocks Setup

See [Windows SmartScreen](#windows-smartscreen-unsigned-or-unrecognized-setup).

## Windows MIDI Services is missing

If Setup refuses to start, or ports stay empty after a “successful” day:

1. Close your DAW and quit **Unitor MT4 Bridge**.
2. **Reboot** the PC.
3. After sign-in, open **Settings → Windows Update** and install pending updates (WMS often arrives with OS updates).
4. If Microsoft documents an optional **Windows MIDI Services** / MIDI enablement step on your build, follow that UI — **no** PowerShell Admin is required for this musician path.
5. Run the Win11 Setup again, or launch Bridge once and rescan MIDI in the DAW.

Still stuck on Windows 10 hardware? Use the [Win10 / virtualMIDI guide](unitor-mt4-bridge-win10-virtualmidi-user-guide.md) instead.

## USB association (WinUSB) failed

Setup reports that it could not associate the MT4 with WinUSB; install is often rolled back.

### Supported fix — guided WinUSB (Zadig)

1. Confirm the MT4 is plugged in and powered.
2. Confirm you ran this project’s own Setup.
3. Use **[Zadig](https://zadig.akeo.ie/)** to associate **WinUSB** with the MT4 composite MIDI interface **MI_02** (`USB\VID_086A&PID_0003&MI_02`) — not a random sibling interface.
4. Re-run Setup if it rolled back, then launch **Unitor MT4 Bridge**.
5. Contributors: [`docs/dev/winusb-bind.md`](../dev/winusb-bind.md).

## No ports visible

1. Bridge running (Auto-Start or manual launch)
2. MT4 plugged in
3. WMS available
4. MIDI device rescan in your DAW

## No SysEx reply

Send CC or notes first for Computer Mode, then retry SysEx.

# Sticky MIDI after stop / restart

If MIDI looks stuck after you stop the Bridge or after a messy quit (Windows MIDI Services / midisrv sticky case), use this **non-geek** recovery — **no** PowerShell, **no** Administrator terminal:

1. Close your DAW (and other MIDI apps).
2. Quit **Unitor MT4 Bridge** if it is still running.
3. **Reboot** the PC.
4. After sign-in, launch **Unitor MT4 Bridge** **once** (or rely on Auto-Start), then open the DAW.

Do not treat Admin repair scripts as the Win11 musician path.

# Unplug and replug the MT4

1. Unplug: that interface’s ports disappear.
2. Plug back in: the Bridge recreates the ports.
3. If needed, rescan MIDI in your software, or quit and relaunch the Bridge, then rescan.

**OK recovery:** host MIDI rescan and/or supervised Bridge restart.  
**Not the normal hot-plug fix:** a full Windows reboot (reboot is reserved for the sticky MIDI case above).

# What works / what does not

## What works (Win11 WMS community path)

- Install without virtualMIDI
- WinUSB Setup when Windows accepts the package; otherwise guided Zadig
- 2 inputs / 4 outputs as `MT4 In` / `MT4 Out`
- Notes, CC, clock / transport, MTC, SysEx (editor scale)
- Auto-Start without daily Administrator
- Hot-plug with rescan / Bridge relaunch
- Second MT4 naming when you have one (see below)

## What this does not do

- Setup-alone WinUSB success on every clean PC without guided steps
- Embedding or shipping virtualMIDI / teVirtualMIDI.dll
- A paid Authenticode certificate / silent SmartScreen
- Promising Windows 10 on this WMS Setup (use the Win10 guide)
- Patch mode, LTC/VITC, Fast Mode / AMT features, cascaded Emagic topologies
- Guaranteed AMT8 / Unitor8 without validated hardware
- A custom kernel MIDI driver
- Treating midisrv Admin repair as the default musician recovery

License honesty: [license-and-backends.md](../dev/license-and-backends.md). Signing: [authenticode-and-smartscreen.md](../dev/authenticode-and-smartscreen.md).

# Two MT4 interfaces

| Interface | Port names |
|---|---|
| First | `MT4 In N` / `MT4 Out N` |
| Second and later | `MT4 #2 In N` / `MT4 #2 Out N`, and so on |

**Validation honesty:** daily use for a **single** physical MT4 is the proven path documented here. Dual naming is implemented; do not assume a closed dual-unit lab claim from this guide alone.
