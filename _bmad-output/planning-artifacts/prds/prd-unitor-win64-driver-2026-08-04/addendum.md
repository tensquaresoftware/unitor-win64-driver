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
- **Multi-instance / port naming:** per `prd.md` FR-5, FR-10, §11 — exact multi-MT4 strings in Architecture/UX.
- **MIDI backends:** VirtualMIDI **SDK** for V1 (programmatic port create/destroy; Win10+11); abstract layer so Windows MIDI Services can follow as a second backend (**Win11 only**).
- **Reference only:** Linux `sound/usb/midi.c` and `quirks-table.h` — reimplement under MIT; no vendored GPL.
- **External integration proof (not a fork base):** https://github.com/aaron1a12/virtual-midi demonstrates VirtualMIDI SDK integration is possible; it is **GPL** with a vendored SDK — useful as existence proof only; **do not fork** as the project base.
- **Pattern inspiration:** Prodikeys64 (WinUSB + virtual MIDI).

## VirtualMIDI licensing (research snapshot)

From Tobias Erichsen’s VirtualMIDI SDK materials (paraphrase; confirm with author):

- SDK is for evaluation/integration; **not** freeware.
- The Bridge is expected to create/destroy ports **via the SDK API** (not rely solely on the end-user VirtualMIDI application UI).
- Software linking to the SDK **may not be distributed** without prior clearance.
- Without a commercial license, evaluation typically expects a pre-installed loopMIDI/rtpMIDI (driver present).
- Licensees can obtain an MSI / merge module to integrate the driver into application setup.
- Project MIT license does **not** cover this dependency.
- Author outreach is a **release gate** for any redistributable Public Installer — **not** a blocker for PRD finalization or Architecture start. Contact status and owner: `prd.md` OQ-1.

Useful URLs:
- https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html
- https://www.tobias-erichsen.de/software.html

## Rejected / deferred alternatives

| Idea | Status | Rationale |
| --- | --- | --- |
| Custom KMDF / PortCls MIDI driver | Rejected for V1 | Signing + complexity vs community ship |
| Home-grown kernel “VirtualMIDI Plan B” (equivalent driver) | Deferred / rejected for V1 | Too long for V1; independence via backend abstraction + possible Windows MIDI Services later |
| Windows MIDI Services as only V1 backend | Rejected | Not a Win10 solution; Win10 is mandatory |
| Vendor GPL Linux sources into tree | Rejected | Would force GPL; conflicts with MIT intent |
| Fork `aaron1a12/virtual-midi` as project base | Rejected | GPL + vendored SDK; use only as integration existence proof |
| Guarantee AMT8/Unitor8 in V1 | Rejected | No test hardware commitment |
| ShowMIDI / MidiView as Validation Matrix utility | Retired | Lab: MidiView BSOD; ShowMIDI weak / no practical file log. PRD locks **MIDI-OX** as the V1 multi-client utility (2026-08-10) |

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

Background for Architecture and harness design — normative thresholds and Studio-Done Gate policy: `prd.md` NFR-P1–P3, SM-9, OQ-2.

- USB full-speed class MIDI historically ties to ~1 ms frame timing.
- Studio control/sequencing often tolerates small single-digit ms end-to-end; feel and clock stability depend more on **jitter** than absolute latency alone.
- Public historical USB-MIDI figures are order-of-magnitude only (class devices / older OS stacks) — not contractual SLAs for Emagic bulk + VirtualMIDI.
- Always measure the **MIDI Path** (not ASIO buffer size); harness design in Architecture.

## Installer UX bar (product intent)

Normative requirement: `prd.md` FR-12. Implementation technology is left to Architecture.

## Validation Matrix lock (PRD session)

Locked during PRD Fast path (2026-08-04). Canonical host/OS table and pass rules: `prd.md` §10.

## Matrix-Control SysEx pass vectors (grounded extract)

Normative minimum pass vectors and mixed-wire tolerance: `prd.md` §10. This section provides grounded extract detail for Architecture and test design.

Source: Matrix-Control repo (`SysExConstants.h`, encoders, bank import/export). Full extract: `matrix-control-sysex-extract.md`.

**Protocol family:** Oberheim Matrix (`F0 10 06 …`); Matrix-Control does **not** speak Emagic framing — the Bridge must carry these frames transparently.

| Vector | Shape (summary) | Approx size |
| --- | --- | --- |
| Device Inquiry | `F0 7E 7F 06 01 F7` → Universal reply | 6 → 15 B |
| Single patch dump | Request `04 01 <patch>` → response opcode `01` | 7 → **275 B** |
| Master dump | Request `04 03 00` → response `03 03` | → **351 B** |
| Patch / edit-buffer push | Outbound `01` (slot) or `0D` (edit buffer) | **275 B** |
| Live edits | Remote param `06` / matrix-mod `0B` | **7 B** / **9 B** |
| Bank stress (optional) | ~100× sequential patch dumps / restores | ~28 KB series |

Packed payloads: patch **134 B**, master **172 B** (nibble-encoded on the wire). Primary validation target: **Matrix-1000**; Matrix-6/6R member bytes are provisional in Matrix-Control.

## Windows MIDI Services landscape note

As of early 2026 public messaging, Windows MIDI Services GA focus is Windows 11. It does not replace WinUSB + Emagic protocol decode for this hardware. Relevant later as an optional Virtual Port backend on **Win11 only** — not a Win10 V1 substitute.
