---
baseline_commit: ea3e21d
---

# Story 4.1: Public Installer meeting AD-12 UX bar

Status: in-progress

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a community Windows user,
I want a short, clear Public Installer that binds WinUSB, installs the Bridge, wires Auto-Start, and checks VirtualMIDI,
so that I can reach first MIDI the same evening without a developer toolchain feel.

## Acceptance Criteria

1. **Given** a clean Win10 x64 or Win11 x64 machine and MT4 available  
   **When** the user runs the Public Installer path  
   **Then** the AD-12 UX checklist holds: (1) few steps, (2) visible progress, (3) clear success screen, (4) VirtualMIDI prerequisite explicit, (5) WinUSB association succeeds, (6) Auto-Start wired, (7) one-time admin OK / daily admin not required, (8) minimal jargon — FR-12 / CAP-12

2. **And** if VirtualMIDI is missing, install blocks with an obvious fix path (eval: loopMIDI/rtpMIDI; licensed MSI when cleared) — not an empty port list as success

3. **And** installer technology (WiX / Inno / other) is an implementation choice under this checklist only

4. **And** OQ-1 (Tobias MSI redistribution clearance) is a **release gate for redistributable Public Installer embedding VirtualMIDI only** — not a blocker to implement the installer path / eval prerequisite messaging

**Traces:** FR-12, CAP-12, AD-12, SM-5 (install portion); release gate OQ-1

## Tasks / Subtasks

- [x] Task 1: Lock Public Installer contract + fences (AC: 1–4)
  - [x] Author operator smoke guide `docs/tests/smoke-epic4-public-installer-mt4.md` (kebab-case) with English Pass/Fail matrix mapped 1:1 to AD-12 items (1)–(8) plus VirtualMIDI-missing block
  - [x] State product intent in that guide: community path feels closer to a polished macOS installer than a developer toolchain; Zadig is **not** the primary user path
  - [x] Choose installer technology under AD-12 only (WiX / Inno / other) and document the choice + rationale in the smoke guide / Dev Notes — **do not** reopen as a product decision
  - [x] Explicit fences:
    - polished end-user docs (`docs/user/` first MIDI / SysEx / troubleshooting) → **4.2**
    - three-way MIT vs VirtualMIDI vs Windows MIDI Services honesty polish → **4.3**
    - Authenticode / public catalog signing / SmartScreen honesty → **4.4**
    - Tobias MSI **embed** redistributable → **OQ-1 release gate only** (eval prerequisite messaging ships now)
    - WinUSB bind materials / GUID open policy → already **1.3** (reuse; do not rewrite transport)
    - Auto-Start runtime CLI → already **3.1** (wire; do not invent a second mechanism or Session-0 service)
    - MIDI Path harness → Epic **5**
  - [x] Honesty bar: blank lab rows ≠ Pass; Win10 x64 mandatory in matrix; Win11 when available; do not claim full SM-5 closed without **4.2** docs; do not claim SM-6 Authenticode closed (**4.4**)
  - [x] Cite SSOT: epics Story 4.1; PRD FR-12 / SM-5 (install); AD-12 / AD-20 / AD-19 (facade branding only); SPEC CAP-12
  - [x] Cross-link from `docs/dev/winusb-bind.md` and `docs/tests/smoke-epic3-autostart-mt4.md` (replace “→ 4.1” fences with the new smoke guide)

