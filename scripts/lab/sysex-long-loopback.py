#!/usr/bin/env python3
"""Automate MT4 long SysEx DIN loopback lab (exact byte match, optional Bridge).

Palier 3 gate (option A): physical DIN Out1 -> In1 (no Matrix). Host sends a
large F0…F7 frame on the MT4 OUT port and expects the identical frame on IN.

macOS Apple-driver control (no Bridge):

  # Cable Out1 -> In1. Disconnect Matrix / other gear from those jacks.
  python3 scripts/lab/sysex-long-loopback.py --list-ports
  python3 scripts/lab/sysex-long-loopback.py \\
    --out-port \"MT4 Port 1\" --in-port \"MT4 Port 1\" \\
    --pass-percent 100 --fresh-sessions 2 \\
    --log-dir tests/lab-logs/sysex-long-loopback-macos

Windows Bridge (later):

  python scripts/lab/sysex-long-loopback.py --with-bridge --pass-percent 100
"""

from __future__ import annotations

import argparse
import subprocess
import sys
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path

_LAB_DIR = Path(__file__).resolve().parent
if str(_LAB_DIR) not in sys.path:
    sys.path.insert(0, str(_LAB_DIR))

import lab_midi_common as lab_midi  # noqa: E402
import sysex_long_loopback_lib as sx  # noqa: E402

EXIT_ABORTED = 3


def _default_log_dir() -> Path:
    return lab_midi.repo_root() / "tests" / "lab-logs" / "sysex-long-loopback"


def _resolve_log_dir(args: argparse.Namespace) -> Path:
    if args.log_dir:
        out_dir = Path(args.log_dir)
        if not out_dir.is_absolute():
            out_dir = lab_midi.repo_root() / out_dir
    else:
        out_dir = _default_log_dir()
    out_dir.mkdir(parents=True, exist_ok=True)
    return out_dir


def _resolve_log_path(args: argparse.Namespace, stamp: str, log_dir: Path) -> Path:
    if args.log:
        log_path = Path(args.log)
        if not log_path.is_absolute():
            log_path = log_dir / log_path
    else:
        log_path = log_dir / f"sysex-long-loopback-{stamp}.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    return log_path


@dataclass
class LoopbackSession:
    mido: object
    out_name: str
    in_name: str
    args: argparse.Namespace
    lines: list[str]
    start_index: int
    deadline_mono: float | None = None


@dataclass
class OpenPorts:
    inport: object
    outport: object


@dataclass
class PayloadWork:
    name: str
    payload: bytes
    stats: sx.ScenarioStats


@dataclass
class LabPaths:
    log_path: Path
    log_dir: Path
    stamp: str


def _append_session_header(
    session: LoopbackSession, plan: list[tuple[str, bytes]]
) -> None:
    args = session.args
    lines = session.lines
    lines.append(f"# start_index: {session.start_index}")
    lines.append(f"# out_port: {session.out_name}")
    lines.append(f"# in_port: {session.in_name}")
    lines.append(f"# count_per_payload: {args.count}")
    lines.append(f"# interval_s: {args.interval}")
    lines.append(f"# reply_timeout_s: {args.reply_timeout}")
    lines.append(f"# abort_after_timeouts: {args.abort_after_timeouts}")
    if args.max_wall_seconds > 0:
        lines.append(f"# max_wall_seconds: {args.max_wall_seconds}")
    lines.append(
        "# payloads: "
        + ", ".join(f"{name}={len(payload)}B" for name, payload in plan)
    )
    lines.append("---")


def _emit_line(session: LoopbackSession, line: str) -> None:
    session.lines.append(line)
    print(line)


