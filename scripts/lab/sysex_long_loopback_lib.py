"""Long SysEx loopback helpers (assembler, wait, synthetic payloads, stats)."""

from __future__ import annotations

import argparse
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path

_LAB_DIR = Path(__file__).resolve().parent
if str(_LAB_DIR) not in sys.path:
    sys.path.insert(0, str(_LAB_DIR))

import lab_midi_common as lab_midi  # noqa: E402

DEFAULT_SIZES = (1024, 4096)
DEFAULT_ABORT_AFTER_TIMEOUTS = 3
MIN_INTERVAL_S = 0.05
SYNTH_MARK = bytes([0x7D, 0x4C, 0x42])


def default_fixture() -> Path:
    return lab_midi.repo_root() / "tests" / "fixtures" / "sysex" / "long-loopback-14708.syx"


def patch_rtmidi_input_queue() -> None:
    """Raise python-rtmidi MidiIn queue + WinMM SysEx buffers for long loopback.

    Stock 1.5.8 on Windows discards SysEx > ~1024 B (RtMidi WinMM MIDIHDR size).
    Need python-rtmidi >= 1.6 with MidiIn.set_buffer_size (GitHub main / newer wheels).
    """
    try:
        import rtmidi
    except ImportError:
        return
    if getattr(rtmidi, "_unitor_queue_patched", False):
        return
    _orig = rtmidi.MidiIn

    def _MidiIn(*args, **kwargs):
        kwargs.setdefault("queue_size_limit", 65536)
        inst = _orig(*args, **kwargs)
        # Must run before open_port — WinMM MIDIHDR size is fixed at open.
        if hasattr(inst, "set_buffer_size"):
            inst.set_buffer_size(65535, 16)
        return inst

    rtmidi.MidiIn = _MidiIn  # type: ignore[misc,assignment]
    rtmidi._unitor_queue_patched = True  # type: ignore[attr-defined]


def require_mido():
    try:
        import mido  # noqa: F401
    except ImportError as exc:
        raise SystemExit(
            "Missing dependency: mido / python-rtmidi.\n"
            "Install with:\n"
            "  python -m pip install -r scripts/lab/requirements-device-inquiry.txt\n"
        ) from exc
    import mido

    patch_rtmidi_input_queue()
    return mido


def prepare_mido_input(inport) -> None:
    """Prepare MidiIn for long SysEx; require WinMM buffer enlarge only on Windows."""
    rt = getattr(inport, "_rt", None)
    # Stock 1.5.8 WinMM drops SysEx > ~1024 B unless set_buffer_size ran before open.
    # CoreMIDI (macOS) does not need that API — do not hard-fail Darwin labs on 1.5.8.
    if sys.platform == "win32" and (
        rt is None or not hasattr(rt, "set_buffer_size")
    ):
        raise SystemExit(
            "This lab needs python-rtmidi >= 1.6 with MidiIn.set_buffer_size "
            "(Windows WinMM otherwise drops SysEx above ~1024 bytes).\n"
            "Install with:\n"
            "  python -m pip install -U \"git+https://github.com/SpotlightKid/python-rtmidi.git\"\n"
        )
    # Belt-and-suspenders: mido already enables SysEx in Input._open.
    if rt is not None and hasattr(rt, "ignore_types"):
        rt.ignore_types(False, False, True)


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


def frame_head_tail(data: bytes) -> str:
    if len(data) <= 8:
        return lab_midi.hex_bytes(data)
    return (
        f"head={lab_midi.hex_bytes(data[:4])} tail={lab_midi.hex_bytes(data[-4:])} "
        f"len={len(data)}"
    )


def first_diff_offset(got: bytes, expected: bytes) -> int:
    limit = min(len(got), len(expected))
    for index in range(limit):
        if got[index] != expected[index]:
            return index
    if len(got) != len(expected):
        return limit
    return -1


def mismatch_note(frame: bytes, expected: bytes, data_len: int) -> str:
    offset = first_diff_offset(frame, expected)
    return (
        f"mismatch {frame_head_tail(frame)} "
        f"(expected_len={len(expected)}; mido_data_len={data_len}; "
        f"first_diff_offset={offset}; missing_bytes={len(expected) - len(frame)})"
    )


