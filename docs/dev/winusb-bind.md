# WinUSB bind path for Emagic MT4 (086A:0003)

Primary community path: bind the MT4 composite MIDI interface to Microsoft **WinUSB** with the project DeviceInterfaceGUID using the in-repo INF. **Zadig is contributor fallback only** — not the primary install path.

## What you get

After a successful bind on Windows 10 x64 or Windows 11 x64:

- Device Manager shows the MT4 MIDI interface associated with WinUSB
- Registry exposes DeviceInterfaceGUID `{aa209017-cf8a-49ad-a0e7-701187ff7e05}`
- Bridge can open the device with `Bridge --open-device`

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

4. From a build of Bridge:

```text
Bridge.exe --open-device
```

- Exit code `0` and no stderr diagnostic ⇒ open succeeded (`IsOpen()` true).
- Non-zero exit + English stderr ⇒ fail closed (missing bind, wrong interface, or API error). Never treat empty I/O as success.

Without `--open-device`, Bridge only runs the DeviceProfile smoke checks and exits `0` on success (no USB open).

## Bridge flags

| Flag | Meaning |
|---|---|
| *(none)* | Profile smoke only — does not open WinUSB |
| `--open-device` | GUID-first WinUSB open; fail closed with English stderr on error |
| `--dev-zadig` | With `--open-device`: allow Zadig fallback if the project GUID is absent |

Example (contributor Zadig machine):

```text
Bridge.exe --open-device --dev-zadig
```

Default builds still prefer the project GUID and fail closed without `--dev-zadig` when the GUID is missing.

## Zadig — contributor fallback only

Use Zadig **only** when you cannot stage the INF (personal lab, unsigned package blocked, quick bring-up). It is **not** the primary community path.

If you use Zadig:

1. Bind WinUSB to composite interface **MI_02** (not a random sibling interface).
2. Prefer installing so a DeviceInterfaceGUID is present (Zadig usually writes one).
3. Open with `Bridge.exe --open-device --dev-zadig` so the Bridge may fall back when `{aa209017-cf8a-49ad-a0e7-701187ff7e05}` is missing.
4. Revert to the INF package before validating community install docs or Story 4.1 installer UX.

## Signing note

Authenticode / catalog signing for public release is tracked later (Story 4.4). Unsigned INF is acceptable for contributor bind; document local test-signing or Device Manager browse-install as needed.

## Out of scope here

Polished Public Installer UX (progress UI, Auto-Start, VirtualMIDI MSI gate) is Story 4.1 — not this bind package.
