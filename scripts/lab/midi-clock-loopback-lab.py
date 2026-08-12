#!/usr/bin/env python3
"""MIDI Timing Clock + transport DIN loopback lab (Windows Bridge).

Proves Bridge carry of Timing Clock (0xF8), Start (0xFA), Continue (0xFB),
and Stop (0xFC) host→device→DIN→device→host without Scarlett / DAW.
Default topo: red DIN loopback Out2 -> In2 (Matrix may stay on Out1/In1).

  python scripts/lab/midi-clock-loopback-lab.py --with-bridge

Logs under tests/lab-logs/midi-clock-loopback/ by default.
"""

from __future__ import annotations

import argparse
import signal
import subprocess
import sys
import threading
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

READY_MARKERS = (
    "Device Inquiry lab:",
    "MIDI I/O running",
    "DeviceSession started for MT4",
)

# Local subset — do not expand to lab_midi_common needles (behavior lock).
BRIDGE_FAIL_NEEDLES = (
    "MIDI I/O pump failed",
    "WriteBulk failed",
    "WriteBulk skipped",
    "Host→device WriteBulk",
    "Host->device WriteBulk",
    "Device→host DecodeFromDevice failed",
    "Device→host SendToHost",
)

REALTIME_STATUS = {
    0xF8: "clock",
    0xFA: "start",
    0xFB: "continue",
    0xFC: "stop",
}


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def _default_bridge_exe() -> Path:
    return _repo_root() / "builds" / "debug" / "Debug" / "Bridge.exe"


def _lab_stamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


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


def _normalize_port_label(name: str) -> str:
    """Strip teVirtualMIDI trailing index: 'MT4 Out 2 1' -> 'MT4 Out 2'."""
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


def _find_port(names: list[str], needle: str) -> str | None:
    ranked: list[tuple[int, str]] = []
    for name in names:
        rank = _port_match_rank(name, needle)
        if rank is not None:
            ranked.append((rank, name))
    if not ranked:
        return None
    ranked.sort(key=lambda item: (item[0], item[1]))
    best_rank = ranked[0][0]
    best = [name for rank, name in ranked if rank == best_rank]
    if len(best) != 1:
        return None
    return best[0]