def wait_exact_sysex(
    inport,
    expected: bytes,
    timeout_s: float,
) -> tuple[bytes | None, float, str]:
    assembler = SysexAssembler()
    started = time.monotonic()
    deadline = started + timeout_s
    last_note = "none"
    saw_types: dict[str, int] = {}
    while time.monotonic() < deadline:
        for message in inport.iter_pending():
            saw_types[message.type] = saw_types.get(message.type, 0) + 1
            if message.type != "sysex":
                continue
            data_len = len(message.data)
            for frame in assembler.push_mido_sysex_data(message.data):
                if frame == expected:
                    return frame, (time.monotonic() - started) * 1000.0, "match"
                last_note = mismatch_note(frame, expected, data_len)
        time.sleep(0.005)
    if saw_types and last_note == "none":
        last_note = "none types=" + ",".join(
            f"{k}:{v}" for k, v in sorted(saw_types.items())
        )
    return None, (time.monotonic() - started) * 1000.0, last_note


def build_synthetic_sysex(size: int) -> bytes:
    """Build a valid MIDI SysEx of exactly `size` bytes (includes F0 and F7)."""
    if size < 8:
        raise ValueError("synthetic SysEx size must be >= 8")
    body_len = size - 2
    if body_len < len(SYNTH_MARK) + 2:
        raise ValueError("synthetic SysEx size too small for lab mark")
    body = bytearray(SYNTH_MARK)
    body.append((size >> 7) & 0x7F)
    body.append(size & 0x7F)
    fill_needed = body_len - len(body)
    for index in range(fill_needed):
        body.append(index & 0x7F)
    frame = bytes([0xF0]) + bytes(body) + bytes([0xF7])
    if len(frame) != size:
        raise RuntimeError(f"synthetic size mismatch {len(frame)} != {size}")
    if any(byte >= 0x80 for byte in frame[1:-1]):
        raise RuntimeError("synthetic body contains non-data MIDI bytes")
    return frame


def load_fixture(path: Path) -> bytes:
    if not path.is_file():
        raise SystemExit(f"Fixture not found: {path}")
    data = path.read_bytes()
    if len(data) < 3 or data[0] != 0xF0 or data[-1] != 0xF7:
        raise SystemExit(f"Fixture {path} is not a SysEx frame (F0…F7)")
    if any(byte >= 0x80 for byte in data[1:-1]):
        raise SystemExit(f"Fixture {path} has non-data bytes in SysEx body")
    return data


def parse_sizes(raw: str) -> list[int]:
    if not raw.strip():
        return list(DEFAULT_SIZES)
    sizes: list[int] = []
    for part in raw.split(","):
        part = part.strip()
        if not part:
            continue
        value = int(part)
        if value < 8:
            raise SystemExit(f"size {value} must be >= 8")
        sizes.append(value)
    if not sizes:
        raise SystemExit("--sizes must list at least one size")
    return sizes


@dataclass
class ScenarioStats:
    name: str
    sent: int = 0
    ok: int = 0
    fail_lines: list[str] = field(default_factory=list)
    elapsed_s: float = 0.0
    aborted: bool = False
    abort_reason: str = ""

    @property
    def rate(self) -> float:
        return (100.0 * self.ok / self.sent) if self.sent else 0.0

    def summary(self, pass_percent: float) -> str:
        passed = (not self.aborted) and self.rate >= pass_percent and self.sent > 0
        extra = f" abort={self.abort_reason}" if self.aborted else ""
        return (
            f"summary[{self.name}]: sent={self.sent} ok={self.ok} "
            f"rate={self.rate:.1f}% elapsed_s={self.elapsed_s:.2f} "
            f"pass={str(passed).lower()} (need>={pass_percent:.0f}%){extra}"
        )


class LabAbort(RuntimeError):
    """Raised when wall-clock or consecutive-timeout guard fires."""


def payload_count(args: argparse.Namespace) -> int:
    return len(args.sizes) + (0 if args.skip_fixture else 1)


def session_count(args: argparse.Namespace) -> int:
    if args.with_bridge:
        return args.fresh_starts
    return args.fresh_sessions


def auto_max_wall_seconds(args: argparse.Namespace) -> float:
    """Healthy-ish budget plus a few timeouts — not a full timeout storm."""
    sessions = session_count(args)
    payloads = payload_count(args)
    abort_n = max(1, int(args.abort_after_timeouts))
    slow_per_payload = abort_n * args.reply_timeout
    fast_per_payload = max(0, args.count - abort_n) * max(args.interval, 0.2)
    per_session = payloads * (slow_per_payload + fast_per_payload + 5.0)
    if args.with_bridge:
        bridge_overhead = sessions * (args.bridge_ready_timeout + 20.0)
    else:
        bridge_overhead = sessions * 10.0
    return bridge_overhead + sessions * per_session + 30.0


def check_wall_deadline(deadline_mono: float | None, where: str) -> None:
    if deadline_mono is None:
        return
    if time.monotonic() >= deadline_mono:
        raise LabAbort(f"max-wall-seconds exceeded at {where}")


