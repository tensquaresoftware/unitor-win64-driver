#!/usr/bin/env python3
"""Automate MT4 Matrix bank-burst SysEx lab (100× patch dump, optional Bridge).

macOS Apple-driver control (no Bridge):

  python3 scripts/lab/sysex-matrix-bank-loop.py --list-ports
  python3 scripts/lab/sysex-matrix-bank-loop.py \\
    --out-port \"MT4 Port 1\" --in-port \"MT4 Port 1\" \\
    --count 100 --interval 0.01 --pass-percent 100 \\
    --fresh-sessions 2 --log-dir tests/lab-logs/sysex-matrix-bank-macos

Windows Bridge (later):

  python scripts/lab/sysex-matrix-bank-loop.py --with-bridge --pass-percent 100

Matrix powered + DIN Out1 <-> In1. Close DAWs / Matrix-Control on MT4 ports.
"""

from __future__ import annotations

import argparse
import signal
import subprocess
import sys
import threading
import time
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path

PATCH_PREFIX = bytes([0xF0, 0x10, 0x06, 0x01])
PATCH_SIZE = 275
MIN_INTERVAL_S = 0.01

READY_MARKERS = (
    "Device Inquiry lab:",
    "MIDI I/O running",
    "DeviceSession started for MT4",
)

BRIDGE_FAIL_NEEDLES = (
    "MIDI I/O pump failed",
    "WriteBulk failed",
    "WriteBulk skipped",
    "Host→device WriteBulk",
    "Host->device WriteBulk",
)


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def _default_bridge_exe() -> Path:
    return _repo_root() / "builds" / "debug" / "Debug" / "Bridge.exe"


def _lab_stamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def _default_log_dir() -> Path:
    return _repo_root() / "tests" / "lab-logs" / "sysex-matrix-bank"


def _resolve_log_dir(args: argparse.Namespace) -> Path:
    if args.log_dir:
        out_dir = Path(args.log_dir)
        if not out_dir.is_absolute():
            out_dir = _repo_root() / out_dir
    else:
        out_dir = _default_log_dir()
    out_dir.mkdir(parents=True, exist_ok=True)
    return out_dir


def _hex_bytes(data: bytes, limit: int | None = None) -> str:
    if limit is not None and len(data) > limit:
        head = " ".join(f"{byte:02X}" for byte in data[:limit])
        return f"{head} … ({len(data)} B)"
    return " ".join(f"{byte:02X}" for byte in data)


def _require_mido():
    try:
        import mido  # noqa: F401
    except ImportError as exc:
        raise SystemExit(
            "Missing dependency: mido / python-rtmidi.\n"
            "Install with:\n"
            "  python -m pip install -r scripts/lab/requirements-device-inquiry.txt\n"
        ) from exc
    import mido

    return mido


def _fresh_midi_port_names() -> tuple[list[str], list[str]]:
    code = (
        "import mido\n"
        "print('OUT')\n"
        "print('\\n'.join(mido.get_output_names()))\n"
        "print('IN')\n"
        "print('\\n'.join(mido.get_input_names()))\n"
    )
    completed = subprocess.run(
        [sys.executable, "-c", code],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        timeout=20,
        check=False,
    )
    if completed.returncode != 0:
        raise SystemExit(
            "Failed to enumerate MIDI ports in a fresh process:\n"
            + (completed.stderr or completed.stdout or "(no output)")
        )
    lines = completed.stdout.splitlines()
    try:
        out_idx = lines.index("OUT")
        in_idx = lines.index("IN")
    except ValueError as exc:
        raise SystemExit(
            "Unexpected MIDI port enumeration output:\n" + completed.stdout
        ) from exc
    outputs = [line for line in lines[out_idx + 1 : in_idx] if line.strip()]
    inputs = [line for line in lines[in_idx + 1 :] if line.strip()]
    return outputs, inputs


def _normalize_port_label(name: str) -> str:
    parts = name.rsplit(" ", 1)
    if len(parts) == 2 and parts[1].isdigit():
        return parts[0]
    return name


def _port_match_rank(name: str, needle: str) -> int | None:
    if name == needle:
        return 0
    if _normalize_port_label(name) == needle:
        return 1
    if not name.lower().startswith(needle.lower()):
        return None
    rest = name[len(needle) :]
    if rest == "":
        return 0
    if rest[0] == " " and rest[1:].replace(" ", "").isdigit():
        return 2
    return None


