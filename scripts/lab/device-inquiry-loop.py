#!/usr/bin/env python3
"""Automate MT4 Device Inquiry lab (optional Bridge start/stop + host MIDI loop).

Preferred one-shot (starts Bridge, runs 20 inquiries, stops Bridge, writes logs):

  python -m pip install -r scripts/lab/requirements-device-inquiry.txt
  python scripts/lab/device-inquiry-loop.py --with-bridge --count 20 --interval 5

Close MIDI-OX on MT4 ports first. Matrix-1000 must already be powered and DIN-cabled.

Logs (default under tests/lab-logs/device-inquiry/):
  device-inquiry-<UTC>.log  — SEND / RECV / TIMEOUT + summary
  bridge-<UTC>.log          — Bridge stdout/stderr (when --with-bridge)
"""

from __future__ import annotations

import argparse
import signal
import subprocess
import sys
import threading
import time
from datetime import datetime, timezone
from pathlib import Path


INQUIRY = bytes([0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7])
READY_MARKERS = (
    "Device Inquiry lab:",
    "MIDI I/O running",
    "DeviceSession started for MT4",
)


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def _default_bridge_exe() -> Path:
    return _repo_root() / "builds" / "debug" / "Debug" / "Bridge.exe"


def _lab_stamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def _default_log_paths(stamp: str) -> tuple[Path, Path]:
    out_dir = _repo_root() / "tests" / "lab-logs" / "device-inquiry"
    out_dir.mkdir(parents=True, exist_ok=True)
    return (
        out_dir / f"device-inquiry-{stamp}.log",
        out_dir / f"bridge-{stamp}.log",
    )


def _hex_bytes(data: bytes) -> str:
    return " ".join(f"{byte:02X}" for byte in data)


def _is_identity_reply(data: bytes) -> bool:
    return (
        len(data) >= 5
        and data[0] == 0xF0
        and data[1] == 0x7E
        and data[3] == 0x06
        and data[4] == 0x02
        and data[-1] == 0xF7
    )


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
    """Enumerate MIDI ports in a fresh Python process.

    WinMM often does not expose teVirtualMIDI ports created after this process
    started; a child interpreter sees the current device list.
    """
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


def _run_midi_lab_in_fresh_process(args: argparse.Namespace, log_path: Path) -> int:
    """Run the host MIDI loop after Bridge ports exist (fresh WinMM view)."""
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
        "--log",
        str(log_path),
    ]
    print("Launching fresh MIDI lab process (WinMM port refresh) ...")
    print(" ".join(cmd))
    completed = subprocess.run(cmd, cwd=str(_repo_root()))
    return int(completed.returncode)


def _normalize_port_label(name: str) -> str:
    """Strip teVirtualMIDI / rtmidi trailing ' <n>' index: 'MT4 Output 1 1' -> 'MT4 Output 1'."""
    parts = name.rsplit(" ", 1)
    if len(parts) == 2 and parts[1].isdigit():
        return parts[0]
    return name


def _port_match_rank(name: str, needle: str) -> int | None:
    """Lower rank is better. None = not a match. Avoids 'MT4 Output 1' matching 'MT4 Output 10'."""
    if name == needle:
        return 0
    if _normalize_port_label(name) == needle:
        return 1
    if not name.lower().startswith(needle.lower()):
        return None
    rest = name[len(needle) :]
    if rest == "":
        return 0
    # Only allow a trailing numeric client index after the logical port name.
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


def _pick_port(names: list[str], needle: str, kind: str) -> str:
    return _select_port(names, needle, kind)


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


def _wait_identity(inport, timeout_s: float) -> tuple[bytes | None, float]:
    started = time.monotonic()
    deadline = started + timeout_s
    while time.monotonic() < deadline:
        for message in inport.iter_pending():
            if message.type != "sysex":
                continue
            payload = bytes((0xF0, *message.data, 0xF7))
            if _is_identity_reply(payload):
                return payload, (time.monotonic() - started) * 1000.0
        time.sleep(0.005)
    return None, (time.monotonic() - started) * 1000.0


class BridgeSession:
    """Owns a Bridge.exe --start-session child and its console log."""

    def __init__(self, exe: Path, bridge_log: Path, extra_args: list[str]):
        self.exe = exe
        self.bridge_log = bridge_log
        self.extra_args = extra_args
        self.proc: subprocess.Popen[str] | None = None
        self._ready = threading.Event()
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
            handle.write(f"# Bridge console capture started_utc={datetime.now(timezone.utc).isoformat()}\n")
            handle.flush()
            for line in self.proc.stdout:
                handle.write(line)
                handle.flush()
                with self._lock:
                    self._lines.append(line.rstrip("\n"))
                # Avoid Unicode arrows from Bridge logs on cp1252 consoles.
                printable = (
                    line.rstrip()
                    .replace("\u2192", "->")
                    .replace("\u2190", "<-")
                    .encode("ascii", "replace")
                    .decode("ascii")
                )
                print(f"[bridge] {printable}", flush=True)
                if any(marker in line for marker in READY_MARKERS):
                    self._ready.set()

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
            # Fresh WinMM enumeration is enough once teVirtualMIDI ports appear.
            if out_name and in_name:
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
                # Ctrl+Break to the process group (Bridge handles console cancel).
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


