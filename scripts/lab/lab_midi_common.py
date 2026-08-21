"""Shared Bridge + MIDI port helpers for lab scripts (Boy Scout extract)."""

from __future__ import annotations

import signal
import subprocess
import sys
import threading
import time
from dataclasses import dataclass, field
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
    "deliver queue full",
    "IN demux failed during host",
    "Device→host DecodeFromDevice failed",
    "Device→host SendToHost",
)

# Fresh-process mido enum timeout (post-stop WMS hangs often exceed this).
MIDI_ENUM_TIMEOUT_S = 20.0

WMS_BRIDGE_EXTRA_ARGS = ("--dev-zadig", "--midi-backend=wms")

MIDISRV_SUSPECT_MESSAGE = (
    "midisrv suspect: MIDI port enumeration timed out. "
    "Windows MIDI Services / midisrv may be unhealthy after a Bridge WMS stop. "
    "Do not spin aggressive Bridge relaunches."
)

MIDISRV_RESET_PROCEDURE = """\
Documented midisrv reset (lab, admin shell if needed) — one shot only:
  1. Close DAW / MIDI-OX / Matrix-Control on MT4 ports.
  2. Ensure no Bridge.exe is running.
  3. In an elevated PowerShell:
       Restart-Service midisrv
     If that fails:
       Stop-Process -Name MidiSrv -Force -ErrorAction SilentlyContinue
       Start-Service midisrv
  4. Confirm: Get-Service midisrv  → Status Running
  5. Start exactly ONE clean Bridge session
       (--start-session --dev-zadig --midi-backend=wms), then re-enum ports.
  6. If enum still hangs after that single reset + one Bridge start, stop and
     report (do not loop Bridge restarts).
"""


@dataclass
class MidiEnumResult:
    """Outcome of a bounded fresh-process MIDI port enumeration."""

    ok: bool
    outputs: list[str] = field(default_factory=list)
    inputs: list[str] = field(default_factory=list)
    error: str = ""
    timed_out: bool = False
    elapsed_s: float = 0.0
    returncode: int | None = None


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def default_bridge_exe() -> Path:
    return repo_root() / "builds" / "debug" / "Debug" / "Bridge.exe"


def lab_stamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def hex_bytes(data: bytes, limit: int | None = None) -> str:
    if limit is not None and len(data) > limit:
        head = " ".join(f"{byte:02X}" for byte in data[:limit])
        return f"{head} … ({len(data)} B)"
    return " ".join(f"{byte:02X}" for byte in data)


