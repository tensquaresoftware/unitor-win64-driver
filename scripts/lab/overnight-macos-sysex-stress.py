#!/usr/bin/env python3
"""Overnight macOS MT4 hardware stress — mid + bank + long SysEx until duration elapses.

Apple driver only (no Bridge). Topology required:

  - Matrix-1000 on MT4 Out 1 <-> In 1  → mid + bank (Port 1)
  - Red DIN loop Out 2 -> In 2         → long SysEx (Port 2)

Close DAW / editors that monopolize MT4 ports first.

Example (≈8 h; keep the Mac awake):

  caffeinate -dims python3 scripts/lab/overnight-macos-sysex-stress.py --hours 8

Logs:
  tests/lab-logs/overnight-macos/overnight-<UTC>.log
  tests/lab-logs/overnight-macos/cycle-NNNN-{mid,bank,long}/...

Ctrl+C stops between cycles (current child is terminated).
Child labs keep their own pass/fail; this wrapper never aborts on a failed cycle.
"""

from __future__ import annotations

import argparse
import signal
import subprocess
import sys
import time
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def _utc_stamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


@dataclass
class CycleStats:
    mid_ok: int = 0
    mid_fail: int = 0
    bank_ok: int = 0
    bank_fail: int = 0
    long_ok: int = 0
    long_fail: int = 0
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
    cmd = [python, str(script), *args, "--log-dir", str(log_dir)]
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
    long_script = root / "scripts" / "lab" / "sysex-long-loopback.py"
    for path in (mid_script, bank_script, long_script):
        if not path.is_file():
            raise SystemExit(f"Lab script missing: {path}")

    out_root = root / "tests" / "lab-logs" / "overnight-macos"
    out_root.mkdir(parents=True, exist_ok=True)
    stamp = _utc_stamp()
    journal = out_root / f"overnight-{stamp}.log"

    deadline = time.monotonic() + args.hours * 3600.0
    stats = CycleStats()
    stats.note(
        f"START overnight-macos hours={args.hours} "
        f"mid_count={args.mid_count} bank_count={args.bank_count} "
        f"long_count={args.long_count} gap_s={args.cycle_gap} "
        f"matrix_ports={args.matrix_out!r}/{args.matrix_in!r} "
        f"long_ports={args.long_out!r}/{args.long_in!r}"
    )
    stats.note(
        "TOPO expect: Matrix on Out1/In1 (Port 1); red DIN loop Out2->In2 (Port 2)"
    )

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
                    "--out-port",
                    args.matrix_out,
                    "--in-port",
                    args.matrix_in,
                    "--pass-percent",
                    "100",
                    "--fresh-sessions",
                    "1",
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
                    "--out-port",
                    args.matrix_out,
                    "--in-port",
                    args.matrix_in,
                    "--pass-percent",
                    "100",
                    "--fresh-sessions",
                    "1",
                    "--count",
                    str(args.bank_count),
                    "--interval",
                    "0.01",
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

            long_dir = out_root / f"cycle-{cycle:04d}-long"
            long_dir.mkdir(parents=True, exist_ok=True)
            long_rc = _run_child(
                python,
                long_script,
                [
                    "--out-port",
                    args.long_out,
                    "--in-port",
                    args.long_in,
                    "--pass-percent",
                    "100",
                    "--fresh-sessions",
                    "1",
                    "--count",
                    str(args.long_count),
                    "--interval",
                    "0.05",
                    "--reply-timeout",
                    "8",
                    "--sizes",
                    args.long_sizes,
                ],
                long_dir,
                stats,
            )
            if stats.stopped:
                break
            if long_rc == 0:
                stats.long_ok += 1
                stats.note(f"CYCLE {cycle} long exit=0 PASS")
            else:
                stats.long_fail += 1
                stats.note(f"CYCLE {cycle} long exit={long_rc} FAIL")

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
        summary = (
            f"DONE cycles={cycle} "
            f"mid_ok={stats.mid_ok} mid_fail={stats.mid_fail} "
            f"bank_ok={stats.bank_ok} bank_fail={stats.bank_fail} "
            f"long_ok={stats.long_ok} long_fail={stats.long_fail} "
            f"stopped={stats.stopped}"
        )
        stats.note(summary)
        journal.write_text("\n".join(stats.lines) + "\n", encoding="utf-8")
        print(f"Wrote {journal}", flush=True)

    # Soft exit: overnight always "succeeds" as a harness; inspect FAIL counts.
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Overnight macOS MT4 mid+bank+long SysEx stress (Apple driver)"
    )
    parser.add_argument(
        "--hours",
        type=float,
        default=8.0,
        help="Wall-clock duration in hours (default: 8)",
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
        "--long-count",
        type=int,
        default=10,
        help="Reps per long payload size (default: 10)",
    )
    parser.add_argument(
        "--long-sizes",
        default="1024,4096",
        help="Synthetic long sizes (+ fixture unless child skips) (default: 1024,4096)",
    )
    parser.add_argument(
        "--cycle-gap",
        type=float,
        default=5.0,
        help="Seconds between full mid/bank/long cycles (default: 5)",
    )
    parser.add_argument(
        "--matrix-out",
        default="MT4 Port 1",
        help="Apple OUT port for Matrix labs (default: MT4 Port 1)",
    )
    parser.add_argument(
        "--matrix-in",
        default="MT4 Port 1",
        help="Apple IN port for Matrix labs (default: MT4 Port 1)",
    )
    parser.add_argument(
        "--long-out",
        default="MT4 Port 2",
        help="Apple OUT port for long DIN loop (default: MT4 Port 2)",
    )
    parser.add_argument(
        "--long-in",
        default="MT4 Port 2",
        help="Apple IN port for long DIN loop (default: MT4 Port 2)",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.hours <= 0:
        raise SystemExit("--hours must be > 0")
    if args.mid_count < 1 or args.bank_count < 1 or args.long_count < 1:
        raise SystemExit("--mid-count, --bank-count, and --long-count must be >= 1")
    if args.cycle_gap < 0:
        raise SystemExit("--cycle-gap must be >= 0")

    signal.signal(signal.SIGINT, signal.default_int_handler)
    return run_overnight(args)


if __name__ == "__main__":
    sys.exit(main())
