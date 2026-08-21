# Unitor MT4 Bridge {{VERSION}}

Dual community release assets for this version:

- `UnitorMt4Bridge-Setup-win11-wms-{{VERSION}}.exe` — Windows 11 + Windows MIDI Services (comfort path; no virtualMIDI prerequisite)
- `UnitorMt4Bridge-Setup-win10-virtualmidi-{{VERSION}}.exe` — Windows 10 + user-installed virtualMIDI (parallel motivated path; never embeds teVirtualMIDI.dll / MSI)
- `UnitorMt4Bridge-{{VERSION}}-docs.zip` — four path-specific manuals (Win11/Win10 × EN/FR) + README router + shared honesty pages (`license-and-backends.md`, `authenticode-and-smartscreen.md`)
- `SHA256SUMS.txt` — checksums for the files above

Start with `README.md` inside the docs zip (or https://github.com/tensquaresoftware/unitor-win64-driver/blob/main/docs/user/README.md) to pick Windows 11 vs Windows 10 in about 20 seconds.

Honesty: MIT covers this project's Bridge/Setup sources and docs. virtualMIDI is proprietary and separate (self-install only). Windows MIDI Services is the Win11 community backend. No Authenticode certificate in this hobby line — expect SmartScreen “Run anyway” / “Exécuter quand même”. Clean-PC WinUSB often needs guided association (Zadig), not Setup-alone magic. Same AppId: one Setup replaces the other on a PC.