def midisrv_status() -> tuple[bool, str]:
    """Lightweight midisrv liveness (process and/or service Running).

    Returns (ok, detail). Non-Windows always reports ok/skipped.
    """
    if sys.platform != "win32":
        return True, "skipped (non-Windows)"

    script = (
        "$proc = Get-Process midisrv -ErrorAction SilentlyContinue; "
        "if ($proc) { Write-Output ('process pid=' + $proc.Id); exit 0 }; "
        "$svc = Get-Service -ErrorAction SilentlyContinue | "
        "Where-Object { $_.Name -match 'midi' -or $_.DisplayName -match 'MIDI' }; "
        "$running = @($svc | Where-Object { $_.Status -eq 'Running' }); "
        "if ($running.Count -gt 0) { "
        "  Write-Output ('service ' + (($running | ForEach-Object { $_.Name }) -join ',')); "
        "  exit 0 "
        "}; "
        "Write-Output 'midisrv process missing and no Running MIDI-related service'; "
        "exit 1"
    )
    try:
        completed = subprocess.run(
            [
                "powershell",
                "-NoProfile",
                "-NonInteractive",
                "-Command",
                script,
            ],
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=20,
            check=False,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        return False, f"midisrv probe failed: {exc}"

    detail = (completed.stdout or completed.stderr or "").strip() or "(no detail)"
    return completed.returncode == 0, detail


def midisrv_suspect_exit_message(timeout_s: float = MIDI_ENUM_TIMEOUT_S) -> str:
    return (
        f"MIDI port enumeration timed out after {timeout_s:g}s — "
        f"{MIDISRV_SUSPECT_MESSAGE}\n\n{MIDISRV_RESET_PROCEDURE}"
    )


def apply_midisrv_reset_once() -> tuple[bool, str]:
    """One-shot elevated midisrv reset (explicit lab opt-in only).

    Tries Restart-Service midisrv, then Stop-Process + Start-Service fallback.
    Never call from automatic loops. Requires admin.
    """
    if sys.platform != "win32":
        return False, "midisrv reset skipped (non-Windows)"

    script = (
        "function Assert-Running { "
        "  $s = Get-Service midisrv -ErrorAction Stop; "
        "  Write-Output ('midisrv Status=' + $s.Status); "
        "  if ($s.Status -ne 'Running') { exit 2 }; "
        "  exit 0 "
        "} "
        "try { "
        "  Restart-Service midisrv -Force -ErrorAction Stop; "
        "  Write-Output 'Restart-Service midisrv ok'; "
        "  Assert-Running "
        "} catch { "
        "  Write-Output ('Restart-Service midisrv failed: ' + $_.Exception.Message); "
        "  try { "
        "    Stop-Process -Name midisrv -Force -ErrorAction SilentlyContinue; "
        "    Stop-Process -Name MidiSrv -Force -ErrorAction SilentlyContinue; "
        "    Start-Service midisrv -ErrorAction Stop; "
        "    Write-Output 'Fallback Stop-Process + Start-Service midisrv ok'; "
        "    Assert-Running "
        "  } catch { "
        "    Write-Output ('Fallback midisrv reset failed: ' + $_.Exception.Message); "
        "    exit 1 "
        "  } "
        "}"
    )
    try:
        completed = subprocess.run(
            [
                "powershell",
                "-NoProfile",
                "-NonInteractive",
                "-Command",
                script,
            ],
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=60,
            check=False,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        return False, f"midisrv reset failed: {exc}"

    detail = (completed.stdout or completed.stderr or "").strip() or "(no detail)"
    return completed.returncode == 0, detail


def enumerate_midi_ports(
    timeout_s: float = MIDI_ENUM_TIMEOUT_S,
) -> MidiEnumResult:
    """Bounded fresh-process mido enum; never hangs silently past timeout_s."""
    code = (
        "import mido\n"
        "print('OUT')\n"
        "print('\\n'.join(mido.get_output_names()))\n"
        "print('IN')\n"
        "print('\\n'.join(mido.get_input_names()))\n"
    )
    started = time.monotonic()
    try:
        completed = subprocess.run(
            [sys.executable, "-c", code],
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout_s,
            check=False,
        )
    except subprocess.TimeoutExpired as exc:
        elapsed = time.monotonic() - started
        return MidiEnumResult(
            ok=False,
            timed_out=True,
            elapsed_s=elapsed,
            error=(
                f"TimeoutExpired after {timeout_s:g}s "
                f"(partial stdout={((exc.stdout or '')[:200])!r})"
            ),
        )
    except OSError as exc:
        elapsed = time.monotonic() - started
        return MidiEnumResult(
            ok=False,
            elapsed_s=elapsed,
            error=f"MIDI enum spawn failed: {exc}",
        )

    elapsed = time.monotonic() - started
    if completed.returncode != 0:
        return MidiEnumResult(
            ok=False,
            elapsed_s=elapsed,
            returncode=completed.returncode,
            error=(completed.stderr or completed.stdout or "(no output)"),
        )

    lines = completed.stdout.splitlines()
    try:
        out_idx = lines.index("OUT")
        in_idx = lines.index("IN")
    except ValueError:
        return MidiEnumResult(
            ok=False,
            elapsed_s=elapsed,
            returncode=completed.returncode,
            error="Unexpected MIDI port enumeration output:\n" + completed.stdout,
        )

    outputs = [line for line in lines[out_idx + 1 : in_idx] if line.strip()]
    inputs = [line for line in lines[in_idx + 1 :] if line.strip()]
    return MidiEnumResult(
        ok=True,
        outputs=outputs,
        inputs=inputs,
        elapsed_s=elapsed,
        returncode=completed.returncode,
    )


def fresh_midi_port_names(
    timeout_s: float = MIDI_ENUM_TIMEOUT_S,
) -> tuple[list[str], list[str]]:
    result = enumerate_midi_ports(timeout_s=timeout_s)
    if result.timed_out:
        raise SystemExit(midisrv_suspect_exit_message(timeout_s))
    if not result.ok:
        raise SystemExit(
            "Failed to enumerate MIDI ports in a fresh process:\n" + result.error
        )
    return result.outputs, result.inputs


def normalize_port_label(name: str) -> str:
    parts = name.rsplit(" ", 1)
    if len(parts) == 2 and parts[1].isdigit():
        return parts[0]
    return name


def port_match_rank(name: str, needle: str) -> int | None:
    if name == needle:
        return 0
    if normalize_port_label(name) == needle:
        return 1
    if not name.lower().startswith(needle.lower()):
        return None
    rest = name[len(needle) :]
    if rest == "":
        return 0
    if rest[0] == " " and rest[1:].replace(" ", "").isdigit():
        return 2
    return None


def select_port(names: list[str], needle: str, kind: str) -> str:
    ranked: list[tuple[int, str]] = []
    for name in names:
        rank = port_match_rank(name, needle)
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


def find_port(names: list[str], needle: str) -> str | None:
    try:
        return select_port(names, needle, "port")
    except SystemExit:
        return None


def list_ports(mido) -> int:
    print("MIDI outputs:")
    for name in mido.get_output_names():
        print(f"  - {name}")
    print("MIDI inputs:")
    for name in mido.get_input_names():
        print(f"  - {name}")
    return 0


def drain_input(inport, settle_s: float = 0.05) -> None:
    deadline = time.monotonic() + settle_s
    while time.monotonic() < deadline:
        for _ in inport.iter_pending():
            pass
        time.sleep(0.005)


def bridge_fail_lines(text: str) -> list[str]:
    hits: list[str] = []
    for line in text.splitlines():
        for needle in BRIDGE_FAIL_NEEDLES:
            if needle in line:
                hits.append(line.rstrip())
                break
    return hits


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
            cwd=str(repo_root()),
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
            outs, inns = fresh_midi_port_names()
            out_name = find_port(outs, out_needle)
            in_name = find_port(inns, in_needle)
            if out_name and in_name:
                _ = READY_MARKERS
                time.sleep(0.4)
                return out_name, in_name
            time.sleep(0.25)
        raise SystemExit(
            "Timed out waiting for Bridge MIDI ports "
            f"{out_needle!r} / {in_needle!r}. See {self.bridge_log}\n"
            f"If fresh mido enum also hangs, treat as midisrv suspect:\n"
            f"{MIDISRV_RESET_PROCEDURE}"
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
            print("Bridge did not exit after signal; terminating.")
            self.proc.terminate()
            try:
                self.proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self.proc.kill()
        if self._reader is not None:
            self._reader.join(timeout=2)
        print(f"Bridge stopped (code={self.proc.returncode})")


@dataclass(frozen=True)
class CleanWmsBridgeStart:
    """Options for exactly one clean Bridge WMS session (no relaunch loops)."""

    bridge_exe: Path
    bridge_log: Path
    out_needle: str = "MT4 Out 1"
    in_needle: str = "MT4 In 1"
    ready_timeout_s: float = 45.0
    extra_args: tuple[str, ...] = WMS_BRIDGE_EXTRA_ARGS


def start_one_clean_wms_bridge(options: CleanWmsBridgeStart) -> BridgeSession:
    """Start exactly one Bridge WMS session and wait until ports are ready.

    Callers must not wrap this in aggressive restart loops after enum failure —
    reset midisrv first (documented), then call this once.
    """
    session = BridgeSession(
        options.bridge_exe,
        options.bridge_log,
        list(options.extra_args),
    )
    session.start()
    try:
        session.wait_until_ready(
            options.out_needle,
            options.in_needle,
            options.ready_timeout_s,
        )
    except BaseException:
        session.stop()
        raise
    return session