def _run_inquiry_loop(
    mido,
    out_name: str,
    in_name: str,
    args: argparse.Namespace,
    lines: list[str],
) -> tuple[int, bool, str]:
    replies = 0
    late_replies = 0
    with mido.open_input(in_name) as inport, mido.open_output(out_name) as outport:
        _drain_input(inport)
        for index in range(1, args.count + 1):
            cycle_started = time.monotonic()
            _drain_input(inport, settle_s=0.02)
            outport.send(mido.Message("sysex", data=list(INQUIRY[1:-1])))
            send_line = (
                f"{index:04d} SEND inquiry {_hex_bytes(INQUIRY)} "
                f"t_mono={cycle_started:.3f}"
            )
            lines.append(send_line)
            print(send_line)

            reply, dt_ms = _wait_identity(inport, args.reply_timeout)
            if reply is not None:
                replies += 1
                result = (
                    f"{index:04d} RECV identity {_hex_bytes(reply)} "
                    f"dt_ms={dt_ms:.1f}"
                )
                lines.append(result)
                print(result)
            else:
                # Keep listening until the next send so late VirtualMIDI delivery
                # is not counted as a hard miss when Bridge already SendToHost'ed.
                remaining = args.interval - (time.monotonic() - cycle_started)
                late_timeout = max(0.0, remaining)
                late_reply, late_ms = _wait_identity(inport, late_timeout)
                if late_reply is None:
                    result = (
                        f"{index:04d} TIMEOUT no identity within "
                        f"{args.reply_timeout:.1f}s (+{late_timeout:.1f}s slack) "
                        f"waited_ms={dt_ms + late_ms:.1f}"
                    )
                    lines.append(result)
                    print(result)
                else:
                    replies += 1
                    late_replies += 1
                    result = (
                        f"{index:04d} LATE identity {_hex_bytes(late_reply)} "
                        f"dt_ms={dt_ms + late_ms:.1f}"
                    )
                    lines.append(result)
                    print(result)

            if index < args.count:
                remaining = args.interval - (time.monotonic() - cycle_started)
                if remaining > 0:
                    time.sleep(remaining)

    rate = (100.0 * replies / args.count) if args.count else 0.0
    passed = rate >= args.pass_percent
    summary = (
        f"summary: sent={args.count} recv={replies} late={late_replies} "
        f"rate={rate:.1f}% pass={str(passed).lower()} "
        f"(need>={args.pass_percent:.0f}%)"
    )
    return replies, passed, summary


def run_lab(args: argparse.Namespace) -> int:
    mido = _require_mido()
    if args.list_ports:
        return _list_ports(mido)

    stamp = _lab_stamp()
    default_inquiry_log, default_bridge_log = _default_log_paths(stamp)
    log_path = Path(args.log) if args.log else default_inquiry_log
    bridge_log = Path(args.bridge_log) if args.bridge_log else default_bridge_log
    log_path.parent.mkdir(parents=True, exist_ok=True)

    if args.with_bridge:
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
            print(f"Bridge ports ready: OUT={out_name} IN={in_name}")
            print(f"Bridge log: {bridge_log}")
            # MIDI open must happen in a process started AFTER the ports exist.
            return _run_midi_lab_in_fresh_process(args, log_path)
        finally:
            bridge.stop()
            print(f"Wrote {bridge_log}")

    lines: list[str] = []
    started_iso = datetime.now(timezone.utc).isoformat()
    lines.append("# Device Inquiry lab log (automated; no MIDI-OX Thru)")
    lines.append(f"# started_utc: {started_iso}")
    lines.append("# with_bridge: false")
    lines.append(f"# count: {args.count}")
    lines.append(f"# interval_s: {args.interval}")
    lines.append(f"# reply_timeout_s: {args.reply_timeout}")
    lines.append(f"# inquiry: {_hex_bytes(INQUIRY)}")

    out_name = _pick_port(list(mido.get_output_names()), args.out_port, "output")
    in_name = _pick_port(list(mido.get_input_names()), args.in_port, "input")
    lines.append(f"# out_port: {out_name}")
    lines.append(f"# in_port: {in_name}")
    lines.append("---")

    print(f"OUT={out_name}")
    print(f"IN={in_name}")
    print(f"log={log_path}")
    print(
        f"Sending {args.count} Device Inquiry(s), every {args.interval}s "
        f"(reply timeout {args.reply_timeout}s)."
    )

    _, passed, summary = _run_inquiry_loop(mido, out_name, in_name, args, lines)
    lines.append("---")
    lines.append(summary)
    lines.append(f"# finished_utc: {datetime.now(timezone.utc).isoformat()}")
    log_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(summary)
    print(f"Wrote {log_path}")
    return 0 if passed else 2


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Automate MT4 Device Inquiry lab (host MIDI; optional Bridge start/stop)."
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
        help="Bridge console log path (default: tests/lab-logs/device-inquiry/bridge-<utc>.log)",
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
        default=20,
        help="Number of inquiries to send (default: 20)",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=5.0,
        help="Seconds between inquiry starts (default: 5)",
    )
    parser.add_argument(
        "--reply-timeout",
        type=float,
        default=4.0,
        help="Seconds to wait for Identity Reply after each send (default: 4)",
    )
    parser.add_argument(
        "--pass-percent",
        type=float,
        default=95.0,
        help="Pass threshold percentage (default: 95)",
    )
    parser.add_argument(
        "--log",
        default="",
        help="Inquiry log path (default: tests/lab-logs/device-inquiry/device-inquiry-<utc>.log)",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.count < 1:
        raise SystemExit("--count must be >= 1")
    if args.interval <= 0:
        raise SystemExit("--interval must be > 0")
    if args.reply_timeout <= 0:
        raise SystemExit("--reply-timeout must be > 0")
    if args.bridge_ready_timeout <= 0:
        raise SystemExit("--bridge-ready-timeout must be > 0")
    return run_lab(args)


if __name__ == "__main__":
    sys.exit(main())
