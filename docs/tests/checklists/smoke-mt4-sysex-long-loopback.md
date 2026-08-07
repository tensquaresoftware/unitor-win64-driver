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

**Spec:** `_bmad-output/implementation-artifacts/spec-sysex-long-loopback.md`

## Prerequisites

1. **DIN Out 1 → In 1 physical loopback cable** (Matrix disconnected from those jacks).
2. MT4 USB connected; close DAWs holding ports.
3. Deps: `python -m pip install -r scripts/lab/requirements-device-inquiry.txt`
4. **macOS:** Apple driver ports (e.g. `MT4 Port 1`).
5. **Windows later:** Zadig + Bridge + same DIN loopback.

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

## Pass criteria

| Item | Pass |
|---|---|
| Each trial | Received SysEx **byte-identical** to sent (reassembled to `F7`) |
| Payloads | Synthetic 1024 + 4096 + fixture `long-loopback-14708.syx` (~14708 B) |
| Fresh sessions | 100 % on each of ≥2 cold opens |
| Overall | Exit 0 and `overall_pass=true` |

## Out of scope

- Python ACK/dump responder (option B)
- Matrix-Control UI gate
- Bridge C++ fixes in this lab

## Lab notes

- macOS Apple 2026-08-07: clean `overall_pass=true` on 2×(20×1024 + 20×4096 + 20×14708)
  with DIN Out1→In1 loopback — see `tests/lab-logs/sysex-long-loopback-macos/`.
- Early aborted run had first-session `synth_1024` TIMEOUTs before loopback was live;
  re-run after settle is the evidence SSOT.
