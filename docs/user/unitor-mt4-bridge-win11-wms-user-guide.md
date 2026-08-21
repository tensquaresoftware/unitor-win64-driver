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

**Unitor MT4 Bridge** lets you use an **Emagic MT4** MIDI interface on **Windows 64-bit**. Emagic no longer ships an official driver for modern Windows: the last Emagic Windows driver only supports **32-bit** systems.

This open-source project (Ten Square Software) is a community **hobby** effort: free sources on GitHub, **no** paid code-signing certificate, and **no** promise that installation alone always succeeds on every brand-new PC.

## What this version covers

- In this **first version (v1)**, only the **MT4** is guaranteed to work: it is the model the developer has tested and documented.
- In a later **v2**, support for **AMT8**, **Unitor8**, and **Unitor8 mk2** may follow — depending on community interest and user demand. Those models are **not** promised in v1.

## Two editions of the Bridge

| Edition | Windows | How MIDI reaches your software | Install effort |
|---|---|---|---|
| **This guide** | **Windows 11** | Built-in **Windows MIDI Services** | Relatively simple |
| Other guide | **Windows 10** | **virtualMIDI** (Tobias Erichsen) — you install it yourself | More technical |

virtualMIDI is **not** redistributed with the Bridge (licence / rights). The Windows 10 guide explains why and how.

**Wrong PC?** On Windows 10, open the [Windows 10 guide](unitor-mt4-bridge-win10-virtualmidi-user-guide.md). French: [`unitor-mt4-bridge-win11-wms-guide-utilisateur.md`](unitor-mt4-bridge-win11-wms-guide-utilisateur.md). Choice page: [`README.md`](README.md).

## Contents

