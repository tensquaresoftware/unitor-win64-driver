---
organization: Ten Square Software
project: unitor-win64-driver
title: Unitor MT4 Bridge — User guide
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
version: "1.1"
product_version: "0.1.0"
---

This guide explains how to install and use **Unitor MT4 Bridge** with an **Emagic MT4** MIDI interface on Windows 10 or 11 (64-bit).

This is a **hobby / open-source** project (Ten Square Software facade): free sources on GitHub, **no** code-signing certificate in this release line, and **no** promise that Setup alone always succeeds on every brand-new PC.

Follow the sections in order. After Bridge + virtualMIDI + **WinUSB association** succeed, you can send and receive MIDI the same day, then complete a first SysEx exchange. On a **clean** PC, Setup often **cannot** finish WinUSB association without a trusted catalog — use the **guided WinUSB** steps (for example **Zadig**) described under [USB association (WinUSB) failed](#usb-association-winusb-failed).

French version: [`unitor-mt4-bridge-guide-utilisateur.md`](unitor-mt4-bridge-guide-utilisateur.md).

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
| virtualMIDI (interim) | The **[virtualMIDI](https://www.tobias-erichsen.de/software/virtualmidi.html)** driver (Tobias Erichsen), already installed — **lab / current Bridge** path |

## Install virtualMIDI (interim lab path)

Today’s Bridge uses **virtualMIDI** to create the virtual MIDI ports your DAW can see. A later **Windows MIDI Services** backend on **Windows 11** is planned for community ready-to-run binaries (without redistributing the proprietary virtualMIDI SDK). Until then, install virtualMIDI before the Bridge, for example with:

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

1. Plug in the **MT4** (power + USB) **before** or while running Setup — the wizard does **not** pause to ask you to plug it in.
2. Download and run `UnitorMt4Bridge-Setup.exe`.
3. Accept the suggested folder, usually:

   `C:\Program Files\Ten Square Software\Unitor MT4 Bridge\`

4. Allow the Administrator prompt **once** (Program Files and USB association).
5. Wait for the screen that confirms a successful install — or an **incomplete** screen if a gate failed (that is not a successful community install).

Setup succeeds when:

- virtualMIDI is detected,
- USB association (WinUSB) is OK,
- Auto-Start is registered for your Windows user account.

If something is missing (for example virtualMIDI), the wizard shows a help message: fix that point, then run the installer again.

**WinUSB** is Microsoft’s standard USB component (already in Windows). What often fails on a clean PC is *associating this Emagic MT4* with WinUSB when the project INF has no trusted catalog — this hobby project **does not ship** a signing certificate.

**Hobby install honesty:**

- If Setup’s WinUSB step **succeeds**, you are done for USB — no further tooling for daily use.
- If Setup’s WinUSB step **fails** (common on a clean PC), that is expected without a paid catalog. Follow [USB association (WinUSB) failed](#usb-association-winusb-failed) — **guided association with Zadig** (or the contributor INF path) is the supported fix, not “wait for a certificate.”
- Lab self-signing scripts are **not** community trust.

## Windows SmartScreen (unsigned or unrecognized Setup)

Windows may show **Microsoft Defender SmartScreen** — often “Windows protected your PC” or an unrecognized-app warning — when you open `UnitorMt4Bridge-Setup.exe`. That can happen when a community build is **unsigned**, or when a build is signed but **not yet reputation-trusted**. A warning does **not** automatically mean the file is malware.

**Only continue if you downloaded Setup from this project’s own download page / Releases** (not a random third-party mirror). Until the first tagged public community release publishes a fixed URL, use the project’s Releases or download page for this repository.

To check whether **this** Setup file is signed: right-click → **Properties** → **Digital Signatures**. If that tab is missing, the file is typically **unsigned**. This project **does not ship** an Authenticode certificate; expect unsigned community builds when binaries are published.

When your PC’s policy allows it:

1. Choose **More info**.
2. Choose **Run anyway**.

Alternate mitigation when the file was downloaded from the web: right-click the Setup file → **Properties** → enable **Unblock** if shown (mark-of-the-web), then Apply / OK, and run Setup again.

**Honesty:**

- On **enterprise / managed** PCs, policy may block **Run anyway** entirely — contact your PC admin, or try a personal machine. Do not expect the override to always work.
- Do **not** turn SmartScreen off globally, whitelist whole folders, or run copies from untrusted mirrors.
- SmartScreen is usually easier to work through than the WinUSB catalog problem — see [USB association (WinUSB) failed](#usb-association-winusb-failed).

Contributor / release signing policy (English): [`docs/dev/authenticode-and-smartscreen.md`](../dev/authenticode-and-smartscreen.md).

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

In the Start menu, choose **Unregister Auto-Start** if you no longer want the Bridge to launch at sign-in. Uninstalling the software also removes Auto-Start **for the Windows user who runs the uninstall**. Other Windows accounts on the same PC may still have their own Auto-Start entry — open **Unregister Auto-Start** (or run `Bridge.exe --unregister-auto-start`) while signed in as that user.

Uninstall removes the Bridge program files. The WinUSB association for the MT4 **may remain** in Windows Driver Store until an administrator removes that driver package — that is normal and does not by itself keep Auto-Start alive.

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

## SmartScreen blocks Setup (“Windows protected your PC”)

See [Windows SmartScreen (unsigned or unrecognized Setup)](#windows-smartscreen-unsigned-or-unrecognized-setup). Confirm the download came from this project’s own download page / Releases, then use **More info → Run anyway** when policy allows, or Properties → **Unblock**. Do not disable SmartScreen globally.

## virtualMIDI missing

Install **loopMIDI** or **rtpMIDI**, confirm `teVirtualMIDI.dll` in `C:\Windows\System32\`, then run the Unitor MT4 Bridge installer again.

## USB association (WinUSB) failed

Setup reports that it could not associate the MT4 with WinUSB, and the install is often rolled back (program files / Add-Remove entry are not left as a successful install).

Common cause on a **clean** PC: Windows rejects an **unsigned** driver package / missing trusted catalog (lab error often looks like `0xE000022F`). Replugging the MT4 and clicking Setup again will usually hit the **same** failure — this project is **not** buying a certificate to “fix” that.

### Supported fix without a paid certificate — guided WinUSB (Zadig)

1. Confirm the MT4 is plugged in and powered.
2. Confirm you ran the project’s own Setup (see SmartScreen honesty above) — not a random mirror.
3. Read the Setup message: if it mentions an unsigned driver package, this is the catalog gate (not “forgot to plug the MT4”).
4. Use **[Zadig](https://zadig.akeo.ie/)** (or another documented guided bind) to associate **WinUSB** with the MT4 composite MIDI interface **MI_02** (`USB\VID_086A&PID_0003&MI_02`) — not a random sibling USB interface.
5. Install / place the Bridge (re-run Setup if it rolled back, or use a contributor build), ensure virtualMIDI is present, then launch **Unitor MT4 Bridge**.
6. Contributors who prefer the in-repo INF: see [`docs/dev/winusb-bind.md`](../dev/winusb-bind.md).

When association **does** succeed, Device Manager should show the MT4 MIDI interface under WinUSB.

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

- Run the provided setup wizard (Microsoft WinUSB association — not a custom kernel driver), **when Windows accepts the driver package**
- Use **2 inputs** and **4 outputs** as virtual ports (`MT4 In` / `MT4 Out`) after a successful install
- Pass notes, CC, clock / transport, MTC, and SysEx (editor / librarian scale)
- Use Auto-Start without daily Administrator (user-session Bridge, not a Windows Service)
- Unplug and replug the MT4, then recover ports with rescan or Bridge relaunch (without Windows reboot)
- Open several applications on the same ports (within virtualMIDI limits, about eight per port)
- Connect a second MT4 when you have one (see [Two MT4 interfaces](#two-mt4-interfaces))
- Consult published studio MIDI Path timing anchors (healthy ≤4–5 ms p99 latency and ≤1–2 ms p99 classical jitter, Gate **(a)** 2026-08-11 from one quiet-lab Win10 DIN Out2→In2 hardware-loop — **lab targets, not a DAW-session guarantee**; method under [`docs/dev/measurements/`](../dev/measurements/))

## What this does not do

Do **not** expect the following:

- Setup-alone WinUSB success on every clean PC **without** a guided step (this hobby project **does not ship** a signing certificate — see [USB association (WinUSB) failed](#usb-association-winusb-failed))
- A polished commercial installer experience as the project goal
- Public GitHub Releases of Bridge/Setup binaries that depend on the proprietary virtualMIDI SDK (out of community scope — community binaries planned after **Windows MIDI Services** on Win11)
- Patch mode, LTC/VITC, Fast Mode / AMT features from Unitor-family manuals
- Cascaded / stacked Emagic multi-interface topologies
- Guaranteed AMT8 / Unitor8 support without validated hardware for those models
- Windows MIDI Services as the **current** shipping backend (planned next community backend, Win11-only)
- A custom kernel MIDI driver
- Instant SmartScreen silence on unsigned downloads (see [Windows SmartScreen](#windows-smartscreen-unsigned-or-unrecognized-setup))

See also: [License and MIDI backends](../dev/license-and-backends.md) — MIT ≠ virtualMIDI (interim lab) ≠ Windows MIDI Services (next community). Signing policy: [Authenticode and SmartScreen](../dev/authenticode-and-smartscreen.md) (no certificate purchase in this hobby project).

# Two MT4 interfaces

| Interface | Port names |
|---|---|
| First | `MT4 In N` / `MT4 Out N` |
| Second and later | `MT4 #2 In N` / `MT4 #2 Out N`, and so on |

Each interface has its own set of ports. Unplugging one does not rename the other.

If you only have one MT4, ignore names with `#2` until you need them.

**Validation honesty:** daily use and naming for a **single** physical MT4 are the proven path documented here. Dual-MT4 naming is implemented; treat a second unit as supported in software, but do not assume a closed dual-unit lab claim from this guide alone.
