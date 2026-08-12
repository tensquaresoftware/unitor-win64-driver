"""Matrix mid-size SysEx lab helpers (assembler, scenarios, CLI builders)."""

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

PATCH_DUMP_REQUEST = bytes([0xF0, 0x10, 0x06, 0x04, 0x01, 0x00, 0xF7])
MASTER_DUMP_REQUEST = bytes([0xF0, 0x10, 0x06, 0x04, 0x03, 0x00, 0xF7])
PATCH_PREFIX = bytes([0xF0, 0x10, 0x06, 0x01])
MASTER_PREFIX = bytes([0xF0, 0x10, 0x06, 0x03])
PATCH_SIZE = 275
MASTER_SIZE = 351

# Subset of lab_midi.BRIDGE_FAIL_NEEDLES — keep local for Pass/Fail parity.
BRIDGE_FAIL_NEEDLES = (
    "MIDI I/O pump failed",
    "WriteBulk failed",
    "WriteBulk skipped",
    "Host→device WriteBulk",
    "Host->device WriteBulk",
)


def default_patch_fixture() -> Path:
    return lab_midi.repo_root() / "tests" / "fixtures" / "sysex" / "Patch.syx"


def default_master_fixture() -> Path:
    return lab_midi.repo_root() / "tests" / "fixtures" / "sysex" / "Master.syx"


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


def load_sysex_fixture(path: Path, expected_size: int) -> bytes:
    if not path.is_file():
        raise SystemExit(f"Fixture not found: {path}")
    data = path.read_bytes()
    if len(data) != expected_size:
        raise SystemExit(
            f"Fixture {path} size {len(data)} != expected {expected_size}"
        )
    if data[0] != 0xF0 or data[-1] != 0xF7:
        raise SystemExit(f"Fixture {path} is not a SysEx frame (F0…F7)")
    return data


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

    def reset(self) -> None:
        self._buf.clear()

    def push_mido_sysex_data(self, data: bytes | list[int]) -> list[bytes]:
        """Feed one mido sysex payload (bytes between F0 and F7).

        mido normally delivers only after F7. We still route through the
        assembler so Pass never happens on an incomplete buffer.
        """
        if not self._buf:
            self._buf.append(0xF0)
        self._buf.extend(bytes(data))
        self._buf.append(0xF7)
        frame = bytes(self._buf)
        self._buf.clear()
        return [frame]

    def push_raw_bytes(self, raw: bytes) -> list[bytes]:
        """Optional raw MIDI stream path (byte-by-byte until F7)."""
        completed: list[bytes] = []
        for byte in raw:
            if not self._buf:
                if byte == 0xF0:
                    self._buf.append(byte)
                continue
            self._buf.append(byte)
            if byte == 0xF7:
                completed.append(bytes(self._buf))
                self._buf.clear()
        return completed


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


def is_master_dump(data: bytes) -> bool:
    return (
        len(data) == MASTER_SIZE
        and data.startswith(MASTER_PREFIX)
        and data[-1] == 0xF7
    )


def wait_matched_sysex(
    inport,
    timeout_s: float,
    predicate: Callable[[bytes], bool],
) -> tuple[bytes | None, float, str]:
    """Wait until a completed F0…F7 frame matches predicate."""
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


@dataclass
class ScenarioStats:
    name: str
    sent: int = 0
    ok: int = 0
    fail_lines: list[str] = field(default_factory=list)

    @property
    def rate(self) -> float:
        return (100.0 * self.ok / self.sent) if self.sent else 0.0

    def summary(self, pass_percent: float) -> str:
        passed = self.rate >= pass_percent and self.sent > 0
        return (
            f"summary[{self.name}]: sent={self.sent} ok={self.ok} "
            f"rate={self.rate:.1f}% pass={str(passed).lower()} "
            f"(need>={pass_percent:.0f}%)"
        )


def send_sysex(outport, mido, payload: bytes) -> None:
    if len(payload) < 2 or payload[0] != 0xF0 or payload[-1] != 0xF7:
        raise ValueError("payload must be a complete F0…F7 SysEx frame")
    outport.send(mido.Message("sysex", data=list(payload[1:-1])))


@dataclass
class PushScenarioOpts:
    mido: object
    outport: object
    inport: object
    name: str
    payload: bytes
    count: int
    interval_s: float
    lines: list[str]


@dataclass
class DumpScenarioOpts:
    mido: object
    outport: object
    inport: object
    name: str
    request: bytes
    predicate: Callable[[bytes], bool]
    expected_size: int
    count: int
    interval_s: float
    reply_timeout_s: float
    lines: list[str]