- [x] Task 2: Packaging layout + Bridge payload (AC: 1 items 1–3, 5–7)
  - [x] Create installer sources under `installer/` (kebab-case scripts / project files; keep existing `mt4-winusb.inf` as the WinUSB association core)
  - [x] Ship `Bridge.exe` (+ embedded `Bridge.manifest` / `asInvoker`) from a `builds/` artifact to a **stable absolute** install path (recommended: `Program Files\Ten Square Software\Unitor MT4 Bridge\` or equivalent Ten Square Software facade)
  - [x] Include `installer/mt4-winusb.inf` (and any required companion files staged beside it) in the payload — DeviceInterfaceGUID must remain `{aa209017-cf8a-49ad-a0e7-701187ff7e05}` byte-for-byte with `src/Usb/WinUsbTransport.h`
  - [x] Visible progress UI + clear success screen (AD-12 items 2–3); keep steps few and jargon minimal (items 1, 8)
  - [x] One-time Administrator elevation OK for WinUSB association / Program Files write; **daily** Bridge launch and Auto-Start must not require admin (item 7)
  - [x] Optional CMake/CPack or CI helper to assemble the installer from `builds/` — keep artifacts under `builds/`; do not invent a second out-of-tree build root
  - [x] Public facade strings: **Ten Square Software** (AD-19); device string already in INF (`Emagic MT4 (WinUSB)`)
  - [x] Do **not** vendor Tobias SDK binaries, proprietary `.lib`, or GPL trees

- [x] Task 3: VirtualMIDI prerequisite gate (AC: 1 item 4, AC: 2, AC: 4)
  - [x] Before claiming success, installer checks VirtualMIDI driver/DLL presence (align with Bridge: `teVirtualMIDI.dll` via system directories / same fail-closed intent as `VirtualMidiBackend`)
  - [x] If missing: **block** with an obvious English fix path — install loopMIDI or rtpMIDI so the VirtualMIDI driver is present, then retry (mirror `kVirtualMidiMissingDriverFixPath` intent; empty port list ≠ success)
  - [x] Document licensed MSI embed path as **future / gated by OQ-1** — do not block this story on Tobias clearance; do not silently embed/redistribute VirtualMIDI MSI without clearance
  - [x] Prerequisite messaging must be explicit in the UI (AD-12 item 4), not buried only in a log file

- [x] Task 4: WinUSB association in the community path (AC: 1 item 5)
  - [x] Elevated install step associates MT4 `USB\VID_086A&PID_0003` (interface as required, primary HWID `…&MI_02`) with Microsoft WinUSB using the project INF — prefer `pnputil /add-driver … /install` (modern path); **avoid** deprecated DIFx as the primary strategy
  - [x] On 64-bit Windows, call the native 64-bit `pnputil` (beware 32-bit installer → use SysNative when applicable)
  - [x] Reuse / wrap existing `installer/bind-mt4-winusb.ps1` patterns where helpful; do not make Zadig the primary community path (contributor fallback stays in `docs/dev/winusb-bind.md`)
  - [x] Fail closed with English diagnostics if association fails — do not show the success screen
  - [x] Unsigned INF / missing `.cat` may fail on clean machines: document lab mitigation (`sign-lab-package.ps1`) and fence public Authenticode/catalog policy to **4.4** — still implement the association path now
  - [x] Do **not** ship WDF/WinUSB co-installer DLLs; do **not** change `WinUsbTransport` open policy (GUID-first)

- [x] Task 5: Wire Auto-Start without dual-launcher / elevation traps (AC: 1 items 6–7)
  - [x] After Bridge is on a stable absolute path, register Auto-Start via existing CLI: `"<install>\Bridge.exe" --register-auto-start` (action becomes `"<install>\Bridge.exe" --auto-session`)
  - [x] **Critical:** registration must run in the **interactive user** context (AD-20 / Story 3.1). Do **not** register HKCU Run / Task Scheduler while the installer is elevated as Administrator in a way that writes the **admin** profile — use a deferred per-user step, unelevated finish page action, or equivalent
  - [x] Uninstall must call `"<install>\Bridge.exe" --unregister-auto-start` (clears Task Scheduler **and** HKCU Run) before removing binaries
  - [x] Do **not** install a Session-0 Windows Service; do **not** set “run with highest privileges”; do **not** invent a second Auto-Start mechanism beside `AutoStartRegistration`
  - [x] Do **not** leave both Scheduler and Run active (Story 3.1 / post-3.4 harden: dual-launcher races are failures)
  - [x] Preserve `asInvoker` manifest on the shipped binary

- [x] Task 6: Uninstall / upgrade hygiene (AC: 1 item 7; regression)
  - [x] Uninstall: unregister Auto-Start → remove Bridge payload → remove/driver-store cleanup for the INF package as appropriate (document residual Driver Store entries honestly if OS retains them)
  - [x] Default: **preserve** `%LOCALAPPDATA%\unitor-win64-driver\` unit-identity registry across uninstall/reinstall so dual-MT4 ordinal `K` does not reshuffle casually (Story 3.4) — if offering a “remove user data” checkbox, default it off and label clearly
  - [x] Upgrade must not spawn two Bridge hosts or reintroduce dual Auto-Start backends
  - [x] ARP / Add-Remove Programs entry with Ten Square Software identity when the chosen technology supports it

- [x] Task 7: Operator smoke + quality (AC: all)
  - [x] Fill `docs/tests/smoke-epic4-public-installer-mt4.md` Pass/Fail rows (Win10 x64 mandatory):
    1. Few steps / minimal jargon walkthrough (AD-12-1, 8)
    2. Visible progress during install (AD-12-2)
    3. Clear success screen only when all gates passed (AD-12-3)
    4. VirtualMIDI missing → block + fix path; not success (AD-12-4 / AC2)
    5. After install with VirtualMIDI present: Device Manager shows WinUSB on MT4 / GUID path opens (AD-12-5)
    6. Auto-Start registered; logon or plug-after-login yields ports without manual launch (AD-12-6) — reuse Epic 3 Auto-Start expectations
    7. One UAC/admin at install; daily Bridge / register path does not prompt (AD-12-7)
    8. Uninstall unregisters Auto-Start; logon no longer starts Bridge
    9. Regression: lab `Bridge --start-session` / `--test-mapper` / `--test-port-names` still work from install or `builds/` as documented
  - [x] Honesty: blank ≠ Pass; physical MT4 required for bind rows; VirtualMIDI prerequisite rows can use deliberate uninstall/disable of the driver
  - [x] If any C++ changes (prefer minimize): `python scripts/quality/lint-touched.py` exits 0; `ctest` / `BridgeTests` Pass; no French in sources; Protocol/Profile free of VirtualMIDI/WinUSB
  - [x] Confirm no Session-0 service, no Zadig-primary UX, no Tobias MSI embed without OQ-1, no polished `docs/user/` chapter claimed done under this story ID

### Review Findings

- [x] [Review][Patch] On WinUSB/Auto-Start gate failure, abort setup so files/ARP are not left as a successful install [`installer/public-installer.iss:CurStepChanged`] — decided: Abort/rollback (not incomplete-but-installed)
- [x] [Review][Defer] Blank Win10 smoke matrix vs Task 7 fill — deferred, lab evidence still required before story `done`; not a code patch in this review
- [x] [Review][Patch] Warn before CloseApplications kills a live Bridge on upgrade/reinstall [`installer/public-installer.iss`] — decided: explicit warning, then close if user continues

- [x] [Review][Patch] Align finished-page gate with smoke three-gate rule (re-check VirtualMIDI) [`installer/public-installer.iss:AllInstallGatesPassed`]
- [x] [Review][Patch] Treat pnputil reboot-required (3010) as association success with reboot honesty [`installer/public-installer.iss:BindMt4WinUsb`]
- [x] [Review][Patch] Fail when `-BridgeDir` is set but lacks `Bridge.exe` (no silent fallback) [`scripts/packaging/build-public-installer.ps1:Resolve-BridgeDir`]
- [x] [Review][Patch] Prefer Release layouts over Debug for Public Installer packaging defaults [`scripts/packaging/build-public-installer.ps1` / `installer/public-installer.iss`]
- [x] [Review][Patch] Show wizard status/progress during WinUSB bind and Auto-Start register [`installer/public-installer.iss:CurStepChanged`]
- [x] [Review][Patch] Refuse elevated execution in operator Auto-Start helper scripts [`installer/register-autostart-user.ps1` / `installer/unregister-autostart-user.ps1`]
- [x] [Review][Patch] Verify `--unregister-auto-start` exit code on uninstall before removing binaries [`installer/public-installer.iss:UninstallRun`]
- [x] [Review][Patch] Wire `MyAppVersion` from the build script (avoid hard-coded `0.1.0` only) [`installer/public-installer.iss` / `scripts/packaging/build-public-installer.ps1`]
- [x] [Review][Patch] Strengthen offline contract checks (finished VirtualMIDI gate, non-Debug preference, helper elevation notes) [`scripts/packaging/verify-installer-contract.py`]
- [x] [Review][Patch] Replace placeholder `AppPublisherURL` with the real project URL [`installer/public-installer.iss:AppPublisherURL`]
- [x] [Review][Patch] Probe Sysnative for VirtualMIDI under WOW64 PowerShell [`installer/check-virtualmidi.ps1`]
- [x] [Review][Patch] Fail closed when helper `Start-Process` returns a null ExitCode [`installer/register-autostart-user.ps1` / `installer/unregister-autostart-user.ps1`]

- [x] [Review][Defer] Unsigned INF / missing `.cat` on clean machines [`installer/mt4-winusb.inf` / `installer/public-installer.iss`] — deferred, pre-existing fence to Story 4.4 (documented lab `sign-lab-package.ps1`)
- [x] [Review][Defer] No timeout if pnputil/Bridge hang under Inno `Exec` [`installer/public-installer.iss:BindMt4WinUsb`] — deferred, pre-existing Inno limitation

## Dev Notes

### Soft dependencies

| Dependency | Status | How this story treats it |
|---|---|---|
| Epic 1–3 Bridge (WinUSB, mapper, VirtualMIDI, Auto-Start, hot-plug, multi-client, dual-MT4) | **done** | Hard prerequisite — installer **packages and wires** existing runtime; does not reopen MIDI protocol work |
| `installer/mt4-winusb.inf` + bind scripts (Story 1.3) | **done** | Reuse as community association core |
| `Bridge.exe --register-auto-start` / `--unregister-auto-start` / `--auto-session` (Story 3.1) | **done** | Sole Auto-Start mechanism to invoke |
| VirtualMIDI fail-closed in `VirtualMidiBackend` (Story 1.5) | **done** | Behavioral model for installer prerequisite gate |
| OQ-1 Tobias MSI clearance | **open** (Guillaume) | Release gate for **embed only** — eval messaging ships |
| Story **4.2** user docs | backlog | Installer may link to a short placeholder / README install section; full `docs/user/` is 4.2 |
| Story **4.4** Authenticode | backlog | Unsigned INF/lab signing may limit clean-machine bind until 4.4 — document honesty |

### Scope fence

This story lands the **Public Installer product** that meets AD-12’s eight-item UX bar by composing existing WinUSB INF + Bridge + Auto-Start CLI + VirtualMIDI presence check. It is **not** end-user documentation polish, **not** Authenticode policy, and **not** a rewrite of the MIDI session host.

| In scope | Out of scope (later / never) |
|---|---|
| Installer UX under AD-12 (1)–(8) | Polished `docs/user/` UJ-1/UJ-2 chapters → **4.2** |
| Package Bridge + INF; `pnputil` association | Changing Emagic mapper / port naming / hot-plug policy |
| VirtualMIDI **presence check** + eval fix path | Embedding/redistributing Tobias MSI before OQ-1 |
| Call `--register-auto-start` / unregister on uninstall | Session-0 Windows Service; tray GUI rewrite (console host OK) |
| Technology choice WiX/Inno/other | Forcing MSIX as primary (poor fit with INF/driver association) |
| Ten Square Software branding in installer | Full SM-6 Authenticode / SmartScreen docs → **4.4** |
| Operator smoke under `docs/tests/` | Claiming Studio-Done latency proof → Epic **5** |

### Architecture compliance (must follow)

- **AD-12:** custom INF → WinUSB for `086A:0003` + GUID `{aa209017-cf8a-49ad-a0e7-701187ff7e05}`; VirtualMIDI presence gate; UX checklist (1)–(8); Zadig contributor-only; tech choice free under checklist
- **AD-20:** Bridge is user-session process — Auto-Start via logon registration / USB-arrival wait already in `--auto-session`; **forbidden** Session-0 service
- **AD-10:** happy path needs no manual launch after install wires Auto-Start
- **AD-7 / fail-closed convention:** missing VirtualMIDI → user-visible fix path; empty ports ≠ success
- **AD-19:** public facade **Ten Square Software** (branding); full user-doc checklist is Story 4.2
- **AD-13 / conventions:** artifacts under `builds/`; kebab-case installer scripts; C++ under `src/` PascalCase only if touched
- **AD-2:** do not pull installer/packaging dependencies into Protocol/Profile/Midi core

### Reuse — do not reinvent

| Existing artifact | Installer role |
|---|---|
| `installer/mt4-winusb.inf` | WinUSB association package (extend, do not fork GUID/HWID) |
| `installer/bind-mt4-winusb.ps1` | Reference for elevated `pnputil` bind |
| `installer/sign-lab-package.ps1` | Lab-only catalog signing until **4.4** |
| `src/App/AutoStartRegistration.*` + CLI flags | Register/unregister Auto-Start |
| `src/App/Bridge.manifest` (`asInvoker`) | Must ship with Bridge |
| `src/Midi/VirtualMidiBackend.cpp` + `kVirtualMidiMissingDriverFixPath` | Model for prerequisite detection + messaging |
| `docs/dev/winusb-bind.md` | Contributor bind / Zadig fallback (keep; link installer smoke) |
| `docs/tests/smoke-epic3-autostart-mt4.md` | Post-install Auto-Start verification pattern |

### Recommended shape (implementation guidance)

1. **Compose, don’t reimplement** — success = INF bind + Bridge on stable path + unelevated `--register-auto-start` + VirtualMIDI gate + AD-12 UX chrome.
2. **Technology recommendation (non-binding):** prefer **Inno Setup** for V1 community EXE (fast progress/success UI, easy custom `pnputil` + prerequisite checks) **or** **WiX** if an MSI + ARP story is strongly desired. Avoid centering on **MSIX** while INF/Driver Store association is required. Avoid DIFx extensions.
3. **Elevation split:** admin once for Program Files + `pnputil`; Auto-Start registration as the logged-on user afterward.
4. **Presence probe:** check `teVirtualMIDI.dll` in System32 (same constraint class as Bridge `LoadLibraryExA` + `LOAD_LIBRARY_SEARCH_SYSTEM32`) before success.
5. **Success screen rules:** show only when VirtualMIDI present **and** WinUSB association step reported success **and** Auto-Start registration reported success (Scheduler **or** documented Run-key fallback both OK per Story 3.1).
6. **Uninstall order:** `--unregister-auto-start` → remove files → optional driver package removal; preserve LocalAppData identity registry by default.

### Previous story intelligence

- **3.1:** Task Scheduler preferred, HKCU Run acceptable V1; dual-backend unregister must not lie; clear Run when Scheduler wins; absolute path + WorkingDirectory; no highest-privileges.
- **1.3:** intentional layering — bind materials ≠ Public Installer UX; GUID must match transport.
- **1.5:** MSI embed out of scope / OQ-1; eval via loopMIDI/rtpMIDI; no proprietary SDK in repo.
- **3.4 / `ea3e21d`:** installed host is multi-unit `--auto-session`; do not reintroduce Auto-Start dual-launcher or identity races; preserve unit-identity registry path.

### Git intelligence

Recent commits (`ea3e21d`, `83d9226`, `42e4314`, `6b95279`, `e0ca8e0`) harden Epic 3 runtime — they do **not** add packaging. Story 4.1 opens the greenfield `installer/` product surface on a stable Bridge spine.

### Latest tech notes (packaging)

- Prefer `pnputil /add-driver <inf> /install` over deprecated DIFx for Win10/11 association packages.
- From a 32-bit installer on x64 OS, invoke `%windir%\SysNative\pnputil.exe` when needed.
- Inno Setup remains suitable for non-commercial/community packaging; pin a specific version in docs when chosen. WiX 5 remains the MSI-centric alternative.
- MSIX is a weak primary vehicle when custom INF / Driver Store association is central.

### Project structure notes

```text
installer/           # EXTEND — INF (exists) + new Public Installer project/scripts (AD-12)
builds/              # Bridge.exe input + optional installer output artifacts
docs/tests/          # NEW smoke-epic4-public-installer-mt4.md
docs/dev/winusb-bind.md  # UPDATE cross-links only
docs/user/           # DO NOT create full user manual here → 4.2
src/App/             # Prefer NO changes; call existing CLI from installer
```

If a tiny C++ helper is unavoidable for presence/bind, keep it thin under `src/App/` or a small `installer/` helper exe — do not grow Protocol/Midi for packaging.

### Testing requirements

- Operator smoke on **clean** Win10 x64 (mandatory) with physical MT4 for bind rows.
- VirtualMIDI-missing negative test is mandatory (AC2).
- Post-install: sample Auto-Start rows from Epic 3 smoke (ports without manual launch).
- Offline: no requirement to run WiX/Inno compile on macOS edit machine — Windows validation machine is the gate (dual-machine loop).
- Quality: `lint-touched.py` if C++ touched; English-only diagnostics.

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Epic 4 / Story 4.1]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-12, AD-20, AD-19, Structural Seed `installer/`]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — FR-12, OQ-1, SM-5]
- [Source: `_bmad-output/implementation-artifacts/1-3-winusb-bind-path-and-transport-open.md`]
- [Source: `_bmad-output/implementation-artifacts/3-1-auto-start-without-daily-administrator.md`]
- [Source: `_bmad-output/implementation-artifacts/1-5-virtualmidi-backend-and-stable-mt4-port-names.md`]
- [Source: `installer/mt4-winusb.inf`]
- [Source: `src/Midi/VirtualMidiWinSupport.h` — `kVirtualMidiMissingDriverFixPath`]
- [Source: `docs/dev/winusb-bind.md`]
- [Source: `docs/tests/smoke-epic3-autostart-mt4.md`]

## Dev Agent Record

### Agent Model Used

Composer (Cursor agent router)

### Debug Log References

- Inno Setup 6.7.3 installed under `%LOCALAPPDATA%\Programs\Inno Setup 6` (per-user); build script probes that path.
- `builds/debug/Debug/Bridge.exe` is the VS multi-config layout used as BridgeSource.
- Successful compile: `builds/installer/UnitorMt4Bridge-Setup.exe`.

### Implementation Plan

- Locked AD-12 contract in operator smoke guide; chose **Inno Setup 6** under the checklist (not a product reopen).
- Composer installer: INF + Bridge payload, VirtualMIDI gate at setup start, elevated `pnputil` bind, `ExecAsOriginalUser` Auto-Start register, fail-closed finished page.
- Offline `verify-installer-contract.py` guards GUID/HWID/messaging; no C++ / Protocol / Midi changes.

### Completion Notes List

- Public Installer path implemented end-to-end under AD-12 without vendoring Tobias MSI (OQ-1 remains release gate for embed).
- Auto-Start uses existing Bridge CLI only; registration is interactive-user via `ExecAsOriginalUser`; uninstall uses `runascurrentuser` `--unregister-auto-start`.
- Success screen text is gated on VirtualMIDI **and** WinUSB **and** Auto-Start; VirtualMIDI absence also blocks before install starts with English loopMIDI/rtpMIDI fix path.
- Gate failure after file copy calls `Abort` (rollback) instead of leaving an incomplete-but-installed product.
- Upgrade warns when Bridge is running before CloseApplications can interrupt a MIDI session.
- Operator hardware Pass/Fail matrix intentionally **blank** (honesty: blank ≠ Pass) — lab Pass still needed before story `done`. Offline: contract script OK; ISCC compile OK after code-review patches.
- No Session-0 service, no Zadig-primary UX, no `docs/user/` polish claimed, no Authenticode claim (4.4).

### File List

- `docs/tests/smoke-epic4-public-installer-mt4.md` (new)
- `docs/dev/winusb-bind.md` (cross-link)
- `docs/tests/smoke-epic3-autostart-mt4.md` (cross-link)
- `installer/public-installer.iss` (new)
- `installer/check-virtualmidi.ps1` (new)
- `installer/register-autostart-user.ps1` (new)
- `installer/unregister-autostart-user.ps1` (new)
- `scripts/packaging/build-public-installer.ps1` (new)
- `scripts/packaging/verify-installer-contract.py` (new)
- `_bmad-output/implementation-artifacts/4-1-public-installer-meeting-ad-12-ux-bar.md`
- `_bmad-output/implementation-artifacts/sprint-status.yaml`

### Change Log

- 2026-08-10: Implemented Story 4.1 Public Installer (Inno Setup 6) meeting AD-12 UX bar; status → review.
- 2026-08-10: Code review patches applied (abort on gate fail, three-gate success, pnputil 3010, packaging defaults, helpers); status → in-progress pending Win10 smoke matrix.
