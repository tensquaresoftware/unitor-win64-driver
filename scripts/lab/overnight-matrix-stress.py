#!/usr/bin/env python3
"""Overnight Matrix stress — loop palier-1 mid + palier-2 bank labs until duration elapses.

Topo (required): Matrix-1000 on MT4 In1/Out1. No red DIN loopback on those jacks.
Close DAW / MIDI-OX / Matrix-Control on MT4 ports first.

Example (≈8 h, keeps the machine awake while this process runs):

  python scripts/lab/overnight-matrix-stress.py --hours 8

Logs:
  tests/lab-logs/overnight-matrix/overnight-<UTC>.log          — cycle journal
  tests/lab-logs/overnight-matrix/cycle-NNNN-<lab>/...         — child lab logs

Ctrl+C stops between cycles (current child is terminated).
Child labs keep their own pass/fail; this wrapper never aborts on a failed cycle.
"""

from __future__ import annotations

import argparse
import ctypes
import signal
import subprocess
import sys
import time
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path

# Windows: prevent sleep while this process is alive (display may still blank).
ES_CONTINUOUS = 0x80000000
ES_SYSTEM_REQUIRED = 0x00000001


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def _utc_stamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def _hold_awake() -> None:
    if sys.platform != "win32":
        return
    ctypes.windll.kernel32.SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED)


def _release_awake() -> None:
    if sys.platform != "win32":
        return
    ctypes.windll.kernel32.SetThreadExecutionState(ES_CONTINUOUS)


@dataclass
class CycleStats:
    mid_ok: int = 0
    mid_fail: int = 0
    bank_ok: int = 0
    bank_fail: int = 0
    stopped: bool = False
    lines: list[str] = field(default_factory=list)

    def note(self, line: str) -> None:
        stamp = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        self.lines.append(f"{stamp} {line}")
        print(line, flush=True)


def _run_child(
    python: str,
    script: Path,
    args: list[str],
    log_dir: Path,
    stats: CycleStats,
) -> int:
    cmd = [
        python,
        str(script),
        "--with-bridge",
        "--out-port",
        "MT4 Out 1",
        "--in-port",
        "MT4 In 1",
        "--pass-percent",
        "100",
        "--log-dir",
        str(log_dir),
        *args,
    ]
    stats.note("RUN " + " ".join(cmd))
    try:
        completed = subprocess.run(cmd, cwd=str(_repo_root()))
    except KeyboardInterrupt:
        stats.stopped = True
        stats.note("INTERRUPT child")
        return 130
    return int(completed.returncode)


