#!/usr/bin/env python3
"""WMS single-session longevity stress — one Bridge for mid→bank→long loops.

Keeps a single Bridge process alive for the whole run
(`--start-session --dev-zadig --midi-backend=wms`). Child labs reuse existing
ports and must NOT receive `--with-bridge`.

Topology required (same as overnight-combined):

  - Matrix-1000 on MT4 Out 1 <-> In 1  → mid + bank
  - Red DIN loop Out 2 -> In 2         → long SysEx

Close DAW / MIDI-OX / Matrix-Control on MT4 ports first.

Example (~1 h gate):

  .venv-lab\\Scripts\\python.exe scripts/lab/wms-session-longevity-stress.py --hours 1

Preflight (~5 min):

  .venv-lab\\Scripts\\python.exe scripts/lab/wms-session-longevity-stress.py --hours 0.083

Logs:
  tests/lab-logs/wms-session-longevity/longevity-<UTC>.log
  tests/lab-logs/wms-session-longevity/bridge-<UTC>.log
  tests/lab-logs/wms-session-longevity/cycle-NNNN-{mid,bank,long}/...

Aborts immediately on first child FAIL, Bridge death, or midisrv loss mid-run.
Exit non-zero on FAIL (unlike overnight-combined soft-count).
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

_LAB_DIR = Path(__file__).resolve().parent
if str(_LAB_DIR) not in sys.path:
    sys.path.insert(0, str(_LAB_DIR))

import lab_midi_common as lab_midi  # noqa: E402

# Windows: prevent sleep while this process is alive (display may still blank).
ES_CONTINUOUS = 0x80000000
ES_SYSTEM_REQUIRED = 0x00000001

BRIDGE_EXTRA_ARGS = ["--dev-zadig", "--midi-backend=wms"]


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
    aborted: bool = False
    abort_reason: str = ""
    bridge_pid: int | None = None
    lines: list[str] = field(default_factory=list)

    def note(self, line: str) -> None:
        stamp = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        self.lines.append(f"{stamp} {line}")
        print(line, flush=True)

    def abort(self, reason: str) -> None:
        self.aborted = True
        self.stopped = True
        self.abort_reason = reason
        self.note(f"ABORT {reason}")


@dataclass(frozen=True)
class ChildRun:
    python: str
    script: Path
    args: list[str]
    log_dir: Path


@dataclass(frozen=True)
class LongevityPaths:
    mid_script: Path
    bank_script: Path
    long_script: Path
    bridge: Path
    out_root: Path
    journal: Path
    bridge_log: Path


@dataclass
class LongevityContext:
    args: argparse.Namespace
    paths: LongevityPaths
    stats: CycleStats
    deadline: float
    bridge: lab_midi.BridgeSession | None = None


def _preflight_midisrv(stats: CycleStats) -> None:
    ok, detail = lab_midi.midisrv_status()
    if ok:
        stats.note(f"PREFLIGHT midisrv OK ({detail})")
        return
    raise SystemExit(
        "PREFLIGHT FAIL: midisrv / Windows MIDI Services not running "
        f"({detail}).\n\n{lab_midi.MIDISRV_RESET_PROCEDURE}"
        "After reset, start exactly one clean Bridge session "
        "(see lab_midi_common.start_one_clean_wms_bridge / "
        "wms-midisrv-restart-repro.py --apply-midisrv-reset --one-clean-bridge), "
        "then re-run longevity. Do not loop aggressive Bridge restarts."
    )


def _check_midisrv_alive(stats: CycleStats, where: str) -> bool:
    ok, detail = lab_midi.midisrv_status()
    if ok:
        return True
    stats.abort(f"midisrv died mid-run at {where}: {detail}")
    return False


def _bridge_alive(bridge: lab_midi.BridgeSession) -> bool:
    return bridge.proc is not None and bridge.proc.poll() is None


def _check_bridge_alive(ctx: LongevityContext, where: str) -> bool:
    bridge = ctx.bridge
    if bridge is None:
        ctx.stats.abort(f"Bridge missing at {where}")
        return False
    if _bridge_alive(bridge):
        return True
    code = bridge.proc.returncode if bridge.proc is not None else "?"
    ctx.stats.abort(
        f"Bridge died mid-run at {where} "
        f"(pid={ctx.stats.bridge_pid} exit={code} log={ctx.paths.bridge_log})"
    )
    return False


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


def _should_stop(ctx: LongevityContext) -> bool:
    return (
        (not time.monotonic() < ctx.deadline)
        or ctx.stats.stopped
        or ctx.stats.aborted
    )


def _note_phase_result(stats: CycleStats, cycle: int, label: str, rc: int) -> None:
    if rc == 0:
        setattr(stats, f"{label}_ok", getattr(stats, f"{label}_ok") + 1)
        stats.note(f"CYCLE {cycle} {label} exit=0 PASS")
        return
    setattr(stats, f"{label}_fail", getattr(stats, f"{label}_fail") + 1)
    stats.note(f"CYCLE {cycle} {label} exit={rc} FAIL")
    stats.abort(f"child {label} FAIL exit={rc} cycle={cycle}")


def _matrix_child_args(args: argparse.Namespace, count: int) -> list[str]:
    # External Bridge owns the session — never pass --with-bridge.
    return [
        "--out-port",
        args.matrix_out,
        "--in-port",
        args.matrix_in,
        "--pass-percent",
        "100",
        "--count",
        str(count),
    ]


def _long_child_args(args: argparse.Namespace) -> list[str]:
    return [
        "--out-port",
        args.long_out,
        "--in-port",
        args.long_in,
        "--pass-percent",
        "100",
        "--count",
        str(args.long_count),
        "--interval",
        str(args.long_interval),
        "--reply-timeout",
        str(args.long_reply_timeout),
        "--sizes",
        args.long_sizes,
    ]


def _resolve_paths(args: argparse.Namespace) -> LongevityPaths:
    root = _repo_root()
    mid_script = root / "scripts" / "lab" / "sysex-matrix-mid-loop.py"
    bank_script = root / "scripts" / "lab" / "sysex-matrix-bank-loop.py"
    long_script = root / "scripts" / "lab" / "sysex-long-loopback.py"
    for path in (mid_script, bank_script, long_script):
        if not path.is_file():
            raise SystemExit(f"Lab script missing: {path}")

    bridge = Path(args.bridge_exe) if args.bridge_exe else lab_midi.default_bridge_exe()
    if not bridge.is_file():
        raise SystemExit(f"Bridge.exe not found: {bridge}")

    out_root = root / "tests" / "lab-logs" / "wms-session-longevity"
    out_root.mkdir(parents=True, exist_ok=True)
    stamp = _utc_stamp()
    return LongevityPaths(
        mid_script=mid_script,
        bank_script=bank_script,
        long_script=long_script,
        bridge=bridge,
        out_root=out_root,
        journal=out_root / f"longevity-{stamp}.log",
        bridge_log=out_root / f"bridge-{stamp}.log",
    )


def _confirm_ports(args: argparse.Namespace, stats: CycleStats) -> None:
    outs, inns = lab_midi.fresh_midi_port_names()
    matrix_out = lab_midi.select_port(outs, args.matrix_out, "output")
    matrix_in = lab_midi.select_port(inns, args.matrix_in, "input")
    long_out = lab_midi.select_port(outs, args.long_out, "output")
    long_in = lab_midi.select_port(inns, args.long_in, "input")
    stats.note(
        f"PREFLIGHT ports OK matrix={matrix_out!r}/{matrix_in!r} "
        f"long={long_out!r}/{long_in!r}"
    )


def _start_bridge(ctx: LongevityContext) -> None:
    bridge = lab_midi.BridgeSession(
        ctx.paths.bridge,
        ctx.paths.bridge_log,
        list(BRIDGE_EXTRA_ARGS),
    )
    bridge.start()
    ctx.bridge = bridge
    assert bridge.proc is not None
    ctx.stats.bridge_pid = bridge.proc.pid
    ctx.stats.note(
        f"BRIDGE started pid={bridge.proc.pid} "
        f"args={' '.join(BRIDGE_EXTRA_ARGS)} log={ctx.paths.bridge_log}"
    )
    # Wait for Matrix pair first (session ready), then confirm DIN pair.
    out_name, in_name = bridge.wait_until_ready(
        ctx.args.matrix_out,
        ctx.args.matrix_in,
        ctx.args.bridge_ready_timeout,
    )
    ctx.stats.note(f"BRIDGE ready matrix OUT={out_name} IN={in_name}")
    _confirm_ports(ctx.args, ctx.stats)


def _log_start(ctx: LongevityContext) -> None:
    args = ctx.args
    paths = ctx.paths
    ctx.stats.note(
        f"START wms-session-longevity hours={args.hours} "
        f"mid_count={args.mid_count} bank_count={args.bank_count} "
        f"long_count={args.long_count} long_sizes={args.long_sizes!r} "
        f"gap_s={args.cycle_gap} "
        f"matrix_ports={args.matrix_out!r}/{args.matrix_in!r} "
        f"long_ports={args.long_out!r}/{args.long_in!r} "
        f"bridge={paths.bridge} backend=wms single_session=1"
    )
    ctx.stats.note("TOPO expect: Matrix on Out1/In1; red DIN loop Out2->In2")
    ctx.stats.note("POLICY abort_on_first_FAIL=1 children_with_bridge=0")


def _run_labeled_child(
    ctx: LongevityContext,
    cycle: int,
    label: str,
    run: ChildRun,
) -> None:
    if not _check_bridge_alive(ctx, f"before {label}"):
        return
    if not _check_midisrv_alive(ctx.stats, f"before {label}"):
        return
    rc = _run_child(run, ctx.stats)
    if ctx.stats.stopped and not ctx.stats.aborted:
        return
    if not _check_bridge_alive(ctx, f"after {label}"):
        return
    if not _check_midisrv_alive(ctx.stats, f"after {label}"):
        return
    _note_phase_result(ctx.stats, cycle, label, rc)


def _run_cycle_gap(stats: CycleStats, gap_s: float) -> None:
    if gap_s > 0:
        stats.note(f"GAP sleep_s={gap_s}")
        try:
            time.sleep(gap_s)
        except KeyboardInterrupt:
            stats.stopped = True
            stats.note("INTERRUPT gap")


def _phase_dir(ctx: LongevityContext, cycle: int, label: str) -> Path:
    path = ctx.paths.out_root / f"cycle-{cycle:04d}-{label}"
    path.mkdir(parents=True, exist_ok=True)
    return path


def _run_mid_phase(ctx: LongevityContext, cycle: int) -> None:
    mid_dir = _phase_dir(ctx, cycle, "mid")
    _run_labeled_child(
        ctx,
        cycle,
        "mid",
        ChildRun(
            sys.executable,
            ctx.paths.mid_script,
            _matrix_child_args(ctx.args, ctx.args.mid_count),
            mid_dir,
        ),
    )


def _run_bank_phase(ctx: LongevityContext, cycle: int) -> None:
    bank_dir = _phase_dir(ctx, cycle, "bank")
    _run_labeled_child(
        ctx,
        cycle,
        "bank",
        ChildRun(
            sys.executable,
            ctx.paths.bank_script,
            _matrix_child_args(ctx.args, ctx.args.bank_count),
            bank_dir,
        ),
    )


def _run_long_phase(ctx: LongevityContext, cycle: int) -> None:
    long_dir = _phase_dir(ctx, cycle, "long")
    _run_labeled_child(
        ctx,
        cycle,
        "long",
        ChildRun(
            sys.executable,
            ctx.paths.long_script,
            _long_child_args(ctx.args),
            long_dir,
        ),
    )


def _run_one_cycle(ctx: LongevityContext, cycle: int) -> None:
    remaining_h = max(0.0, (ctx.deadline - time.monotonic()) / 3600.0)
    ctx.stats.note(
        f"CYCLE {cycle} remaining_h={remaining_h:.2f} "
        f"bridge_pid={ctx.stats.bridge_pid}"
    )

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
        f"bridge_pid={stats.bridge_pid} "
        f"aborted={stats.aborted} stopped={stats.stopped}"
    )
    if stats.abort_reason:
        summary += f" abort_reason={stats.abort_reason!r}"
    stats.note(summary)
    journal.write_text("\n".join(stats.lines) + "\n", encoding="utf-8")
    print(f"Wrote {journal}", flush=True)


def _exit_code(stats: CycleStats) -> int:
    if stats.aborted:
        return 2
    if stats.stopped:
        return 130
    if stats.mid_fail or stats.bank_fail or stats.long_fail:
        return 2
    return 0


def run_longevity(args: argparse.Namespace) -> int:
    paths = _resolve_paths(args)
    ctx = LongevityContext(
        args=args,
        paths=paths,
        stats=CycleStats(),
        deadline=time.monotonic() + args.hours * 3600.0,
    )
    _log_start(ctx)
    _preflight_midisrv(ctx.stats)

    _hold_awake()
    cycle = 0
    try:
        _start_bridge(ctx)
        while not _should_stop(ctx):
            cycle += 1
            _run_one_cycle(ctx, cycle)
    except KeyboardInterrupt:
        ctx.stats.stopped = True
        ctx.stats.note("INTERRUPT harness")
    finally:
        if ctx.bridge is not None:
            # Leave Bridge up on FAIL for diagnosis when still alive.
            if ctx.stats.aborted and _bridge_alive(ctx.bridge):
                ctx.stats.note(
                    f"LEAVE Bridge up for diagnosis "
                    f"pid={ctx.stats.bridge_pid} log={paths.bridge_log}"
                )
            else:
                ctx.bridge.stop()
        _release_awake()

    _write_journal(ctx.stats, paths.journal, cycle)
    return _exit_code(ctx.stats)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "WMS single-session longevity: one Bridge WMS + mid->bank->long "
            "loop with abort-on-FAIL (Windows)"
        )
    )
    parser.add_argument(
        "--hours",
        type=float,
        default=1.0,
        help="Wall-clock duration in hours (default: 1)",
    )
    parser.add_argument(
        "--mid-count",
        type=int,
        default=10,
        help="Reps per mid scenario (default: 10; overnight-combined)",
    )
    parser.add_argument(
        "--bank-count",
        type=int,
        default=100,
        help="Patch dumps per bank lab (default: 100; overnight-combined)",
    )
    parser.add_argument(
        "--long-count",
        type=int,
        default=10,
        help="Reps per long payload size (default: 10; overnight-combined)",
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
    parser.add_argument(
        "--bridge-ready-timeout",
        type=float,
        default=45.0,
        help="Seconds to wait for Bridge MIDI ports after start (default: 45)",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.hours <= 0:
        raise SystemExit("--hours must be > 0")
    if args.mid_count < 1 or args.bank_count < 1 or args.long_count < 1:
        raise SystemExit("--mid-count, --bank-count, and --long-count must be >= 1")
    if args.long_interval < 0.05:
        raise SystemExit("--long-interval must be >= 0.05")
    if args.long_reply_timeout <= 0:
        raise SystemExit("--long-reply-timeout must be > 0")
    if args.cycle_gap < 0:
        raise SystemExit("--cycle-gap must be >= 0")
    if args.bridge_ready_timeout <= 0:
        raise SystemExit("--bridge-ready-timeout must be > 0")
    for label, value in (
        ("--matrix-out", args.matrix_out),
        ("--matrix-in", args.matrix_in),
        ("--long-out", args.long_out),
        ("--long-in", args.long_in),
    ):
        if not value.strip():
            raise SystemExit(f"{label} must be non-empty")

    signal.signal(signal.SIGINT, signal.default_int_handler)
    return run_longevity(args)


if __name__ == "__main__":
    sys.exit(main())
