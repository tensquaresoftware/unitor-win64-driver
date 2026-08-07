# Device Inquiry lab logs

Automated host-side evidence for the Bridge Device Inquiry round-trip lab
(`docs/tests/checklists/smoke-mt4-device-inquiry-bridge.md`).

## One-shot (preferred)

Matrix powered + DIN cabled. Close MIDI-OX. Then:

```text
python -m pip install -r scripts/lab/requirements-device-inquiry.txt
python scripts/lab/device-inquiry-loop.py --with-bridge --count 20 --interval 5
```

The script starts `builds/debug/Debug/Bridge.exe --start-session --dev-zadig`, waits for
`MT4 Output 1` / `MT4 Input 1`, runs the Inquiry loop, then stops Bridge.

## MIDI-only (Bridge already running)

```text
python scripts/lab/device-inquiry-loop.py --count 20 --interval 5
```

## Files

| Pattern | Meaning |
|---|---|
| `device-inquiry-<UTC>.log` | SEND / RECV or TIMEOUT per inquiry + summary |
| `bridge-<UTC>.log` | Bridge stdout/stderr capture (`--with-bridge` only) |
