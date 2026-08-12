"""Matrix bank-burst SysEx lab helpers (assembler, burst, CLI builders)."""

from __future__ import annotations

import argparse
import sys
import time
from collections.abc import Callable
from dataclasses import dataclass, field
from pathlib import Path

_LAB_DIR = Path(__file__).resolve().parent
if str(_LAB_DIR) not in sys.path:
    sys.path.insert(0, str(_LAB_DIR))

import lab_midi_common as lab_midi  # noqa: E402

PATCH_PREFIX = bytes([0xF0, 0x10, 0x06, 0x01])
PATCH_SIZE = 275
MIN_INTERVAL_S = 0.01

# Subset of lab_midi.BRIDGE_FAIL_NEEDLES — keep local for Pass/Fail parity.
BRIDGE_FAIL_NEEDLES = (
    "MIDI I/O pump failed",
    "WriteBulk failed",
    "WriteBulk skipped",
    "Host→device WriteBulk",
    "Host->device WriteBulk",
)


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

    return mido


def bridge_fail_lines(text: str) -> list[str]:
    hits: list[str] = []
    for line in text.splitlines():
        for needle in BRIDGE_FAIL_NEEDLES:
            if needle in line:
                hits.append(line.rstrip())
                break
    return hits


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


def iter_completed_sysex(inport, assembler: SysexAssembler) -> list[bytes]:
    frames: list[bytes] = []
    for message in inport.iter_pending():
        if message.type != "sysex":
            continue
        frames.extend(assembler.push_mido_sysex_data(message.data))
    return frames


def frame_head_tail(data: bytes) -> str:
    if len(data) <= 8:
        return lab_midi.hex_bytes(data)
    return (
        f"head={lab_midi.hex_bytes(data[:4])} tail={lab_midi.hex_bytes(data[-4:])} "
        f"len={len(data)}"
    )


def is_patch_dump(data: bytes) -> bool:
    return (
        len(data) == PATCH_SIZE
        and data.startswith(PATCH_PREFIX)
        and data[-1] == 0xF7
    )


def wait_matched_sysex(
    inport,
    timeout_s: float,
    predicate: Callable[[bytes], bool],
) -> tuple[bytes | None, float, str]:
    assembler = SysexAssembler()
    started = time.monotonic()
    deadline = started + timeout_s
    last_note = "none"
    while time.monotonic() < deadline:
        for frame in iter_completed_sysex(inport, assembler):
            if predicate(frame):
                return frame, (time.monotonic() - started) * 1000.0, "match"
            last_note = f"discard {frame_head_tail(frame)}"
        time.sleep(0.005)
    return None, (time.monotonic() - started) * 1000.0, last_note


def patch_dump_request(slot: int) -> bytes:
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


def send_sysex(outport, mido, payload: bytes) -> None:
    if len(payload) < 2 or payload[0] != 0xF0 or payload[-1] != 0xF7:
        raise ValueError("payload must be a complete F0…F7 SysEx frame")
    outport.send(mido.Message("sysex", data=list(payload[1:-1])))


@dataclass
class BankBurstOpts:
    mido: object
    out_name: str
    in_name: str
    args: argparse.Namespace
    lines: list[str]
    start_index: int


def run_bank_burst(opts: BankBurstOpts) -> tuple[ScenarioStats, bool]:
    args = opts.args
    lines = opts.lines
    count = args.count
    start_slot = args.start_slot
    interval_s = args.interval
    reply_timeout_s = args.reply_timeout

    lines.append(f"# start_index: {opts.start_index}")
    lines.append(f"# out_port: {opts.out_name}")
    lines.append(f"# in_port: {opts.in_name}")
    lines.append(f"# burst_count: {count}")
    lines.append(f"# start_slot: {start_slot}")
    lines.append(f"# interval_s: {interval_s}")
    lines.append(f"# reply_timeout_s: {reply_timeout_s}")
    lines.append(
        f"# first_request: {lab_midi.hex_bytes(patch_dump_request(start_slot))}"
    )
    lines.append("---")

    stats = ScenarioStats(name="bank_burst_patch")
    burst_started = time.monotonic()
    with opts.mido.open_input(opts.in_name) as inport, opts.mido.open_output(
        opts.out_name
    ) as outport:
        lab_midi.drain_input(inport)
        for index in range(1, count + 1):
            slot = (start_slot + index - 1) & 0x7F
            request = patch_dump_request(slot)
            cycle_started = time.monotonic()
            lab_midi.drain_input(inport, settle_s=0.005)
            stats.sent += 1
            send_sysex(outport, opts.mido, request)
            send_line = (
                f"{index:04d} SEND dump_patch_request slot={slot:02X} "
                f"{lab_midi.hex_bytes(request)} t_mono={cycle_started:.3f}"
            )
            lines.append(send_line)
            print(send_line)

            reply, dt_ms, note = wait_matched_sysex(
                inport, reply_timeout_s, is_patch_dump
            )
            if reply is not None:
                stats.ok += 1
                result = (
                    f"{index:04d} RECV dump_patch slot={slot:02X} "
                    f"len={len(reply)} {frame_head_tail(reply)} "
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


def _add_burst_args(parser: argparse.ArgumentParser) -> None:
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


def _add_log_args(parser: argparse.ArgumentParser) -> None:
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
    _add_bridge_args(parser)
    _add_session_args(parser)
    _add_burst_args(parser)
    _add_log_args(parser)
    return parser


def validate_cli_args(args: argparse.Namespace) -> None:
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