def _run_one_trial(
    session: LoopbackSession,
    ports: OpenPorts,
    work: PayloadWork,
    index: int,
) -> bool:
    """Run one send/recv trial. Returns True on match, False on fail/timeout."""
    args = session.args
    sx.check_wall_deadline(
        session.deadline_mono,
        f"start={session.start_index} payload={work.name} before={index}",
    )
    cycle_started = time.monotonic()
    lab_midi.drain_input(ports.inport, settle_s=0.01)
    work.stats.sent += 1
    try:
        sx.send_sysex(ports.outport, session.mido, work.payload)
    except Exception as exc:  # noqa: BLE001 — lab must record send fails
        fail = (
            f"{index:04d} FAIL {work.name} send_error={exc!r} "
            f"{sx.frame_head_tail(work.payload)}"
        )
        work.stats.fail_lines.append(fail)
        _emit_line(session, fail)
        return False

    _emit_line(
        session,
        f"{index:04d} SEND {work.name} {sx.frame_head_tail(work.payload)} "
        f"t_mono={cycle_started:.3f}",
    )
    reply, dt_ms, note = sx.wait_exact_sysex(
        ports.inport, work.payload, args.reply_timeout
    )
    if reply is not None:
        work.stats.ok += 1
        _emit_line(
            session,
            f"{index:04d} RECV {work.name} match {sx.frame_head_tail(reply)} "
            f"dt_ms={dt_ms:.1f}",
        )
        return True

    result = (
        f"{index:04d} TIMEOUT {work.name} expected_len={len(work.payload)} "
        f"waited_ms={dt_ms:.1f} last={note}"
    )
    work.stats.fail_lines.append(result)
    _emit_line(session, result)
    return False


def _run_payload_trials(
    session: LoopbackSession,
    ports: OpenPorts,
    work: PayloadWork,
) -> sx.ScenarioStats:
    args = session.args
    started = time.monotonic()
    consecutive_timeouts = 0
    try:
        for index in range(1, args.count + 1):
            cycle_started = time.monotonic()
            matched = _run_one_trial(session, ports, work, index)
            consecutive_timeouts = 0 if matched else consecutive_timeouts + 1
            if (
                args.abort_after_timeouts > 0
                and consecutive_timeouts >= args.abort_after_timeouts
            ):
                raise sx.LabAbort(
                    f"{consecutive_timeouts} consecutive timeouts/fails "
                    f"on {work.name} (abort-after-timeouts="
                    f"{args.abort_after_timeouts})"
                )
            if index < args.count:
                remaining = args.interval - (time.monotonic() - cycle_started)
                if remaining > 0:
                    time.sleep(remaining)
    except sx.LabAbort as abort:
        work.stats.aborted = True
        work.stats.abort_reason = str(abort)
        _emit_line(session, f"ABORT {work.name}: {abort}")
        work.stats.elapsed_s = time.monotonic() - started
        _emit_line(session, work.stats.summary(args.pass_percent))
        raise

    work.stats.elapsed_s = time.monotonic() - started
    _emit_line(session, work.stats.summary(args.pass_percent))
    return work.stats


def _run_loopback_session(session: LoopbackSession) -> bool:
    plan = sx.payload_plan(session.args)
    _append_session_header(session, plan)
    all_ok = True
    with session.mido.open_input(session.in_name) as inport, session.mido.open_output(
        session.out_name
    ) as outport:
        sx.prepare_mido_input(inport)
        lab_midi.drain_input(inport, settle_s=0.2)
        time.sleep(0.5)
        lab_midi.drain_input(inport, settle_s=0.1)
        ports = OpenPorts(inport=inport, outport=outport)
        for name, payload in plan:
            work = PayloadWork(
                name=name, payload=payload, stats=sx.ScenarioStats(name=name)
            )
            stats = _run_payload_trials(session, ports, work)
            if stats.rate < session.args.pass_percent or stats.sent == 0:
                all_ok = False
    return all_ok


