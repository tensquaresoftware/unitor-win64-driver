## Unitor MT4 Bridge {{VERSION}}

### What’s new

<!-- Operators: replace this block before finalize/publish. Keep honesty bullets below. -->

- Dual Setups for this version (Windows 11 WMS + Windows 10 virtualMIDI self-install).
- _(Add notable Bridge / docs / installer changes here.)_

### Known limits

- No Authenticode certificate in this hobby line (SmartScreen “Run anyway” / “Exécuter quand même”).
- Clean-PC WinUSB often needs guided Zadig association (no trusted catalog shipped).
- Same Windows product identity: installing one Setup replaces the other under Program Files + automatic start.
- _(Add version-specific limits here.)_

### Download assets

| Edition | Asset | Who it is for |
| --- | --- | --- |
| **win11-wms** | `unitor-mt4-bridge-{{VERSION}}-win11-wms-setup.exe` | Windows 11 — **Windows MIDI Services**; no virtualMIDI install |
| **win10-virtualmidi** | `unitor-mt4-bridge-{{VERSION}}-win10-virtualmidi-setup.exe` | Windows 10 — **self-install** virtualMIDI (Tobias Erichsen); this project never redistributes the DLL/MSI |

Also attached: `unitor-mt4-bridge-{{VERSION}}-docs.zip` (four manuals + README router + honesty pages) and `SHA256SUMS.txt`.

### Before you install

1. Pick your Windows edition in ~20 seconds: [docs/user/README.md](https://github.com/tensquaresoftware/unitor-win64-driver/blob/main/docs/user/README.md)
2. Expect SmartScreen on unsigned builds — **More info → Run anyway** / **Exécuter quand même** (no Authenticode cert; OQ-3)
3. Clean PCs often need **guided WinUSB** (Zadig) when installer-alone association fails
4. Windows 10 edition: install virtualMIDI yourself first and confirm `teVirtualMIDI.dll` in System32

### Honesty (do not collapse)

- **MIT** = this repository’s Bridge/installer sources and docs
- **virtualMIDI** = proprietary, separate; Windows 10 edition = user self-install only (OQ-1 — no redistribution clearance)
- **Windows MIDI Services** = Windows 11 simpler edition backend

See [license-and-backends.md](https://github.com/tensquaresoftware/unitor-win64-driver/blob/main/docs/dev/license-and-backends.md).
