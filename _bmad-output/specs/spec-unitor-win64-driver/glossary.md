# Glossary — unitor-win64-driver V1

Load-bearing terms for Spec consumers. Product names stay in English as used in the field.

| Term | Meaning |
| --- | --- |
| **Bridge** | V1 usermode Windows process that talks to the MT4 over WinUSB, implements Emagic cable mapping, and exposes Virtual Ports. |
| **MT4** | Emagic MT4 USB MIDI interface, USB identity `VID 086A` / `PID 0003`, physical **2 IN / 4 OUT**. |
| **Virtual Port** | Named virtual MIDI endpoint exposed to Windows MIDI clients (DAWs, utilities, editors). |
| **Port Name** | Display name: unit 1 `MT4 Port N`; unit `K≥2` `MT4 #K Port N` (`N` 1..4). |
| **DeviceProfile** | Declarative per-PID hardware profile (masks, interface number, capability flags). |
| **DeviceSession** | Per-MT4 runtime session (own WinUSB handle, mapper state, Virtual Port set). |
| **SysEx Session** | Editor/librarian traffic including large or bursty System Exclusive transfers. |
| **Matrix-Control** | Ten Square first-party SysEx validation target; **not** a Bridge runtime dependency. |
| **Validation Matrix** | Locked V1 host/OS set — see `validation-matrix.md`. |
| **MIDI Path** | End-to-end path used for latency/jitter measurement through Bridge and Virtual Ports — **not** ASIO audio buffer size. |
| **VirtualMIDI** | Tobias Erichsen’s proprietary virtual MIDI **SDK** and driver stack; Bridge creates/destroys ports programmatically; driver must be present. |
| **Windows MIDI Services** | Microsoft MIDI stack; future **second backend on Windows 11 only**; not the V1 target. |
| **Auto-Start** | Bridge starts with Windows and/or on MT4 USB arrival. |
| **Hot-Plug Recovery** | After unplug/replug, usable Virtual Ports return without Windows reboot; host rescan or supervised Bridge restart OK. |
| **Public Installer** | End-user installer for community redistribution (subject to VirtualMIDI author clearance). |
| **Studio-Done Gate** | Timing acceptance: published MIDI Path harness results confirm or revise thresholds. Gate **2026-08-11** outcome **(a)** confirmed healthy ≤4–5 ms p99 latency and ≤1–2 ms p99 classical jitter. |
| **Ten Square Software** | Public facade / organization for the OSS project. |