def _select_port(names: list[str], needle: str, kind: str) -> str:
    ranked: list[tuple[int, str]] = []
    for name in names:
        rank = _port_match_rank(name, needle)
        if rank is not None:
            ranked.append((rank, name))
    if not ranked:
        raise SystemExit(
            f"No MIDI {kind} port matching {needle!r}. Available:\n"
            + "\n".join(f"  - {name}" for name in names)
        )
    ranked.sort(key=lambda item: (item[0], item[1]))
    best_rank = ranked[0][0]
    best = [name for rank, name in ranked if rank == best_rank]
    if len(best) > 1:
        raise SystemExit(
            f"Ambiguous MIDI {kind} port {needle!r}. Matches:\n"
            + "\n".join(f"  - {name}" for name in best)
        )
    return best[0]


def _find_port(names: list[str], needle: str) -> str | None:
    try:
        return _select_port(names, needle, "port")
    except SystemExit:
        return None


def _list_ports(mido) -> int:
    print("MIDI outputs:")
    for name in mido.get_output_names():
        print(f"  - {name}")
    print("MIDI inputs:")
    for name in mido.get_input_names():
        print(f"  - {name}")
    return 0


def _drain_input(inport, settle_s: float = 0.05) -> None:
    deadline = time.monotonic() + settle_s
    while time.monotonic() < deadline:
        for _ in inport.iter_pending():
            pass
        time.sleep(0.005)


class SysexAssembler:
    """Reassemble SysEx until F7. Never treat an open buffer as Pass."""

    def __init__(self) -> None:
        self._buf = bytearray()

    def push_mido_sysex_data(self, data: bytes | list[int]) -> list[bytes]:
        if not self._buf:
            self._buf.append(0xF0)
        self._buf.extend(bytes(data))
        self._buf.append(0xF7)
        frame = bytes(self._buf)
        self._buf.clear()
        return [frame]


def _iter_completed_sysex(inport, assembler: SysexAssembler) -> list[bytes]:
    frames: list[bytes] = []
    for message in inport.iter_pending():
        if message.type != "sysex":
            continue
        frames.extend(assembler.push_mido_sysex_data(message.data))
    return frames


def _frame_head_tail(data: bytes) -> str:
    if len(data) <= 8:
        return _hex_bytes(data)
    return (
        f"head={_hex_bytes(data[:4])} tail={_hex_bytes(data[-4:])} "
        f"len={len(data)}"
    )


def _is_patch_dump(data: bytes) -> bool:
    return (
        len(data) == PATCH_SIZE
        and data.startswith(PATCH_PREFIX)
        and data[-1] == 0xF7
    )


def _wait_matched_sysex(
    inport,
    timeout_s: float,
    predicate,
) -> tuple[bytes | None, float, str]:
    assembler = SysexAssembler()
    started = time.monotonic()
    deadline = started + timeout_s
    last_note = "none"
    while time.monotonic() < deadline:
        for frame in _iter_completed_sysex(inport, assembler):
            if predicate(frame):
                return frame, (time.monotonic() - started) * 1000.0, "match"
            last_note = f"discard {_frame_head_tail(frame)}"
        time.sleep(0.005)
    return None, (time.monotonic() - started) * 1000.0, last_note


def _bridge_fail_lines(text: str) -> list[str]:
    hits: list[str] = []
    for line in text.splitlines():
        for needle in BRIDGE_FAIL_NEEDLES:
            if needle in line:
                hits.append(line.rstrip())
                break
    return hits


def _patch_dump_request(slot: int) -> bytes:
    if not 0 <= slot <= 0x7F:
        raise ValueError(f"slot out of MIDI data range: {slot}")
    return bytes([0xF0, 0x10, 0x06, 0x04, 0x01, slot & 0x7F, 0xF7])


@dataclass
class ScenarioStats:
    name: str
    sent: int = 0
    ok: int = 0
    fail_lines: list[str] = field(default_factory=list)
    elapsed_s: float = 0.0

    @property
    def rate(self) -> float:
        return (100.0 * self.ok / self.sent) if self.sent else 0.0

    def summary(self, pass_percent: float) -> str:
        passed = self.rate >= pass_percent and self.sent > 0
        return (
            f"summary[{self.name}]: sent={self.sent} ok={self.ok} "
            f"rate={self.rate:.1f}% elapsed_s={self.elapsed_s:.2f} "
            f"pass={str(passed).lower()} (need>={pass_percent:.0f}%)"
        )


