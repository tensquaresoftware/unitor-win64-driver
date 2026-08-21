# WMS midisrv restart robustness — durable evidence capsule (2026-08-21)

## Verdict

**provisional cause: midisrv/service** after clean Bridge WMS stop. No Bridge/WMS teardown C++ change in this Build.

## Path A (reproduced)

- Immediate post-stop `enumerate_midi_ports(timeout=20)` → **TimeoutExpired**
- Bridge stop: ~0.047 s, exit 0, no pump/overflow fail needles
- midisrv process still listed Running while enum is dead (liveness ≠ healthy enum)

Ephemeral journals (gitignored lab tree):

- `tests/lab-logs/wms-midisrv-restart/repro-20260821T094519Z.log`
- `tests/lab-logs/wms-midisrv-restart/bridge-20260821T094519Z.log`
- `tests/lab-logs/wms-midisrv-restart/evidence-20260821T094519Z.md`

## Path C (lab harness)

- Shared helpers in `scripts/lab/lab_midi_common.py`: timed enum, **midisrv suspect** exit text, documented reset, `start_one_clean_wms_bridge`
- Repro driver: `scripts/lab/wms-midisrv-restart-repro.py`
- Elevated `--apply-midisrv-reset` requires admin; without elevation recovery stays documented-only (`path_c_documented_only`)

Operator recovery (one shot):

```powershell
# elevated PowerShell
Restart-Service midisrv
# if needed:
# Stop-Process -Name midisrv -Force -ErrorAction SilentlyContinue
# Start-Service midisrv

.\.venv-lab\Scripts\python.exe scripts/lab/wms-midisrv-restart-repro.py `
  --recover-only --apply-midisrv-reset --one-clean-bridge
```

## Follow-up (out of this Build exit)

≥1 h single-session longevity gate remains open — see `spec-wms-session-longevity-stress.md`. Do not start longevity until elevated midisrv reset + one clean Bridge succeed.
