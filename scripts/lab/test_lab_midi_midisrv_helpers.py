#!/usr/bin/env python3
"""Unit checks for midisrv-suspect MIDI enum helpers (no hardware)."""

from __future__ import annotations

import subprocess
import sys
import unittest
from pathlib import Path
from unittest import mock

_LAB_DIR = Path(__file__).resolve().parent
if str(_LAB_DIR) not in sys.path:
    sys.path.insert(0, str(_LAB_DIR))

import lab_midi_common as lab_midi  # noqa: E402


class MidisrvEnumHelpersTests(unittest.TestCase):
    def test_fresh_midi_port_names_timeout_exits_midisrv_suspect(self) -> None:
        with mock.patch(
            "lab_midi_common.subprocess.run",
            side_effect=subprocess.TimeoutExpired(cmd=["python"], timeout=1),
        ):
            with self.assertRaises(SystemExit) as raised:
                lab_midi.fresh_midi_port_names(timeout_s=1)
        message = str(raised.exception)
        self.assertIn("midisrv suspect", message)
        self.assertIn("Restart-Service midisrv", message)

    def test_enumerate_midi_ports_oserror_returns_result(self) -> None:
        with mock.patch(
            "lab_midi_common.subprocess.run",
            side_effect=OSError("spawn failed"),
        ):
            result = lab_midi.enumerate_midi_ports(timeout_s=1)
        self.assertFalse(result.ok)
        self.assertFalse(result.timed_out)
        self.assertIn("spawn failed", result.error)


if __name__ == "__main__":
    raise SystemExit(unittest.main())
