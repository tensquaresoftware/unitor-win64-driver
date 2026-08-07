# SysEx Matrix mid-size lab logs

Automated host-side evidence for Bridge mid-size Matrix SysEx round-trip
(`docs/tests/checklists/smoke-mt4-sysex-matrix-mid.md`).

## One-shot (preferred)

Matrix powered + DIN Out1↔In1. Close MIDI-OX / Matrix-Control on MT4 ports. Then:

```text
python -m pip install -r scripts/lab/requirements-device-inquiry.txt
python scripts/lab/sysex-matrix-mid-loop.py --with-bridge --pass-percent 100
```

The script starts `builds/debug/Debug/Bridge.exe --start-session --dev-zadig`, waits for
`MT4 Output 1` / `MT4 Input 1`, runs push+dump scenarios (default 10× each), repeats
dumps on a second fresh Start, then stops Bridge.

## MIDI-only (Bridge already running)

```text
python scripts/lab/sysex-matrix-mid-loop.py --pass-percent 100
```

## Files

| Pattern | Meaning |
|---|---|
| `sysex-matrix-mid-<UTC>.log` | SEND / RECV / TIMEOUT / FAIL per trial + per-scenario summary |
| `bridge-<UTC>-start<N>.log` | Bridge stdout/stderr capture (`--with-bridge` only) |
