#!/usr/bin/env python3
"""Offline contract checks for dual-flavor Public Installer (Stories 4.1 / 6.2)."""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
GUID = "{aa209017-cf8a-49ad-a0e7-701187ff7e05}"
HWID = r"USB\VID_086A&PID_0003&MI_02"
PUBLISHER_URL = "https://github.com/tensquaresoftware/unitor-win64-driver"

# Shared needles present for both flavors (source always contains these).
ISS_SHARED_NEEDLES: tuple[tuple[str, str], ...] = (
    ("Ten Square Software", "publisher facade"),
    ("teVirtualMIDI.dll", "VirtualMIDI DLL name (Win10 gate / honesty)"),
    ("--register-auto-start", "Auto-Start register"),
    ("--unregister-auto-start", "Auto-Start unregister"),
    ("--midi-backend=", "flavor midi-backend bake-in"),
    ("pnputil", "WinUSB association"),
    ("/add-driver", "pnputil add-driver"),
    ("ExecAsOriginalUser", "unelevated Auto-Start"),
    ("runascurrentuser", "uninstall as user"),
    ("Is64BitInstallMode", "SysNative pnputil guard"),
    ("mt4-winusb.inf", "INF payload"),
    ("PrivilegesRequired=admin", "one-time elevation"),
    ("Installation incomplete", "fail-closed success screen"),
    ("OQ-1", "embed release gate honesty"),
    ("never embeds", "no MSI/DLL embed honesty"),
    ("unitor-win64-driver", "preserve LocalAppData identity note"),
    ("ERROR_SUCCESS_REBOOT_REQUIRED", "pnputil reboot honesty"),
    ("Abort", "gate-failure rollback"),
    ("BridgeRunningWarning", "upgrade MIDI interrupt warning"),
    ("CloseApplicationsFilter=Bridge.exe", "close Bridge only"),
    (PUBLISHER_URL, "real publisher URL"),
    ("builds\\release\\Release", "Release default BridgeSource"),
    ("unregister-autostart-user.ps1", "uninstall helper payload"),
    ("last-autostart-unregister.exit", "unregister exit marker"),
    ("{commonappdata}", "unregister marker under ProgramData"),
    ("RequireVirtualMidi", "dual-flavor virtualMIDI gate define"),
    ("FlavorToken", "dual-flavor artifact token"),
    ("win11-wms", "Win11 WMS flavor token"),
    ("win10-virtualmidi", "Win10 virtualMIDI flavor token"),
    ("Windows MIDI Services", "WMS path messaging"),
    ("VirtualMidiGateRequired", "flavor-aware gate helper"),
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

USER_GUIDES: tuple[Path, ...] = (
    REPO / "docs" / "user" / "unitor-mt4-bridge-win11-wms-user-guide.md",
    REPO / "docs" / "user" / "unitor-mt4-bridge-win11-wms-guide-utilisateur.md",
    REPO / "docs" / "user" / "unitor-mt4-bridge-win10-virtualmidi-user-guide.md",
    REPO / "docs" / "user" / "unitor-mt4-bridge-win10-virtualmidi-guide-utilisateur.md",
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
    prepare_py: Path


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
        prepare_py=REPO / "scripts" / "packaging" / "prepare-release.py",
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
            paths.prepare_py,
        )
    )
    return paths


def check_guid_hwid_ssot(paths: CorePaths) -> None:
    must_contain(paths.inf, GUID, "DeviceInterfaceGUID")
    must_contain(paths.inf, HWID, "primary HWID")
    must_contain(paths.transport, GUID, "transport GUID SSOT")


def _assert_iss_needles(iss_text: str) -> None:
    for needle, label in ISS_SHARED_NEEDLES:
        if needle not in iss_text:
            fail(f"public-installer.iss missing {label} ({needle!r})")


