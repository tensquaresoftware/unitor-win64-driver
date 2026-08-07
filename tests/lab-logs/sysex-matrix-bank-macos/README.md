# SysEx Matrix bank-burst lab logs (macOS Apple MT4 driver)

Hardware control evidence for palier-2 bank-scale device→host SysEx
(~100× 275 B patch dumps) via the official Apple MT4 driver.
Compare with Windows Bridge runs under `../sysex-matrix-bank/` when available.

**Checklist:** `docs/tests/checklists/smoke-mt4-sysex-matrix-bank.md`

## Prerequisites

1. MT4 connected over USB; Apple MIDI ports visible.
2. Matrix-1000 powered; DIN Out 1 ↔ In 1.
3. Close DAWs / editors that hold those ports.
4. Install deps:

```text
python3 -m pip install -r scripts/lab/requirements-device-inquiry.txt
```

## Discover ports

```text
python3 scripts/lab/sysex-matrix-bank-loop.py --list-ports
```

### Ports used on this Mac (2026-08-07 lab)

- OUT: `MT4 Port 1`
- IN: `MT4 Port 1`

## Preferred one-shot (2 fresh sessions × 100 dumps)

```text
python3 scripts/lab/sysex-matrix-bank-loop.py \
  --out-port "MT4 Port 1" \
  --in-port "MT4 Port 1" \
  --count 100 \
  --interval 0.01 \
  --reply-timeout 3 \
  --pass-percent 100 \
  --fresh-sessions 2 \
  --session-gap 2 \
  --log-dir tests/lab-logs/sysex-matrix-bank-macos
```

Never pass `--with-bridge` on macOS. Use `--fresh-sessions` (not `--fresh-starts`).

## Pass bar

Exit 0 and `overall_pass=true` only at 100 % of frames (exactly 275 B each).
Any TIMEOUT or wrong size → overall Fail (`index`, `slot`, `len`, head, tail, `dt_ms`).
