#!/usr/bin/env python3
"""Overnight long SysEx DIN loopback stress — loop palier-3 lab until duration elapses.

Topo (required): red DIN loopback on MT4 Out2 -> In2. Matrix may stay on In1/Out1.
Close DAW / MIDI-OX / Matrix-Control on MT4 ports first.

Example (≈4 h, keeps the machine awake while this process runs):

  python scripts/lab/overnight-long-loopback-stress.py --hours 4

Preflight (~5 min):

  python scripts/lab/overnight-long-loopback-stress.py --hours 0.083

Logs:
  tests/lab-logs/overnight-long-loopback/overnight-<UTC>.log   — cycle journal
  tests/lab-logs/overnight-long-loopback/cycle-NNNN/...        — child lab logs

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
    out_port: str,
    in_port: str,
    log_dir: Path,
    stats: CycleStats,
) -> int:
    cmd = [
        python,
        str(script),
        "--with-bridge",
        "--out-port",
        out_port,
        "--in-port",
        in_port,
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
    long_script = root / "scripts" / "lab" / "sysex-long-loopback.py"
    if not long_script.is_file():
        raise SystemExit("Long loopback lab script missing under scripts/lab/")

    bridge = Path(args.bridge_exe) if args.bridge_exe else (
        root / "builds" / "debug" / "Debug" / "Bridge.exe"
    )
    if not bridge.is_file():
        raise SystemExit(f"Bridge.exe not found: {bridge}")

    out_root = root / "tests" / "lab-logs" / "overnight-long-loopback"
    out_root.mkdir(parents=True, exist_ok=True)
    stamp = _utc_stamp()
    journal = out_root / f"overnight-{stamp}.log"

    deadline = time.monotonic() + args.hours * 3600.0
    stats = CycleStats()
    stats.note(
        f"START overnight-long-loopback hours={args.hours} "
        f"fresh_starts={args.fresh_starts} long_count={args.long_count} "
        f"sizes={args.sizes!r} gap_s={args.cycle_gap} "
        f"ports={args.out_port!r}/{args.in_port!r} bridge={bridge}"
    )
    stats.note("TOPO expect: red DIN loop Out2->In2; Matrix may stay on In1/Out1")

    _hold_awake()
    cycle = 0
    try:
        while time.monotonic() < deadline and not stats.stopped:
            cycle += 1
            remaining_h = max(0.0, (deadline - time.monotonic()) / 3600.0)
            stats.note(f"CYCLE {cycle} remaining_h={remaining_h:.2f}")

            long_dir = out_root / f"cycle-{cycle:04d}"
            long_dir.mkdir(parents=True, exist_ok=True)
            long_rc = _run_child(
                python,
                long_script,
                [
                    "--bridge-exe",
                    str(bridge),
                    "--fresh-starts",
                    str(args.fresh_starts),
                    "--count",
                    str(args.long_count),
                    "--sizes",
                    args.sizes,
                    "--interval",
                    str(args.interval),
                    "--reply-timeout",
                    str(args.reply_timeout),
                ],
                args.out_port,
                args.in_port,
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
        _release_awake()

    summary = (
        f"DONE cycles={cycle} "
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
        description="Overnight long SysEx DIN loopback stress (Windows Bridge)"
    )
    parser.add_argument(
        "--hours",
        type=float,
        default=4.0,
        help="Wall-clock duration in hours (default: 4)",
    )
    parser.add_argument(
        "--fresh-starts",
        type=int,
        default=1,
        help="Bridge Starts per child lab (default: 1 for overnight throughput)",
    )
    parser.add_argument(
        "--long-count",
        type=int,
        default=10,
        help="Reps per long payload size (default: 10)",
    )
    parser.add_argument(
        "--sizes",
        default="1024,4096",
        help="Synthetic SysEx sizes (+ on-disk fixture) (default: 1024,4096)",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=0.05,
        help="Seconds between trial starts in the child lab (default: 0.05)",
    )
    parser.add_argument(
        "--reply-timeout",
        type=float,
        default=8.0,
        help="Seconds to wait for exact loopback reply (default: 8)",
    )
    parser.add_argument(
        "--cycle-gap",
        type=float,
        default=5.0,
        help="Seconds between long cycles (default: 5)",
    )
    parser.add_argument(
        "--out-port",
        default="MT4 Out 2",
        help="MIDI output port for the red DIN loop (default: MT4 Out 2)",
    )
    parser.add_argument(
        "--in-port",
        default="MT4 In 2",
        help="MIDI input port for the red DIN loop (default: MT4 In 2)",
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
    if args.long_count < 1:
        raise SystemExit("--long-count must be >= 1")
    if args.interval < 0.05:
        raise SystemExit("--interval must be >= 0.05")
    if args.reply_timeout <= 0:
        raise SystemExit("--reply-timeout must be > 0")
    if args.cycle_gap < 0:
        raise SystemExit("--cycle-gap must be >= 0")
    if not args.out_port.strip() or not args.in_port.strip():
        raise SystemExit("--out-port and --in-port must be non-empty")

    # Ignore SIGINT in parent until child returns; child inherits default.
    signal.signal(signal.SIGINT, signal.default_int_handler)
    return run_overnight(args)


if __name__ == "__main__":
    sys.exit(main())