def run_overnight(args: argparse.Namespace) -> int:
    root = _repo_root()
    python = sys.executable
    mid_script = root / "scripts" / "lab" / "sysex-matrix-mid-loop.py"
    bank_script = root / "scripts" / "lab" / "sysex-matrix-bank-loop.py"
    if not mid_script.is_file() or not bank_script.is_file():
        raise SystemExit("Matrix lab scripts missing under scripts/lab/")

    bridge = Path(args.bridge_exe) if args.bridge_exe else (
        root / "builds" / "debug" / "Debug" / "Bridge.exe"
    )
    if not bridge.is_file():
        raise SystemExit(f"Bridge.exe not found: {bridge}")

    out_root = root / "tests" / "lab-logs" / "overnight-matrix"
    out_root.mkdir(parents=True, exist_ok=True)
    stamp = _utc_stamp()
    journal = out_root / f"overnight-{stamp}.log"

    deadline = time.monotonic() + args.hours * 3600.0
    stats = CycleStats()
    stats.note(
        f"START overnight-matrix hours={args.hours} "
        f"fresh_starts={args.fresh_starts} mid_count={args.mid_count} "
        f"bank_count={args.bank_count} gap_s={args.cycle_gap} "
        f"bridge={bridge}"
    )
    stats.note("TOPO expect: Matrix on MT4 In1/Out1; no red loop on those jacks")

    _hold_awake()
    cycle = 0
    try:
        while time.monotonic() < deadline and not stats.stopped:
            cycle += 1
            remaining_h = max(0.0, (deadline - time.monotonic()) / 3600.0)
            stats.note(f"CYCLE {cycle} remaining_h={remaining_h:.2f}")

            mid_dir = out_root / f"cycle-{cycle:04d}-mid"
            mid_dir.mkdir(parents=True, exist_ok=True)
            mid_rc = _run_child(
                python,
                mid_script,
                [
                    "--bridge-exe",
                    str(bridge),
                    "--fresh-starts",
                    str(args.fresh_starts),
                    "--count",
                    str(args.mid_count),
                ],
                mid_dir,
                stats,
            )
            if stats.stopped:
                break
            if mid_rc == 0:
                stats.mid_ok += 1
                stats.note(f"CYCLE {cycle} mid exit=0 PASS")
            else:
                stats.mid_fail += 1
                stats.note(f"CYCLE {cycle} mid exit={mid_rc} FAIL")

            if time.monotonic() >= deadline or stats.stopped:
                break

            bank_dir = out_root / f"cycle-{cycle:04d}-bank"
            bank_dir.mkdir(parents=True, exist_ok=True)
            bank_rc = _run_child(
                python,
                bank_script,
                [
                    "--bridge-exe",
                    str(bridge),
                    "--fresh-starts",
                    str(args.fresh_starts),
                    "--count",
                    str(args.bank_count),
                ],
                bank_dir,
                stats,
            )
            if stats.stopped:
                break
            if bank_rc == 0:
                stats.bank_ok += 1
                stats.note(f"CYCLE {cycle} bank exit=0 PASS")
            else:
                stats.bank_fail += 1
                stats.note(f"CYCLE {cycle} bank exit={bank_rc} FAIL")

            if time.monotonic() >= deadline or stats.stopped:
                break
            if args.cycle_gap > 0:
                stats.note(f"GAP sleep_s={args.cycle_gap}")
                try:
                    time.sleep(args.cycle_gap)
                except KeyboardInterrupt:
                    stats.stopped = True
                    stats.note("INTERRUPT gap")
                    break
    finally:
        _release_awake()

    summary = (
        f"DONE cycles={cycle} "
        f"mid_ok={stats.mid_ok} mid_fail={stats.mid_fail} "
        f"bank_ok={stats.bank_ok} bank_fail={stats.bank_fail} "
        f"stopped={stats.stopped}"
    )
    stats.note(summary)
    journal.write_text("\n".join(stats.lines) + "\n", encoding="utf-8")
    print(f"Wrote {journal}", flush=True)

    # Soft exit: overnight always "succeeds" as a harness; inspect FAIL counts.
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Overnight Matrix mid+bank stress (Windows Bridge)"
    )
    parser.add_argument(
        "--hours",
        type=float,
        default=8.0,
        help="Wall-clock duration in hours (default: 8)",
    )
    parser.add_argument(
        "--fresh-starts",
        type=int,
        default=1,
        help="Bridge Starts per child lab (default: 1 for overnight throughput)",
    )
    parser.add_argument(
        "--mid-count",
        type=int,
        default=10,
        help="Reps per mid scenario (default: 10)",
    )
    parser.add_argument(
        "--bank-count",
        type=int,
        default=100,
        help="Patch dumps per bank lab (default: 100)",
    )
    parser.add_argument(
        "--cycle-gap",
        type=float,
        default=5.0,
        help="Seconds between mid/bank cycle pairs (default: 5)",
    )
    parser.add_argument(
        "--bridge-exe",
        default="",
        help="Path to Bridge.exe (default: builds/debug/Debug/Bridge.exe)",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.hours <= 0:
        raise SystemExit("--hours must be > 0")
    if args.fresh_starts < 1:
        raise SystemExit("--fresh-starts must be >= 1")
    if args.mid_count < 1 or args.bank_count < 1:
        raise SystemExit("--mid-count and --bank-count must be >= 1")
    if args.cycle_gap < 0:
        raise SystemExit("--cycle-gap must be >= 0")

    # Ignore SIGINT in parent until child returns; child inherits default.
    signal.signal(signal.SIGINT, signal.default_int_handler)
    return run_overnight(args)


if __name__ == "__main__":
    sys.exit(main())
