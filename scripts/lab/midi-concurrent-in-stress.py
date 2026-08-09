#!/usr/bin/env python3
"""Stress Emagic IN demux with two physical IN paths active at once.

Topo (required):
  - Red DIN loopback Out2 -> In2
  - Matrix-1000 powered on Out1 <-> In1 (not Thru)

Scenario:
  - Thread/loop A: send note bursts on MT4 Out 2; expect them on In 2 only
  - Loop B: send Matrix dump_patch on MT4 Out 1; expect ~275 B replies on In 1 only
  - Fail if Out2 notes appear on In 1, or Matrix dumps appear on In 2

  python scripts/lab/midi-concurrent-in-stress.py --with-bridge
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

PATCH_DUMP_REQUEST = bytes([0xF0, 0x10, 0x06, 0x04, 0x01, 0x00, 0xF7])
PATCH_PREFIX = bytes([0xF0, 0x10, 0x06, 0x01])
PATCH_SIZE = 275


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
    return best[0] if len(best) == 1 else None


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
        cwd=str(_repo_root()),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    if completed.returncode != 0:
        raise SystemExit(
            "Failed to enumerate MIDI ports:\n"
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
    def __init__(self, exe: Path, bridge_log: Path):
        self.exe = exe
        self.bridge_log = bridge_log
        self.proc: subprocess.Popen[str] | None = None
        self._reader: threading.Thread | None = None

    def start(self) -> None:
        if not self.exe.is_file():
            raise SystemExit(f"Bridge executable not found: {self.exe}")
        self.bridge_log.parent.mkdir(parents=True, exist_ok=True)
        cmd = [str(self.exe), "--start-session", "--dev-zadig"]
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
        self._reader = threading.Thread(target=self._pump, daemon=True)
        self._reader.start()
        print(f"Started Bridge pid={self.proc.pid}")

    def _pump(self) -> None:
        assert self.proc is not None and self.proc.stdout is not None
        with self.bridge_log.open("w", encoding="utf-8") as handle:
            for line in self.proc.stdout:
                handle.write(line)
                handle.flush()
                safe = line.rstrip().encode("ascii", "replace").decode("ascii")
                print(f"[bridge] {safe}", flush=True)

    def wait_ports(self, needles: list[str], timeout_s: float) -> dict[str, str]:
        deadline = time.monotonic() + timeout_s
        while time.monotonic() < deadline:
            if self.proc is not None and self.proc.poll() is not None:
                raise SystemExit(f"Bridge exited early: {self.proc.returncode}")
            outs, inns = _fresh_midi_port_names()
            found: dict[str, str] = {}
            ok = True
            for needle in needles:
                pool = outs if "Out" in needle else inns
                name = _find_port(pool, needle)
                if not name:
                    ok = False
                    break
                found[needle] = name
            if ok:
                time.sleep(0.4)
                return found
            time.sleep(0.25)
        raise SystemExit(f"Timed out waiting for ports {needles}")

    def stop(self) -> None:
        if self.proc is None or self.proc.poll() is not None:
            return
        try:
            if sys.platform == "win32":
                self.proc.send_signal(signal.CTRL_BREAK_EVENT)  # type: ignore[attr-defined]
            else:
                self.proc.send_signal(signal.SIGINT)
        except OSError:
            self.proc.terminate()
        try:
            self.proc.wait(timeout=8)
        except subprocess.TimeoutExpired:
            self.proc.kill()
        if self._reader is not None:
            self._reader.join(timeout=2)


class Counters:
    def __init__(self) -> None:
        self.lock = threading.Lock()
        self.in1_notes = 0
        self.in2_notes = 0
        self.in1_patch = 0
        self.in2_patch = 0
        self.in1_other = 0
        self.in2_other = 0
        self.cross_note_on_in1 = 0
        self.cross_patch_on_in2 = 0

    def note(self, which: str, marker_note: int) -> None:
        with self.lock:
            if which == "in1":
                self.in1_notes += 1
                if marker_note == 72:  # Out2 marker
                    self.cross_note_on_in1 += 1
            else:
                self.in2_notes += 1

    def patch(self, which: str) -> None:
        with self.lock:
            if which == "in1":
                self.in1_patch += 1
            else:
                self.in2_patch += 1
                self.cross_patch_on_in2 += 1

    def other(self, which: str) -> None:
        with self.lock:
            if which == "in1":
                self.in1_other += 1
            else:
                self.in2_other += 1


def _run_stress(
    mido,
    ports: dict[str, str],
    rounds: int,
    lines: list[str],
) -> bool:
    counters = Counters()
    # Marker notes: Out1 uses note 60, Out2 uses note 72 — detect cross-talk.
    stop = threading.Event()

    def listen(in_name: str, which: str) -> None:
        with mido.open_input(in_name) as inport:
            hold: bytearray = bytearray()
            while not stop.is_set():
                for msg in inport.iter_pending():
                    if msg.type == "note_on" and msg.velocity > 0:
                        counters.note(which, msg.note)
                    elif msg.type == "sysex":
                        raw = bytes([0xF0]) + bytes(msg.data) + bytes([0xF7])
                        hold.extend(raw)
                        # Score complete Matrix patch dumps.
                        while True:
                            if 0xF0 not in hold:
                                hold.clear()
                                break
                            start = hold.index(0xF0)
                            if start:
                                del hold[:start]
                            if 0xF7 not in hold:
                                break
                            end = hold.index(0xF7)
                            frame = bytes(hold[: end + 1])
                            del hold[: end + 1]
                            if (
                                len(frame) == PATCH_SIZE
                                and frame.startswith(PATCH_PREFIX)
                            ):
                                counters.patch(which)
                            else:
                                counters.other(which)
                    else:
                        counters.other(which)
                time.sleep(0.002)

    t1 = threading.Thread(
        target=listen, args=(ports["MT4 In 1"], "in1"), daemon=True
    )
    t2 = threading.Thread(
        target=listen, args=(ports["MT4 In 2"], "in2"), daemon=True
    )
    t1.start()
    t2.start()
    time.sleep(0.3)

    dumps_ok = 0
    notes2_ok = 0
    with mido.open_output(ports["MT4 Out 1"]) as out1, mido.open_output(
        ports["MT4 Out 2"]
    ) as out2:
        for round_i in range(1, rounds + 1):
            # Interleave: Out2 note burst (loopback -> In2) then Out1 dump (Matrix -> In1).
            for _ in range(8):
                out2.send(
                    mido.Message("note_on", note=72, velocity=64, channel=0)
                )
                time.sleep(0.01)
                out2.send(
                    mido.Message("note_off", note=72, velocity=0, channel=0)
                )
                time.sleep(0.01)
            out1.send(mido.Message("sysex", data=list(PATCH_DUMP_REQUEST[1:-1])))
            time.sleep(0.35)
            # Also send a note on Out1 (Matrix may ignore; must NOT appear on In2 as marker 72).
            out1.send(mido.Message("note_on", note=60, velocity=40, channel=0))
            time.sleep(0.05)
            lines.append(f"ROUND {round_i}/{rounds} interleaved send done")

    time.sleep(1.0)
    stop.set()
    t1.join(timeout=2)
    t2.join(timeout=2)

    with counters.lock:
        notes2_ok = counters.in2_notes
        dumps_ok = counters.in1_patch
        cross_n = counters.cross_note_on_in1
        cross_p = counters.cross_patch_on_in2
        lines.append(
            "counts "
            f"in1_notes={counters.in1_notes} in2_notes={counters.in2_notes} "
            f"in1_patch={counters.in1_patch} in2_patch={counters.in2_patch} "
            f"in1_other={counters.in1_other} in2_other={counters.in2_other} "
            f"cross_note72_on_in1={cross_n} cross_patch_on_in2={cross_p}"
        )

    # Expect: many Out2 notes on In2; at least some Matrix patches on In1; zero cross-talk.
    note_pass = notes2_ok >= rounds * 4
    dump_pass = dumps_ok >= max(1, rounds // 2)
    cross_pass = cross_n == 0 and cross_p == 0
    passed = note_pass and dump_pass and cross_pass
    lines.append(
        f"gates note_pass={note_pass} dump_pass={dump_pass} "
        f"cross_pass={cross_pass} overall_pass={passed}"
    )
    return passed


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="Concurrent dual-IN demux stress")
    p.add_argument("--with-bridge", action="store_true")
    p.add_argument("--bridge-exe", default="")
    p.add_argument("--rounds", type=int, default=10)
    p.add_argument("--log-dir", default="tests/lab-logs/midi-concurrent-in")
    return p


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    mido = _require_mido()
    log_dir = Path(args.log_dir)
    if not log_dir.is_absolute():
        log_dir = _repo_root() / log_dir
    log_dir.mkdir(parents=True, exist_ok=True)
    stamp = _lab_stamp()
    lines = [
        f"# midi-concurrent-in-stress utc={stamp}",
        "# topo: Out2->In2 loopback + Matrix on Out1<->In1 powered ON",
        f"# rounds={args.rounds}",
    ]
    bridge: BridgeSession | None = None
    try:
        needles = ["MT4 Out 1", "MT4 Out 2", "MT4 In 1", "MT4 In 2"]
        if args.with_bridge:
            exe = Path(args.bridge_exe) if args.bridge_exe else _default_bridge_exe()
            if not exe.is_absolute():
                exe = _repo_root() / exe
            bridge = BridgeSession(exe, log_dir / f"bridge-{stamp}.log")
            bridge.start()
            ports = bridge.wait_ports(needles, 45.0)
        else:
            outs, inns = _fresh_midi_port_names()
            ports = {}
            for needle in needles:
                pool = outs if "Out" in needle else inns
                name = _find_port(pool, needle)
                if not name:
                    raise SystemExit(f"Missing port {needle}")
                ports[needle] = name

        lines.append("ports " + " ".join(f"{k}={v!r}" for k, v in ports.items()))
        for line in lines:
            print(line.encode("ascii", "replace").decode("ascii"))

        passed = _run_stress(mido, ports, args.rounds, lines)
        path = log_dir / f"concurrent-in-{stamp}.log"
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")
        print(f"Wrote {path}")
        for line in lines:
            print(line.encode("ascii", "replace").decode("ascii"))
        return 0 if passed else 2
    finally:
        if bridge is not None:
            bridge.stop()


if __name__ == "__main__":
    sys.exit(main())