def _assert_all_install_gates_branches(iss_text: str) -> None:
    gate_match = re.search(
        r"if VirtualMidiGateRequired then\s+"
        r"Result := ([^\n]+)\s+"
        r"else\s+"
        r"Result := ([^\n]+)",
        iss_text,
    )
    if not gate_match:
        fail("public-installer.iss AllInstallGatesPassed dual-branch missing")
    win10_gate, win11_gate = gate_match.group(1), gate_match.group(2)
    if "VirtualMidiPresent" not in win10_gate:
        fail("Win10 AllInstallGatesPassed must include VirtualMidiPresent")
    if "VirtualMidiPresent" in win11_gate:
        fail("Win11 AllInstallGatesPassed must be WITHOUT VirtualMidiPresent")
    if "GWmsPrereqOk" not in win11_gate or "GWinUsbOk" not in win11_gate:
        fail("Win11 AllInstallGatesPassed must require WMS prereq + WinUSB + Auto-Start")
    if "GAutoStartOk" not in win11_gate:
        fail("Win11 AllInstallGatesPassed must include GAutoStartOk")


def _assert_iss_dual_flavor_gates(iss_text: str) -> None:
    needles = (
        ("VirtualMidiGateRequired and (not VirtualMidiPresent)", "conditional Win10 DLL gate"),
        ("loopMIDI or rtpMIDI", "Win10 fix path"),
        ("FlavorConsistency", "RequireVirtualMidi vs MidiBackendArg guard"),
        ("IsWindows11OrNewer", "Win11 OS gate"),
        ("WmsServicePresent", "midisrv gate"),
        ("EmptyPortsNotSuccess", "empty MIDI port honesty"),
        ("A7C3E91F-4B2D-4E8A-9F1C-6D5E8A3B2C10", "single AppId"),
        ("Unitor-MT4-Bridge-", "Luthier-style SetupBaseName prefix"),
        ("OutputDir=..\\dist", "dist/ Inno output"),
    )
    for needle, label in needles:
        if needle not in iss_text:
            fail(f"public-installer.iss missing {label} ({needle!r})")
    if iss_text.count("#define MyAppId") != 1:
        fail("public-installer.iss must keep a single MyAppId define")
    _assert_all_install_gates_branches(iss_text)


def _assert_flavor_define_map(build_text: str) -> None:
    """Exact flavor → ISCC define map (win11: 0+wms; win10: 1+virtualmidi)."""
    win11 = re.search(
        r'"win11-wms"\s*\{[^}]*RequireVirtualMidi\s*=\s*"0"[^}]*'
        r'MidiBackendArg\s*=\s*"wms"',
        build_text,
        re.DOTALL,
    )
    win10 = re.search(
        r'"win10-virtualmidi"\s*\{[^}]*RequireVirtualMidi\s*=\s*"1"[^}]*'
        r'MidiBackendArg\s*=\s*"virtualmidi"',
        build_text,
        re.DOTALL,
    )
    if not win11:
        fail("build-public-installer.ps1 win11-wms must map RequireVirtualMidi=0 + MidiBackendArg=wms")
    if not win10:
        fail(
            "build-public-installer.ps1 win10-virtualmidi must map "
            "RequireVirtualMidi=1 + MidiBackendArg=virtualmidi"
        )
    if "Removed sibling Setup" not in build_text:
        fail("build-public-installer.ps1 must remove sibling Setups after dual-build failure")


def _assert_iss_bridge_source_default(iss_text: str) -> None:
    default_block = iss_text.split("#ifndef BridgeSource", 1)[1].split("#endif", 1)[0]
    if "debug" in default_block.lower():
        fail("public-installer.iss default BridgeSource must not point at Debug")


def _assert_iss_no_session0_or_zadig_primary(iss_text: str) -> None:
    lowered = iss_text.lower()
    if "session-0" in lowered or "createservice" in lowered:
        fail("installer must not install a Session-0 service")
    if re.search(r"\bzadig\b", lowered) and "not zadig-primary" not in lowered:
        if "not the primary" not in lowered and "zadig-primary" not in lowered:
            # Header documents guided WinUSB when Setup-alone fails — OK if not primary.
            if "not zadig-primary" not in lowered and "zadig) when" not in lowered:
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
    _assert_iss_dual_flavor_gates(iss_text)
    _assert_iss_bridge_source_default(iss_text)
    _assert_iss_no_session0_or_zadig_primary(iss_text)
    return iss_text


def check_iss_abort_and_version(iss_text: str) -> None:
    _assert_iss_abort_and_uninstall_copy(iss_text)
    _assert_iss_version_info(iss_text)