class BridgeSession:
    """Owns a Bridge.exe --start-session child and its console log."""

    def __init__(self, exe: Path, bridge_log: Path, extra_args: list[str]):
        self.exe = exe
        self.bridge_log = bridge_log
        self.extra_args = extra_args
        self.proc: subprocess.Popen[str] | None = None
        self._reader: threading.Thread | None = None
        self._lock = threading.Lock()
        self._lines: list[str] = []

    def start(self) -> None:
        if not self.exe.is_file():
            raise SystemExit(f"Bridge executable not found: {self.exe}")
        self.bridge_log.parent.mkdir(parents=True, exist_ok=True)
        cmd = [str(self.exe), "--start-session", *self.extra_args]
        creationflags = 0
        if sys.platform == "win32":
            creationflags = subprocess.CREATE_NEW_PROCESS_GROUP  # type: ignore[attr-defined]
        self.proc = subprocess.Popen(
            cmd,
            cwd=str(_repo_root()),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            bufsize=1,
            creationflags=creationflags,
        )
        self._reader = threading.Thread(target=self._pump_stdout, daemon=True)
        self._reader.start()
        print(f"Started Bridge pid={self.proc.pid}: {' '.join(cmd)}")
        print(f"Bridge log: {self.bridge_log}")

    def _pump_stdout(self) -> None:
        assert self.proc is not None and self.proc.stdout is not None
        with self.bridge_log.open("w", encoding="utf-8") as handle:
            handle.write(
                f"# Bridge console capture started_utc="
                f"{datetime.now(timezone.utc).isoformat()}\n"
            )
            handle.flush()
            for line in self.proc.stdout:
                handle.write(line)
                handle.flush()
                with self._lock:
                    self._lines.append(line.rstrip("\n"))
                printable = (
                    line.rstrip()
                    .replace("\u2192", "->")
                    .replace("\u2190", "<-")
                    .encode("ascii", "replace")
                    .decode("ascii")
                )
                print(f"[bridge] {printable}", flush=True)

    def captured_text(self) -> str:
        with self._lock:
            return "\n".join(self._lines)

    def wait_until_ready(
        self,
        out_needle: str,
        in_needle: str,
        timeout_s: float,
    ) -> tuple[str, str]:
        deadline = time.monotonic() + timeout_s
        while time.monotonic() < deadline:
            if self.proc is not None and self.proc.poll() is not None:
                raise SystemExit(
                    f"Bridge exited early with code {self.proc.returncode}. "
                    f"See {self.bridge_log}"
                )
            outs, inns = _fresh_midi_port_names()
            out_name = _find_port(outs, out_needle)
            in_name = _find_port(inns, in_needle)
            if out_name and in_name:
                _ = READY_MARKERS
                time.sleep(0.4)
                return out_name, in_name
            time.sleep(0.25)
        raise SystemExit(
            "Timed out waiting for Bridge MIDI ports "
            f"{out_needle!r} / {in_needle!r}. See {self.bridge_log}"
        )

    def stop(self, grace_s: float = 8.0) -> None:
        if self.proc is None:
            return
        if self.proc.poll() is not None:
            return
        print(f"Stopping Bridge pid={self.proc.pid} ...")
        try:
            if sys.platform == "win32":
                self.proc.send_signal(signal.CTRL_BREAK_EVENT)  # type: ignore[attr-defined]
            else:
                self.proc.send_signal(signal.SIGINT)
        except OSError:
            self.proc.terminate()
        try:
            self.proc.wait(timeout=grace_s)
        except subprocess.TimeoutExpired:
            print("Bridge did not exit after signal; terminating.")
            self.proc.terminate()
            try:
                self.proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self.proc.kill()
        if self._reader is not None:
            self._reader.join(timeout=2)
        print(f"Bridge stopped (code={self.proc.returncode})")


def _send_sysex(outport, mido, payload: bytes) -> None:
    if len(payload) < 2 or payload[0] != 0xF0 or payload[-1] != 0xF7:
        raise ValueError("payload must be a complete F0…F7 SysEx frame")
    outport.send(mido.Message("sysex", data=list(payload[1:-1])))