def _run_midi_lab_in_fresh_process(
    args: argparse.Namespace,
    log_path: Path,
    start_index: int,
    deadline_mono: float | None = None,
) -> int:
    cmd = [
        sys.executable,
        str(Path(__file__).resolve()),
        "--count",
        str(args.count),
        "--interval",
        str(args.interval),
        "--reply-timeout",
        str(args.reply_timeout),
        "--pass-percent",
        str(args.pass_percent),
        "--sizes",
        ",".join(str(size) for size in args.sizes),
        "--fixture",
        args.fixture,
        "--out-port",
        args.out_port,
        "--in-port",
        args.in_port,
        "--log",
        str(log_path),
        "--start-index",
        str(start_index),
        "--append-log",
        "--abort-after-timeouts",
        str(args.abort_after_timeouts),
    ]
    if args.max_wall_seconds > 0:
        # Child inherits remaining wall budget so nested sessions stop together.
        remaining = (
            max(1.0, deadline_mono - time.monotonic())
            if deadline_mono is not None
            else args.max_wall_seconds
        )
        cmd.extend(["--max-wall-seconds", f"{remaining:.1f}"])
    if args.skip_fixture:
        cmd.append("--skip-fixture")
    print("Launching fresh MIDI lab process (port refresh) ...")
    print(" ".join(cmd))
    popen_timeout = None
    if deadline_mono is not None:
        popen_timeout = max(1.0, deadline_mono - time.monotonic())
    try:
        completed = subprocess.run(
            cmd, cwd=str(lab_midi.repo_root()), timeout=popen_timeout
        )
    except subprocess.TimeoutExpired:
        print(
            f"ABORT: child MIDI lab exceeded wall budget "
            f"(start={start_index}); killed"
        )
        return EXIT_ABORTED
    return int(completed.returncode)


def _validate_lab_mode(args: argparse.Namespace) -> None:
    if args.with_bridge and args.fresh_sessions > 1:
        raise SystemExit(
            "Use --fresh-starts with --with-bridge; "
            "--fresh-sessions is for MIDI-only (no Bridge) runs."
        )
    if args.fresh_sessions > 1 and args.append_log:
        raise SystemExit(
            "--append-log cannot be combined with --fresh-sessions > 1 "
            "(append-log is reserved for child session processes)."
        )
    if args.max_wall_seconds < 0:
        raise SystemExit("--max-wall-seconds must be >= 0 (0 = auto)")


def _apply_auto_wall(args: argparse.Namespace) -> float | None:
    if args.max_wall_seconds == 0 and not args.list_ports:
        args.max_wall_seconds = sx.auto_max_wall_seconds(args)
        print(f"auto max-wall-seconds={args.max_wall_seconds:.1f}")
    if args.max_wall_seconds > 0:
        return time.monotonic() + args.max_wall_seconds
    return None


def _common_header_fields(args: argparse.Namespace) -> list[str]:
    return [
        f"# count_per_payload: {args.count}",
        f"# sizes: {','.join(str(s) for s in args.sizes)}",
        f"# fixture: {args.fixture}",
        f"# skip_fixture: {str(args.skip_fixture).lower()}",
        f"# interval_s: {args.interval}",
        f"# reply_timeout_s: {args.reply_timeout}",
        f"# pass_percent: {args.pass_percent}",
        f"# abort_after_timeouts: {args.abort_after_timeouts}",
        f"# max_wall_seconds: {args.max_wall_seconds}",
    ]


def _write_finish_footer(log_path: Path, overall_ok: bool, aborted: bool) -> int:
    with log_path.open("a", encoding="utf-8") as handle:
        handle.write(f"# finished_utc: {datetime.now(timezone.utc).isoformat()}\n")
        handle.write(f"# overall_pass: {str(overall_ok).lower()}\n")
        if aborted:
            handle.write("# aborted: true\n")
    print(f"Wrote {log_path}")
    print(f"overall_pass={str(overall_ok).lower()}")
    if aborted:
        print("aborted=true")
        return EXIT_ABORTED
    return 0 if overall_ok else 2


def _record_bridge_fail_hits(
    paths: LabPaths,
    start_index: int,
    bridge_log: Path,
    fail_hits: list[str],
) -> bool:
    """Append bridge fail notes. Returns True if any fail hits."""
    with paths.log_path.open("a", encoding="utf-8") as handle:
        handle.write(f"# bridge_log_start{start_index}: {bridge_log}\n")
        if not fail_hits:
            handle.write(f"# bridge_fail_start{start_index}: count=0\n")
            return False
        handle.write(f"# bridge_fail_start{start_index}: count={len(fail_hits)}\n")
        for hit in fail_hits:
            handle.write(f"# bridge_fail: {hit}\n")
            print(
                "BRIDGE_FAIL: "
                + hit.replace("\u2192", "->").replace("\u2190", "<-")
            )
    return True


