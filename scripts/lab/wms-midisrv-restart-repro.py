#!/usr/bin/env python3
"""Journaled WMS post-stop MIDI enum repro + midisrv recovery helpers.

Path A: start one Bridge WMS session → stop → immediate fresh mido enum.
Path B: classify Bridge teardown vs midisrv/service from journal evidence
        (no Bridge code change unless Bridge-owned fault is proven).
Path C: fail-fast midisrv-suspect messaging; optional one-shot midisrv reset
        (explicit --apply-midisrv-reset); exactly one clean Bridge relaunch.

Topology (lab): Matrix In1/Out1; DIN Out2→In2.
Bridge: --start-session --dev-zadig --midi-backend=wms

Example:

  .venv-lab\\Scripts\\python.exe scripts/lab/wms-midisrv-restart-repro.py

Logs:
  tests/lab-logs/wms-midisrv-restart/repro-<UTC>.log
  tests/lab-logs/wms-midisrv-restart/bridge-<UTC>.log
  tests/lab-logs/wms-midisrv-restart/evidence-<UTC>.md
"""

from __future__ import annotations

import argparse
import signal
import sys
import time
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path

_LAB_DIR = Path(__file__).resolve().parent
if str(_LAB_DIR) not in sys.path:
    sys.path.insert(0, str(_LAB_DIR))

import lab_midi_common as lab_midi  # noqa: E402

BRIDGE_EXTRA_ARGS = list(lab_midi.WMS_BRIDGE_EXTRA_ARGS)


@dataclass
class Journal:
    lines: list[str] = field(default_factory=list)

    def note(self, line: str) -> None:
        stamp = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        entry = f"{stamp} {line}"
        self.lines.append(entry)
        print(line, flush=True)

    def write(self, path: Path) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("\n".join(self.lines) + "\n", encoding="utf-8")
        print(f"Wrote {path}", flush=True)


@dataclass
class ReproPaths:
    out_root: Path
    journal: Path
    bridge_log: Path
    evidence: Path
    bridge_exe: Path


@dataclass
class ReproOutcome:
    enum_timed_out: bool = False
    enum_ok: bool = False
    enum_elapsed_s: float = 0.0
    enum_error: str = ""
    bridge_stop_elapsed_s: float = 0.0
    bridge_exit_code: int | None = None
    midisrv_before: str = ""
    midisrv_after_stop: str = ""
    midisrv_after_enum: str = ""
    cause: str = "undetermined"
    path_c_ok: bool = False
    path_c_documented_only: bool = False
    path_c_attempted_recovery: bool = False


def _utc_stamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def _resolve_paths(args: argparse.Namespace) -> ReproPaths:
    root = _repo_root()
    out_root = root / "tests" / "lab-logs" / "wms-midisrv-restart"
    out_root.mkdir(parents=True, exist_ok=True)
    stamp = _utc_stamp()
    bridge = Path(args.bridge_exe) if args.bridge_exe else lab_midi.default_bridge_exe()
    if not bridge.is_file():
        raise SystemExit(f"Bridge.exe not found: {bridge}")
    return ReproPaths(
        out_root=out_root,
        journal=out_root / f"repro-{stamp}.log",
        bridge_log=out_root / f"bridge-{stamp}.log",
        evidence=out_root / f"evidence-{stamp}.md",
        bridge_exe=bridge,
    )


def _probe_midisrv(journal: Journal, label: str) -> str:
    ok, detail = lab_midi.midisrv_status()
    status = f"{'OK' if ok else 'BAD'} ({detail})"
    journal.note(f"MIDISRV {label}: {status}")
    return status


def _tail_bridge_log(bridge_log: Path, max_lines: int = 40) -> str:
    if not bridge_log.is_file():
        return "(no bridge log)"
    lines = bridge_log.read_text(encoding="utf-8", errors="replace").splitlines()
    return "\n".join(lines[-max_lines:])


