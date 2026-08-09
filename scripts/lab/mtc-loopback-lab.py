#!/usr/bin/env python3
"""MTC quarter-frame + full-frame DIN loopback lab (Windows Bridge).

Proves Bridge carry of MTC sync bytes host→device→DIN→device→host without
Scarlett / DAW. Default topo: red DIN loopback Out2 -> In2 (Matrix may stay
on Out1/In1 powered off). Requires Emagic IN demux OUT-hint fix so unlabeled
echo is not stuck on Virtual In 1.

  python scripts/lab/mtc-loopback-lab.py --with-bridge

Logs under tests/lab-logs/mtc-loopback/ by default.
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
    "Device→host DecodeFromDevice failed",
    "Device→host SendToHost",
)

# Universal Real Time MTC full-frame: F0 7F 7F 01 01 hr mn sc fr F7
FULL_FRAME_INNER = bytes([0x7F, 0x7F, 0x01, 0x01, 0x20, 0x15, 0x30, 0x10])
FULL_FRAME = bytes([0xF0]) + FULL_FRAME_INNER + bytes([0xF7])


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


def _quarter_frame_bytes(frame_type: int, nibble: int) -> bytes:
    return bytes([0xF1, ((frame_type & 0x7) << 4) | (nibble & 0x0F)])


def _build_time_frames(count: int) -> list[bytes]:
    """count complete SMPTE times = count * 8 quarter-frames."""
    frames: list[bytes] = []
    # Arbitrary stable time: 01:02:03:04 @ 30 fps non-drop (rate bits in type 7).
    nibbles = [
        0x4,  # frame ls
        0x0,  # frame ms
        0x3,  # seconds ls
        0x0,  # seconds ms
        0x2,  # minutes ls
        0x0,  # minutes ms
        0x1,  # hours ls
        0x1,  # hours ms + 30 fps
    ]
    for _ in range(count):
        for frame_type, nibble in enumerate(nibbles):
            frames.append(_quarter_frame_bytes(frame_type, nibble))
    return frames


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


def _classify_msg(msg) -> str:
    raw = bytes(msg.bytes())
    if msg.type == "quarter_frame" or (len(raw) == 2 and raw[0] == 0xF1):
        return "qf"
    if msg.type == "sysex" and bytes(msg.data) == FULL_FRAME_INNER:
        return "full"
    if raw == FULL_FRAME:
        return "full"
    return "other"


def _drain_pending(inport, got: dict[str, int], samples: list[str]) -> None:
    for msg in inport.iter_pending():
        kind = _classify_msg(msg)
        got[kind] = got.get(kind, 0) + 1
        if len(samples) < 12:
            samples.append(f"RECV {kind} {_hex(bytes(msg.bytes()))}")


def _run_lab(
    mido,
    out_name: str,
    in_name: str,
    time_frames: int,
    qf_interval_s: float,
    settle_s: float,
    lines: list[str],
) -> bool:
    qf_send = _build_time_frames(time_frames)
    expected_qf = len(qf_send)
    got: dict[str, int] = {"qf": 0, "full": 0, "other": 0}
    samples: list[str] = []

    lines.append(f"ports out={out_name!r} in={in_name!r}")
    lines.append(
        f"plan qf_count={expected_qf} full_frame=1 "
        f"interval_s={qf_interval_s} settle_s={settle_s}"
    )

    with mido.open_input(in_name) as inport, mido.open_output(out_name) as outport:
        time.sleep(0.5)
        # Sanity: note-on loopback proves DIN Out2<->In2 before MTC scoring.
        note = mido.Message("note_on", note=60, velocity=64, channel=0)
        outport.send(note)
        time.sleep(0.15)
        _drain_pending(inport, got, samples)
        note_hits = got.get("other", 0)
        lines.append(f"sanity_note_on other_recv={note_hits}")
        # Reset counters for MTC scoring (keep samples).
        got = {"qf": 0, "full": 0, "other": 0}

        t0 = time.monotonic()
        for index, frame in enumerate(qf_send, start=1):
            outport.send(mido.Message.from_bytes(frame))
            if index == 1 or index == expected_qf or index % 8 == 0:
                lines.append(f"SEND qf {index}/{expected_qf} {_hex(frame)}")
            _drain_pending(inport, got, samples)
            time.sleep(qf_interval_s)

        lines.append(f"SEND full_frame {_hex(FULL_FRAME)}")
        outport.send(mido.Message("sysex", data=list(FULL_FRAME_INNER)))
        _drain_pending(inport, got, samples)

        dense = _build_time_frames(1)
        for frame in dense:
            outport.send(mido.Message.from_bytes(frame))
            _drain_pending(inport, got, samples)
            time.sleep(qf_interval_s)
        expected_qf += len(dense)
        lines.append(f"SEND dense_tail qf=+{len(dense)}")

        deadline = time.monotonic() + settle_s
        while time.monotonic() < deadline:
            _drain_pending(inport, got, samples)
            time.sleep(0.02)
        elapsed = time.monotonic() - t0

    for sample in samples:
        lines.append(sample)

    qf_ok = got["qf"]
    full_ok = got["full"]
    other_n = got["other"]
    qf_need = max(1, int(expected_qf * 0.98))
    qf_pass = qf_ok >= qf_need
    full_pass = full_ok >= 1
    passed = qf_pass and full_pass

    lines.append(
        f"RECV qf={qf_ok}/{expected_qf} (need>={qf_need}) "
        f"full_frame={full_ok} other={other_n} elapsed_s={elapsed:.2f}"
    )
    lines.append(
        f"summary: qf_pass={str(qf_pass).lower()} "
        f"full_pass={str(full_pass).lower()} overall_pass={str(passed).lower()}"
    )
    return passed


def _hex(data: bytes) -> str:
    return " ".join(f"{b:02X}" for b in data)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="MTC quarter-frame + full-frame DIN loopback lab"
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
        "--time-frames",
        type=int,
        default=8,
        help="Complete SMPTE times to send as quarter-frames (default: 8 → 64 QF)",
    )
    parser.add_argument(
        "--qf-interval-ms",
        type=float,
        default=8.0,
        help="Gap between quarter-frames in ms (default: 8 ≈ 125 QF/s)",
    )
    parser.add_argument(
        "--settle-s",
        type=float,
        default=1.0,
        help="Wait after last send before scoring (default: 1)",
    )
    parser.add_argument(
        "--log-dir",
        default="tests/lab-logs/mtc-loopback",
        help="Log directory (default: tests/lab-logs/mtc-loopback)",
    )
    parser.add_argument(
        "--list-ports", action="store_true", help="List MIDI ports and exit"
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    mido = _require_mido()
    if args.list_ports:
        outs, inns = _fresh_midi_port_names()
        print("OUT:")
        for name in outs:
            print(f"  {name}")
        print("IN:")
        for name in inns:
            print(f"  {name}")
        return 0

    if args.time_frames < 1:
        raise SystemExit("--time-frames must be >= 1")

    log_dir = Path(args.log_dir)
    if not log_dir.is_absolute():
        log_dir = _repo_root() / log_dir
    log_dir.mkdir(parents=True, exist_ok=True)
    stamp = _lab_stamp()
    lab_log = log_dir / f"mtc-loopback-{stamp}.log"
    lines: list[str] = [
        f"# mtc-loopback-lab utc={stamp}",
        f"# out_port={args.out_port!r} in_port={args.in_port!r}",
        "# topo expect: DIN loopback Out2->In2; Matrix may stay on Out1/In1 powered off",
    ]

    bridge: BridgeSession | None = None
    try:
        if args.with_bridge:
            exe = (
                Path(args.bridge_exe)
                if args.bridge_exe
                else _default_bridge_exe()
            )
            if not exe.is_absolute():
                exe = _repo_root() / exe
            bridge = BridgeSession(
                exe,
                log_dir / f"bridge-{stamp}.log",
                ["--dev-zadig"],
            )
            bridge.start()
            out_name, in_name = bridge.wait_until_ready(
                args.out_port, args.in_port, timeout_s=45.0
            )
            lines.append(f"bridge_ready out={out_name!r} in={in_name!r}")
        else:
            outs, inns = _fresh_midi_port_names()
            out_name = _find_port(outs, args.out_port)
            in_name = _find_port(inns, args.in_port)
            if not out_name or not in_name:
                raise SystemExit(
                    f"Ports not found: out={args.out_port!r} in={args.in_port!r}. "
                    "Use --list-ports / --with-bridge."
                )

        passed = _run_lab(
            mido,
            out_name,
            in_name,
            time_frames=args.time_frames,
            qf_interval_s=args.qf_interval_ms / 1000.0,
            settle_s=args.settle_s,
            lines=lines,
        )

        if bridge is not None:
            text = bridge.captured_text()
            hits = [n for n in BRIDGE_FAIL_NEEDLES if n in text]
            if hits:
                lines.append(f"bridge_fail_needles: {hits}")
                passed = False
            else:
                lines.append("bridge_fail_needles: none")

        lab_log.write_text("\n".join(lines) + "\n", encoding="utf-8")
        print(f"Wrote {lab_log}")
        for line in lines:
            safe = line.encode("ascii", "replace").decode("ascii")
            print(safe)
        return 0 if passed else 2
    finally:
        if bridge is not None:
            bridge.stop()


if __name__ == "__main__":
    sys.exit(main())