@dataclass
class AllScenariosOpts:
    mido: object
    out_name: str
    in_name: str
    args: argparse.Namespace
    lines: list[str]
    start_index: int


def run_push_scenario(opts: PushScenarioOpts) -> ScenarioStats:
    stats = ScenarioStats(name=opts.name)
    for index in range(1, opts.count + 1):
        cycle_started = time.monotonic()
        lab_midi.drain_input(opts.inport, settle_s=0.02)
        stats.sent += 1
        try:
            send_sysex(opts.outport, opts.mido, opts.payload)
        except Exception as exc:  # noqa: BLE001 — lab must record any send failure
            fail = (
                f"{index:04d} FAIL {opts.name} send_error={exc!r} "
                f"len={len(opts.payload)} {frame_head_tail(opts.payload)}"
            )
            stats.fail_lines.append(fail)
            opts.lines.append(fail)
            print(fail)
        else:
            stats.ok += 1
            ok = (
                f"{index:04d} SEND {opts.name} len={len(opts.payload)} "
                f"{frame_head_tail(opts.payload)} ok"
            )
            opts.lines.append(ok)
            print(ok)
        if index < opts.count:
            remaining = opts.interval_s - (time.monotonic() - cycle_started)
            if remaining > 0:
                time.sleep(remaining)
    return stats


def run_dump_scenario(opts: DumpScenarioOpts) -> ScenarioStats:
    stats = ScenarioStats(name=opts.name)
    for index in range(1, opts.count + 1):
        cycle_started = time.monotonic()
        lab_midi.drain_input(opts.inport, settle_s=0.02)
        stats.sent += 1
        send_sysex(opts.outport, opts.mido, opts.request)
        send_line = (
            f"{index:04d} SEND {opts.name}_request "
            f"{lab_midi.hex_bytes(opts.request)} "
            f"t_mono={cycle_started:.3f}"
        )
        opts.lines.append(send_line)
        print(send_line)

        reply, dt_ms, note = wait_matched_sysex(
            opts.inport, opts.reply_timeout_s, opts.predicate
        )
        if reply is not None:
            stats.ok += 1
            result = (
                f"{index:04d} RECV {opts.name} len={len(reply)} "
                f"{frame_head_tail(reply)} dt_ms={dt_ms:.1f}"
            )
            opts.lines.append(result)
            print(result)
        else:
            result = (
                f"{index:04d} TIMEOUT {opts.name} "
                f"expected_len={opts.expected_size} "
                f"waited_ms={dt_ms:.1f} last={note}"
            )
            stats.fail_lines.append(result)
            opts.lines.append(result)
            print(result)

        if index < opts.count:
            remaining = opts.interval_s - (time.monotonic() - cycle_started)
            if remaining > 0:
                time.sleep(remaining)
    return stats


def _append_scenario_header(
    opts: AllScenariosOpts, patch: bytes, master: bytes
) -> None:
    args = opts.args
    lines = opts.lines
    lines.append(f"# start_index: {opts.start_index}")
    lines.append(f"# out_port: {opts.out_name}")
    lines.append(f"# in_port: {opts.in_name}")
    lines.append(f"# patch_fixture: {args.patch_fixture} ({len(patch)} B)")
    lines.append(f"# master_fixture: {args.master_fixture} ({len(master)} B)")
    lines.append(f"# patch_dump_request: {lab_midi.hex_bytes(PATCH_DUMP_REQUEST)}")
    lines.append(f"# master_dump_request: {lab_midi.hex_bytes(MASTER_DUMP_REQUEST)}")
    lines.append("---")


@dataclass
class OpenPorts:
    outport: object
    inport: object


@dataclass
class FixturePair:
    patch: bytes
    master: bytes


def _run_dump_pair(opts: AllScenariosOpts, ports: OpenPorts) -> list[ScenarioStats]:
    args = opts.args
    return [
        run_dump_scenario(
            DumpScenarioOpts(
                mido=opts.mido,
                outport=ports.outport,
                inport=ports.inport,
                name="dump_patch",
                request=PATCH_DUMP_REQUEST,
                predicate=is_patch_dump,
                expected_size=PATCH_SIZE,
                count=args.count,
                interval_s=args.interval,
                reply_timeout_s=args.reply_timeout,
                lines=opts.lines,
            )
        ),
        run_dump_scenario(
            DumpScenarioOpts(
                mido=opts.mido,
                outport=ports.outport,
                inport=ports.inport,
                name="dump_master",
                request=MASTER_DUMP_REQUEST,
                predicate=is_master_dump,
                expected_size=MASTER_SIZE,
                count=args.count,
                interval_s=args.interval,
                reply_timeout_s=args.reply_timeout,
                lines=opts.lines,
            )
        ),
    ]


