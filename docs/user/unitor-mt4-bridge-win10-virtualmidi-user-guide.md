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

**Unitor MT4 Bridge** lets you use an **Emagic MT4** MIDI interface on **Windows 64-bit**. Emagic no longer ships an official driver for modern Windows: the last Emagic Windows driver only supports **32-bit** systems.

This open-source project (Ten Square Software) is a community **hobby** effort: free sources on GitHub, **no** paid code-signing certificate, and **no** promise that installation alone always succeeds on every brand-new PC.

## What this version covers

- In this **first version (v1)**, only the **MT4** is guaranteed to work: it is the model the developer has tested and documented.
- In a later **v2**, support for **AMT8**, **Unitor8**, and **Unitor8 mk2** may follow — depending on community interest and user demand. Those models are **not** promised in v1.

## Two editions of the Bridge

| Edition | Windows | How MIDI reaches your software | Installation |
|---|---|---|---|
| **This guide** | **Windows 10** | **virtualMIDI** (by Tobias Erichsen) — install it yourself | More technical |
| Other guide | **Windows 11** | **Windows MIDI Services** (built in) | Relatively simple |

This Windows 10 path is a **parallel** offer for people who stay on Windows 10. It is **not** the “comfort” community promise (that is the Windows 11 edition).

virtualMIDI is **never** redistributed with the Bridge (rights & licence). You download and install it yourself from Tobias Erichsen’s website.

Want to install and use the Bridge on Windows 11? Open the [Windows 11 guide](unitor-mt4-bridge-win11-wms-user-guide.md). French: [`unitor-mt4-bridge-win10-virtualmidi-guide-utilisateur.md`](unitor-mt4-bridge-win10-virtualmidi-guide-utilisateur.md). Choice page: [`README.md`](README.md).

## Contents

1. [Check the prerequisites](#check-the-prerequisites)
2. [Install virtualMIDI](#install-virtualmidi)
3. [Install the Bridge](#install-the-bridge)
4. [Pass the Windows SmartScreen warning](#pass-the-windows-smartscreen-warning)
5. [Use automatic start](#use-automatic-start)
6. [Try your first MIDI notes](#try-your-first-midi-notes)
7. [Try your first SysEx transfer](#try-your-first-sysex-transfer)
8. [Fix common problems](#fix-common-problems)
9. [Unplug and replug the MT4](#unplug-and-replug-the-mt4)
10. [Know what works and what does not](#know-what-works-and-what-does-not)
11. [Use two MT4 interfaces](#use-two-mt4-interfaces)
12. [Glossary](#glossary)

---

# Check the prerequisites

| Need | Detail |
|---|---|
| Computer | **Windows 10**, **64-bit** (this edition also runs on Windows 11 if you choose the virtualMIDI installer) |
| Hardware | An **Emagic MT4** |
| Extra MIDI driver | **virtualMIDI**, installed **by you** — see below |

## Software you may use later

For everyday checks, open **your usual music software** (your DAW) — for example Ableton Live, Cubase, Reaper, or Bitwig. Any editor that can send and receive MIDI notes, controllers (CC), or even SysEx is fine for a first try in your home studio.

# Install virtualMIDI

On this path, the Bridge talks to your DAW through **virtualMIDI**. Install it from Tobias Erichsen’s website **before** the Bridge installer, for example by downloading:

- **[loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html)** (most common), or
- **[rtpMIDI](https://www.tobias-erichsen.de/software/rtpmidi.html)**

Then confirm that the file `teVirtualMIDI.dll` exists in `C:\Windows\System32\`.

**Why install this software yourself?** virtualMIDI is proprietary. This MIT repository does **not** have redistribution clearance and will **never** ship the DLL or installer package inside the Bridge download.

Without the DLL, the Bridge installer **fails closed** — an empty MIDI port list is not a successful install.

# Install the Bridge

Download name looks like: `UnitorMt4Bridge-Setup-win10-virtualmidi-{version}.exe` (from this project’s **Releases**).

1. Plug in the **MT4** before or while running the installer — the wizard does **not** pause to ask you to plug it in.
2. Run the installer you downloaded.
3. Accept the usual folder under Program Files (`Ten Square Software\Unitor MT4 Bridge`).
4. Allow Administrator **once**.
5. Wait for success — or incomplete if a check failed.

Install succeeds when:

- `teVirtualMIDI.dll` is present,
- USB association (WinUSB) is OK,
- automatic start is registered.

**One installer at a time:** the Windows 11 and Windows 10 installers share the **same Windows product identity**. Installing one **replaces** the other under Program Files and rewires automatic start to that edition.

**WinUSB** is already in Windows. On a clean PC, association often fails without a **trusted catalog**. Guided **[Zadig](https://zadig.akeo.ie/)** for **MI_02** (`USB\VID_086A&PID_0003&MI_02`) is the supported hobby fix.

# Pass the Windows SmartScreen warning

Windows may show **Microsoft Defender SmartScreen** (“Windows protected your PC”). That can happen when a community build is **unsigned**, or signed but **not yet trusted by reputation**. A warning does **not** automatically mean malware.

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

The exact menu depends on the software (Ableton Live, Cubase, Reaper, Bitwig, …), but the idea is always the same: do not reserve the port for a single application only.

# Try your first SysEx transfer

1. Bridge running; `MT4 In` / `MT4 Out` visible.
2. Send a short ordinary MIDI message for Computer Mode.
3. In **your usual DAW** or synthesizer editor, select the matching ports and complete a short SysEx exchange (for example a patch dump / restore).

# Fix common problems

## virtualMIDI missing

Install loopMIDI or rtpMIDI, confirm `teVirtualMIDI.dll` in System32, re-run the Bridge installer. This project will **never** embed the DLL.

## SmartScreen, WinUSB, no ports, no SysEx

Same ideas as the Windows 11 guide: SmartScreen honesty, Zadig when there is no **trusted catalog**, rescan MIDI, Computer Mode before SysEx. On this more technical path, PowerShell checks (service status, DLL probe) are acceptable if you need them — they are **not** the primary recovery on the Windows 11 comfort path.

## Ports missing after unplug / replug

Wait, rescan MIDI, or quit/relaunch the Bridge. A Windows reboot is **not** the normal unplug/replug fix.

# Unplug and replug the MT4

Unplug → ports for that unit disappear. Replug → Bridge recreates them. Rescan or supervised Bridge restart = OK. Requiring reboot for ordinary unplug/replug = fail.

# Know what works and what does not

## What works

- Parallel Windows 10 community path with **user-installed** virtualMIDI
- Same WinUSB / automatic start / MIDI / SysEx / unplug-replug spine as Windows 11 once prerequisites are met

## What this does not do

- Ship or embed virtualMIDI packages or DLLs
- Claim Windows 10 as the **comfort** community promise (that is Windows 11 + Windows MIDI Services)
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
| **Windows MIDI Services** | Microsoft MIDI stack on Windows 11 | Used by the **other** edition — not required on this Windows 10 path |

---

Another project successfully completed with [BMad](https://github.com/bmad-code-org/bmad-method)!
