# WinUSB bind path for Emagic MT4 (086A:0003)

**Community readers:** start with the user guide ([Installation](../user/unitor-mt4-bridge-user-guide.md#installation) and [USB association failed](../user/unitor-mt4-bridge-user-guide.md#usb-association-winusb-failed)). This page is the **contributor / detailed bind** reference.

**Hobby posture (2026-08-10):** no paid INF catalog. On a clean PC, Setup-alone association often fails (`0xE000022F`). The supported **hobby install** USB path is **guided WinUSB** — typically **Zadig** for musicians, or the INF / Device Manager steps below for contributors.

Primary packaging still ships the project INF + DeviceInterfaceGUID `{aa209017-cf8a-49ad-a0e7-701187ff7e05}` for machines that can accept it (lab signed catalog, or rare environments that allow unsigned OEM INF).

## What you get

After a successful bind on Windows 10 x64 or Windows 11 x64:

- Device Manager shows the MT4 MIDI interface associated with WinUSB
- Registry exposes DeviceInterfaceGUID `{aa209017-cf8a-49ad-a0e7-701187ff7e05}`
- Bridge can open the device with `Bridge --open-device`
- With virtualMIDI installed, Bridge can create stable `MT4 Input N` / `MT4 Output Y` endpoints and run notes/CC with `--start-session` / `--run-midi`

Hardware ID targeted by the package:

```text
USB\VID_086A&PID_0003&MI_02
```

That node is interface number `2` (MT4 Emagic MIDI), matching `DeviceProfile::ifnum`.

## Files

| Path | Role |
|---|---|
| `installer/mt4-winusb.inf` | Custom INF: Include/Needs in-box `winusb.inf`, registers the project GUID |
| `installer/bind-mt4-winusb.ps1` | Optional elevated `pnputil` helper |

No WDF/WinUSB co-installer DLLs are shipped (Win10/11 in-box WinUSB).

## Guided bind (primary)

### Option A — pnputil helper

1. Copy or clone the repo onto the Windows machine.
2. Open **elevated** PowerShell.
3. Run:

```powershell
.\installer\bind-mt4-winusb.ps1
```

4. If the INF is unsigned, Windows may reject automatic install. Continue with Option B or enable test signing for lab machines only.

### Option B — Device Manager

1. Plug in the MT4.
2. Open **Device Manager**.
3. Locate the MT4 / unknown USB composite child for interface 2 (often under Universal Serial Bus devices or Other devices).
4. Right-click → **Update driver** → **Browse my computer for drivers**.
5. Point at the repo `installer/` folder (contains `mt4-winusb.inf`).
6. Complete the wizard. Accept test-mode / unsigned prompts only on contributor lab machines when policy allows.

### Verify

1. Device Manager → device Properties → **Driver**: provider / WinUSB stack loaded (not a custom kernel driver of this project).
2. Confirm hardware ID includes `USB\VID_086A&PID_0003&MI_02`.
3. Under the device’s registry **Device Parameters**, confirm `DeviceInterfaceGUIDs` contains:

```text
{aa209017-cf8a-49ad-a0e7-701187ff7e05}
```

4. From a build of Bridge — open-only smoke:

```text
Bridge.exe --open-device
```

- Exit code `0` and no stderr diagnostic ⇒ open succeeded (`IsOpen()` true).
- Non-zero exit + English stderr ⇒ fail closed (missing bind, wrong interface, or API error). Never treat empty I/O as success.

5. Epic 1 MIDI path (requires teVirtualMIDI / loopMIDI so `teVirtualMIDI.dll` is present):

```text
Bridge.exe --start-session
```

or equivalently:

```text
Bridge.exe --run-midi
```

- Creates 2 IN + 4 OUT Virtual Ports named `MT4 Input 1`…`MT4 Input 2` and `MT4 Output 1`…`MT4 Output 4` (separate faces; apps should use MIDI From = Input, MIDI To = Output). If a prior Bridge left old `MT4 Port N` endpoints, close that session / reboot MIDI stack before judging a rename failure.
- Runs the notes/CC pump until Ctrl+C (or console close)
- Fail closed with English stderr if WinUSB open or virtualMIDI is missing

Without `--open-device` / `--start-session` / `--run-midi`, Bridge only runs the DeviceProfile smoke checks and exits `0` on success (no USB open, no Virtual Ports).

## Bridge flags

| Flag | Meaning |
|---|---|
| *(none)* | Profile smoke only — does not open WinUSB |
| `--open-device` | GUID-first WinUSB open; fail closed with English stderr on error |
| `--dev-zadig` | With `--open-device` or session flags: allow Zadig fallback if the project GUID is absent |
| `--start-session` | Open MT4, create `MT4 Input` / `MT4 Output` Virtual Ports, run notes/CC pump (Ctrl+C to stop). Leave this running for the ~4 h longevity soak (`docs/tests/checklists/smoke-epic2-longevity-mt4.md`) |
| `--run-midi` | Alias of `--start-session` |

Example (contributor Zadig machine, open only):

```text
Bridge.exe --open-device --dev-zadig
```

Example (notes/CC session with Zadig fallback):

```text
Bridge.exe --start-session --dev-zadig
```

Default builds still prefer the project GUID and fail closed without `--dev-zadig` when the GUID is missing.

## Zadig — supported guided path without a paid catalog

On clean PCs without a trusted production `.cat`, **Zadig** is the practical **hobby install** association path (same honesty as the user guide). Prefer it when Setup’s `pnputil` step fails with an unsigned-INF error.

If you use Zadig:

1. Bind WinUSB to composite interface **MI_02** (not a random sibling interface).
2. Prefer installing so a DeviceInterfaceGUID is present (Zadig usually writes one).
3. Open with `Bridge.exe --open-device --dev-zadig` (or `--start-session --dev-zadig`) so the Bridge may fall back when `{aa209017-cf8a-49ad-a0e7-701187ff7e05}` is missing.
4. For INF-based lab validation with a self-signed catalog, use [`installer/sign-lab-package.ps1`](../../installer/sign-lab-package.ps1) — **not** community trust.

Zadig fallback refuses to open when **more than one** USB device matches the MT4 hardware ID (same fail-closed rule as the GUID path).

## Signing note (lab vs public)

Two separate trust domains — do not conflate them:

1. **Authenticode** on `Bridge.exe` / `UnitorMt4Bridge-Setup.exe` (SmartScreen / publisher trust)
2. **WinUSB INF catalog** (`.cat`) for clean-machine Driver Store association (`CatalogFile=mt4-winusb.cat`)

**Lab only:** [`installer/sign-lab-package.ps1`](../../installer/sign-lab-package.ps1) builds a self-signed catalog and stages LocalMachine Root / TrustedPublisher. That is **not** public Authenticode.

**Public / hobby policy:** no code-signing certificate in this project line. Unsigned Setup + SmartScreen docs; clean-PC WinUSB via **guided** association (Zadig / INF browse). Full runbook: [`authenticode-and-smartscreen.md`](authenticode-and-smartscreen.md). Musician-facing steps: [user guide](../user/unitor-mt4-bridge-user-guide.md#windows-smartscreen-unsigned-or-unrecognized-setup).

## Daily use after bind (Auto-Start)

Once WinUSB bind works and Epic 1–2 sessions are green, register user-session Auto-Start (no daily Administrator) with the Story 3.1 smoke guide:

- [`docs/tests/smoke-epic3-autostart-mt4.md`](../tests/smoke-epic3-autostart-mt4.md)

One-time INF bind / Zadig / test-signing may still need Administrator. Daily `--register-auto-start` / logon launch / `--unregister-auto-start` must not.

## Public Installer (packaging path)

Story **4.1** packages the INF into the Public Installer (progress UI, virtualMIDI presence gate, Auto-Start wiring). On clean PCs without a trusted catalog, expect WinUSB association to **fail** inside Setup — then use guided WinUSB. End-user prose: [`docs/user/unitor-mt4-bridge-user-guide.md`](../user/unitor-mt4-bridge-user-guide.md). Operator smokes:

- [`docs/tests/smoke-epic4-public-installer-mt4.md`](../tests/smoke-epic4-public-installer-mt4.md)
- [`docs/tests/smoke-epic4-user-docs-mt4.md`](../tests/smoke-epic4-user-docs-mt4.md)

This document remains the detailed bind reference (INF + Zadig). Runtime Auto-Start register/unregister lives in Story 3.1 (see smoke guide above).
