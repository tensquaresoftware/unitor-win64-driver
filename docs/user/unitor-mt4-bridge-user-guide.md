---
organization: Ten Square Software
project: unitor-win64-driver
title: Unitor MT4 Bridge — User guide
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
version: "1.0"
product_version: "0.1.0"
---

This guide explains how to install and use **Unitor MT4 Bridge** with an **Emagic MT4** MIDI interface on Windows 10 or 11 (64-bit).

Follow the sections in order. You should be able to send and receive MIDI the same day, then complete a first SysEx exchange with an editor or librarian.

French version: [`unitor-mt4-bridge-manuel-utilisateur.md`](unitor-mt4-bridge-manuel-utilisateur.md).

## Contents

1. [Prerequisites](#prerequisites)
2. [Installation](#installation)
3. [Auto-Start](#auto-start)
4. [First MIDI test](#first-midi-test)
5. [First SysEx test](#first-sysex-test)
6. [Troubleshooting](#troubleshooting)
7. [Unplug and replug the MT4](#unplug-and-replug-the-mt4)
8. [What works / what does not](#what-works--what-does-not)
9. [Two MT4 interfaces](#two-mt4-interfaces)

---

# Prerequisites

Before you install, have the following ready:

| Need | Detail |
|---|---|
| Computer | **Windows 10** or **Windows 11**, **64-bit** |
| Hardware | An **Emagic MT4** MIDI interface |
| virtualMIDI | The **[virtualMIDI](https://www.tobias-erichsen.de/software/virtualmidi.html)** driver (Tobias Erichsen), already installed |

## Install virtualMIDI

Unitor MT4 Bridge uses **virtualMIDI** to create the virtual MIDI ports your DAW can see. Install it before the Bridge, for example with:

- **[loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html)** (most common choice), or
- **[rtpMIDI](https://www.tobias-erichsen.de/software/rtpmidi.html)**

Then check that `teVirtualMIDI.dll` is present in `C:\Windows\System32\`. Without this driver, Bridge setup cannot finish successfully.

## Useful software later on

These programs are not included with the Bridge; install them if you need them:

| Purpose | Example |
|---|---|
| First MIDI test | Ableton Live 12 or MIDI-OX |
| First SysEx test | Matrix-Control, or MIDI-OX |

# Installation

1. Download and run `UnitorMt4Bridge-Setup.exe`.
2. Accept the suggested folder, usually:

   `C:\Program Files\Ten Square Software\Unitor MT4 Bridge\`

3. Plug in the MT4 when the wizard asks (USB association).
4. Allow the Administrator prompt **once** (Program Files and USB association).
5. Wait for the screen that confirms a successful install.

Setup succeeds when:

- virtualMIDI is detected,
- USB association (WinUSB) is OK,
- Auto-Start is registered for your Windows user account.

If something is missing (for example virtualMIDI), the wizard shows a help message: fix that point, then run the installer again.

**WinUSB** is Microsoft’s standard USB component. The installer associates it with the MT4 so the Bridge can talk to the interface. You do not need further USB setup for normal use.

# Auto-Start

After installation, Unitor MT4 Bridge starts with your Windows session. MIDI ports appear without opening the program every day, and without an Administrator password for daily use.

The Bridge is a **normal user-session program**. It is **not** a Windows Service, and you should not install it as one.

## How to tell everything is ready

1. Sign in to Windows (or plug in the MT4 if you are already signed in).
2. Open your DAW or MIDI-OX.
3. Look for ports named **`MT4 In 1`**, **`MT4 In 2`**, **`MT4 Out 1`** … **`MT4 Out 4`**.

If those names are present, the Bridge is running correctly.

You can also start **Unitor MT4 Bridge** from the Start menu: that launches the same session.

## Turn off Auto-Start

In the Start menu, choose **Unregister Auto-Start** if you no longer want the Bridge to launch at sign-in. Uninstalling the software also removes Auto-Start.

# First MIDI test

Goal: send and receive notes (and optionally CC) between the MT4 and your software.

## Ports to use

| Direction | Names in Windows |
|---|---|
| Inputs | `MT4 In 1`, `MT4 In 2` |
| Outputs | `MT4 Out 1`, `MT4 Out 2`, `MT4 Out 3`, `MT4 Out 4` |

## Steps

1. Confirm [Installation](#installation) and [Auto-Start](#auto-start) (or launch **Unitor MT4 Bridge** once).
2. Plug in the MT4.
3. Open Ableton Live 12 or MIDI-OX.
4. Enable at least **`MT4 In 1`** and **`MT4 Out 1`**.
5. Send notes or CC from a keyboard into an MT4 input, or from the software to an output, and check the other direction.

When you see activity on at least one input and one output, the first MIDI test is done.

### Computer Mode

For the MT4 to respond correctly, send a little ordinary MIDI first — for example a short CC or a few notes. That activates **Computer Mode**.

**SysEx alone does not wake Computer Mode.** Always send ordinary MIDI (notes or CC) before a SysEx exchange. See also [First SysEx test](#first-sysex-test).

## Several applications at once

You can open more than one application on the same ports (for example Ableton and MIDI-OX). virtualMIDI allows about eight applications per port. If software says the port is already in use, check its MIDI options for an “exclusive” mode and turn that off.

# First SysEx test

Goal: send and receive SysEx (System Exclusive) through the MT4, for example a dump / restore with an editor.

## With Matrix-Control

Matrix-Control is an external editor used here as a **validation target** (install it separately; it is not part of the Bridge runtime). Recommended steps:

1. The Bridge is running and `MT4 In` / `MT4 Out` ports are visible ([First MIDI test](#first-midi-test)).
2. Send a short CC to activate **Computer Mode** (SysEx alone will not wake it).
3. In Matrix-Control, select the virtual ports that match the cable to your Matrix synth.
4. Run a dump and restore (or a patch write) through to the end.

The exchange should finish without restarting the Bridge.

## With MIDI-OX

If you do not have Matrix-Control:

1. Open MIDI-OX.
2. Select the `MT4 In` / `MT4 Out` ports.
3. Send ordinary MIDI (CC or notes) first if Computer Mode may be asleep.
4. Send a SysEx file and check the reply (or a round-trip on the cable under test).

## If you unplug during a dump

Plug the MT4 back in, ask your software to rescan MIDI ports, and restart **Unitor MT4 Bridge** if needed. See [Unplug and replug the MT4](#unplug-and-replug-the-mt4). A Windows reboot is **not** the recovery path.

# Troubleshooting

## virtualMIDI missing

Install **loopMIDI** or **rtpMIDI**, confirm `teVirtualMIDI.dll` in `C:\Windows\System32\`, then run the Unitor MT4 Bridge installer again.

## USB association (WinUSB) failed

Run the installer again with the MT4 plugged in. In Device Manager, the MT4 MIDI interface should show as associated with WinUSB.

## No ports visible

Check in order:

1. virtualMIDI installed
2. Bridge running (Auto-Start or manual launch)
3. MT4 plugged in
4. MIDI device rescan in your DAW or MIDI-OX

Expected ports: `MT4 In 1`…`MT4 In 2`, `MT4 Out 1`…`MT4 Out 4`.

## No SysEx reply

Send a CC or some notes first to activate Computer Mode (**SysEx alone does not wake it**), then retry the SysEx exchange.

## Ports missing after unplug / replug

1. Wait a moment (the Bridge recreates the session).
2. Rescan MIDI in your software.
3. If needed, quit and relaunch **Unitor MT4 Bridge**, then rescan.

Do **not** treat a Windows reboot as the normal fix. See [Unplug and replug the MT4](#unplug-and-replug-the-mt4).

## “Port already in use”

Close the other application, or turn off exclusive MIDI mode in the DAW. You can usually leave up to eight programs on the same port.

# Unplug and replug the MT4

You can unplug and plug the MT4 back in during a session.

1. Unplug the MT4: that interface’s ports disappear.
2. Plug it back in: the Bridge recreates the ports.
3. If your DAW does not see them right away, run a MIDI rescan.
4. If needed, relaunch **Unitor MT4 Bridge**, then rescan.

**Recovery that counts as OK:** host MIDI rescan, and/or a supervised Bridge restart (quit and relaunch).

**Recovery that counts as fail:** needing a full **Windows reboot** to get ports back. You should not need to reboot Windows for hot-plug recovery.

To quit the Bridge cleanly when it is open in a console window, use **Ctrl+C**.

# What works / what does not

## What works (V1)

With Unitor MT4 Bridge and an MT4 on Windows 10 / 11 64-bit you can:

- Install with the provided setup wizard (WinUSB association — not a custom kernel driver)
- Use **2 inputs** and **4 outputs** as virtual ports (`MT4 In` / `MT4 Out`)
- Pass notes, CC, clock / transport, MTC, and SysEx (editor / librarian scale)
- Use Auto-Start without daily Administrator (user-session Bridge, not a Windows Service)
- Unplug and replug the MT4, then recover ports with rescan or Bridge relaunch (without Windows reboot)
- Open several applications on the same ports (within virtualMIDI limits, about eight per port)
- Connect a second MT4 when you have one (see [Two MT4 interfaces](#two-mt4-interfaces))

## What this does not do (V1)

Do **not** expect the following in this version:

- Patch mode, LTC/VITC, Fast Mode / AMT features from Unitor-family manuals
- Cascaded / stacked Emagic multi-interface topologies
- Guaranteed AMT8 / Unitor8 support without validated hardware for those models
- Windows MIDI Services as the V1 MIDI backend (possible later Win11-only option)
- A custom kernel MIDI driver
- Zadig as the recommended community install path (contributor fallback only)
- Published “studio-done” MIDI latency / jitter numbers (measured later)

See also (contributors / evaluators; English technical page): [License and MIDI backends](../dev/license-and-backends.md) — MIT (this repo) ≠ virtualMIDI (proprietary) ≠ Windows MIDI Services (not V1).

# Two MT4 interfaces

| Interface | Port names |
|---|---|
| First | `MT4 In N` / `MT4 Out N` |
| Second and later | `MT4 #2 In N` / `MT4 #2 Out N`, and so on |

Each interface has its own set of ports. Unplugging one does not rename the other.

If you only have one MT4, ignore names with `#2` until you need them.

**Validation honesty:** daily use and naming for a **single** physical MT4 are the proven path documented here. Dual-MT4 naming is implemented; treat a second unit as supported in software, but do not assume a closed dual-unit lab claim from this guide alone.