def _bridge_teardown_looks_healthy(bridge_log: Path, exit_code: int | None) -> bool:
    """Heuristic: log has no pump/overflow fail needles.

    CTRL_BREAK often yields a non-zero exit; that alone is not a Bridge fault.
    Extreme codes (still running / never started) still fail the heuristic.
    """
    if exit_code is not None and exit_code < 0:
        return False
    hits = lab_midi.bridge_fail_lines(
        bridge_log.read_text(encoding="utf-8", errors="replace")
        if bridge_log.is_file()
        else ""
    )
    return len(hits) == 0


def _classify_cause(outcome: ReproOutcome, bridge_log: Path) -> str:
    if not outcome.enum_timed_out and outcome.enum_ok:
        return "no-fault-this-run (enum succeeded after stop)"
    teardown_ok = _bridge_teardown_looks_healthy(bridge_log, outcome.bridge_exit_code)
    if outcome.enum_timed_out and teardown_ok:
        return (
            "provisional cause: midisrv/service — Bridge stop log has no "
            "pump/overflow fail needles; post-stop mido enum timed out "
            "(TimeoutExpired). Heuristic only: ghost/incomplete Destroy would "
            "not match those needles. No Bridge teardown code change."
        )
    if outcome.enum_timed_out and not teardown_ok:
        return (
            "cause: Bridge/WMS teardown suspect — Bridge log has fail needles "
            "and post-stop enum timed out; investigate Destroy/Create before "
            "assuming midisrv-only."
        )
    return (
        f"cause: undetermined — enum_ok={outcome.enum_ok} "
        f"timed_out={outcome.enum_timed_out} error={outcome.enum_error!r}"
    )


def _run_path_a(
    args: argparse.Namespace,
    paths: ReproPaths,
    journal: Journal,
    outcome: ReproOutcome,
) -> lab_midi.BridgeSession | None:
    journal.note("=== PATH A: Bridge WMS clean session -> stop -> immediate enum ===")
    outcome.midisrv_before = _probe_midisrv(journal, "before-start")

    session = lab_midi.BridgeSession(
        paths.bridge_exe,
        paths.bridge_log,
        list(BRIDGE_EXTRA_ARGS),
    )
    session.start()
    try:
        assert session.proc is not None
        journal.note(
            f"BRIDGE started pid={session.proc.pid} "
            f"args={' '.join(BRIDGE_EXTRA_ARGS)} log={paths.bridge_log}"
        )

        out_name, in_name = session.wait_until_ready(
            args.matrix_out,
            args.matrix_in,
            args.bridge_ready_timeout,
        )
        journal.note(f"BRIDGE ready matrix OUT={out_name} IN={in_name}")

        # Confirm DIN pair while session is live (topology gate).
        outs, inns = lab_midi.fresh_midi_port_names()
        long_out = lab_midi.select_port(outs, args.long_out, "output")
        long_in = lab_midi.select_port(inns, args.long_in, "input")
        journal.note(f"TOPOLOGY din OUT={long_out} IN={long_in}")

        if args.hold_s > 0:
            journal.note(f"HOLD live session {args.hold_s:g}s")
            time.sleep(args.hold_s)

        stop_started = time.monotonic()
        session.stop(grace_s=args.stop_grace)
        outcome.bridge_stop_elapsed_s = time.monotonic() - stop_started
        outcome.bridge_exit_code = (
            session.proc.returncode if session.proc is not None else None
        )
        journal.note(
            f"BRIDGE stopped elapsed_s={outcome.bridge_stop_elapsed_s:.3f} "
            f"exit={outcome.bridge_exit_code}"
        )
    except BaseException:
        try:
            session.stop(grace_s=args.stop_grace)
        except Exception:
            pass
        raise

    outcome.midisrv_after_stop = _probe_midisrv(journal, "after-stop")

    journal.note(
        f"ENUM immediate enumerate_midi_ports timeout={args.enum_timeout:g}s"
    )
    enum_started = time.monotonic()
    result = lab_midi.enumerate_midi_ports(timeout_s=args.enum_timeout)
    outcome.enum_elapsed_s = time.monotonic() - enum_started
    outcome.enum_timed_out = result.timed_out
    outcome.enum_ok = result.ok
    outcome.enum_error = result.error
    outcome.midisrv_after_enum = _probe_midisrv(journal, "after-enum")

    if result.timed_out:
        journal.note(
            f"ENUM TimeoutExpired elapsed_s={outcome.enum_elapsed_s:.3f} "
            f"— {lab_midi.MIDISRV_SUSPECT_MESSAGE}"
        )
        journal.note("ENUM detail: " + result.error)
    elif result.ok:
        journal.note(
            f"ENUM OK elapsed_s={outcome.enum_elapsed_s:.3f} "
            f"outs={len(result.outputs)} ins={len(result.inputs)}"
        )
        for name in result.outputs:
            journal.note(f"  OUT {name}")
        for name in result.inputs:
            journal.note(f"  IN  {name}")
    else:
        journal.note(
            f"ENUM FAIL elapsed_s={outcome.enum_elapsed_s:.3f} "
            f"error={result.error!r}"
        )

    journal.note("BRIDGE log tail:")
    for line in _tail_bridge_log(paths.bridge_log).splitlines():
        journal.note("  | " + line)

    return session


