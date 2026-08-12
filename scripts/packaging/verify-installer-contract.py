#!/usr/bin/env python3
"""Offline contract checks for Story 4.1 Public Installer (no hardware / no ISCC)."""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
GUID = "{aa209017-cf8a-49ad-a0e7-701187ff7e05}"
HWID = r"USB\VID_086A&PID_0003&MI_02"
PUBLISHER_URL = "https://github.com/tensquaresoftware/unitor-win64-driver"

ISS_REQUIRED_NEEDLES: tuple[tuple[str, str], ...] = (
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
)

SMOKE_REQUIRED_ITEMS: tuple[str, ...] = (
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
)


@dataclass(frozen=True)
class CorePaths:
    iss: Path
    inf: Path
    transport: Path
    smoke: Path
    check_vm: Path
    register_ps1: Path
    unregister_ps1: Path
    build_ps1: Path


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    raise SystemExit(1)


def must_contain(path: Path, needle: str, label: str | None = None) -> None:
    text = path.read_text(encoding="utf-8")
    if needle not in text:
        fail(f"{path.relative_to(REPO)} missing {label or needle!r}")


def require_files(paths: tuple[Path, ...]) -> None:
    for path in paths:
        if not path.is_file():
            fail(f"missing required file: {path.relative_to(REPO)}")


def resolve_core_paths() -> CorePaths:
    paths = CorePaths(
        iss=REPO / "installer" / "public-installer.iss",
        inf=REPO / "installer" / "mt4-winusb.inf",
        transport=REPO / "src" / "Usb" / "WinUsbTransport.h",
        smoke=REPO / "docs" / "tests" / "smoke-epic4-public-installer-mt4.md",
        check_vm=REPO / "installer" / "check-virtualmidi.ps1",
        register_ps1=REPO / "installer" / "register-autostart-user.ps1",
        unregister_ps1=REPO / "installer" / "unregister-autostart-user.ps1",
        build_ps1=REPO / "scripts" / "packaging" / "build-public-installer.ps1",
    )
    require_files(
        (
            paths.iss,
            paths.inf,
            paths.transport,
            paths.smoke,
            paths.check_vm,
            paths.register_ps1,
            paths.unregister_ps1,
            paths.build_ps1,
        )
    )
    return paths


def check_guid_hwid_ssot(paths: CorePaths) -> None:
    must_contain(paths.inf, GUID, "DeviceInterfaceGUID")
    must_contain(paths.inf, HWID, "primary HWID")
    must_contain(paths.transport, GUID, "transport GUID SSOT")


def _assert_iss_needles(iss_text: str) -> None:
    for needle, label in ISS_REQUIRED_NEEDLES:
        if needle not in iss_text:
            fail(f"public-installer.iss missing {label} ({needle!r})")


def _assert_iss_bridge_source_default(iss_text: str) -> None:
    if 'BridgeSource "..\\builds\\debug\\Debug"' in iss_text or "builds\\debug\\Debug" in iss_text.split(
        "ifndef BridgeSource", 1
    )[-1].split("#endif", 1)[0]:
        # Default BridgeSource block must not prefer Debug.
        default_block = iss_text.split("#ifndef BridgeSource", 1)[1].split("#endif", 1)[0]
        if "debug" in default_block.lower():
            fail("public-installer.iss default BridgeSource must not point at Debug")


def _assert_iss_no_session0_or_zadig_primary(iss_text: str) -> None:
    # Must not invent Session-0 service or Zadig-primary UX.
    lowered = iss_text.lower()
    if "session-0" in lowered or "createservice" in lowered:
        fail("installer must not install a Session-0 service")
    if re.search(r"\bzadig\b", lowered) and "not the primary" not in lowered:
        if "primary" not in lowered:
            fail("unexpected Zadig-primary wording in installer")