def _require_build_needles(build_text: str, needles: tuple[tuple[str, str], ...]) -> None:
    for needle, label in needles:
        if needle not in build_text:
            fail(f"build-public-installer.ps1 must {label}")


def check_build_script(build_ps1: Path) -> str:
    build_text = build_ps1.read_text(encoding="utf-8")
    rel_idx = build_text.find("builds\\release\\Release")
    dbg_idx = build_text.find("builds\\debug\\Debug")
    if rel_idx < 0 or dbg_idx < 0 or rel_idx > dbg_idx:
        fail("build-public-installer.ps1 must prefer Release layouts before Debug")
    if re.search(r'\[string\]\$AppVersion\s*=\s*"0\.1\.0"', build_text):
        fail("build-public-installer.ps1 must not default -AppVersion to a hard-coded 0.1.0")
    _require_build_needles(
        build_text,
        (
            ("Refusing silent fallback", "refuse invalid -BridgeDir"),
            ("/DMyAppVersion=", "pass /DMyAppVersion to ISCC"),
            ("/DMyAppVersionInfo=", "pass /DMyAppVersionInfo (four-part) to ISCC"),
            ("/DFlavorToken=", "pass /DFlavorToken for dual artifacts"),
            ("/DRequireVirtualMidi=", "pass /DRequireVirtualMidi per flavor"),
            ("/DMidiBackendArg=", "pass /DMidiBackendArg per flavor"),
            ("win11-wms", "name win11-wms flavor"),
            ("win10-virtualmidi", "name win10-virtualmidi flavor"),
            ("bridge-version.txt", "resolve version from bridge-version.txt (CMake SSOT)"),
            ("Get-VersionFromCMakeLists", "fall back to CMakeLists.txt project(VERSION)"),
            ("--version", "cross-check Bridge --version against AppVersion"),
            ("Refusing to package a mismatched Setup", "refuse mismatched Bridge/Setup versions"),
        ),
    )
    _assert_flavor_define_map(build_text)
    return build_text


