#!/usr/bin/env python3
"""Offline contract checks for Story 4.1 Public Installer (no hardware / no ISCC)."""

from __future__ import annotations

import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
GUID = "{aa209017-cf8a-49ad-a0e7-701187ff7e05}"
HWID = r"USB\VID_086A&PID_0003&MI_02"
PUBLISHER_URL = "https://github.com/tensquaresoftware/unitor-win64-driver"


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    raise SystemExit(1)


def must_contain(path: Path, needle: str, label: str | None = None) -> None:
    text = path.read_text(encoding="utf-8")
    if needle not in text:
        fail(f"{path.relative_to(REPO)} missing {label or needle!r}")


def main() -> None:
    iss = REPO / "installer" / "public-installer.iss"
    inf = REPO / "installer" / "mt4-winusb.inf"
    transport = REPO / "src" / "Usb" / "WinUsbTransport.h"
    smoke = REPO / "docs" / "tests" / "smoke-epic4-public-installer-mt4.md"
    check_vm = REPO / "installer" / "check-virtualmidi.ps1"
    register_ps1 = REPO / "installer" / "register-autostart-user.ps1"
    unregister_ps1 = REPO / "installer" / "unregister-autostart-user.ps1"
    build_ps1 = REPO / "scripts" / "packaging" / "build-public-installer.ps1"

    for path in (
        iss,
        inf,
        transport,
        smoke,
        check_vm,
        register_ps1,
        unregister_ps1,
        build_ps1,
    ):
        if not path.is_file():
            fail(f"missing required file: {path.relative_to(REPO)}")

    must_contain(inf, GUID, "DeviceInterfaceGUID")
    must_contain(inf, HWID, "primary HWID")
    must_contain(transport, GUID, "transport GUID SSOT")

    iss_text = iss.read_text(encoding="utf-8")
    for needle, label in (
        ("Ten Square Software", "publisher facade"),
        ("teVirtualMIDI.dll", "VirtualMIDI gate"),
        ("--register-auto-start", "Auto-Start register"),
        ("--unregister-auto-start", "Auto-Start unregister"),
        ("pnputil", "WinUSB association"),
        ("/add-driver", "pnputil add-driver"),
        ("ExecAsOriginalUser", "unelevated Auto-Start"),
        ("runascurrentuser", "uninstall as user"),
        ("Is64BitInstallMode", "SysNative pnputil guard"),
        ("mt4-winusb.inf", "INF payload"),
        ("PrivilegesRequired=admin", "one-time elevation"),
        ("Installation incomplete", "fail-closed success screen"),
        ("loopMIDI or rtpMIDI", "eval fix path"),
        ("OQ-1", "embed release gate honesty"),
        ("unitor-win64-driver", "preserve LocalAppData identity note"),
        ("VirtualMidiPresent and GWinUsbOk and GAutoStartOk", "finished three-gate rule"),
        ("ERROR_SUCCESS_REBOOT_REQUIRED", "pnputil reboot honesty"),
        ("Abort", "gate-failure rollback"),
        ("BridgeRunningWarning", "upgrade MIDI interrupt warning"),
        ("CloseApplicationsFilter=Bridge.exe", "close Bridge only"),
        (PUBLISHER_URL, "real publisher URL"),
        ("builds\\release\\Release", "Release default BridgeSource"),
        ("unregister-autostart-user.ps1", "uninstall helper payload"),
        ("last-autostart-unregister.exit", "unregister exit marker"),
        ("{commonappdata}", "unregister marker under ProgramData"),
    ):
        if needle not in iss_text:
            fail(f"public-installer.iss missing {label} ({needle!r})")

    if 'BridgeSource "..\\builds\\debug\\Debug"' in iss_text or "builds\\debug\\Debug" in iss_text.split("ifndef BridgeSource", 1)[-1].split("#endif", 1)[0]:
        # Default BridgeSource block must not prefer Debug.
        default_block = iss_text.split("#ifndef BridgeSource", 1)[1].split("#endif", 1)[0]
        if "debug" in default_block.lower():
            fail("public-installer.iss default BridgeSource must not point at Debug")

    # Must not invent Session-0 service or Zadig-primary UX.
    lowered = iss_text.lower()
    if "session-0" in lowered or "createservice" in lowered:
        fail("installer must not install a Session-0 service")
    if re.search(r"\bzadig\b", lowered) and "not the primary" not in lowered:
        if "primary" not in lowered:
            fail("unexpected Zadig-primary wording in installer")

    build_text = build_ps1.read_text(encoding="utf-8")
    if "Refusing silent fallback" not in build_text:
        fail("build-public-installer.ps1 must refuse invalid -BridgeDir")
    # Release candidates should appear before Debug in the auto-detect list.
    rel_idx = build_text.find("builds\\release\\Release")
    dbg_idx = build_text.find("builds\\debug\\Debug")
    if rel_idx < 0 or dbg_idx < 0 or rel_idx > dbg_idx:
        fail("build-public-installer.ps1 must prefer Release layouts before Debug")
    if "/DMyAppVersion=" not in build_text:
        fail("build-public-installer.ps1 must pass /DMyAppVersion to ISCC")

    smoke_text = smoke.read_text(encoding="utf-8")
    for item in (
        "AD-12-1",
        "AD-12-2",
        "AD-12-3",
        "AD-12-4",
        "AD-12-5",
        "AD-12-6",
        "AD-12-7",
        "Inno Setup 6",
        "blank cell is **not** Pass",
        "Win10 x64",
        "OQ-1",
        "4.2",
        "4.4",
    ):
        if item not in smoke_text:
            fail(f"smoke guide missing {item!r}")

    must_contain(check_vm, "teVirtualMIDI.dll", "VirtualMIDI DLL name")
    must_contain(check_vm, "loopMIDI or rtpMIDI", "fix path")
    must_contain(check_vm, "Sysnative", "WOW64 Sysnative probe")

    for helper, label in (
        (register_ps1, "register-autostart-user.ps1"),
        (unregister_ps1, "unregister-autostart-user.ps1"),
    ):
        text = helper.read_text(encoding="utf-8")
        if "Refusing to" not in text or "elevated" not in text.lower():
            fail(f"{label} must refuse elevated execution")
        if "ExitCode" not in text or "null" not in text.lower():
            fail(f"{label} must fail closed on null ExitCode")

    must_contain(
        unregister_ps1,
        "last-autostart-unregister.exit",
        "unregister exit marker",
    )
    must_contain(unregister_ps1, "ProgramData", "ProgramData unregister marker")

    print("OK: installer contract checks passed")
    raise SystemExit(0)


if __name__ == "__main__":
    main()
