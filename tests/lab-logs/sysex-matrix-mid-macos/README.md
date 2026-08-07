# SysEx Matrix mid-size lab logs (macOS Apple MT4 driver)

Hardware control evidence for editor-sized Matrix SysEx via the official Apple
MT4 driver (no Bridge, no WinUSB, no VirtualMIDI). Compare with the Windows
Bridge lab under `../sysex-matrix-mid/`.

## Prerequisites

1. MT4 connected over USB; Apple MIDI ports visible (Audio MIDI Setup).
2. Matrix-1000 powered; DIN Out 1 ↔ In 1.
3. Close DAWs / editors that hold those ports.
4. Install deps:

```text
python3 -m pip install -r scripts/lab/requirements-device-inquiry.txt
```

## Discover ports (do not invent names)

```text
python3 scripts/lab/sysex-matrix-mid-loop.py --list-ports
```

Pick the Apple/Emagic/MT4 pair that matches Out 1 / In 1 from the printed list.
Defaults are Windows Bridge names (`MT4 Output 1` / `MT4 Input 1`) — always pass
explicit `--out-port` / `--in-port` on macOS.

### Ports used on this Mac (2026-08-07 lab)

- OUT: `MT4 Port 1`
- IN: `MT4 Port 1`

Resolved names appear in each session body as `# out_port:` / `# in_port:`.
The multi-session parent header records the CLI needles as
`# out_port_needle:` / `# in_port_needle:`.

## Preferred one-shot (2 fresh sessions)

```text
python3 scripts/lab/sysex-matrix-mid-loop.py \
  --out-port "MT4 Port 1" \
  --in-port "MT4 Port 1" \
  --pass-percent 100 \
  --count 10 \
  --reply-timeout 3 \
  --interval 1 \
  --fresh-sessions 2 \
  --session-gap 2 \
  --log-dir tests/lab-logs/sysex-matrix-mid-macos
```

Session 1 runs pushes + dumps. Session 2 starts a **new Python process** (ports
close/reopen) after `--session-gap` and runs dumps only — first dump after cold
open. This is a software reopen, not a full USB re-plug; for a stronger cold
start, unplug/replug USB, then run a second dumps-only capture into a **new**
`--log` file (or use `--append-log` explicitly if you intentionally extend an
existing journal — default re-run without that flag overwrites).

Use `--fresh-sessions` on macOS (not `--fresh-starts`; that flag is Bridge-only).
Never pass `--with-bridge` on macOS.
Do not combine `--pushes-only` with `--fresh-sessions > 1`.

## Files

| Pattern | Meaning |
|---|---|
| `sysex-matrix-mid-<UTC>.log` | SEND / RECV / TIMEOUT / FAIL + summaries + `overall_pass` |
| `README.md` | This operator note |

## Pass bar

Exit 0 and `overall_pass=true` only at 100 % per scenario. Any dump TIMEOUT or
wrong size → overall Fail (log `len`, head, tail, `dt_ms`).
