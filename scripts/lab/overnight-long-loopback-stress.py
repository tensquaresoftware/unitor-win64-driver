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


@dataclass(frozen=True)
class ChildRun:
    python: str
    script: Path
    args: list[str]
    out_port: str
    in_port: str
    log_dir: Path


@dataclass(frozen=True)
class OvernightPaths:
    long_script: Path
    bridge: Path
    out_root: Path
    journal: Path


@dataclass
class OvernightContext:
    args: argparse.Namespace
    paths: OvernightPaths
    stats: CycleStats
    deadline: float


def _run_child(run: ChildRun, stats: CycleStats) -> int:
    cmd = [
        run.python,
        str(run.script),
        "--with-bridge",
        "--out-port",
        run.out_port,
        "--in-port",
        run.in_port,
        "--pass-percent",
        "100",
        "--log-dir",
        str(run.log_dir),
        *run.args,
    ]
    stats.note("RUN " + " ".join(cmd))
    try:
        completed = subprocess.run(cmd, cwd=str(_repo_root()))
    except KeyboardInterrupt:
        stats.stopped = True
        stats.note("INTERRUPT child")
        return 130
    return int(completed.returncode)


def _should_stop(ctx: OvernightContext) -> bool:
    # Match pre-refactor `while monotonic < deadline` (NaN deadline ⇒ stop).
    return (not time.monotonic() < ctx.deadline) or ctx.stats.stopped


def _long_child_args(bridge: Path, args: argparse.Namespace) -> list[str]:
    return [
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
    ]


def _note_long_result(stats: CycleStats, cycle: int, rc: int) -> None:
    if rc == 0:
        stats.long_ok += 1
        stats.note(f"CYCLE {cycle} long exit=0 PASS")
        return
    stats.long_fail += 1
    stats.note(f"CYCLE {cycle} long exit={rc} FAIL")


def _run_cycle_gap(stats: CycleStats, gap_s: float) -> None:
    # Keep `> 0` so NaN skips the gap like pre-refactor.
    if gap_s > 0:
        stats.note(f"GAP sleep_s={gap_s}")
        try:
            time.sleep(gap_s)
        except KeyboardInterrupt:
            stats.stopped = True
            stats.note("INTERRUPT gap")


def _resolve_paths(args: argparse.Namespace) -> OvernightPaths:
    root = _repo_root()
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
    return OvernightPaths(
        long_script=long_script,
        bridge=bridge,
        out_root=out_root,
        journal=out_root / f"overnight-{stamp}.log",
    )


def _log_start(ctx: OvernightContext) -> None:
    args = ctx.args
    ctx.stats.note(
        f"START overnight-long-loopback hours={args.hours} "
        f"fresh_starts={args.fresh_starts} long_count={args.long_count} "
        f"sizes={args.sizes!r} gap_s={args.cycle_gap} "
        f"ports={args.out_port!r}/{args.in_port!r} bridge={ctx.paths.bridge}"
    )
    ctx.stats.note("TOPO expect: red DIN loop Out2->In2; Matrix may stay on In1/Out1")


def _run_one_cycle(ctx: OvernightContext, cycle: int) -> None:
    remaining_h = max(0.0, (ctx.deadline - time.monotonic()) / 3600.0)
    ctx.stats.note(f"CYCLE {cycle} remaining_h={remaining_h:.2f}")

    long_dir = ctx.paths.out_root / f"cycle-{cycle:04d}"
    long_dir.mkdir(parents=True, exist_ok=True)
    rc = _run_child(
        ChildRun(
            python=sys.executable,
            script=ctx.paths.long_script,
            args=_long_child_args(ctx.paths.bridge, ctx.args),
            out_port=ctx.args.out_port,
            in_port=ctx.args.in_port,
            log_dir=long_dir,
        ),
        ctx.stats,
    )
    if ctx.stats.stopped:
        return
    _note_long_result(ctx.stats, cycle, rc)
    if _should_stop(ctx):
        return
    _run_cycle_gap(ctx.stats, ctx.args.cycle_gap)


def _write_journal(stats: CycleStats, journal: Path, cycle: int) -> None:
    summary = (
        f"DONE cycles={cycle} "
        f"long_ok={stats.long_ok} long_fail={stats.long_fail} "
        f"stopped={stats.stopped}"
    )
    stats.note(summary)
    journal.write_text("\n".join(stats.lines) + "\n", encoding="utf-8")
    print(f"Wrote {journal}", flush=True)


def run_overnight(args: argparse.Namespace) -> int:
    paths = _resolve_paths(args)
    ctx = OvernightContext(
        args=args,
        paths=paths,
        stats=CycleStats(),
        deadline=time.monotonic() + args.hours * 3600.0,
    )
    _log_start(ctx)

    _hold_awake()
    cycle = 0
    try:
        while not _should_stop(ctx):
            cycle += 1
            _run_one_cycle(ctx, cycle)
    finally:
        _release_awake()

    _write_journal(ctx.stats, paths.journal, cycle)
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