def _run_path_c(
    args: argparse.Namespace,
    paths: ReproPaths,
    journal: Journal,
    outcome: ReproOutcome,
) -> None:
    journal.note("=== PATH C: detect + documented reset + one clean Bridge ===")
    journal.note(lab_midi.MIDISRV_SUSPECT_MESSAGE)
    journal.note("RESET PROCEDURE:\n" + lab_midi.MIDISRV_RESET_PROCEDURE.rstrip())

    reset_applied_ok = False
    if args.apply_midisrv_reset:
        outcome.path_c_attempted_recovery = True
        journal.note("APPLY midisrv reset once (explicit --apply-midisrv-reset)")
        ok, detail = lab_midi.apply_midisrv_reset_once()
        journal.note(f"RESET result ok={ok} detail={detail}")
        if not ok:
            journal.note(
                "RESET failed — run documented steps in an elevated shell, "
                "then re-run with --recover-only (no aggressive Bridge loops)."
            )
            outcome.path_c_ok = False
            return
        reset_applied_ok = True
    else:
        journal.note(
            "RESET skipped (no --apply-midisrv-reset). "
            "Documented procedure printed above for operator."
        )

    if not args.one_clean_bridge:
        journal.note(
            "ONE clean Bridge skipped (pass --one-clean-bridge after reset)."
        )
        # Documenting the procedure is not a successful recovery.
        outcome.path_c_documented_only = True
        outcome.path_c_ok = False
        return

    outcome.path_c_attempted_recovery = True
    if not reset_applied_ok:
        # Require a healthy enum (operator already reset) before one Bridge.
        probe = lab_midi.enumerate_midi_ports(timeout_s=args.enum_timeout)
        if probe.timed_out or not probe.ok:
            journal.note(
                "ONE clean Bridge blocked: MIDI enum still unhealthy. "
                "Apply elevated midisrv reset first "
                "(--apply-midisrv-reset), then --one-clean-bridge once."
            )
            outcome.path_c_ok = False
            return

    recover_log = paths.out_root / f"bridge-recover-{_utc_stamp()}.log"
    journal.note(
        f"ONE clean Bridge start log={recover_log} "
        "(exactly once — no relaunch loop)"
    )
    session: lab_midi.BridgeSession | None = None
    try:
        session = lab_midi.start_one_clean_wms_bridge(
            lab_midi.CleanWmsBridgeStart(
                bridge_exe=paths.bridge_exe,
                bridge_log=recover_log,
                out_needle=args.matrix_out,
                in_needle=args.matrix_in,
                ready_timeout_s=args.bridge_ready_timeout,
            )
        )
        outs, inns = lab_midi.fresh_midi_port_names(timeout_s=args.enum_timeout)
        matrix_out = lab_midi.select_port(outs, args.matrix_out, "output")
        matrix_in = lab_midi.select_port(inns, args.matrix_in, "input")
        long_out = lab_midi.select_port(outs, args.long_out, "output")
        long_in = lab_midi.select_port(inns, args.long_in, "input")
        journal.note(
            f"RECOVER ports OK matrix={matrix_out!r}/{matrix_in!r} "
            f"long={long_out!r}/{long_in!r}"
        )
        session.stop(grace_s=args.stop_grace)
        journal.note("ONE clean Bridge stopped after port confirm")
        outcome.path_c_ok = True
    except SystemExit as exc:
        journal.note(f"ONE clean Bridge FAIL: {exc}")
        outcome.path_c_ok = False
    finally:
        if session is not None and session.proc is not None and session.proc.poll() is None:
            try:
                session.stop(grace_s=args.stop_grace)
            except Exception:
                pass