def _run_one_bridge_start(
    args: argparse.Namespace,
    paths: LabPaths,
    start_index: int,
    deadline_mono: float | None,
) -> tuple[int, bool]:
    """Returns (child_rc_or_zero, saw_bridge_fail)."""
    bridge_log = (
        Path(args.bridge_log)
        if args.bridge_log and args.fresh_starts == 1
        else paths.log_dir / f"bridge-{paths.stamp}-start{start_index}.log"
    )
    bridge = lab_midi.BridgeSession(
        Path(args.bridge_exe),
        bridge_log,
        ["--dev-zadig"] if args.dev_zadig else [],
    )
    try:
        bridge.start()
        out_name, in_name = bridge.wait_until_ready(
            args.out_port,
            args.in_port,
            args.bridge_ready_timeout,
        )
        print(
            f"Bridge start {start_index}/{args.fresh_starts} ports ready: "
            f"OUT={out_name} IN={in_name}"
        )
        rc = _run_midi_lab_in_fresh_process(
            args, paths.log_path, start_index, deadline_mono
        )
        fail_hits = lab_midi.bridge_fail_lines(bridge.captured_text())
        saw_fail = _record_bridge_fail_hits(paths, start_index, bridge_log, fail_hits)
        return rc, saw_fail
    finally:
        bridge.stop()
        print(f"Wrote {bridge_log}")


def _run_with_bridge(
    args: argparse.Namespace,
    paths: LabPaths,
    deadline_mono: float | None,
) -> int:
    overall_ok = True
    aborted = False
    header = [
        "# SysEx long DIN loopback lab log",
        f"# started_utc: {datetime.now(timezone.utc).isoformat()}",
        "# with_bridge: true",
        f"# fresh_starts: {args.fresh_starts}",
        *_common_header_fields(args),
        f"# stamp: {paths.stamp}",
        "---",
    ]
    paths.log_path.write_text("\n".join(header) + "\n", encoding="utf-8")

    for start_index in range(1, args.fresh_starts + 1):
        try:
            sx.check_wall_deadline(
                deadline_mono, f"before Bridge start {start_index}"
            )
        except sx.LabAbort as abort:
            aborted = True
            overall_ok = False
            with paths.log_path.open("a", encoding="utf-8") as handle:
                handle.write(f"# ABORT: {abort}\n")
            print(f"ABORT: {abort}")
            break

        rc, saw_fail = _run_one_bridge_start(
            args, paths, start_index, deadline_mono
        )
        if rc == EXIT_ABORTED:
            aborted = True
            overall_ok = False
        elif rc != 0:
            overall_ok = False
        if saw_fail:
            overall_ok = False
        if aborted:
            break

    return _write_finish_footer(paths.log_path, overall_ok, aborted)


def _run_fresh_midi_sessions(
    args: argparse.Namespace,
    paths: LabPaths,
    deadline_mono: float | None,
) -> int:
    overall_ok = True
    aborted = False
    header = [
        "# SysEx long DIN loopback lab log",
        f"# started_utc: {datetime.now(timezone.utc).isoformat()}",
        "# with_bridge: false",
        f"# fresh_sessions: {args.fresh_sessions}",
        f"# session_gap_s: {args.session_gap}",
        *_common_header_fields(args),
        f"# out_port_needle: {args.out_port}",
        f"# in_port_needle: {args.in_port}",
        f"# stamp: {paths.stamp}",
        "---",
    ]
    paths.log_path.write_text("\n".join(header) + "\n", encoding="utf-8")

    for start_index in range(1, args.fresh_sessions + 1):
        try:
            sx.check_wall_deadline(
                deadline_mono, f"before MIDI session {start_index}"
            )
        except sx.LabAbort as abort:
            aborted = True
            overall_ok = False
            with paths.log_path.open("a", encoding="utf-8") as handle:
                handle.write(f"# ABORT: {abort}\n")
            print(f"ABORT: {abort}")
            break
        if start_index > 1 and args.session_gap > 0:
            print(
                f"Fresh session gap {args.session_gap:.1f}s "
                f"before session {start_index}/{args.fresh_sessions} ..."
            )
            time.sleep(args.session_gap)
        print(
            f"MIDI-only fresh session {start_index}/{args.fresh_sessions} "
            f"(new process; ports reopen)"
        )
        rc = _run_midi_lab_in_fresh_process(
            args, paths.log_path, start_index, deadline_mono
        )
        if rc == EXIT_ABORTED:
            aborted = True
            overall_ok = False
            break
        if rc != 0:
            overall_ok = False

    return _write_finish_footer(paths.log_path, overall_ok, aborted)


