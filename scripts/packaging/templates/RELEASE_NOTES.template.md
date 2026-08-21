## Unitor MT4 Bridge {{VERSION}}

### What’s new

<!-- Operators: replace this block before finalize/publish. Keep honesty bullets below. -->

- Dual community Setups for this version (Win11 WMS + Win10 virtualMIDI self-install).
- _(Add notable Bridge / docs / installer changes here.)_

### Known limits

- No Authenticode certificate in this hobby line (SmartScreen “Run anyway” / “Exécuter quand même”).
- Clean-PC WinUSB often needs guided Zadig association (no trusted catalog shipped).
- Same AppId: installing one flavor Setup replaces the other under Program Files + Auto-Start backend.
- _(Add version-specific limits here.)_

### Dual community assets

| Flavor | Asset | Who it is for |
| --- | --- | --- |
| **win11-wms** | `UnitorMt4Bridge-Setup-win11-wms-{{VERSION}}.exe` | Windows 11 musicians — **Windows MIDI Services**; no virtualMIDI install |
| **win10-virtualmidi** | `UnitorMt4Bridge-Setup-win10-virtualmidi-{{VERSION}}.exe` | Windows 10 motivated users — **self-install** virtualMIDI (Tobias Erichsen); this project never redistributes the DLL/MSI |

Also attached: `UnitorMt4Bridge-{{VERSION}}-docs.zip` (four manuals + README router + shared honesty pages `license-and-backends.md` and `authenticode-and-smartscreen.md`) and `SHA256SUMS.txt`.

### Before you install

1. Pick your OS path in ~20 seconds: [docs/user/README.md](https://github.com/tensquaresoftware/unitor-win64-driver/blob/main/docs/user/README.md)
2. Expect SmartScreen on unsigned builds — **More info → Run anyway** / **Exécuter quand même** (no Authenticode cert in this hobby line; OQ-3)
3. Clean PCs often need **guided WinUSB** (Zadig) when Setup-alone association fails — not a Setup-alone promise
4. Win10 flavor: install virtualMIDI yourself first and confirm `teVirtualMIDI.dll` in System32

### Honesty (do not collapse)

- **MIT** = this repository’s Bridge/Setup sources and docs
- **virtualMIDI** = proprietary, separate; community Win10 path = user self-install only (OQ-1 — no redistribution clearance)
- **Windows MIDI Services** = Win11 community comfort backend

See [license-and-backends.md](https://github.com/tensquaresoftware/unitor-win64-driver/blob/main/docs/dev/license-and-backends.md).