def _fresh_midi_port_names() -> tuple[list[str], list[str]]:
    # Fresh interpreter avoids stale rtmidi device lists after Bridge start.
    code = (
        "import mido\n"
        "print('OUT')\n"
        "print('\\n'.join(mido.get_output_names()))\n"
        "print('IN')\n"
        "print('\\n'.join(mido.get_input_names()))\n"
    )
    completed = subprocess.run(
        [sys.executable, "-c", code],
        cwd=str(_repo_root()),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    if completed.returncode != 0:
        raise SystemExit(
            "Failed to enumerate MIDI ports in a fresh process:\n"
            + (completed.stderr or completed.stdout or "")
        )
    outs: list[str] = []
    inns: list[str] = []
    mode = ""
    for line in completed.stdout.splitlines():
        if line == "OUT":
            mode = "out"
            continue
        if line == "IN":
            mode = "in"
            continue
        if not line.strip():
            continue
        if mode == "out":
            outs.append(line)
        elif mode == "in":
            inns.append(line)
    return outs, inns


class BridgeSession:
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
        self, out_needle: str, in_needle: str, timeout_s: float
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
        if self.proc is None or self.proc.poll() is not None:
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
            self.proc.terminate()
            try:
                self.proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self.proc.kill()
        if self._reader is not None:
            self._reader.join(timeout=2)
        print(f"Bridge stopped (code={self.proc.returncode})")


@dataclass(frozen=True)
class LabRun:
    mido: Any
    out_name: str
    in_name: str
    clock_count: int
    clock_interval_s: float
    settle_s: float


@dataclass(frozen=True)
class StatusSend:
    outport: Any
    mido: Any
    status: int
    label: str


@dataclass
class RecvState:
    got: dict[str, int]
    samples: list[str]
    lines: list[str]


@dataclass(frozen=True)
class ClockBatch:
    outport: Any
    inport: Any
    mido: Any
    count: int
    interval_s: float
    label: str
    log_mid: bool


@dataclass(frozen=True)
class OpenPorts:
    inport: Any
    outport: Any


def _hex(data: bytes) -> str:
    return " ".join(f"{b:02X}" for b in data)


def _classify_msg(msg) -> str:
    raw = bytes(msg.bytes())
    if len(raw) == 1 and raw[0] in REALTIME_STATUS:
        return REALTIME_STATUS[raw[0]]
    # mido named types for system realtime
    if msg.type in ("clock", "start", "continue", "stop"):
        return msg.type
    return "other"


def _empty_got() -> dict[str, int]:
    return {
        "clock": 0,
        "start": 0,
        "continue": 0,
        "stop": 0,
        "other": 0,
    }


def _drain_pending(inport, state: RecvState) -> None:
    for msg in inport.iter_pending():
        kind = _classify_msg(msg)
        state.got[kind] = state.got.get(kind, 0) + 1
        # Keep enough samples to retain Start/Continue/Stop after the clock flood.
        if len(state.samples) < 48:
            state.samples.append(f"RECV {kind} {_hex(bytes(msg.bytes()))}")


def _send_status(send: StatusSend, lines: list[str]) -> None:
    msg = send.mido.Message.from_bytes(bytes([send.status]))
    send.outport.send(msg)
    lines.append(f"SEND {send.label} {_hex(bytes([send.status]))}")


def _send_clock_batch(batch: ClockBatch, state: RecvState) -> None:
    mid = max(1, batch.count // 2)
    for index in range(1, batch.count + 1):
        batch.outport.send(batch.mido.Message("clock"))
        if index == 1 or index == batch.count or (batch.log_mid and index == mid):
            state.lines.append(f"SEND {batch.label} {index}/{batch.count} F8")
        _drain_pending(batch.inport, state)
        time.sleep(batch.interval_s)


def _sanity_note_on(cfg: LabRun, ports: OpenPorts, state: RecvState) -> None:
    probe = RecvState(got=_empty_got(), samples=state.samples, lines=state.lines)
    note = cfg.mido.Message("note_on", note=60, velocity=64, channel=0)
    ports.outport.send(note)
    time.sleep(0.15)
    _drain_pending(ports.inport, probe)
    state.lines.append(f"sanity_note_on other_recv={probe.got.get('other', 0)}")


def _run_transport_sequence(cfg: LabRun, ports: OpenPorts, state: RecvState) -> int:
    """Start → clocks → Stop → Continue → clock tail → Stop. Returns expected clock total."""
    expected_clock = cfg.clock_count
    interval = cfg.clock_interval_s
    mido = cfg.mido
    outport = ports.outport
    inport = ports.inport

    _send_status(StatusSend(outport, mido, 0xFA, "start"), state.lines)
    _drain_pending(inport, state)
    time.sleep(interval)

    _send_clock_batch(
        ClockBatch(outport, inport, mido, expected_clock, interval, "clock", True),
        state,
    )

    _send_status(StatusSend(outport, mido, 0xFC, "stop"), state.lines)
    _drain_pending(inport, state)
    time.sleep(interval)

    _send_status(StatusSend(outport, mido, 0xFB, "continue"), state.lines)
    _drain_pending(inport, state)
    time.sleep(interval)

    tail = min(24, max(8, expected_clock // 8))
    _send_clock_batch(
        ClockBatch(outport, inport, mido, tail, interval, "clock_tail", False),
        state,
    )
    expected_clock += tail
    state.lines.append(f"SEND clock_tail count=+{tail}")

    _send_status(StatusSend(outport, mido, 0xFC, "stop"), state.lines)
    _drain_pending(inport, state)
    return expected_clock


def _settle_recv(inport, state: RecvState, settle_s: float) -> None:
    deadline = time.monotonic() + settle_s
    while time.monotonic() < deadline:
        _drain_pending(inport, state)
        time.sleep(0.02)


def _score_pass(state: RecvState, expected_clock: int, elapsed: float) -> bool:
    got = state.got
    clock_ok = got["clock"]
    start_ok = got["start"]
    cont_ok = got["continue"]
    stop_ok = got["stop"]
    other_n = got["other"]
    clock_need = max(1, int(expected_clock * 0.98))
    # Sequence sends two Stops (after main clocks + after Continue tail).
    # Require both so a lost mid-sequence Stop cannot hide behind the trailing one.
    stop_need = 2
    clock_pass = clock_ok >= clock_need
    start_pass = start_ok >= 1
    cont_pass = cont_ok >= 1
    stop_pass = stop_ok >= stop_need
    passed = clock_pass and start_pass and cont_pass and stop_pass

    state.lines.append(
        f"recv clock={clock_ok}/{expected_clock} (need>={clock_need}) "
        f"start={start_ok} continue={cont_ok} stop={stop_ok} (need>={stop_need}) "
        f"other={other_n} elapsed_s={elapsed:.2f}"
    )
    state.lines.append(
        f"summary: clock_pass={str(clock_pass).lower()} "
        f"start_pass={str(start_pass).lower()} "
        f"continue_pass={str(cont_pass).lower()} "
        f"stop_pass={str(stop_pass).lower()} "
        f"overall_pass={str(passed).lower()}"
    )
    return passed


def _run_lab(cfg: LabRun, lines: list[str]) -> bool:
    expected_clock = cfg.clock_count
    samples: list[str] = []
    state = RecvState(got=_empty_got(), samples=samples, lines=lines)

    lines.append(f"ports out={cfg.out_name!r} in={cfg.in_name!r}")
    lines.append(
        f"plan clocks={expected_clock} start=1 continue=1 stop=2 "
        f"interval_s={cfg.clock_interval_s} settle_s={cfg.settle_s}"
    )

    with cfg.mido.open_input(cfg.in_name) as inport, cfg.mido.open_output(
        cfg.out_name
    ) as outport:
        ports = OpenPorts(inport=inport, outport=outport)
        time.sleep(0.5)
        # Sanity: note-on loopback proves DIN before realtime scoring.
        _sanity_note_on(cfg, ports, state)
        state.got = _empty_got()

        t0 = time.monotonic()
        expected_clock = _run_transport_sequence(cfg, ports, state)
        _settle_recv(inport, state, cfg.settle_s)
        elapsed = time.monotonic() - t0

    for sample in samples:
        lines.append(sample)
    return _score_pass(state, expected_clock, elapsed)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="MIDI Timing Clock + Start/Continue/Stop DIN loopback lab"
    )
    parser.add_argument("--with-bridge", action="store_true")
    parser.add_argument(
        "--bridge-exe",
        default="",
        help="Bridge.exe path (default: builds/debug/Debug/Bridge.exe)",
    )
    parser.add_argument(
        "--out-port",
        default="MT4 Out 2",
        help="Host OUT port (default: MT4 Out 2; red DIN loopback Out2->In2)",
    )
    parser.add_argument(
        "--in-port",
        default="MT4 In 2",
        help="Host IN port (default: MT4 In 2; red DIN loopback Out2->In2)",
    )
    parser.add_argument(
        "--clock-count",
        type=int,
        default=96,
        help="Timing Clock (0xF8) messages in the main stream (default: 96)",
    )
    parser.add_argument(
        "--clock-interval-ms",
        type=float,
        default=10.0,
        help="Gap between clocks in ms (default: 10)",
    )
    parser.add_argument(
        "--settle-s",
        type=float,
        default=1.0,
        help="Wait after last send before scoring (default: 1)",
    )
    parser.add_argument(
        "--log-dir",
        default="tests/lab-logs/midi-clock-loopback",
        help="Log directory (default: tests/lab-logs/midi-clock-loopback)",
    )
    parser.add_argument(
        "--list-ports", action="store_true", help="List MIDI ports and exit"
    )
    return parser


def _list_ports_exit() -> int:
    outs, inns = _fresh_midi_port_names()
    print("OUT:")
    for name in outs:
        print(f"  {name}")
    print("IN:")
    for name in inns:
        print(f"  {name}")
    return 0


def _prepare_log_dir(args: argparse.Namespace) -> tuple[Path, str, Path, list[str]]:
    log_dir = Path(args.log_dir)
    if not log_dir.is_absolute():
        log_dir = _repo_root() / log_dir
    log_dir.mkdir(parents=True, exist_ok=True)
    stamp = _lab_stamp()
    lab_log = log_dir / f"midi-clock-loopback-{stamp}.log"
    lines: list[str] = [
        f"# midi-clock-loopback-lab utc={stamp}",
        f"# out_port={args.out_port!r} in_port={args.in_port!r}",
        "# topo expect: DIN loopback Out2->In2; Matrix may stay on Out1/In1",
    ]
    return log_dir, stamp, lab_log, lines


def _resolve_ports(
    args: argparse.Namespace, log_dir: Path, stamp: str
) -> tuple[str, str, BridgeSession | None]:
    if args.with_bridge:
        exe = Path(args.bridge_exe) if args.bridge_exe else _default_bridge_exe()
        if not exe.is_absolute():
            exe = _repo_root() / exe
        bridge = BridgeSession(
            exe,
            log_dir / f"bridge-{stamp}.log",
            ["--dev-zadig"],
        )
        try:
            bridge.start()
            out_name, in_name = bridge.wait_until_ready(
                args.out_port, args.in_port, timeout_s=45.0
            )
        except BaseException:
            # Match pre-refactor: stop even if wait_until_ready fails after start.
            bridge.stop()
            raise
        return out_name, in_name, bridge

    outs, inns = _fresh_midi_port_names()
    out_name = _find_port(outs, args.out_port)
    in_name = _find_port(inns, args.in_port)
    if not out_name or not in_name:
        raise SystemExit(
            f"Ports not found: out={args.out_port!r} in={args.in_port!r}. "
            "Use --list-ports / --with-bridge."
        )
    return out_name, in_name, None


def _apply_bridge_fail_needles(bridge: BridgeSession | None, lines: list[str], passed: bool) -> bool:
    if bridge is None:
        return passed
    # Full-text substring match (local semantics — not line-based common helper).
    text = bridge.captured_text()
    hits = [n for n in BRIDGE_FAIL_NEEDLES if n in text]
    if hits:
        lines.append(f"bridge_fail_needles: {hits}")
        return False
    lines.append("bridge_fail_needles: none")
    return passed


def _write_and_print_log(lab_log: Path, lines: list[str]) -> None:
    lab_log.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote {lab_log}")
    for line in lines:
        safe = line.encode("ascii", "replace").decode("ascii")
        print(safe)


def run_lab(args: argparse.Namespace) -> int:
    mido = _require_mido()
    if args.list_ports:
        return _list_ports_exit()
    if args.clock_count < 1:
        raise SystemExit("--clock-count must be >= 1")

    log_dir, stamp, lab_log, lines = _prepare_log_dir(args)
    bridge: BridgeSession | None = None
    try:
        out_name, in_name, bridge = _resolve_ports(args, log_dir, stamp)
        if bridge is not None:
            lines.append(f"bridge_ready out={out_name!r} in={in_name!r}")

        passed = _run_lab(
            LabRun(
                mido=mido,
                out_name=out_name,
                in_name=in_name,
                clock_count=args.clock_count,
                clock_interval_s=args.clock_interval_ms / 1000.0,
                settle_s=args.settle_s,
            ),
            lines,
        )
        passed = _apply_bridge_fail_needles(bridge, lines, passed)
        _write_and_print_log(lab_log, lines)
        return 0 if passed else 2
    finally:
        if bridge is not None:
            bridge.stop()


def main(argv: list[str] | None = None) -> int:
    return run_lab(build_parser().parse_args(argv))


if __name__ == "__main__":
    sys.exit(main())