def send_sysex(outport, mido, payload: bytes) -> None:
    if len(payload) < 2 or payload[0] != 0xF0 or payload[-1] != 0xF7:
        raise ValueError("payload must be a complete F0…F7 SysEx frame")
    outport.send(mido.Message("sysex", data=list(payload[1:-1])))


def payload_plan(args: argparse.Namespace) -> list[tuple[str, bytes]]:
    plan: list[tuple[str, bytes]] = []
    for size in args.sizes:
        plan.append((f"synth_{size}", build_synthetic_sysex(size)))
    if not args.skip_fixture:
        fixture = load_fixture(Path(args.fixture))
        plan.append((f"fixture_{len(fixture)}", fixture))
    return plan


def _add_bridge_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--with-bridge",
        action="store_true",
        help="Start Bridge before the lab and stop it afterward",
    )
    parser.add_argument(
        "--bridge-exe",
        default=str(lab_midi.default_bridge_exe()),
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


def _add_session_args(parser: argparse.ArgumentParser) -> None:
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
        default="MT4 Out 1",
        help="MIDI output port name (default: MT4 Out 1)",
    )
    parser.add_argument(
        "--in-port",
        default="MT4 In 1",
        help="MIDI input port name (default: MT4 In 1)",
    )


def _add_payload_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--count",
        type=int,
        default=20,
        help="Repetitions per payload size (default: 20)",
    )
    parser.add_argument(
        "--sizes",
        default="1024,4096",
        help="Comma-separated synthetic SysEx sizes in bytes (default: 1024,4096)",
    )
    parser.add_argument(
        "--fixture",
        default=str(default_fixture()),
        help="Path to long SysEx fixture (default: long-loopback-14708.syx)",
    )
    parser.add_argument(
        "--skip-fixture",
        action="store_true",
        help="Skip the on-disk fixture; run synthetic sizes only",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=MIN_INTERVAL_S,
        help="Seconds between trial starts (default: 0.05)",
    )
    parser.add_argument(
        "--reply-timeout",
        type=float,
        default=8.0,
        help="Seconds to wait for exact loopback reply (default: 8.0)",
    )
    parser.add_argument(
        "--abort-after-timeouts",
        type=int,
        default=DEFAULT_ABORT_AFTER_TIMEOUTS,
        help=(
            "Abort the current session after N consecutive TIMEOUT/FAIL trials "
            f"per payload (default: {DEFAULT_ABORT_AFTER_TIMEOUTS}; 0 disables)"
        ),
    )
    parser.add_argument(
        "--max-wall-seconds",
        type=float,
        default=0.0,
        help=(
            "Hard wall-clock budget for the whole lab (default: 0 = auto from "
            "count/sizes/timeouts/starts). Child processes inherit the remaining budget."
        ),
    )
    parser.add_argument(
        "--pass-percent",
        type=float,
        default=100.0,
        help="Pass threshold percentage (default: 100)",
    )


def _add_log_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--log-dir",
        default="",
        help=(
            "Lab log directory (default: tests/lab-logs/sysex-long-loopback). "
            "Use tests/lab-logs/sysex-long-loopback-macos on Apple-driver labs."
        ),
    )
    parser.add_argument(
        "--log",
        default="",
        help="Lab log path (default: <log-dir>/sysex-long-loopback-<utc>.log)",
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


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Automate MT4 long SysEx DIN loopback lab "
            "(exact byte match; optional Bridge)."
        )
    )
    parser.add_argument(
        "--list-ports",
        action="store_true",
        help="List MIDI input/output names and exit",
    )
    _add_bridge_args(parser)
    _add_session_args(parser)
    _add_payload_args(parser)
    _add_log_args(parser)
    return parser


def validate_cli_args(args: argparse.Namespace) -> None:
    if args.count < 1:
        raise SystemExit("--count must be >= 1")
    if args.interval < MIN_INTERVAL_S:
        raise SystemExit(f"--interval must be >= {MIN_INTERVAL_S}")
    if args.reply_timeout <= 0:
        raise SystemExit("--reply-timeout must be > 0")
    if args.abort_after_timeouts < 0:
        raise SystemExit("--abort-after-timeouts must be >= 0")
    if args.max_wall_seconds < 0:
        raise SystemExit("--max-wall-seconds must be >= 0 (0 = auto)")
    if args.bridge_ready_timeout <= 0:
        raise SystemExit("--bridge-ready-timeout must be > 0")
    if args.fresh_starts < 1:
        raise SystemExit("--fresh-starts must be >= 1")
    if args.fresh_sessions < 1:
        raise SystemExit("--fresh-sessions must be >= 1")
    if args.session_gap < 0:
        raise SystemExit("--session-gap must be >= 0")
