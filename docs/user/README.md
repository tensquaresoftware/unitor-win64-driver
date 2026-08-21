# Unitor MT4 Bridge — Start here

**Unitor MT4 Bridge** lets an **Emagic MT4** MIDI interface work on **Windows 64-bit**. There is no official Emagic driver for modern Windows (the last one was **32-bit** only).

This hobby / open-source project (Ten Square Software) ships **two editions** of the same version:

| Your PC | Edition | Effort | Guide |
|---|---|---|---|
| **Windows 11** | **Windows MIDI Services** — no virtualMIDI install | Relatively simple | [English](unitor-mt4-bridge-win11-wms-user-guide.md) · [Français](unitor-mt4-bridge-win11-wms-guide-utilisateur.md) |
| **Windows 10** | **virtualMIDI** — you install Tobias Erichsen’s driver yourself | More technical | [English](unitor-mt4-bridge-win10-virtualmidi-user-guide.md) · [Français](unitor-mt4-bridge-win10-virtualmidi-guide-utilisateur.md) |

**v1** guarantees the **MT4** only (tested and documented). Later support for **AMT8** / **Unitor8** / **Unitor8 mk2** may come in a **v2**, depending on community interest — not promised today.

**One installer at a time:** installing the Windows 11 edition **replaces** the Windows 10 edition on the same PC (and the other way around). They share the same Windows product identity — not a side-by-side dual install.

**Pick the matching Setup:** GitHub Releases ship two files — names contain `win11-wms` or `win10-virtualmidi`. Download the one that matches this table. The wrong flavor fails closed (for example Windows 10 Setup on a machine that needs Windows MIDI Services, or the reverse).

**Windows tip:** prefer Windows 11 when you can. Stay on Windows 10 only if you accept the more technical virtualMIDI self-install (this project never ships that driver).

## Suggested reading order

1. Check the prerequisites  
2. Install the Bridge (and virtualMIDI first on Windows 10)  
3. Pass the Windows SmartScreen warning if it appears  
4. Use automatic start  
5. Read the MT4 front-panel lights  
6. Try your first MIDI notes  
7. Try your first SysEx transfer  
8. Fix common problems  
9. Recover when MIDI is stuck (Windows 11 guide — reboot path)  
10. Unplug / replug the MT4  
11. Know what works and what does not  
12. Use two MT4 interfaces (if you have a second unit)  
13. Glossary at the end of each guide  

**SmartScreen:** unsigned or little-known builds — **Run anyway** / **Exécuter quand même**. No Authenticode certificate in this hobby line. Clean-PC USB association often needs **guided** help (**Zadig**), not installer-alone success.

**Licence honesty:** MIT ≠ virtualMIDI ≠ Windows MIDI Services — [`docs/dev/license-and-backends.md`](../dev/license-and-backends.md).
