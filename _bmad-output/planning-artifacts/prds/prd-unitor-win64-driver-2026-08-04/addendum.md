# Addendum — PRD: unitor-win64-driver

Supporting depth for Architecture and implementation. Read `prd.md` first for requirements and acceptance. Source brief package: `briefs/brief-unitor-win64-driver-2026-08-04/`.

## Hardware & cable masks (Linux quirk reference)

| Model | VID | PID | Expected I/O | `in_cables` | `out_cables` | `ifnum` | V1 role |
| --- | --- | --- | --- | --- | --- | --- | --- |
| MT4 | `086A` | `0003` | 2 IN / 4 OUT | `0x8003` | `0x800f` | `2` | Validated MVP |
| AMT8 | `086A` | `0002` | 8×8 (expected) | `0x80ff` | `0x80ff` | `2` | Post-MVP hardware validation |
| Unitor8 | `086A` | `0001` | 8×8 (expected) | `0x80ff` | `0x80ff` | `2` | Post-MVP hardware validation |

Linux uses `QUIRK_MIDI_EMAGIC` with the same endpoint-info shape; only bitmasks differ (`quirks-table.h`).

## Architecture orientation (not PRD requirements restated)

Pointers for Architecture — locked product decisions live in `prd.md` §11 / brief.

- **Transport:** WinUSB (`winusb.sys` + `winusb.dll`); INF and/or Zadig during development; installer-driven for end users.
- **Protocol:** single Emagic cable mapper; `DeviceProfile` holds per-PID masks, interface number, and optional capability flags (Patch/LTC/etc. off in V1).
- **Multi-instance:** multiple MT4 devices concurrently (separate sessions and port sets); distinct from Emagic cascade topologies.
- **MIDI backends:** VirtualMIDI for V1 (Win10+11); abstract layer so Windows MIDI Services can follow as a second backend (Win11-oriented).
- **Reference only:** Linux `sound/usb/midi.c` and `quirks-table.h` — reimplement under MIT; no vendored GPL.
- **Pattern inspiration:** Prodikeys64 (WinUSB + virtual MIDI).
- **Port naming:** macOS-like `MT4 Port N`; stable across sessions; per-unit disambiguation when multiple MT4s are present (exact strings in Architecture/UX).

## VirtualMIDI licensing (research snapshot)

From Tobias Erichsen’s VirtualMIDI SDK materials (paraphrase; confirm with author):

- SDK is for evaluation/integration; **not** freeware.
- Software linking to the SDK **may not be distributed** without prior clearance.
- Without a commercial licence, evaluation typically expects a pre-installed loopMIDI/rtpMIDI (driver present).
- Licensees can obtain an MSI / merge module to integrate the driver into application setup.
- Project MIT license does **not** cover this dependency.
- Author outreach is a **release gate** for any redistributable Public Installer.

Useful URLs:
- https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html
- https://www.tobias-erichsen.de/software.html

## Rejected / deferred alternatives

| Idea | Status | Rationale |
| --- | --- | --- |
| Custom KMDF / PortCls MIDI driver | Rejected for V1 | Signing + complexity vs community ship |
| Windows MIDI Services as only V1 backend | Rejected | Not a Win10 solution; Win10 is mandatory |
| Vendor GPL Linux sources into tree | Rejected | Would force GPL; conflicts with MIT intent |
| Guarantee AMT8/Unitor8 in V1 | Rejected | No test hardware commitment |
| MIDI-OX as primary Validation Matrix utility | Deferred/replaced | PRD locks **ShowMIDI** as the V1 multi-client utility |

## Community evidence (problem persistence)

- https://www.modwiggler.com/forum/viewtopic.php?t=142226
- https://gearspace.com/board/music-computers/141084-emagic-amt-8-drivers-2.html
- https://forum.cockos.com/archive/index.php/t-191736.html

Functional manual (features mostly out of V1 scope):  
https://www.deepsonic.ch/deep/docs_manuals/emagic_unitor8_mkII_amt8_manual.pdf

## Naming / trademarks

Repo name `unitor-win64-driver` uses family protocol name **Unitor**, not the Emagic trademark in the project title. Emagic/product names appear descriptively in docs. Public facade: **Ten Square Software**.

## Dev / validation environment

| Role | Machine |
| --- | --- |
| Primary editing | macOS + Cursor |
| Build, USB, DAW, SysEx validation | Windows 10/11 64-bit PC (Win10 mandatory in matrix) |

Quality gate when C++ exists: `python scripts/quality/lint-touched.py` per `conventions.md` §3.

## Latency planning context (for Studio-Done Gate)

- USB full-speed class MIDI historically ties to ~1 ms frame timing.
- Studio control/sequencing often tolerates small single-digit ms end-to-end; feel and clock stability depend more on **jitter** than absolute latency alone.
- Public historical USB-MIDI figures are order-of-magnitude only (class devices / older OS stacks) — not contractual SLAs for Emagic bulk + VirtualMIDI.
- PRD provisional targets: bridge-added p99 latency in low single-digit ms; jitter sub-ms to low-ms; always publish MIDI Path method.

## Installer UX bar (product intent)

End-user install should feel closer to a polished macOS installer than to a developer toolchain: short steps, visible progress, clear success, explicit VirtualMIDI prerequisite messaging, minimal jargon. One-time admin elevation for install is expected; daily use is not. Implementation technology is left to Architecture.

## Validation Matrix lock (PRD session)

Locked during PRD Fast path (2026-08-04):

| Role | Product |
| --- | --- |
| DAW | Ableton Live 12 |
| DAW | Reason Studios 12 |
| SysEx | Matrix-Control |
| Utility | ShowMIDI |
| OS | Windows 10 x64 (mandatory) + Windows 11 x64 |

Timing policy: provisional anchors + measurement harness gate before Studio-Done (not hard-final numbers in first draft).

## Windows MIDI Services landscape note

As of early 2026 public messaging, Windows MIDI Services GA focus is Windows 11. It does not replace WinUSB + Emagic protocol decode for this hardware. Relevant later as an optional Virtual Port backend on Win11 — not a Win10 V1 substitute.