def _run_bank_burst(
    mido,
    out_name: str,
    in_name: str,
    args: argparse.Namespace,
    lines: list[str],
    start_index: int,
) -> tuple[ScenarioStats, bool]:
    count = args.count
    start_slot = args.start_slot
    interval_s = args.interval
    reply_timeout_s = args.reply_timeout

    lines.append(f"# start_index: {start_index}")
    lines.append(f"# out_port: {out_name}")
    lines.append(f"# in_port: {in_name}")
    lines.append(f"# burst_count: {count}")
    lines.append(f"# start_slot: {start_slot}")
    lines.append(f"# interval_s: {interval_s}")
    lines.append(f"# reply_timeout_s: {reply_timeout_s}")
    lines.append(
        f"# first_request: {_hex_bytes(_patch_dump_request(start_slot))}"
    )
    lines.append("---")

    stats = ScenarioStats(name="bank_burst_patch")
    burst_started = time.monotonic()
    with mido.open_input(in_name) as inport, mido.open_output(out_name) as outport:
        _drain_input(inport)
        for index in range(1, count + 1):
            slot = (start_slot + index - 1) & 0x7F
            request = _patch_dump_request(slot)
            cycle_started = time.monotonic()
            _drain_input(inport, settle_s=0.005)
            stats.sent += 1
            _send_sysex(outport, mido, request)
            send_line = (
                f"{index:04d} SEND dump_patch_request slot={slot:02X} "
                f"{_hex_bytes(request)} t_mono={cycle_started:.3f}"
            )
            lines.append(send_line)
            print(send_line)

            reply, dt_ms, note = _wait_matched_sysex(
                inport, reply_timeout_s, _is_patch_dump
            )
            if reply is not None:
                stats.ok += 1
                result = (
                    f"{index:04d} RECV dump_patch slot={slot:02X} "
                    f"len={len(reply)} {_frame_head_tail(reply)} "
                    f"dt_ms={dt_ms:.1f}"
                )
                lines.append(result)
                print(result)
            else:
                result = (
                    f"{index:04d} TIMEOUT dump_patch slot={slot:02X} "
                    f"expected_len={PATCH_SIZE} waited_ms={dt_ms:.1f} "
                    f"last={note}"
                )
                stats.fail_lines.append(result)
                lines.append(result)
                print(result)

            if index < count:
                remaining = interval_s - (time.monotonic() - cycle_started)
                if remaining > 0:
                    time.sleep(remaining)

    stats.elapsed_s = time.monotonic() - burst_started
    summary = stats.summary(args.pass_percent)
    lines.append(summary)
    print(summary)
    all_ok = stats.rate >= args.pass_percent and stats.sent > 0
    return stats, all_ok


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
        "--start-slot",
        str(args.start_slot),
        "--out-port",
        args.out_port,
        "--in-port",
        args.in_port,
        "--log",
        str(log_path),
        "--start-index",
        str(start_index),
        "--append-log",
    ]
    print("Launching fresh MIDI lab process (port refresh) ...")
    print(" ".join(cmd))
    completed = subprocess.run(cmd, cwd=str(_repo_root()))
    return int(completed.returncode)


