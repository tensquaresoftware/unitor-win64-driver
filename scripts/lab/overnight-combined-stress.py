#!/usr/bin/env python3
"""Overnight combined Windows stress — Matrix mid+bank then long DIN loopback.

Topology required:

  - Matrix-1000 on MT4 Out 1 <-> In 1  → mid + bank
  - Red DIN loop Out 2 -> In 2         → long SysEx

Close DAW / MIDI-OX / Matrix-Control on MT4 ports first.

Example (≈4 h, keeps the machine awake while this process runs):

  python scripts/lab/overnight-combined-stress.py --hours 4

Preflight (~5 min):

  python scripts/lab/overnight-combined-stress.py --hours 0.083

Logs:
  tests/lab-logs/overnight-combined/overnight-<UTC>.log
  tests/lab-logs/overnight-combined/cycle-NNNN-{mid,bank,long}/...

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
    log_dir: Path


@dataclass(frozen=True)
class OvernightPaths:
    mid_script: Path
    bank_script: Path
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
    cmd = [run.python, str(run.script), *run.args, "--log-dir", str(run.log_dir)]
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


def _note_phase_result(stats: CycleStats, cycle: int, label: str, rc: int) -> None:
    if rc == 0:
        setattr(stats, f"{label}_ok", getattr(stats, f"{label}_ok") + 1)
        stats.note(f"CYCLE {cycle} {label} exit=0 PASS")
        return
    setattr(stats, f"{label}_fail", getattr(stats, f"{label}_fail") + 1)
    stats.note(f"CYCLE {cycle} {label} exit={rc} FAIL")


def _matrix_child_args(args: argparse.Namespace, bridge: Path, count: int) -> list[str]:
    return [
        "--with-bridge",
        "--bridge-exe",
        str(bridge),
        "--out-port",
        args.matrix_out,
        "--in-port",
        args.matrix_in,
        "--pass-percent",
        "100",
        "--fresh-starts",
        str(args.fresh_starts),
        "--count",
        str(count),
    ]


def _long_child_args(args: argparse.Namespace, bridge: Path) -> list[str]:
    return [
        "--with-bridge",
        "--bridge-exe",
        str(bridge),
        "--out-port",
        args.long_out,
        "--in-port",
        args.long_in,
        "--pass-percent",
        "100",
        "--fresh-starts",
        str(args.fresh_starts),
        "--count",
        str(args.long_count),
        "--interval",
        str(args.long_interval),
        "--reply-timeout",
        str(args.long_reply_timeout),
        "--sizes",
        args.long_sizes,
    ]


def _resolve_paths(args: argparse.Namespace) -> OvernightPaths:
    root = _repo_root()
    mid_script = root / "scripts" / "lab" / "sysex-matrix-mid-loop.py"
    bank_script = root / "scripts" / "lab" / "sysex-matrix-bank-loop.py"
    long_script = root / "scripts" / "lab" / "sysex-long-loopback.py"
    for path in (mid_script, bank_script, long_script):
        if not path.is_file():
            raise SystemExit(f"Lab script missing: {path}")

    bridge = Path(args.bridge_exe) if args.bridge_exe else (
        root / "builds" / "debug" / "Debug" / "Bridge.exe"
    )
    if not bridge.is_file():
        raise SystemExit(f"Bridge.exe not found: {bridge}")

    out_root = root / "tests" / "lab-logs" / "overnight-combined"
    out_root.mkdir(parents=True, exist_ok=True)
    journal = out_root / f"overnight-{_utc_stamp()}.log"
    return OvernightPaths(
        mid_script=mid_script,
        bank_script=bank_script,
        long_script=long_script,
        bridge=bridge,
        out_root=out_root,
        journal=journal,
    )


def _log_start(ctx: OvernightContext) -> None:
    args = ctx.args
    paths = ctx.paths
    ctx.stats.note(
        f"START overnight-combined hours={args.hours} "
        f"fresh_starts={args.fresh_starts} mid_count={args.mid_count} "
        f"bank_count={args.bank_count} long_count={args.long_count} "
        f"long_sizes={args.long_sizes!r} gap_s={args.cycle_gap} "
        f"matrix_ports={args.matrix_out!r}/{args.matrix_in!r} "
        f"long_ports={args.long_out!r}/{args.long_in!r} bridge={paths.bridge}"
    )
    ctx.stats.note("TOPO expect: Matrix on Out1/In1; red DIN loop Out2->In2")


def _run_labeled_child(
    stats: CycleStats,
    cycle: int,
    label: str,
    run: ChildRun,
) -> None:
    rc = _run_child(run, stats)
    if stats.stopped:
        return
    _note_phase_result(stats, cycle, label, rc)


def _run_cycle_gap(stats: CycleStats, gap_s: float) -> None:
    # Keep `> 0` (not `<= 0` early-return) so NaN skips the gap like pre-refactor.
    if gap_s > 0:
        stats.note(f"GAP sleep_s={gap_s}")
        try:
            time.sleep(gap_s)
        except KeyboardInterrupt:
            stats.stopped = True
            stats.note("INTERRUPT gap")


def _phase_dir(ctx: OvernightContext, cycle: int, label: str) -> Path:
    path = ctx.paths.out_root / f"cycle-{cycle:04d}-{label}"
    path.mkdir(parents=True, exist_ok=True)
    return path


def _run_mid_phase(ctx: OvernightContext, cycle: int) -> None:
    mid_dir = _phase_dir(ctx, cycle, "mid")
    _run_labeled_child(
        ctx.stats,
        cycle,
        "mid",
        ChildRun(
            sys.executable,
            ctx.paths.mid_script,
            _matrix_child_args(ctx.args, ctx.paths.bridge, ctx.args.mid_count),
            mid_dir,
        ),
    )


def _run_bank_phase(ctx: OvernightContext, cycle: int) -> None:
    bank_dir = _phase_dir(ctx, cycle, "bank")
    _run_labeled_child(
        ctx.stats,
        cycle,
        "bank",
        ChildRun(
            sys.executable,
            ctx.paths.bank_script,
            _matrix_child_args(ctx.args, ctx.paths.bridge, ctx.args.bank_count),
            bank_dir,
        ),
    )


def _run_long_phase(ctx: OvernightContext, cycle: int) -> None:
    long_dir = _phase_dir(ctx, cycle, "long")
    _run_labeled_child(
        ctx.stats,
        cycle,
        "long",
        ChildRun(
            sys.executable,
            ctx.paths.long_script,
            _long_child_args(ctx.args, ctx.paths.bridge),
            long_dir,
        ),
    )


def _run_one_cycle(ctx: OvernightContext, cycle: int) -> None:
    remaining_h = max(0.0, (ctx.deadline - time.monotonic()) / 3600.0)
    ctx.stats.note(f"CYCLE {cycle} remaining_h={remaining_h:.2f}")

    _run_mid_phase(ctx, cycle)
    if _should_stop(ctx):
        return
    _run_bank_phase(ctx, cycle)
    if _should_stop(ctx):
        return
    _run_long_phase(ctx, cycle)
    if _should_stop(ctx):
        return
    _run_cycle_gap(ctx.stats, ctx.args.cycle_gap)


def _write_journal(stats: CycleStats, journal: Path, cycle: int) -> None:
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
        description=(
            "Overnight combined Matrix mid+bank + long DIN loopback "
            "(Windows Bridge)"
        )
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
        help="Synthetic long sizes (+ fixture) (default: 1024,4096)",
    )
    parser.add_argument(
        "--long-interval",
        type=float,
        default=0.05,
        help="Seconds between long-loopback trial starts (default: 0.05)",
    )
    parser.add_argument(
        "--long-reply-timeout",
        type=float,
        default=8.0,
        help="Seconds to wait for exact long loopback reply (default: 8)",
    )
    parser.add_argument(
        "--cycle-gap",
        type=float,
        default=5.0,
        help="Seconds between full mid/bank/long cycles (default: 5)",
    )
    parser.add_argument(
        "--matrix-out",
        default="MT4 Out 1",
        help="Virtual OUT for Matrix labs (default: MT4 Out 1)",
    )
    parser.add_argument(
        "--matrix-in",
        default="MT4 In 1",
        help="Virtual IN for Matrix labs (default: MT4 In 1)",
    )
    parser.add_argument(
        "--long-out",
        default="MT4 Out 2",
        help="Virtual OUT for red DIN loop (default: MT4 Out 2)",
    )
    parser.add_argument(
        "--long-in",
        default="MT4 In 2",
        help="Virtual IN for red DIN loop (default: MT4 In 2)",
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
    if args.mid_count < 1 or args.bank_count < 1 or args.long_count < 1:
        raise SystemExit("--mid-count, --bank-count, and --long-count must be >= 1")
    if args.long_interval < 0.05:
        raise SystemExit("--long-interval must be >= 0.05")
    if args.long_reply_timeout <= 0:
        raise SystemExit("--long-reply-timeout must be > 0")
    if args.cycle_gap < 0:
        raise SystemExit("--cycle-gap must be >= 0")
    for label, value in (
        ("--matrix-out", args.matrix_out),
        ("--matrix-in", args.matrix_in),
        ("--long-out", args.long_out),
        ("--long-in", args.long_in),
    ):
        if not value.strip():
            raise SystemExit(f"{label} must be non-empty")

    signal.signal(signal.SIGINT, signal.default_int_handler)
    return run_overnight(args)


if __name__ == "__main__":
    sys.exit(main())