def _assert_iss_abort_and_uninstall_copy(iss_text: str) -> None:
    if "AbortFailedGates" not in iss_text:
        fail("public-installer.iss must centralize gate-failure Abort cleanup (AbortFailedGates)")
    if "UnregisterAutoStartBestEffort" not in iss_text:
        fail("public-installer.iss must best-effort unregister Auto-Start before Abort")
    if "GDriverStoreMayRemain" not in iss_text:
        fail("public-installer.iss must be honest about Driver Store residue after Abort")
    if "Nothing was left installed" in iss_text:
        fail("public-installer.iss must not claim absolute 'Nothing was left installed'")
    if "Other Windows user accounts" not in iss_text and "Other Windows accounts" not in iss_text:
        fail("public-installer.iss uninstall copy must mention other Windows accounts")


def _assert_iss_version_info(iss_text: str) -> None:
    if "MyAppVersionInfo" not in iss_text:
        fail("public-installer.iss must define/use MyAppVersionInfo for PE File version")
    if "VersionInfoVersion={#MyAppVersionInfo}" not in iss_text:
        fail("public-installer.iss VersionInfoVersion must use MyAppVersionInfo")


def check_iss_needles_and_guards(iss: Path) -> str:
    iss_text = iss.read_text(encoding="utf-8")
    _assert_iss_needles(iss_text)
    _assert_iss_bridge_source_default(iss_text)
    _assert_iss_no_session0_or_zadig_primary(iss_text)
    return iss_text


def check_iss_abort_and_version(iss_text: str) -> None:
    _assert_iss_abort_and_uninstall_copy(iss_text)
    _assert_iss_version_info(iss_text)


def check_build_script(build_ps1: Path) -> str:
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
    if "/DMyAppVersionInfo=" not in build_text:
        fail("build-public-installer.ps1 must pass /DMyAppVersionInfo (four-part) to ISCC")
    if "bridge-version.txt" not in build_text:
        fail("build-public-installer.ps1 must resolve version from bridge-version.txt (CMake SSOT)")
    if "Get-VersionFromCMakeLists" not in build_text:
        fail("build-public-installer.ps1 must fall back to CMakeLists.txt project(VERSION)")
    if "Bridge.exe --version" not in build_text and "--version" not in build_text:
        fail("build-public-installer.ps1 must cross-check Bridge --version against AppVersion")
    if "Refusing to package a mismatched Setup" not in build_text:
        fail("build-public-installer.ps1 must refuse mismatched Bridge/Setup versions")
    # Default -AppVersion must not hard-code a second SSOT (empty = resolve from CMake).
    if re.search(r'\[string\]\$AppVersion\s*=\s*"0\.1\.0"', build_text):
        fail("build-public-installer.ps1 must not default -AppVersion to a hard-coded 0.1.0")
    return build_text


def check_cmake_version_ssot() -> None:
    cmake_text = (REPO / "CMakeLists.txt").read_text(encoding="utf-8")
    if "bridge-version.txt" not in cmake_text:
        fail("CMakeLists.txt must emit bridge-version.txt for packaging SSOT")
    if not re.search(
        r"project\s*\(\s*unitor-win64-driver\s+VERSION\s+\d+\.\d+\.\d+",
        cmake_text,
    ):
        fail("CMakeLists.txt must declare project(unitor-win64-driver VERSION x.y.z)")
    must_contain(
        REPO / "src" / "App" / "BridgeVersion.h.in",
        "kBridgeVersionString",
        "Bridge --version SSOT template",
    )


def check_smoke_guide(smoke: Path) -> None:
    smoke_text = smoke.read_text(encoding="utf-8")
    for item in SMOKE_REQUIRED_ITEMS:
        if item not in smoke_text:
            fail(f"smoke guide missing {item!r}")


