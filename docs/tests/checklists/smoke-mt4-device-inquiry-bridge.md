---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke — MT4 Device Inquiry every-other drop (Bridge)
author: Guillaume DUPONT
created: 2026-08-07
updated: 2026-08-07
---

# Smoke — Device Inquiry round-trip (Bridge + host MIDI)

Lab gate for the Bridge bug where Universal Device Inquiry (`F0 7E 7F 06 01 F7`) on `MT4 Output Y` returned Identity Reply on `MT4 Input X` only about every other time (~50 %).

**Related:** Epic 2.4 vector #1 in `smoke-epic2-matrix-control-mt4.md`. This checklist is the focused host-MIDI retest before Matrix-Control presence.

## Prerequisites

1. Win10/11 x64 Boot Camp (or native), Zadig-bound MT4, Bridge built under `builds/`.
2. Session: `Bridge --start-session --dev-zadig` (or project equivalent).
3. Matrix-1000 on physical DIN Out 1 ↔ In 1 (or matching Y/X under test).
4. Host MIDI tool: automated `scripts/lab/device-inquiry-loop.py` (preferred) or MIDI-OX (not MidiView).
5. Prefer opening **only** `MT4 Output 1` and `MT4 Input 1` (close MIDI-OX if the script uses those ports).

## Bridge console counters

On Inquiry traffic, Bridge should print lines including:

- `inquiry_out` — Device Inquiry WriteBulk succeeded
- `identity_reply_in` — Identity Reply reached `SendToHost` on an Input face

**Healthy:** after a fresh Start, `identity_reply_in` equals `inquiry_out` over ≥20 sends (including #1).

**Bucket if still failing:**

| Pattern | Likely stage |
|---------|----------------|
| `inquiry_out` does not increment | Host→device never wrote (queue / encode / WriteBulk) |
| `inquiry_out` up, `identity_reply_in` flat, `bulk_in_bytes` flat | OUT accepted but no USB IN reply (device / half-duplex) |
| `bulk_in_bytes` up, `identity_reply_in` flat | Demux / framer / SendToHost drop |

## Automated procedure (preferred)

1. Matrix-1000 powered; DIN Out 1 ↔ In 1 cabled. Close MIDI-OX on MT4 ports.
2. One-time deps: `python -m pip install -r scripts/lab/requirements-device-inquiry.txt`
3. One-shot (starts Bridge, runs lab, stops Bridge):

```text
python scripts/lab/device-inquiry-loop.py --with-bridge --count 20 --interval 5
```

4. Logs:
   - `tests/lab-logs/device-inquiry/device-inquiry-<UTC>.log` — SEND / RECV or TIMEOUT + summary
   - `tests/lab-logs/device-inquiry/bridge-<UTC>.log` — Bridge console capture

**Pass:** inquiry log summary `pass=true` (**100 %** Identity Reply after fresh Start, including Inquiry #1) and Bridge `inquiry_out` == `identity_reply_in`. Repeat ≥3 fresh Starts.

Optional: `--interval 10` — success rate should match ~5 s (not a rate-limit artifact).

MIDI-only (if Bridge is already running in a terminal): omit `--with-bridge`.

## MIDI-OX procedure (~5 s) — manual fallback

1. Start Bridge session; confirm Computer Mode wake in console.
2. MIDI-OX: Output = `MT4 Output 1`, Input = `MT4 Input 1`.
3. Disable MIDI-OX Thru if you want a clean Input log (Thru can display `06 02` on the Output pane — that is Thru, not Bridge echo of `06 01`).
4. Send `F0 7E 7F 06 01 F7` **≥20 times**, ~5 seconds apart.
5. Count Identity Reply frames on Input (`F0 7E … 06 02 … F7`).

**Pass:** **100 %** replies including the first after Start. No enduring every-other silence.

## Matrix-Control presence (optional after host-MIDI Pass)

1. Matrix-Control on `MT4 Input 1` / `MT4 Output 1` only.
2. Idle presence 2–5 minutes.
3. **Pass:** no cyclic `deviceMidiUnresponsive` / ERROR footer from missing Inquiry replies.

## Control (already proven)

Scarlett (or other non-Bridge interface) at 1 Inquiry/s should remain ~100 %. Do not blame the Matrix-1000 if Bridge still flaps.

## Out of scope here

- Changing Matrix-Control heartbeat as the primary fix
- MidiView
- AMT8 / Unitor8
- Windows MIDI Services migration

## Lab notes (2026-08-07)

- Full-packet OUT `0xFF` pad to `wMaxPacketSize` did **not** help (~45 % Identity). Reverted to Linux single trailing `0xFF`.
- Async multi-buffer WinUSB bulk IN (Linux `INPUT_URBS` = 7) cleared every-other ~50 % → harness **19/20 = 95 %** (`20260807T121808Z`); residual was **Inquiry #1 after Start only**.
- First-shot fix: arm async IN ring **before** host MIDI sink; brief post-kick IN drain; harvest completed IN slots before sleeping.
- **Gate met:** 3x fresh Start x 20 @ 5 s -> **100 %** each (124924Z, 125116Z, 125305Z); Inquiry #1 RECV ~46-47 ms; inquiry_out == identity_reply_in == 20. No warm-up Inquiry.
- Console: ring armed before sink; Inquiry lines with irst_after_start / ms_since_ring_arm / 5_switch.
- CLI Inquiry log midi_bytes is the host SysEx size; encoded_bytes is the Emagic USB frame length (includes optional F5 + pad).
