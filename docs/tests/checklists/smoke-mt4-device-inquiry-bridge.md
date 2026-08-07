---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke — MT4 Device Inquiry every-other drop (Bridge)
author: Guillaume DUPONT
created: 2026-08-07
updated: 2026-08-07
---

# Smoke — Device Inquiry round-trip (Bridge + MIDI-OX)

Lab gate for the Bridge bug where Universal Device Inquiry (`F0 7E 7F 06 01 F7`) on `MT4 Output Y` returned Identity Reply on `MT4 Input X` only about every other time (~50 %).

**Related:** Epic 2.4 vector #1 in `smoke-epic2-matrix-control-mt4.md`. This checklist is the focused MIDI-OX retest before Matrix-Control presence.

## Prerequisites

1. Win10/11 x64 Boot Camp (or native), Zadig-bound MT4, Bridge built under `builds/`.
2. Session: `Bridge --start-session --dev-zadig` (or project equivalent).
3. Matrix-1000 on physical DIN Out 1 ↔ In 1 (or matching Y/X under test).
4. MIDI-OX (not MidiView).
5. Prefer opening **only** `MT4 Output 1` and `MT4 Input 1` in MIDI-OX.

## Bridge console counters

On Inquiry traffic, Bridge should print lines including:

- `inquiry_out` — Device Inquiry WriteBulk succeeded
- `identity_reply_in` — Identity Reply reached `SendToHost` on an Input face

**Healthy:** `identity_reply_in` stays within ~5 % of `inquiry_out` over ≥20 sends.

**Bucket if still failing:**

| Pattern | Likely stage |
|---------|----------------|
| `inquiry_out` does not increment | Host→device never wrote (queue / encode / WriteBulk) |
| `inquiry_out` up, `identity_reply_in` flat, `bulk_in_bytes` flat | OUT accepted but no USB IN reply (device / half-duplex) |
| `bulk_in_bytes` up, `identity_reply_in` flat | Demux / framer / SendToHost drop |

## MIDI-OX procedure (~5 s)

1. Start Bridge session; confirm Computer Mode wake in console.
2. MIDI-OX: Output = `MT4 Output 1`, Input = `MT4 Input 1`.
3. Disable MIDI-OX Thru if you want a clean Input log (Thru can display `06 02` on the Output pane — that is Thru, not Bridge echo of `06 01`).
4. Send `F0 7E 7F 06 01 F7` **≥20 times**, ~5 seconds apart.
5. Count Identity Reply frames on Input (`F0 7E … 06 02 … F7`).

**Pass:** ≥95 % replies (target 100 %). No enduring every-other silence.

Optional: repeat at ~10 s — success rate should match ~5 s (not a rate-limit artifact).

## Matrix-Control presence (optional after MIDI-OX Pass)

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
