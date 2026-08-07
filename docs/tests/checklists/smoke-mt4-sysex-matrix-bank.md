---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke — MT4 Matrix bank burst SysEx (palier 2)
author: Guillaume DUPONT
created: 2026-08-07
updated: 2026-08-07
---

# Smoke — Matrix bank burst (100× patch dump)

Lab gate for bank-scale device→host SysEx (~100× 275 B patch frames) after
palier-1 mid-size patch/master is already proven.

**Related:** Epic 2 optional vector #6 in `smoke-epic2-matrix-control-mt4.md`.
**Spec:** `_bmad-output/implementation-artifacts/spec-sysex-matrix-bank-burst.md`

## Prerequisites

1. Matrix-1000 powered; DIN Out 1 ↔ In 1.
2. Close DAWs / Matrix-Control / MIDI-OX on the MT4 ports under test.
3. Deps: `python -m pip install -r scripts/lab/requirements-device-inquiry.txt`
4. **macOS (Apple driver):** MT4 USB; ports visible (e.g. `MT4 Port 1`).
5. **Windows (Bridge, later):** Zadig MT4 + `builds/debug/Debug/Bridge.exe`.

## Automated procedure — macOS Apple control

```text
python3 scripts/lab/sysex-matrix-bank-loop.py --list-ports
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

## Automated procedure — Windows Bridge (later)

```text
python scripts/lab/sysex-matrix-bank-loop.py --with-bridge --pass-percent 100
```

Defaults: `MT4 Output 1` / `MT4 Input 1`, `--fresh-starts 2`, same count/interval/timeout.

## Pass criteria (strict 100 %)

| Item | Pass |
|---|---|
| Each frame | Request `F0 10 06 04 01 <n> F7` → exactly **275** B `F0 10 06 01 … F7` within reply timeout |
| Pacing | ≥10 ms between trial starts (default `--interval 0.01`) |
| Fresh sessions / Starts | Full burst **100 %** on each of ≥2 cold opens |
| Overall | Script exit 0 and log `overall_pass=true` |

Harness reassembles SysEx until `F7` (never Pass on a fragment). No opaque Inquiry warm-up.

## Fail diagnostics

On TIMEOUT / wrong size, log records `index`, `slot`, `len`, head/tail, `dt_ms`.
Do not blame the Matrix if MT4 DIN LEDs show activity on a Fail.

## Out of scope

- Real Oberheim Dump All export (Ask First / follow-up)
- Palier 3 mega-SysEx / echo responder
- Matrix-Control UI as primary gate
- AMT8 / Unitor8 / Windows MIDI Services

## Lab notes

- macOS Apple control 2026-08-07: stamp `20260807T171139Z`, ports `MT4 Port 1`/`MT4 Port 1`,
  2×100 = 100 %, `overall_pass=true` — see `tests/lab-logs/sysex-matrix-bank-macos/`.
- Windows Bridge bank lab: not run in this pass (harness supports `--with-bridge`).
- Palier 3 mega-SysEx: deferred pending device→host hardware decision.