def check_prepare_release(prepare_py: Path) -> None:
    text = prepare_py.read_text(encoding="utf-8")
    for needle, label in (
        ("win11-wms", "Win11 flavor"),
        ("win10-virtualmidi", "Win10 flavor"),
        ("Unitor-MT4-Bridge-", "Luthier-style asset name prefix"),
        ("publish-ci", "CI publish command"),
        ("SHA256SUMS", "checksums"),
        ("_local", "gitignored staging"),
        ("Ask First", "publish gate comment or abort messaging"),
        ("gh_release_upload_only", "upload-only publish-ci"),
        ("Distributable missing from", "checksum completeness assert"),
        ("nothing packed", "all-or-nothing pack"),
    ):
        if needle not in text:
            fail(f"prepare-release.py missing {label} ({needle!r})")
    if "def publish_release" not in text:
        fail("prepare-release.py must keep a gated publish_release path")
    must_contain(
        REPO / "docs" / "dev" / "release-guide.md",
        "prepare-release.py verify",
        "operator dry-run verify",
    )


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
    user_router = REPO / "docs" / "user" / "README.md"
    auth_policy = REPO / "docs" / "dev" / "authenticode-and-smartscreen.md"
    auth_smoke = REPO / "docs" / "tests" / "smoke-epic4-authenticode-smartscreen-mt4.md"
    sign_public = REPO / "scripts" / "packaging" / "sign-public-artifacts.ps1"
    require_files((user_router, *USER_GUIDES, auth_policy, auth_smoke, sign_public))

    win11_en = USER_GUIDES[0]
    win11_fr = USER_GUIDES[1]
    win10_en = USER_GUIDES[2]
    win10_fr = USER_GUIDES[3]

    for guide, label in (
        (win11_en, "Win11 EN"),
        (win11_fr, "Win11 FR"),
        (win10_en, "Win10 EN"),
        (win10_fr, "Win10 FR"),
    ):
        must_contain(guide, "SmartScreen", f"{label} SmartScreen honesty")
        must_contain(guide, "trusted catalog", f"{label} WinUSB catalog honesty")
        must_contain(guide, "Zadig", f"{label} guided WinUSB")

    must_contain(win11_en, "Run anyway", "Win11 EN SmartScreen mitigation")
    must_contain(win11_en, "Digital Signatures", "Win11 EN signed-check tip")
    must_contain(win11_en, "does **not** pause", "Win11 EN plug-before-Setup")
    must_contain(win11_en, "reboot", "Win11 EN non-geek MIDI recovery")
    must_contain(win11_en, "Windows MIDI Services is missing", "Win11 EN WMS-missing recovery")
    must_contain(win11_fr, "Exécuter quand même", "Win11 FR SmartScreen mitigation")
    must_contain(win11_fr, "Débloquer", "Win11 FR SmartScreen Unblock")
    must_contain(win11_fr, "ne s’arrête pas", "Win11 FR plug-before-Setup")
    must_contain(win11_fr, "Signatures numériques", "Win11 FR signed-check tip")
    must_contain(win11_fr, "Windows MIDI Services", "Win11 FR WMS naming")
    must_contain(win10_en, "teVirtualMIDI.dll", "Win10 EN DLL self-install")
    must_contain(win10_en, "never", "Win10 EN no-embed honesty")
    must_contain(win10_en, "Run anyway", "Win10 EN SmartScreen mitigation")
    must_contain(win10_en, "does **not** pause", "Win10 EN plug-before-Setup")
    must_contain(win10_en, "Digital Signatures", "Win10 EN signed-check tip")
    must_contain(win10_fr, "teVirtualMIDI.dll", "Win10 FR DLL self-install")
    must_contain(win10_fr, "Exécuter quand même", "Win10 FR SmartScreen mitigation")
    must_contain(win10_fr, "Débloquer", "Win10 FR SmartScreen Unblock")
    must_contain(win10_fr, "ne s’arrête pas", "Win10 FR plug-before-Setup")
    must_contain(win10_fr, "Plusieurs applications", "Win10 FR multi-client subsection")
    must_contain(user_router, "Windows 11", "README OS aiguillage")
    must_contain(user_router, "Windows 10", "README OS aiguillage")
    must_contain(user_router, "same Windows product identity", "README overwrite honesty")
    must_contain(user_router, "replaces", "README overwrite replaces wording")
    must_contain(user_router, "Unplug / replug", "README unplug reading order")
    must_contain(user_router, "two MT4", "README dual-MT4 reading order")
    must_contain(auth_policy, "no certificate purchase", "Authenticode hobby / no cert purchase")
    must_contain(auth_policy, "Not a hard packaging gate", "Authenticode not hard gate")
    must_contain(auth_policy, "OQ-3", "OQ-3 deferred")
    must_contain(auth_policy, "win11-wms", "Authenticode dual Setup naming")
    must_contain(auth_smoke, "FR-15", "Authenticode smoke FR-15")
    must_contain(auth_smoke, "AD-19", "Authenticode smoke AD-19")

    sign_text = sign_public.read_text(encoding="utf-8")
    if "Distinct from installer/sign-lab-package.ps1" not in sign_text:
        fail("sign-public-artifacts.ps1 must stay distinct from lab catalog signing")
    if "UNITOR_CODE_SIGNING_CERT_SUBJECT" not in sign_text:
        fail("sign-public-artifacts.ps1 must gate on UNITOR_CODE_SIGNING_CERT_SUBJECT")
    if "win11-wms" not in sign_text and "win10-virtualmidi" not in sign_text:
        fail("sign-public-artifacts.ps1 examples must use dual flavored Setup names")
    if "sign-public-artifacts.ps1" not in build_text:
        fail("build-public-installer.ps1 must invoke optional public signing helper")
    if "authenticode-and-smartscreen.md" not in iss_text:
        fail("public-installer.iss bind-fail copy must point at authenticode-and-smartscreen.md")


def main() -> None:
    paths = resolve_core_paths()
    check_guid_hwid_ssot(paths)
    iss_text = check_iss_needles_and_guards(paths.iss)
    build_text = check_build_script(paths.build_ps1)
    check_prepare_release(paths.prepare_py)
    check_iss_abort_and_version(iss_text)
    check_cmake_version_ssot()
    check_smoke_guide(paths.smoke)
    check_autostart_helpers(paths)
    check_authenticode_surfaces(iss_text, build_text)
    print("OK: dual-flavor installer contract checks passed")
    raise SystemExit(0)


if __name__ == "__main__":
    main()
