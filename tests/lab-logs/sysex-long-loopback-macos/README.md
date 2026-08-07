# SysEx long DIN loopback lab logs (macOS Apple MT4 driver)

Palier-3 option A: physical **Out 1 → In 1** DIN loopback (no Matrix). Exact
byte match of large SysEx through the Apple MT4 driver.

**Checklist:** `docs/tests/checklists/smoke-mt4-sysex-long-loopback.md`

## Prerequisites

1. Cable DIN **Out 1 → In 1**. Disconnect Matrix from those jacks.
2. MT4 USB; close DAWs on MT4 ports.
3. `python3 -m pip install -r scripts/lab/requirements-device-inquiry.txt`

## Ports used on this Mac

- OUT: `MT4 Port 1`
- IN: `MT4 Port 1`

## One-shot (2 fresh sessions)

```text
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

Payloads: synthetic 1024 B + 4096 B + fixture `long-loopback-14708.syx`.

## Pass bar

Exit 0 and `overall_pass=true` only when every reply equals the sent frame.
