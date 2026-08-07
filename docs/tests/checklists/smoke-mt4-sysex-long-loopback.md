---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke — MT4 long SysEx DIN loopback (palier 3)
author: Guillaume DUPONT
created: 2026-08-07
updated: 2026-08-07
---

# Smoke — long SysEx DIN loopback (palier 3, option A)

Prove large SysEx integrity through MT4 USB↔DIN without Matrix-1000.

**Specs:**
- macOS Apple (done): `_bmad-output/implementation-artifacts/spec-sysex-long-loopback.md`
- Windows Bridge (Mac parity): `_bmad-output/implementation-artifacts/spec-sysex-long-loopback-2.md`

## Prerequisites

1. **DIN Out 1 → In 1 physical loopback cable** (Matrix disconnected from those jacks).
2. MT4 USB connected; close DAWs holding ports.
3. Deps: `python -m pip install -r scripts/lab/requirements-device-inquiry.txt`
4. **macOS:** Apple driver ports (e.g. `MT4 Port 1`).
5. **Windows (Bridge):** Zadig + Bridge Debug; paliers 1–2 Windows already 100 % on this machine.
6. Bridge ceilings for Mac-parity sizes: `kMaxSysexHoldBytes = 16384` and
   `kEncodeBufferCapacity = 16384` (see Windows spec). Oversize above that remains an
   English-observable reject.

## Automated procedure — macOS

```text
python3 scripts/lab/sysex-long-loopback.py --list-ports
python3 scripts/lab/sysex-long-loopback.py \
  --out-port "MT4 Port 1" \
  --in-port "MT4 Port 1" \
  --count 20 \
  --interval 0.05 \
  --reply-timeout 8 \
  --sizes 1024,4096 \
  --pass-percent 100 \
  --fresh-sessions 2 \
  --session-gap 2 \
  --log-dir tests/lab-logs/sysex-long-loopback-macos
```

Never pass `--with-bridge` on macOS.

## Automated procedure — Windows Bridge

Mac-parity gate (synthetic 1024 + 4096 + fixture ~14708):

```text
python scripts/lab/sysex-long-loopback.py --with-bridge --pass-percent 100
```

Defaults: `MT4 Out 1` / `MT4 In 1`, `--fresh-starts 2`, fixture on.
Logs under `tests/lab-logs/sysex-long-loopback/` (+ bridge start logs).

Hardware control (do not reopen): macOS Apple already 100 % on 1024/4096/14708 —
`docs/tests/lab-reports/macos-sysex-paliers-2026-08-07.md`.

## Pass criteria

| Item | Pass |
|---|---|
| Each trial | Received SysEx **byte-identical** to sent (reassembled to `F7`) |
| Payloads | Synthetic 1024 + 4096 + fixture `long-loopback-14708.syx` (~14708 B) |
| Fresh sessions | 100 % on each of ≥2 cold opens / Bridge Starts |
| Overall | Exit 0 and `overall_pass=true` |

## Out of scope

- Python ACK/dump responder (option B) without Ask First
- Matrix-Control UI gate
- Lowering the 100 % bar

## Lab notes

- macOS Apple 2026-08-07: clean `overall_pass=true` on 2×(20×1024 + 20×4096 + 20×14708)
  with DIN Out1→In1 loopback — see `tests/lab-logs/sysex-long-loopback-macos/`.
- Early aborted run had first-session `synth_1024` TIMEOUTs before loopback was live;
  re-run after settle is the evidence SSOT.
- Windows Bridge: use prompt `docs/tests/lab-prompts/lab-palier-3-sysex-long-loopback.md`
  and spec `spec-sysex-long-loopback-2.md`.
