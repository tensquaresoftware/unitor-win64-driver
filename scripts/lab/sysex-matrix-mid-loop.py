#!/usr/bin/env python3
"""Automate MT4 mid-size Matrix SysEx lab (push + dump, optional Bridge start/stop).

Preferred one-shot on Windows (starts Bridge, runs scenarios, optional fresh
Starts, stops):

  python -m pip install -r scripts/lab/requirements-device-inquiry.txt
  python scripts/lab/sysex-matrix-mid-loop.py --with-bridge --pass-percent 100

macOS Apple-driver control (no Bridge) — list ports, then pass real names:

  python3 scripts/lab/sysex-matrix-mid-loop.py --list-ports
  python3 scripts/lab/sysex-matrix-mid-loop.py \\
    --out-port \"<Apple Out1>\" --in-port \"<Apple In1>\" \\
    --pass-percent 100 --fresh-sessions 2 \\
    --log-dir tests/lab-logs/sysex-matrix-mid-macos

Close MIDI-OX / Matrix-Control / DAWs on MT4 ports first. Matrix-1000 must be
powered and DIN-cabled (Out1 <-> In1).

Logs (default under tests/lab-logs/sysex-matrix-mid/):
  sysex-matrix-mid-<UTC>.log  — SEND / RECV / FAIL / TIMEOUT + per-scenario summary
  bridge-<UTC>-start<N>.log   — Bridge stdout/stderr (when --with-bridge)
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
import sysex_matrix_mid_loop_lib as sx  # noqa: E402


@dataclass
class LabPaths:
    log_path: Path
    log_dir: Path
    stamp: str


def _default_log_dir() -> Path:
    return lab_midi.repo_root() / "tests" / "lab-logs" / "sysex-matrix-mid"


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
            # Prefer landing under --log-dir when both are given.
            log_path = log_dir / log_path
    else:
        log_path = log_dir / f"sysex-matrix-mid-{stamp}.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    return log_path


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
    if args.pushes_only and args.fresh_sessions > 1:
        raise SystemExit(
            "--pushes-only cannot be combined with --fresh-sessions > 1 "
            "(later sessions are dumps-only by design)."
        )


def _common_header_fields(args: argparse.Namespace) -> list[str]:
    return [
        f"# count_per_scenario: {args.count}",
        f"# interval_s: {args.interval}",
        f"# reply_timeout_s: {args.reply_timeout}",
        f"# pass_percent: {args.pass_percent}",
    ]


def _write_finish_footer(log_path: Path, overall_ok: bool) -> int:
    with log_path.open("a", encoding="utf-8") as handle:
        handle.write(f"# finished_utc: {datetime.now(timezone.utc).isoformat()}\n")
        handle.write(f"# overall_pass: {str(overall_ok).lower()}\n")
    print(f"Wrote {log_path}")
    print(f"overall_pass={str(overall_ok).lower()}")
    return 0 if overall_ok else 2


def _run_midi_lab_in_fresh_process(
    args: argparse.Namespace,
    log_path: Path,
    start_index: int,
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
        "--out-port",
        args.out_port,
        "--in-port",
        args.in_port,
        "--patch-fixture",
        args.patch_fixture,
        "--master-fixture",
        args.master_fixture,
        "--log",
        str(log_path),
        "--start-index",
        str(start_index),
        "--append-log",
    ]
    if not args.include_push:
        cmd.append("--dumps-only")
    if not args.include_dump:
        cmd.append("--pushes-only")
    print("Launching fresh MIDI lab process (port refresh) ...")
    print(" ".join(cmd))
    completed = subprocess.run(cmd, cwd=str(lab_midi.repo_root()))
    return int(completed.returncode)


def _record_bridge_fail_hits(
    log_path: Path,
    start_index: int,
    bridge_log: Path,
    fail_hits: list[str],
) -> bool:
    """Append bridge fail notes. Returns True if any fail hits."""
    with log_path.open("a", encoding="utf-8") as handle:
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
) -> tuple[int, bool]:
    """Returns (child_rc, saw_bridge_fail)."""
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
        # First Start: pushes + dumps. Later Starts: dumps only
        # (fresh-Start gate targets the dump pair).
        args.include_push = start_index == 1 and not args.dumps_only
        args.include_dump = not args.pushes_only
        rc = _run_midi_lab_in_fresh_process(args, paths.log_path, start_index)
        # Scan before Stop so teardown noise is not counted as a lab Fail.
        fail_hits = sx.bridge_fail_lines(bridge.captured_text())
        saw_fail = _record_bridge_fail_hits(
            paths.log_path, start_index, bridge_log, fail_hits
        )
        return rc, saw_fail
    finally:
        bridge.stop()
        print(f"Wrote {bridge_log}")


def _run_with_bridge(args: argparse.Namespace, paths: LabPaths) -> int:
    overall_ok = True
    header = [
        "# SysEx Matrix mid-size lab log (automated; no MIDI-OX Thru)",
        f"# started_utc: {datetime.now(timezone.utc).isoformat()}",
        "# with_bridge: true",
        f"# fresh_starts: {args.fresh_starts}",
        *_common_header_fields(args),
        f"# stamp: {paths.stamp}",
        "---",
    ]
    paths.log_path.write_text("\n".join(header) + "\n", encoding="utf-8")

    for start_index in range(1, args.fresh_starts + 1):
        rc, saw_fail = _run_one_bridge_start(args, paths, start_index)
        if rc != 0:
            overall_ok = False
        if saw_fail:
            overall_ok = False

    return _write_finish_footer(paths.log_path, overall_ok)


def _run_fresh_midi_sessions(args: argparse.Namespace, paths: LabPaths) -> int:
    overall_ok = True
    header = [
        "# SysEx Matrix mid-size lab log (automated; no MIDI-OX Thru)",
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
        # First session: pushes + dumps. Later sessions: dumps only
        # (fresh-session gate targets the dump pair / first dump).
        args.include_push = start_index == 1 and not args.dumps_only
        args.include_dump = not args.pushes_only
        rc = _run_midi_lab_in_fresh_process(args, paths.log_path, start_index)
        if rc != 0:
            overall_ok = False

    return _write_finish_footer(paths.log_path, overall_ok)


def _run_single_midi_session(
    args: argparse.Namespace,
    mido,
    paths: LabPaths,
) -> int:
    lines: list[str] = []
    if not (args.append_log and paths.log_path.is_file()):
        lines.extend(
            [
                "# SysEx Matrix mid-size lab log (automated; no MIDI-OX Thru)",
                f"# started_utc: {datetime.now(timezone.utc).isoformat()}",
                "# with_bridge: false",
                f"# fresh_sessions: {args.fresh_sessions}",
                *_common_header_fields(args),
            ]
        )

    args.include_push = not args.dumps_only
    args.include_dump = not args.pushes_only

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

    _, all_ok = sx.run_all_scenarios(
        sx.AllScenariosOpts(
            mido=mido,
            out_name=out_name,
            in_name=in_name,
            args=args,
            lines=lines,
            start_index=args.start_index,
        )
    )
    lines.append(f"# finished_utc: {datetime.now(timezone.utc).isoformat()}")
    lines.append(f"# start_pass: {str(all_ok).lower()}")
    if not args.append_log:
        lines.append(f"# overall_pass: {str(all_ok).lower()}")

    mode = "a" if args.append_log else "w"
    with paths.log_path.open(mode, encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")
    print(f"Wrote {paths.log_path}")
    if not args.append_log:
        print(f"overall_pass={str(all_ok).lower()}")
    return 0 if all_ok else 2


def run_lab(args: argparse.Namespace) -> int:
    mido = sx.require_mido()
    if args.list_ports:
        return lab_midi.list_ports(mido)

    _validate_lab_mode(args)
    stamp = lab_midi.lab_stamp()
    log_dir = _resolve_log_dir(args)
    paths = LabPaths(
        log_path=_resolve_log_path(args, stamp, log_dir),
        log_dir=log_dir,
        stamp=stamp,
    )

    if args.with_bridge:
        return _run_with_bridge(args, paths)
    if args.fresh_sessions > 1 and not args.append_log:
        return _run_fresh_midi_sessions(args, paths)
    return _run_single_midi_session(args, mido, paths)


def build_parser() -> argparse.ArgumentParser:
    return sx.build_parser()


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    sx.validate_cli_args(args)
    return run_lab(args)


if __name__ == "__main__":
    sys.exit(main())