def _run_push_pair(
    opts: AllScenariosOpts,
    ports: OpenPorts,
    fixtures: FixturePair,
) -> list[ScenarioStats]:
    args = opts.args
    return [
        run_push_scenario(
            PushScenarioOpts(
                mido=opts.mido,
                outport=ports.outport,
                inport=ports.inport,
                name="push_patch",
                payload=fixtures.patch,
                count=args.count,
                interval_s=args.interval,
                lines=opts.lines,
            )
        ),
        run_push_scenario(
            PushScenarioOpts(
                mido=opts.mido,
                outport=ports.outport,
                inport=ports.inport,
                name="push_master",
                payload=fixtures.master,
                count=args.count,
                interval_s=args.interval,
                lines=opts.lines,
            )
        ),
    ]


def run_all_scenarios(opts: AllScenariosOpts) -> tuple[list[ScenarioStats], bool]:
    fixtures = FixturePair(
        patch=load_sysex_fixture(Path(opts.args.patch_fixture), PATCH_SIZE),
        master=load_sysex_fixture(Path(opts.args.master_fixture), MASTER_SIZE),
    )
    _append_scenario_header(opts, fixtures.patch, fixtures.master)

    all_stats: list[ScenarioStats] = []
    with opts.mido.open_input(opts.in_name) as inport, opts.mido.open_output(
        opts.out_name
    ) as outport:
        lab_midi.drain_input(inport)
        ports = OpenPorts(outport=outport, inport=inport)

        # Dumps before pushes on a shared Start: Matrix may stay busy after
        # librarian writes and miss the first immediate dump request.
        if opts.args.include_dump:
            all_stats.extend(_run_dump_pair(opts, ports))
        if opts.args.include_push:
            all_stats.extend(_run_push_pair(opts, ports, fixtures))

    all_ok = True
    for stats in all_stats:
        summary = stats.summary(opts.args.pass_percent)
        opts.lines.append(summary)
        print(summary)
        if stats.rate < opts.args.pass_percent or stats.sent == 0:
            all_ok = False
    return all_stats, all_ok


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
            "Session 1: pushes+dumps; later: dumps only. Default: 1"
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


def _add_scenario_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--count",
        type=int,
        default=10,
        help="Repetitions per scenario (default: 10)",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=1.0,
        help="Seconds between scenario trial starts (default: 1.0)",
    )
    parser.add_argument(
        "--reply-timeout",
        type=float,
        default=3.0,
        help="Seconds to wait for dump reply (default: 3.0)",
    )
    parser.add_argument(
        "--pass-percent",
        type=float,
        default=100.0,
        help="Pass threshold percentage (default: 100)",
    )
    parser.add_argument(
        "--patch-fixture",
        default=str(default_patch_fixture()),
        help="Path to Patch.syx (275 B)",
    )
    parser.add_argument(
        "--master-fixture",
        default=str(default_master_fixture()),
        help="Path to Master.syx (351 B)",
    )


def _add_log_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--log-dir",
        default="",
        help=(
            "Lab log directory (default: tests/lab-logs/sysex-matrix-mid). "
            "Use tests/lab-logs/sysex-matrix-mid-macos on Apple-driver labs."
        ),
    )
    parser.add_argument(
        "--log",
        default="",
        help="Lab log path (default: <log-dir>/sysex-matrix-mid-<utc>.log)",
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


def _add_mode_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--dumps-only",
        action="store_true",
        help="Run dump scenarios only (skip pushes)",
    )
    parser.add_argument(
        "--pushes-only",
        action="store_true",
        help="Run push scenarios only (skip dumps)",
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Automate MT4 mid-size Matrix SysEx lab "
            "(host MIDI; optional Bridge start/stop)."
        )
    )
    parser.add_argument(
        "--list-ports",
        action="store_true",
        help="List MIDI input/output names and exit",
    )
    _add_bridge_args(parser)
    _add_session_args(parser)
    _add_scenario_args(parser)
    _add_log_args(parser)
    _add_mode_args(parser)
    return parser


def validate_cli_args(args: argparse.Namespace) -> None:
    if args.count < 1:
        raise SystemExit("--count must be >= 1")
    if args.interval <= 0:
        raise SystemExit("--interval must be > 0")
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
    if args.dumps_only and args.pushes_only:
        raise SystemExit("Use only one of --dumps-only / --pushes-only")
    args.include_push = not args.dumps_only
    args.include_dump = not args.pushes_only