def _repo_rel(path: Path) -> str:
    try:
        return path.resolve().relative_to(_repo_root()).as_posix()
    except ValueError:
        return path.as_posix()


def _write_evidence(
    paths: ReproPaths,
    journal: Journal,
    outcome: ReproOutcome,
) -> None:
    outcome.cause = _classify_cause(outcome, paths.bridge_log)
    journal.note(f"CLASSIFY {outcome.cause}")

    body = "\n".join(
        [
            "# WMS midisrv restart robustness — evidence",
            "",
            f"- journal: `{_repo_rel(paths.journal)}`",
            f"- bridge_log: `{_repo_rel(paths.bridge_log)}`",
            f"- bridge_stop_elapsed_s: {outcome.bridge_stop_elapsed_s:.3f}",
            f"- bridge_exit_code: {outcome.bridge_exit_code}",
            f"- enum_timed_out: {outcome.enum_timed_out}",
            f"- enum_ok: {outcome.enum_ok}",
            f"- enum_elapsed_s: {outcome.enum_elapsed_s:.3f}",
            f"- enum_error: {outcome.enum_error!r}",
            f"- midisrv_before: {outcome.midisrv_before}",
            f"- midisrv_after_stop: {outcome.midisrv_after_stop}",
            f"- midisrv_after_enum: {outcome.midisrv_after_enum}",
            f"- classification: {outcome.cause}",
            f"- path_c_ok: {outcome.path_c_ok}",
            f"- path_c_documented_only: {outcome.path_c_documented_only}",
            f"- path_c_attempted_recovery: {outcome.path_c_attempted_recovery}",
            "",
            "Follow-up (out of this Build exit): ≥1 h single-session longevity",
            "gate remains open — see spec-wms-session-longevity-stress.md.",
            "",
            "## Journal",
            "",
            "```",
            *journal.lines,
            "```",
            "",
        ]
    )
    paths.evidence.write_text(body, encoding="utf-8")
    print(f"Wrote {paths.evidence}", flush=True)