def run_lab(args: argparse.Namespace) -> int:
    mido = _require_mido()
    if args.list_ports:
        return _list_ports(mido)

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

    stamp = _lab_stamp()
    log_dir = _resolve_log_dir(args)
    if args.log:
        log_path = Path(args.log)
        if not log_path.is_absolute():
            log_path = log_dir / log_path
    else:
        log_path = log_dir / f"sysex-matrix-bank-{stamp}.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)

    if args.with_bridge:
        overall_ok = True
        header_lines = [
            "# SysEx Matrix bank-burst lab log",
            f"# started_utc: {datetime.now(timezone.utc).isoformat()}",
            "# with_bridge: true",
            f"# fresh_starts: {args.fresh_starts}",
            f"# burst_count: {args.count}",
            f"# start_slot: {args.start_slot}",
            f"# interval_s: {args.interval}",
            f"# reply_timeout_s: {args.reply_timeout}",
            f"# pass_percent: {args.pass_percent}",
            f"# stamp: {stamp}",
            "---",
        ]
        log_path.write_text("\n".join(header_lines) + "\n", encoding="utf-8")

        for start_index in range(1, args.fresh_starts + 1):
            bridge_log = (
                Path(args.bridge_log)
                if args.bridge_log and args.fresh_starts == 1
                else log_dir / f"bridge-{stamp}-start{start_index}.log"
            )
            bridge = BridgeSession(
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
                rc = _run_midi_lab_in_fresh_process(args, log_path, start_index)
                if rc != 0:
                    overall_ok = False
                fail_hits = _bridge_fail_lines(bridge.captured_text())
                with log_path.open("a", encoding="utf-8") as handle:
                    handle.write(f"# bridge_log_start{start_index}: {bridge_log}\n")
                    if fail_hits:
                        overall_ok = False
                        handle.write(
                            f"# bridge_fail_start{start_index}: "
                            f"count={len(fail_hits)}\n"
                        )
                        for hit in fail_hits:
                            handle.write(f"# bridge_fail: {hit}\n")
                            print(
                                "BRIDGE_FAIL: "
                                + hit.replace("\u2192", "->").replace("\u2190", "<-")
                            )
                    else:
                        handle.write(f"# bridge_fail_start{start_index}: count=0\n")
            finally:
                bridge.stop()
                print(f"Wrote {bridge_log}")

        with log_path.open("a", encoding="utf-8") as handle:
            handle.write(
                f"# finished_utc: {datetime.now(timezone.utc).isoformat()}\n"
            )
            handle.write(f"# overall_pass: {str(overall_ok).lower()}\n")
        print(f"Wrote {log_path}")
        print(f"overall_pass={str(overall_ok).lower()}")
        return 0 if overall_ok else 2

    if args.fresh_sessions > 1 and not args.append_log:
        overall_ok = True
        header_lines = [
            "# SysEx Matrix bank-burst lab log",
            f"# started_utc: {datetime.now(timezone.utc).isoformat()}",
            "# with_bridge: false",
            f"# fresh_sessions: {args.fresh_sessions}",
            f"# session_gap_s: {args.session_gap}",
            f"# burst_count: {args.count}",
            f"# start_slot: {args.start_slot}",
            f"# interval_s: {args.interval}",
            f"# reply_timeout_s: {args.reply_timeout}",
            f"# pass_percent: {args.pass_percent}",
            f"# out_port_needle: {args.out_port}",
            f"# in_port_needle: {args.in_port}",
            f"# stamp: {stamp}",
            "---",
        ]
        log_path.write_text("\n".join(header_lines) + "\n", encoding="utf-8")

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
            rc = _run_midi_lab_in_fresh_process(args, log_path, start_index)
            if rc != 0:
                overall_ok = False

        with log_path.open("a", encoding="utf-8") as handle:
            handle.write(
                f"# finished_utc: {datetime.now(timezone.utc).isoformat()}\n"
            )
            handle.write(f"# overall_pass: {str(overall_ok).lower()}\n")
        print(f"Wrote {log_path}")
        print(f"overall_pass={str(overall_ok).lower()}")
        return 0 if overall_ok else 2

    lines: list[str] = []
    if args.append_log and log_path.is_file():
        pass
    else:
        lines.extend(
            [
                "# SysEx Matrix bank-burst lab log",
                f"# started_utc: {datetime.now(timezone.utc).isoformat()}",
                "# with_bridge: false",
                f"# fresh_sessions: {args.fresh_sessions}",
                f"# burst_count: {args.count}",
                f"# start_slot: {args.start_slot}",
                f"# interval_s: {args.interval}",
                f"# reply_timeout_s: {args.reply_timeout}",
                f"# pass_percent: {args.pass_percent}",
            ]
        )

    out_name = _select_port(list(mido.get_output_names()), args.out_port, "output")
    in_name = _select_port(list(mido.get_input_names()), args.in_port, "input")
    print(f"OUT={out_name}")
    print(f"IN={in_name}")
    print(f"log={log_path}")
    lines.append(f"# session_index: {args.start_index}")

    _, all_ok = _run_bank_burst(mido, out_name, in_name, args, lines, args.start_index)
    lines.append(f"# finished_utc: {datetime.now(timezone.utc).isoformat()}")
    lines.append(f"# start_pass: {str(all_ok).lower()}")
    if not args.append_log:
        lines.append(f"# overall_pass: {str(all_ok).lower()}")

    mode = "a" if args.append_log else "w"
    with log_path.open(mode, encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")
    print(f"Wrote {log_path}")
    if not args.append_log:
        print(f"overall_pass={str(all_ok).lower()}")
    return 0 if all_ok else 2


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Automate MT4 Matrix bank-burst SysEx lab "
            "(N sequential patch dumps; optional Bridge)."
        )
    )
    parser.add_argument(
        "--list-ports",
        action="store_true",
        help="List MIDI input/output names and exit",
    )
    parser.add_argument(
        "--with-bridge",
        action="store_true",
        help="Start Bridge before the lab and stop it afterward",
    )
    parser.add_argument(
        "--bridge-exe",
        default=str(_default_bridge_exe()),
        help="Path to Bridge.exe (default: builds/debug/Debug/Bridge.exe)",
    )
    parser.add_argument(
        "--dev-zadig",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Pass --dev-zadig to Bridge (default: true)",
    )
    parser.add_argument(
        "--bridge-ready-timeout",
        type=float,
        default=45.0,
        help="Seconds to wait for Bridge MIDI ports (default: 45)",
    )
    parser.add_argument(
        "--bridge-log",
        default="",
        help="Bridge console log path (single-start override)",
    )
    parser.add_argument(
        "--fresh-starts",
        type=int,
        default=2,
        help="Fresh Bridge Starts when --with-bridge (default: 2)",
    )
    parser.add_argument(
        "--fresh-sessions",
        type=int,
        default=1,
        help=(
            "Fresh MIDI-only sessions (new process / reopen ports; no Bridge). "
            "Default: 1"
        ),
    )
    parser.add_argument(
        "--session-gap",
        type=float,
        default=2.0,
        help="Seconds between MIDI-only fresh sessions (default: 2.0)",
    )
    parser.add_argument(
        "--out-port",
        default="MT4 Output 1",
        help="MIDI output port name (default: MT4 Output 1)",
    )
    parser.add_argument(
        "--in-port",
        default="MT4 Input 1",
        help="MIDI input port name (default: MT4 Input 1)",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=100,
        help="Number of sequential patch dump requests (default: 100)",
    )
    parser.add_argument(
        "--start-slot",
        type=int,
        default=0,
        help="First Matrix patch slot byte <n> (default: 0)",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=MIN_INTERVAL_S,
        help="Seconds between trial starts (default: 0.01 = 10 ms)",
    )
    parser.add_argument(
        "--reply-timeout",
        type=float,
        default=3.0,
        help="Seconds to wait for each dump reply (default: 3.0)",
    )
    parser.add_argument(
        "--pass-percent",
        type=float,
        default=100.0,
        help="Pass threshold percentage (default: 100)",
    )
    parser.add_argument(
        "--log-dir",
        default="",
        help=(
            "Lab log directory (default: tests/lab-logs/sysex-matrix-bank). "
            "Use tests/lab-logs/sysex-matrix-bank-macos on Apple-driver labs."
        ),
    )
    parser.add_argument(
        "--log",
        default="",
        help="Lab log path (default: <log-dir>/sysex-matrix-bank-<utc>.log)",
    )
    parser.add_argument(
        "--start-index",
        type=int,
        default=1,
        help=argparse.SUPPRESS,
    )
    parser.add_argument(
        "--append-log",
        action="store_true",
        help=argparse.SUPPRESS,
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.count < 1:
        raise SystemExit("--count must be >= 1")
    if args.start_slot < 0 or args.start_slot > 0x7F:
        raise SystemExit("--start-slot must be in 0..127")
    if args.interval < MIN_INTERVAL_S:
        raise SystemExit(
            f"--interval must be >= {MIN_INTERVAL_S} "
            "(Matrix stock pacing ≥10 ms)"
        )
    if args.reply_timeout <= 0:
        raise SystemExit("--reply-timeout must be > 0")
    if args.bridge_ready_timeout <= 0:
        raise SystemExit("--bridge-ready-timeout must be > 0")
    if args.fresh_starts < 1:
        raise SystemExit("--fresh-starts must be >= 1")
    if args.fresh_sessions < 1:
        raise SystemExit("--fresh-sessions must be >= 1")
    if args.session_gap < 0:
        raise SystemExit("--session-gap must be >= 0")
    return run_lab(args)


if __name__ == "__main__":
    sys.exit(main())