def check_autostart_helpers(paths: CorePaths) -> None:
    must_contain(paths.check_vm, "teVirtualMIDI.dll", "VirtualMIDI DLL name")
    must_contain(paths.check_vm, "loopMIDI or rtpMIDI", "fix path")
    must_contain(paths.check_vm, "Sysnative", "WOW64 Sysnative probe")

    for helper, label in (
        (paths.register_ps1, "register-autostart-user.ps1"),
        (paths.unregister_ps1, "unregister-autostart-user.ps1"),
    ):
        text = helper.read_text(encoding="utf-8")
        if "Refusing to" not in text or "elevated" not in text.lower():
            fail(f"{label} must refuse elevated execution")
        if "ExitCode" not in text or "null" not in text.lower():
            fail(f"{label} must fail closed on null ExitCode")

    must_contain(
        paths.unregister_ps1,
        "last-autostart-unregister.exit",
        "unregister exit marker",
    )
    must_contain(paths.unregister_ps1, "ProgramData", "ProgramData unregister marker")


def check_authenticode_surfaces(iss_text: str, build_text: str) -> None:
    # Story 4.4 — SmartScreen / unsigned honesty on shipped surfaces (no cert required).
    user_en = REPO / "docs" / "user" / "unitor-mt4-bridge-user-guide.md"
    user_fr = REPO / "docs" / "user" / "unitor-mt4-bridge-guide-utilisateur.md"
    auth_policy = REPO / "docs" / "dev" / "authenticode-and-smartscreen.md"
    auth_smoke = REPO / "docs" / "tests" / "smoke-epic4-authenticode-smartscreen-mt4.md"
    sign_public = REPO / "scripts" / "packaging" / "sign-public-artifacts.ps1"
    require_files((user_en, user_fr, auth_policy, auth_smoke, sign_public))

    must_contain(user_en, "SmartScreen", "EN SmartScreen honesty")
    must_contain(user_en, "Run anyway", "EN SmartScreen mitigation")
    must_contain(user_en, "Digital Signatures", "EN signed-check tip")
    must_contain(user_en, "trusted catalog", "EN WinUSB catalog honesty")
    must_contain(user_en, "does **not** pause", "EN plug-before-Setup (no invented wizard ask)")
    must_contain(user_fr, "SmartScreen", "FR SmartScreen honesty")
    must_contain(user_fr, "Exécuter quand même", "FR SmartScreen mitigation")
    must_contain(user_fr, "Débloquer", "FR SmartScreen Unblock")
    must_contain(user_fr, "catalogue", "FR WinUSB catalog honesty")
    must_contain(user_fr, "ne s’arrête pas", "FR plug-before-Setup (no invented wizard ask)")
    must_contain(user_fr, "Signatures numériques", "FR signed-check tip")
    must_contain(auth_policy, "no certificate purchase", "Authenticode hobby / no cert purchase")
    must_contain(auth_policy, "Not a hard packaging gate", "Authenticode not hard gate")
    must_contain(auth_policy, "OQ-3", "OQ-3 deferred")
    must_contain(auth_smoke, "FR-15", "Authenticode smoke FR-15")
    must_contain(auth_smoke, "AD-19", "Authenticode smoke AD-19")

    sign_text = sign_public.read_text(encoding="utf-8")
    if "Distinct from installer/sign-lab-package.ps1" not in sign_text:
        fail("sign-public-artifacts.ps1 must stay distinct from lab catalog signing")
    if "UNITOR_CODE_SIGNING_CERT_SUBJECT" not in sign_text:
        fail("sign-public-artifacts.ps1 must gate on UNITOR_CODE_SIGNING_CERT_SUBJECT")
    if "sign-public-artifacts.ps1" not in build_text:
        fail("build-public-installer.ps1 must invoke optional public signing helper")
    if "authenticode-and-smartscreen.md" not in iss_text:
        fail("public-installer.iss bind-fail copy must point at authenticode-and-smartscreen.md")


def main() -> None:
    paths = resolve_core_paths()
    check_guid_hwid_ssot(paths)
    iss_text = check_iss_needles_and_guards(paths.iss)
    build_text = check_build_script(paths.build_ps1)
    check_iss_abort_and_version(iss_text)
    check_cmake_version_ssot()
    check_smoke_guide(paths.smoke)
    check_autostart_helpers(paths)
    check_authenticode_surfaces(iss_text, build_text)
    print("OK: installer contract checks passed")
    raise SystemExit(0)


if __name__ == "__main__":
    main()