def run_repro(args: argparse.Namespace) -> int:
    paths = _resolve_paths(args)
    journal = Journal()
    outcome = ReproOutcome()
    journal.note(
        f"START wms-midisrv-restart-repro enum_timeout={args.enum_timeout:g}s "
        f"out_root={paths.out_root}"
    )

    # If midisrv already unhealthy before work: reset path then one clean Bridge.
    ok, detail = lab_midi.midisrv_status()
    if not ok:
        journal.note(f"PREWORK midisrv unhealthy ({detail}) — recovery first")
        if not args.apply_midisrv_reset:
            journal.note(lab_midi.MIDISRV_RESET_PROCEDURE.rstrip())
            raise SystemExit(
                "midisrv unhealthy before repro. Apply documented reset "
                "(elevated), or re-run with --apply-midisrv-reset "
                "--one-clean-bridge, then re-run path A."
            )
        _run_path_c(args, paths, journal, outcome)
        journal.write(paths.journal)
        _write_evidence(paths, journal, outcome)
        return 0 if outcome.path_c_ok else 2

    if args.recover_only:
        _run_path_c(args, paths, journal, outcome)
        journal.write(paths.journal)
        _write_evidence(paths, journal, outcome)
        return 0 if outcome.path_c_ok else 2

    try:
        _run_path_a(args, paths, journal, outcome)
    except SystemExit as exc:
        journal.note(f"PATH A interrupted: {exc}")
        journal.write(paths.journal)
        _write_evidence(paths, journal, outcome)
        raise

    # Path C always documents detect + reset; apply/relaunch only when flagged
    # or when enum timed out (document procedure; optional apply).
    if outcome.enum_timed_out or args.apply_midisrv_reset or args.one_clean_bridge:
        _run_path_c(args, paths, journal, outcome)
    else:
        journal.note(
            "PATH C deferred (enum did not time out). "
            "Messaging helpers are live in lab_midi_common; "
            "use --apply-midisrv-reset / --one-clean-bridge to exercise recovery."
        )
        outcome.path_c_documented_only = False
        outcome.path_c_ok = True

    if outcome.enum_timed_out:
        journal.note(
            "PATH A proof captured (TimeoutExpired). "
            "Cause classification written to evidence."
        )

    journal.write(paths.journal)
    _write_evidence(paths, journal, outcome)

    # A proof can be exit 0 even when recovery was only documented.
    # Explicit recovery attempts that fail must surface non-zero.
    if outcome.path_c_attempted_recovery and not outcome.path_c_ok:
        return 2
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "WMS post-stop MIDI enum repro + midisrv-suspect recovery helpers"
        )
    )
    parser.add_argument(
        "--enum-timeout",
        type=float,
        default=lab_midi.MIDI_ENUM_TIMEOUT_S,
        help=f"Fresh mido enum timeout seconds (default: {lab_midi.MIDI_ENUM_TIMEOUT_S:g})",
    )
    parser.add_argument(
        "--hold-s",
        type=float,
        default=2.0,
        help="Seconds to hold live Bridge before stop (default: 2)",
    )
    parser.add_argument(
        "--stop-grace",
        type=float,
        default=8.0,
        help="Bridge stop grace seconds (default: 8)",
    )
    parser.add_argument(
        "--bridge-ready-timeout",
        type=float,
        default=45.0,
        help="Seconds to wait for Bridge MIDI ports after start (default: 45)",
    )
    parser.add_argument(
        "--matrix-out",
        default="MT4 Out 1",
        help="Virtual OUT for Matrix (default: MT4 Out 1)",
    )
    parser.add_argument(
        "--matrix-in",
        default="MT4 In 1",
        help="Virtual IN for Matrix (default: MT4 In 1)",
    )
    parser.add_argument(
        "--long-out",
        default="MT4 Out 2",
        help="Virtual OUT for DIN loop (default: MT4 Out 2)",
    )
    parser.add_argument(
        "--long-in",
        default="MT4 In 2",
        help="Virtual IN for DIN loop (default: MT4 In 2)",
    )
    parser.add_argument(
        "--bridge-exe",
        default="",
        help="Path to Bridge.exe (default: builds/debug/Debug/Bridge.exe)",
    )
    parser.add_argument(
        "--apply-midisrv-reset",
        action="store_true",
        help=(
            "Explicit one-shot Restart-Service midisrv (admin). "
            "Never used automatically without this flag."
        ),
    )
    parser.add_argument(
        "--one-clean-bridge",
        action="store_true",
        help="After reset docs/apply, start exactly one clean Bridge WMS session",
    )
    parser.add_argument(
        "--recover-only",
        action="store_true",
        help="Skip path A; run path C recovery helpers only",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.enum_timeout <= 0:
        raise SystemExit("--enum-timeout must be > 0")
    if args.hold_s < 0:
        raise SystemExit("--hold-s must be >= 0")
    if args.bridge_ready_timeout <= 0:
        raise SystemExit("--bridge-ready-timeout must be > 0")
    signal.signal(signal.SIGINT, signal.default_int_handler)
    return run_repro(args)


if __name__ == "__main__":
    sys.exit(main())