def _run_single_midi_session(
    args: argparse.Namespace,
    mido,
    paths: LabPaths,
    deadline_mono: float | None,
) -> int:
    lines: list[str] = []
    if not (args.append_log and paths.log_path.is_file()):
        lines.extend(
            [
                "# SysEx long DIN loopback lab log",
                f"# started_utc: {datetime.now(timezone.utc).isoformat()}",
                "# with_bridge: false",
                f"# fresh_sessions: {args.fresh_sessions}",
                f"# count_per_payload: {args.count}",
                f"# sizes: {','.join(str(s) for s in args.sizes)}",
                f"# fixture: {args.fixture}",
                f"# skip_fixture: {str(args.skip_fixture).lower()}",
                f"# interval_s: {args.interval}",
                f"# reply_timeout_s: {args.reply_timeout}",
                f"# pass_percent: {args.pass_percent}",
                f"# abort_after_timeouts: {args.abort_after_timeouts}",
                f"# max_wall_seconds: {args.max_wall_seconds}",
            ]
        )

    out_name = lab_midi.select_port(
        list(mido.get_output_names()), args.out_port, "output"
    )
    in_name = lab_midi.select_port(
        list(mido.get_input_names()), args.in_port, "input"
    )
    print(f"OUT={out_name}")
    print(f"IN={in_name}")
    print(f"log={paths.log_path}")
    lines.append(f"# session_index: {args.start_index}")

    aborted = False
    try:
        all_ok = _run_loopback_session(
            LoopbackSession(
                mido=mido,
                out_name=out_name,
                in_name=in_name,
                args=args,
                lines=lines,
                start_index=args.start_index,
                deadline_mono=deadline_mono,
            )
        )
    except sx.LabAbort as abort:
        aborted = True
        all_ok = False
        lines.append(f"# ABORT: {abort}")
        print(f"ABORT: {abort}")

    lines.append(f"# finished_utc: {datetime.now(timezone.utc).isoformat()}")
    lines.append(f"# start_pass: {str(all_ok).lower()}")
    if aborted:
        lines.append("# aborted: true")
    if not args.append_log:
        lines.append(f"# overall_pass: {str(all_ok).lower()}")

    mode = "a" if args.append_log else "w"
    with paths.log_path.open(mode, encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")
    print(f"Wrote {paths.log_path}")
    if not args.append_log:
        print(f"overall_pass={str(all_ok).lower()}")
    if aborted:
        print("aborted=true")
        return EXIT_ABORTED
    return 0 if all_ok else 2


def run_lab(args: argparse.Namespace) -> int:
    mido = sx.require_mido()
    if args.list_ports:
        return lab_midi.list_ports(mido)

    _validate_lab_mode(args)
    deadline_mono = _apply_auto_wall(args)
    stamp = lab_midi.lab_stamp()
    log_dir = _resolve_log_dir(args)
    paths = LabPaths(
        log_path=_resolve_log_path(args, stamp, log_dir),
        log_dir=log_dir,
        stamp=stamp,
    )

    if args.with_bridge:
        return _run_with_bridge(args, paths, deadline_mono)
    if args.fresh_sessions > 1 and not args.append_log:
        return _run_fresh_midi_sessions(args, paths, deadline_mono)
    return _run_single_midi_session(args, mido, paths, deadline_mono)


def build_parser() -> argparse.ArgumentParser:
    return sx.build_parser()


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    args.sizes = sx.parse_sizes(args.sizes)
    sx.validate_cli_args(args)
    return run_lab(args)


if __name__ == "__main__":
    sys.exit(main())
