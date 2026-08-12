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
        "MT4 Out 1",
        "--in-port",
        "MT4 In 1",
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


def _note_phase_result(stats: CycleStats, cycle: int, label: str, rc: int) -> None:
    if rc == 0:
        setattr(stats, f"{label}_ok", getattr(stats, f"{label}_ok") + 1)
        stats.note(f"CYCLE {cycle} {label} exit=0 PASS")
        return
    setattr(stats, f"{label}_fail", getattr(stats, f"{label}_fail") + 1)
    stats.note(f"CYCLE {cycle} {label} exit={rc} FAIL")


def _child_extra_args(bridge: Path, fresh_starts: int, count: int) -> list[str]:
    return [
        "--bridge-exe",
        str(bridge),
        "--fresh-starts",
        str(fresh_starts),
        "--count",
        str(count),
    ]


def _resolve_paths(args: argparse.Namespace) -> OvernightPaths:
    root = _repo_root()
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
    journal = out_root / f"overnight-{_utc_stamp()}.log"
    return OvernightPaths(
        mid_script=mid_script,
        bank_script=bank_script,
        bridge=bridge,
        out_root=out_root,
        journal=journal,
    )


def _log_start(ctx: OvernightContext) -> None:
    args = ctx.args
    ctx.stats.note(
        f"START overnight-matrix hours={args.hours} "
        f"fresh_starts={args.fresh_starts} mid_count={args.mid_count} "
        f"bank_count={args.bank_count} gap_s={args.cycle_gap} "
        f"bridge={ctx.paths.bridge}"
    )
    ctx.stats.note("TOPO expect: Matrix on MT4 In1/Out1; no red loop on those jacks")


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
            _child_extra_args(
                ctx.paths.bridge, ctx.args.fresh_starts, ctx.args.mid_count
            ),
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
            _child_extra_args(
                ctx.paths.bridge, ctx.args.fresh_starts, ctx.args.bank_count
            ),
            bank_dir,
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
    _run_cycle_gap(ctx.stats, ctx.args.cycle_gap)


def _write_journal(stats: CycleStats, journal: Path, cycle: int) -> None:
    summary = (
        f"DONE cycles={cycle} "
        f"mid_ok={stats.mid_ok} mid_fail={stats.mid_fail} "
        f"bank_ok={stats.bank_ok} bank_fail={stats.bank_fail} "
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