1. [Check the prerequisites](#check-the-prerequisites)
2. [Install the Bridge](#install-the-bridge)
3. [Pass the Windows SmartScreen warning](#pass-the-windows-smartscreen-warning)
4. [Use automatic start](#use-automatic-start)
5. [Try your first MIDI notes](#try-your-first-midi-notes)
6. [Try your first SysEx transfer](#try-your-first-sysex-transfer)
7. [Fix common problems](#fix-common-problems)
8. [Recover when MIDI is stuck](#recover-when-midi-is-stuck)
9. [Unplug and replug the MT4](#unplug-and-replug-the-mt4)
10. [Know what works and what does not](#know-what-works-and-what-does-not)
11. [Use two MT4 interfaces](#use-two-mt4-interfaces)
12. [Glossary](#glossary)

---

# Check the prerequisites

| Need | Detail |
|---|---|
| Computer | **Windows 11**, **64-bit**, with **Windows MIDI Services** available |
| Hardware | An **Emagic MT4** |
| Extra MIDI driver | **Not required** on this path — you do **not** install virtualMIDI |

If Windows MIDI Services is missing, the Bridge **refuses** to pretend that empty ports are a success. Update Windows, or use the Windows 10 path if that is your machine.

## Software you may use later

For everyday checks, open **your usual music software** (your DAW) — for example Ableton Live, Cubase, Logic (via a Windows host), Reaper, or Bitwig. Any editor that can send and receive MIDI SysEx with your synthesizer will do for a first SysEx check.

# Install the Bridge

Download name looks like: `UnitorMt4Bridge-Setup-win11-wms-{version}.exe` (from this project’s **Releases** page — not a random mirror).

1. Plug in the **MT4** (power + USB) **before** or while running the installer — the wizard does **not** pause to ask you to plug it in.
2. Run the installer you downloaded.
3. Accept the suggested folder, usually:

   `C:\Program Files\Ten Square Software\Unitor MT4 Bridge\`

4. Allow the Administrator prompt **once** (Program Files and USB association).
5. Wait for a clear success message — or an incomplete install if a check failed (that is **not** a successful community install).

Install succeeds when:

- Windows 11 with Windows MIDI Services is available,
- USB association (WinUSB) is OK,
- automatic start is registered for your Windows user account.

virtualMIDI is **not** checked on this edition. An empty MIDI port list after install is **not** success.

**One installer at a time:** the Windows 11 and Windows 10 installers share the **same Windows product identity**. Installing one **replaces** the other under Program Files and rewires automatic start to that edition. Do not expect both editions side-by-side on one PC.

**WinUSB** is already in Windows. What often fails on a clean PC is *associating this Emagic MT4* with WinUSB when the project package has no **trusted catalog** — this hobby project **does not ship** a signing certificate.

**Honesty about USB:**

- If the installer’s WinUSB step **succeeds**, you are done for USB.
- If that step **fails** (common on a clean PC), follow [Associate the MT4 with WinUSB](#associate-the-mt4-with-winusb) — guided association with **Zadig** is the supported fix.

# Pass the Windows SmartScreen warning

Windows may show **Microsoft Defender SmartScreen** (“Windows protected your PC”). That can happen when a community build is **unsigned**, or signed but **not yet trusted by reputation**. A warning does **not** automatically mean malware.

**Only continue if you downloaded the installer from this project’s Releases.**

To check whether **this** file is signed: right-click → **Properties** → **Digital Signatures**. If that tab is missing, the file is typically **unsigned**. This project **does not ship** an Authenticode certificate.

When your PC’s policy allows it:

1. Choose **More info**.
2. Choose **Run anyway**.

Alternate: right-click the installer → **Properties** → enable **Unblock** if shown, then Apply / OK, and run it again.

Do **not** turn SmartScreen off globally or run copies from untrusted mirrors. On managed PCs, policy may block **Run anyway** — try a personal machine or ask your admin.

# Use automatic start

After installation, Unitor MT4 Bridge starts with your Windows session. MIDI ports appear without opening the program every day, and without an Administrator password for daily use.

The Bridge is a **normal user-session program**. It is **not** a Windows Service.

## How to tell everything is ready

1. Sign in to Windows (or plug in the MT4 if you are already signed in).
2. Open **your usual DAW**.
3. Look for ports named **`MT4 In 1`**, **`MT4 In 2`**, **`MT4 Out 1`** … **`MT4 Out 4`**.

You can also start **Unitor MT4 Bridge** from the Start menu.

## Turn off automatic start

In the Start menu, choose **Unregister Auto-Start**. Uninstalling removes automatic start **for the Windows user who runs the uninstall**. Other accounts may still have their own entry.

# Try your first MIDI notes

Goal: send and receive notes (and optionally continuous controllers) between the MT4 and your software.

| Direction | Names in Windows |
|---|---|
| Inputs | `MT4 In 1`, `MT4 In 2` |
| Outputs | `MT4 Out 1`, `MT4 Out 2`, `MT4 Out 3`, `MT4 Out 4` |

1. Confirm installation and automatic start (or launch **Unitor MT4 Bridge** once).
2. Plug in the MT4.
3. Open **your usual DAW**.
4. Enable at least **`MT4 In 1`** and **`MT4 Out 1`**.
5. Send notes (or controllers) and check the other direction.

### Wake Computer Mode

Send a little ordinary MIDI first (a short controller change or a few notes) to activate **Computer Mode** on the MT4. **SysEx alone does not wake Computer Mode.**

# Try your first SysEx transfer

1. Bridge running; `MT4 In` / `MT4 Out` visible.
2. Send a short ordinary MIDI message for Computer Mode.
3. In **your usual DAW** or synthesizer editor, select the matching ports and complete a short SysEx exchange (for example a patch dump / restore with your gear).

The exchange should finish without restarting the Bridge for a normal editor session.

# Fix common problems

## Windows MIDI Services is missing

If the installer refuses to start, or ports stay empty after what looked like a good day:

1. Close your DAW and quit **Unitor MT4 Bridge**.
2. **Reboot** the PC.
3. After sign-in, open **Settings → Windows Update** and install pending updates (Windows MIDI Services often arrives with OS updates).
4. If Microsoft shows an optional **Windows MIDI Services** / MIDI enablement step on your build, follow that screen — **no** PowerShell Admin is required for this musician path.
5. Run the Windows 11 installer again, or launch the Bridge once and rescan MIDI in the DAW.

Still on Windows 10 hardware? Use the [Windows 10 guide](unitor-mt4-bridge-win10-virtualmidi-user-guide.md) instead.

## Associate the MT4 with WinUSB

The installer reports that it could not associate the MT4 with WinUSB; install is often rolled back.

1. Confirm the MT4 is plugged in and powered.
2. Confirm you ran this project’s own installer.
3. Use **[Zadig](https://zadig.akeo.ie/)** to associate **WinUSB** with the MT4 composite MIDI interface **MI_02** (`USB\VID_086A&PID_0003&MI_02`) — not a random sibling interface.
4. Re-run the installer if it rolled back, then launch **Unitor MT4 Bridge**.

## No ports visible

1. Bridge running (automatic start or manual launch)
2. MT4 plugged in
3. Windows MIDI Services available
4. MIDI device rescan in your DAW

## No SysEx reply

Send controllers or notes first for Computer Mode, then retry SysEx.

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

## What works on this Windows 11 path

- Install **without** virtualMIDI
- WinUSB via the installer when Windows accepts the package; otherwise guided Zadig
- 2 inputs / 4 outputs as `MT4 In` / `MT4 Out`
- Notes, controllers, clock / transport, MTC, SysEx at editor scale
- Automatic start without daily Administrator
- Unplug/replug with rescan / Bridge relaunch
- Second MT4 naming when you have one (see below)

## What this does not do

- Installer-alone WinUSB success on every clean PC without guided steps
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
| **virtualMIDI** | Tobias Erichsen driver | Used on the **Windows 10** edition only — not part of this Windows 11 path |
