# Unitor MT4 Bridge — Start here

Community end-user docs for **Unitor MT4 Bridge** (Ten Square Software) and the **Emagic MT4**.

## Choose your Windows path (~20 seconds)

| Your PC | Path | English | Français |
|---|---|---|---|
| **Windows 11** (comfort) | **Windows MIDI Services** — no virtualMIDI install | [Win11 WMS guide](unitor-mt4-bridge-win11-wms-user-guide.md) | [Guide Win11 WMS](unitor-mt4-bridge-win11-wms-guide-utilisateur.md) |
| **Windows 10** (motivated parallel) | **virtualMIDI** — you self-install Tobias Erichsen’s driver | [Win10 virtualMIDI guide](unitor-mt4-bridge-win10-virtualmidi-user-guide.md) | [Guide Win10 virtualMIDI](unitor-mt4-bridge-win10-virtualmidi-guide-utilisateur.md) |

**Win11 tip:** prefer the WMS Setup (`…-win11-wms-…`). You should not need virtualMIDI.  
**Win10 tip:** install virtualMIDI yourself first; confirm `teVirtualMIDI.dll` in System32. This project never ships that DLL/MSI.

**Same AppId:** both Setups share one Windows product id. Installing one **replaces** the other under Program Files and rewires Auto-Start to that flavor’s MIDI backend — not a side-by-side dual install.

Wrong guide? Each manual cross-links the other path. Old bookmarks to the former single guides redirect here via stubs.

## Suggested reading order (inside your chosen guide)

1. Prerequisites  
2. Installation (SmartScreen + WinUSB / Zadig honesty)  
3. Auto-Start  
4. First MIDI test  
5. First SysEx test  
6. Troubleshooting (Win11: non-geek sticky-MIDI reboot path; Win10: technical checks allowed)  
7. Unplug / replug (**hot-plug**)  
8. What works / what does not  
9. **Two MT4** interfaces (when you have a second unit)  

**SmartScreen:** unsigned or low-reputation builds — **Run anyway** / **Exécuter quand même**. No Authenticode certificate in this hobby line (OQ-3). Clean-PC WinUSB often needs **guided** association (Zadig), not Setup-alone success.

**License honesty:** MIT ≠ virtualMIDI ≠ Windows MIDI Services — [`docs/dev/license-and-backends.md`](../dev/license-and-backends.md).
