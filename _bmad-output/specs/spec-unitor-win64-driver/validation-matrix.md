# Validation Matrix — unitor-win64-driver V1

Inherited from PRD §10. Host SKUs and OS targets are locked; bumping DAW majors is a PRD change only.

## Hosts and OS

| Layer | Target | OS | Notes |
| --- | --- | --- | --- |
| DAW | **Ableton Live 12** | Win10 x64, Win11 x64 | Performance MIDI + clock |
| DAW | **Reason Studios 12** | Win10 x64, Win11 x64 | Second DAW host |
| SysEx editor | **Matrix-Control** | Win10 x64, Win11 x64 | First-party SysEx path; not a Bridge dependency |
| MIDI utility | **MIDI-OX** | Win10 x64, Win11 x64 | Multi-client concurrent with a DAW |
| Hardware | ≥1 **MT4** (`086A:0003`) | — | Second MT4 when available for multi-instance proof |
| Timing harness | MIDI Path method (Architecture AD-11; `docs/dev/measurements/`) | Win10 x64 (min) | Required for Studio-Done Gate — **(a)** confirmed 2026-08-11 |

## Pass rules (non-SysEx)

- Each DAW row: open ports, notes/CC, clock + Start/Stop/Continue + MTC smoke on both OS targets.
- MIDI-OX + one DAW: concurrent observation without exclusive-lock failure.
- Stability sample: ~4h including SysEx activity on at least Win10 x64.
- Hot-plug: one documented recovery drill without Windows reboot.

## Minimum SysEx pass vectors (Matrix-Control)

Protocol family: **Oberheim Matrix** (`F0 10 06 …`). Matrix-Control does **not** speak Emagic framing — the Bridge carries these frames **transparently**. Detail opcodes and pacing: adopted companion `matrix-control-sysex-extract.md`.

| # | Vector | Shape (summary) | Approx size |
| --- | ---: | --- | --- |
| 1 | **Device Inquiry** round-trip | `F0 7E 7F 06 01 F7` → Universal reply incl. Oberheim/Matrix identity | 6 → 15 B |
| 2 | **Single patch dump** | Request `F0 10 06 04 01 <patch> F7` (7 B) → patch frame | **275 B** |
| 3 | **Master dump** | Request `F0 10 06 04 03 00 F7` → master frame | **351 B** |
| 4 | **Edit-buffer / patch push** | Outbound **275 B** patch write (slot `01` and/or edit-buffer `0D`) completes without Bridge restart | **275 B** |
| 5 | **Live editor stream** | Sustained short remote edits (**7 B** param / **9 B** matrix-mod) with normal Matrix-Control spacing; no Bridge restart | 7 / 9 B |
| 6 | **Bank stress (optional)** | Bank export/import path ≈ **100×** sequential 275 B patch frames (~28 KB inbound series) when hardware/time allow | ~28 KB series |
| 7 | **Mixed-wire tolerance** | Non-patch SysEx during a dump must not permanently block a later valid patch frame | — |

Primary validation target: **Matrix-1000**. Packed payloads: patch **134 B**, master **172 B** (nibble-encoded on the wire).

## Timing anchors (Studio-Done Gate)

> **Status:** Gate **2026-08-11** outcome **(a) confirm** — healthy targets stand. Decision: [`docs/dev/measurements/studio-done-gate-decision.md`](../../../docs/dev/measurements/studio-done-gate-decision.md).

| Metric | Healthy target | Do-not-ship-worse ceiling |
| --- | --- | --- |
| Bridge-added latency (MIDI Path, p99) | ≤ **4–5 ms** (**confirmed**) | ~**8–10 ms** (above requires explicit product decision) |
| Classical jitter (MIDI Path, `jitter_us_p99`) | ≤ **1–2 ms** (**confirmed**) | Excessive jitter is not excused because path is usermode |

Measure **MIDI Path** only — never ASIO buffer size. Harness: Architecture AD-11. Software-loop plumbing ≠ bridge-added WinUSB/MT4 proof alone; `latency_spread_us` ≠ classical jitter.
