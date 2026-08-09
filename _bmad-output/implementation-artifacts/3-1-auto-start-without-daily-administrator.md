---
baseline_commit: 569a2a2
---

# Story 3.1: Auto-Start without daily Administrator

Status: done

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a studio user,
I want the Bridge to start with Windows logon and/or when the MT4 arrives on USB,
so that Virtual Ports are ready without launching the Bridge by hand every session.

## Acceptance Criteria

1. **Given** Epic 1–2 Bridge functionality installed on a user Windows session  
   **When** the machine reboots with MT4 connected, or the user plugs the MT4 after login  
   **Then** Virtual Ports become available on the happy path without a manual Bridge launch — FR-3 / CAP-3 / AD-10

2. **And** Auto-Start uses user-session registration (Task Scheduler at logon and/or Run-key equivalent and/or USB device-arrival start) — not a Session-0 Windows Service — AD-20

3. **And** daily operation does not require Administrator elevation — NFR-D2

4. **And** missing VirtualMIDI still fails closed with an obvious fix path (no silent empty success)

**Traces:** FR-3, CAP-3, AD-10, AD-20, SM-5 (Auto-Start portion)

## Tasks / Subtasks

- [x] Task 1: Lock Auto-Start host design (AC: 1, 2, 3)
  - [x] Document the V1 Auto-Start contract in English under `docs/tests/smoke-epic3-autostart-mt4.md` (kebab-case) — one operator-facing smoke guide
  - [x] State explicitly: Bridge is a **user-session** process; **forbidden** Session-0 Windows Service / SCM service project (AD-20)
  - [x] Choose and document the registration mechanism (implementation detail under AD-20). **Recommended default:** per-user Task Scheduler task at logon with `TASK_LOGON_INTERACTIVE_TOKEN` / “Run only when user is logged on”, **RunLevel Limited** (not “highest privileges”), action = absolute path to `Bridge.exe` + auto-session flag. **Allowed alternate:** `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` equivalent. USB-arrival may be combined (see Task 3)
  - [x] State privilege bar: WinUSB INF bind / lab signing remain one-time Administrator (install) — daily Auto-Start register/run/unregister must work as the interactive user without elevation (NFR-D2)
  - [x] Explicit fences: mid-session unplug/replug recovery → **3.2**; multi-client DAW+MIDI-OX policy → **3.3**; dual-MT4 ordinal naming → **3.4**; Public Installer UX packaging of Auto-Start → **4.1**; end-user docs polish → **4.2**; MIDI Path latency harness → Epic **5**
  - [x] Cite SSOT: epics Story 3.1; PRD FR-3 / NFR-D2 / SM-5 (Auto-Start portion); AD-10, AD-12 items (6)(7), AD-20; SPEC CAP-3

- [x] Task 2: Registration + unregister CLI (AC: 2, 3)
  - [x] Extend `src/App/` as the Auto-Start host (Structural Seed) — prefer new PascalCase helpers (e.g. `AutoStartRegistration.h/.cpp`) over burying COM/registry details in `Main.cpp`
  - [x] Add Bridge flags (names may vary; keep English kebab CLI style consistent with existing `--start-session`):
    - register Auto-Start for the **current** interactive user (writes Task Scheduler and/or Run-key; no admin required)
    - unregister Auto-Start (idempotent success if already absent)
  - [x] Registered action must invoke an absolute path to the **same** `Bridge.exe` being registered, plus the auto-session mode flag (do not hardcode a lab-only relative path)
  - [x] Registration must **not** set “Run with highest privileges” / requireAdministrator
  - [x] Optional but recommended: add `Bridge.manifest` with `asInvoker` and wire it in CMake so the binary never requests elevation at launch
  - [x] English stdout/stderr on success/failure (task name, path, mechanism used)
  - [x] Keep existing lab flags working unchanged: `--start-session` / `--run-midi`, `--open-device`, `--probe-usb`, `--test-mapper`, `--test-port-names`, `--dev-zadig`

- [x] Task 3: Auto-session host — logon and/or USB arrival happy path (AC: 1, 4)
  - [x] Add an auto-session entry path (flag used by registration) that starts a live `DeviceSession` via existing `runMt4MidiSession` / `DeviceSession::Start` wiring — **do not** create Virtual Ports from App code (AD-9)
  - [x] If MT4 is already present at launch (reboot-with-device): open session and expose ports without further user action
  - [x] If MT4 is absent at launch (plug-after-login): wait/rescan for the WinUSB DeviceInterfaceGUID `{aa209017-cf8a-49ad-a0e7-701187ff7e05}` then start the session — this satisfies USB-arrival for **first availability** without stealing Story **3.2** mid-session recovery
  - [x] Bound the wait (document timeout / retry cadence in the smoke guide); fail closed with English diagnostics if device never appears — do not hang forever silently
  - [x] Preserve VirtualMIDI fail-closed path already in `VirtualMidiBackend` (missing driver/DLL → obvious fix path; empty port list ≠ success)
  - [x] Do **not** implement full hot-plug teardown/recreate policy here (no claim that unplug/replug mid-session recovers without reboot — that is **3.2**)
  - [x] Note known deferred risk: `CTRL_CLOSE` may kill before `Stop` finishes → orphan ports (`deferred-work.md`). Prefer documenting Ctrl+C / clean exit for Auto-Start hosts; harden teardown only if this story’s shareable path proves it necessary — do not invent a second lifecycle owner

- [x] Task 4: Operator smoke + contributor docs (AC: 1, 2, 3, 4)
  - [x] Author `docs/tests/smoke-epic3-autostart-mt4.md` with Pass/Fail rows for Win10 x64 (mandatory) and Win11 x64 when available:
    1. Register Auto-Start as non-admin user
    2. Logoff/logon **or** reboot with MT4 already connected → ports appear without manual `Bridge` launch
    3. After login with MT4 disconnected, plug MT4 → ports appear without manual launch (wait/rescan path)
    4. Confirm process is user-session (Task Manager details / no Session-0 service)
    5. Confirm no UAC elevation prompt on daily launch
    6. With VirtualMIDI removed/disabled: fail closed + English fix path (no silent empty success)
    7. Unregister Auto-Start; confirm logon no longer starts Bridge
  - [x] Cross-link from `docs/dev/winusb-bind.md` and Epic 2 smoke ownership tables (replace bare “→ 3.1” fences with the new guide path where helpful)
  - [x] Honesty bar: blank lab rows ≠ Pass; do not claim full SM-5 (install+docs) closed — that spans Epic **4**; this story closes the **Auto-Start runtime** portion

- [x] Task 5: Regression + quality (AC: all)
  - [x] `Bridge --test-mapper` still exit 0; `Bridge --test-port-names` still exit 0
  - [x] `ctest` / `BridgeTests` Pass when any C++ changed
  - [x] Lab scripts that spawn `builds/.../Bridge.exe --start-session` must keep working (Auto-Start must not break the flag-gated lab path)
  - [x] If any C++ changes: `python scripts/quality/lint-touched.py` exits 0; compile under `builds/`; no French in sources; Protocol/Profile free of VirtualMIDI/WinUSB
  - [x] Confirm no Windows Service project, no `requireAdministrator` manifest, no GPL trees, no Public Installer MSI work under this story ID

### Review Findings

- [x] [Review][Decision] Epic 2 status/docs edits inside the 3-1 review surface — resolved: peel unrelated Epic 2 closure out of the 3-1 lot (A2)
- [x] [Review][Patch] Peel Epic 2 `done` / narrative edits from the 3-1 change set; keep only Auto-Start work + allowed cross-links [`_bmad-output/implementation-artifacts/sprint-status.yaml` / `docs/tests/smoke-epic2-mt4.md`]
- [x] [Review][Decision] Task Scheduler often ACCESS_DENIED without UserId — resolved: accept HKCU Run as normal V1 when Scheduler refuses; align smoke/docs (B2)
- [x] [Review][Patch] Document Run-key as acceptable V1 primary when Task Scheduler refuses; do not treat Scheduler-only success as the sole Pass for register [`docs/tests/smoke-epic3-autostart-mt4.md`]

- [x] [Review][Patch] Clear leftover HKCU Run when Task Scheduler register succeeds [`src/App/AutoStartRegistration.cpp`]
- [x] [Review][Patch] Unregister must not exit success if one backend fails while the other clears [`src/App/AutoStartRegistration.cpp`]
- [x] [Review][Patch] Treat SetupDi enum hard errors as failures, not “device absent” [`src/App/Mt4WinUsbPresence.cpp`]
- [x] [Review][Patch] Honor Ctrl+C after wait succeeds; do not clear cancel before nested session start [`src/App/MidiSessionCli.cpp`]
- [x] [Review][Patch] Set Task Scheduler WorkingDirectory to the Bridge.exe directory [`src/App/AutoStartTaskScheduler.cpp`]
- [x] [Review][Patch] Smoke matrix row 1 must accept Task Scheduler or documented Run fallback [`docs/tests/smoke-epic3-autostart-mt4.md`]
- [x] [Review][Patch] Include HRESULT when Task Scheduler Connect fails [`src/App/AutoStartTaskScheduler.cpp`]
- [x] [Review][Patch] Widen Run-key args with a real UTF-8 conversion (not byte-cast) [`src/App/AutoStartRegistration.cpp`]
- [x] [Review][Patch] Reject Bridge.exe paths containing `"` before writing HKCU Run [`src/App/AutoStartRunKey.cpp`]
- [x] [Review][Patch] Avoid installing the console Ctrl handler twice on `--auto-session` [`src/App/MidiSessionCli.cpp`]
- [x] [Review][Patch] Check WideCharToMultiByte write result in path conversion [`src/App/AutoStartRegistration.cpp`]
- [x] [Review][Patch] Support long Bridge.exe paths beyond MAX_PATH for registration [`src/App/AutoStartRegistration.cpp`]

- [x] [Review][Defer] Offline unit tests only assert Auto-Start constants, not register/unregister behavior [`tests/unit/AutoStartContractTests.cpp`] — deferred, pre-existing honesty/practicality bar (Windows COM lab is the real gate)
- [x] [Review][Defer] Daily Auto-Start still uses a console window; CTRL_CLOSE orphan-port risk remains [`src/App/MidiSessionCli.cpp:307`] — deferred, pre-existing (already in deferred-work; smoke warns Ctrl+C)

## Dev Notes

### Soft dependencies

| Dependency | Status | How this story treats it |
|---|---|---|
| Epic 1 notes/CC + WinUSB + Virtual Ports | **done** | Hard prerequisite — Auto-Start only hosts the existing session path |
| Epic 2 transport / SysEx / longevity design | **done** (some DAW/Matrix UAT deferred) | Soft — do not reopen clock/MTC/SysEx work; Auto-Start must not regress `--start-session` |
| WinUSB INF bind (lab) | contributor path exists | Install-time admin OK; Auto-Start registration is separate and must be non-elevated |
| VirtualMIDI driver present | env prerequisite | Fail closed if missing (already implemented — preserve) |
| Story **4.1** Public Installer | backlog | Must not block 3.1; 3.1 ships working register/unregister + auto-session; 4.1 later wires the same mechanism into AD-12 UX |
| Story **3.2** hot-plug | backlog | Do not implement mid-session unplug/replug recovery here |

### Scope fence

This story lands **FR-3 / CAP-3 Auto-Start runtime** on a user Windows session (register + auto-session happy path + no daily admin). It is **not** the Public Installer and **not** hot-plug recovery.

| In scope | Out of scope (later / never) |
|---|---|
| User-session Auto-Start registration (Task Scheduler logon and/or Run-key) | Session-0 Windows Service / SCM host (**forbidden** — AD-20) |
| Auto-session mode: start DeviceSession without manual launch | Mid-session unplug/replug recovery → **3.2** |
| Wait/rescan for first USB arrival after login | Dual-MT4 ordinal stability → **3.4** |
| Non-admin daily register / run / unregister | Multi-client exclusivity policy → **3.3** |
| Preserve VirtualMIDI fail-closed messaging | Polished AD-12 installer UX / MSI → **4.1** / OQ-1 |
| Smoke guide + contributor cross-links | Full `docs/user/` Auto-Start chapter → **4.2** |
| Optional `asInvoker` manifest | MIDI Path latency/jitter harness → Epic **5** |
| | Windows MIDI Services backend |
| | AMT8 / Unitor8 product claims |

### Epic context

Epic 3 (“Daily Studio Resilience”) makes daily studio use survivable: no hand-launch, hot-plug without reboot, multi-client, and two-unit naming. Story **3.1** is the first slice — ports ready after logon/USB arrival without daily Administrator.

### Architecture compliance (must follow)

| Decision | Rule for this story |
|---|---|
| **AD-20** | Bridge = interactive user-session process only. Auto-Start = logon registration and/or USB-arrival start. Exact Task Scheduler vs Run-key is an implementation detail under this rule. |
| **AD-10** | Happy path needs no manual launch; reboot-as-recovery is a V1 failure for Auto-Start availability (hot-plug depth remains **3.2**). |
| **AD-9** | Only a live `DeviceSession` creates/destroys Virtual Ports via `MidiBackend`. Auto-Start host starts a session — it must not become a second port authority. |
| **AD-12** | Checklist items (6) Auto-Start wired and (7) one-time admin OK / daily admin not required — **runtime proof in 3.1**; installer packaging in **4.1**. |
| **AD-7** | Missing VirtualMIDI → fail closed; empty ports ≠ success. |
| Structural Seed | Auto-Start host lives in `src/App/`; session ownership stays in `src/Device/`. |

### Current code baseline (UPDATE files)

**`src/App/Main.cpp` — today**
- Comment already states user-session host (not a Windows Service).
- Default launch runs profile smoke only and exits `0` — **no** MIDI session.
- MIDI session is flag-gated: `--start-session` / `--run-midi` → `runMt4MidiSession()`.
- No Auto-Start registration, no wait-for-device loop, no app manifest.

**What this story changes**
- Add register/unregister Auto-Start for the current user.
- Add auto-session mode used by registration (start session; wait/rescan if MT4 absent).
- Keep lab flag path intact for `scripts/lab/*` and smoke guides.

**What must be preserved**
- `DeviceSession::Start` order: WinUSB open → Emagic init → `MidiBackend::CreatePortSet` → bulk-IN pump.
- GUID-first WinUSB open (`aa209017-cf8a-49ad-a0e7-701187ff7e05`); `--dev-zadig` lab escape only.
- VirtualMIDI `LoadLibrary` fail-closed English fix path.
- Console cancel handler / Ctrl+C → `session.Stop()` behavior for lab soaks.
- Binary output under `builds/` only.

**`src/App/MidiSessionCli.cpp` — today**
- Long-running poll loop with console Ctrl handler (`CTRL_C` / `CTRL_BREAK` / `CTRL_CLOSE`).
- Reuse for auto-session once device is present; extend only as needed for pre-open wait/rescan (prefer small helper rather than duplicating Start wiring).

**Likely NEW**
- `src/App/AutoStartRegistration.h` / `.cpp` (or equivalent PascalCase name)
- Optional `src/App/Bridge.manifest` (`asInvoker`)
- `docs/tests/smoke-epic3-autostart-mt4.md`
- Optional thin PowerShell helper under `installer/` **only if** COM registration from C++ proves too heavy — prefer Bridge self-registration so the absolute exe path stays correct

**Likely UPDATE**
- `src/App/Main.cpp` — wire new flags
- `src/App/MidiSessionCli.*` — optional wait-for-device before Start
- `CMakeLists.txt` — new TUs / manifest
- `docs/dev/winusb-bind.md` — point daily use at Auto-Start smoke guide

**Likely leave alone**
- `src/Protocol/*`, `src/Profile/*`, Epic 2 transport behavior
- Full WiX/Inno Public Installer tree (Epic **4.1**)
- `DeviceSession` port lifecycle contract (call it; do not fork it)

### Recommended implementation shape (not a second product decision)

1. **Register (non-admin):** Task Scheduler task in the current user’s context, logon trigger, interactive token, limited rights, action = `"<absolute>\Bridge.exe" --auto-session` (flag name flexible).
2. **Run:** `--auto-session` → if GUID device present, `runMt4MidiSession(false)` (no Zadig by default on product Auto-Start); else poll/rescan until present or timeout, then Start.
3. **Unregister:** delete the task / Run-key value; idempotent.
4. **Lab escape:** contributors keep `Bridge.exe --start-session` (optionally `--dev-zadig`) without registering Auto-Start.

Do **not** invent a tray GUI, ServiceMain, or second Bridge binary unless lab proves the console host cannot stay alive for Auto-Start — and even then it must remain a user-session process (AD-20).

### Previous epic intelligence (Epic 2 → Epic 3)

- Epic 2 deliberately fenced Auto-Start to **3.1**; longevity soak still uses manual `--start-session`.
- Deferred: `CTRL_CLOSE` orphan ports — relevant once Auto-Start makes the session “always on”; document clean exit; fix only with evidence.
- Deferred: shared `MidiBackend` across concurrent `DeviceSession` — Epic 3 multi-unit (**3.4**), not this story.
- PC-only Epic 2 closure must not be reopened; do not start Epic 4 installer depth under 3.1.
- Recent git focus (IN demux, Matrix soak, overnight SysEx) is transport/lab — Auto-Start is a greenfield App-host concern on a stable session spine.

### Git intelligence summary

Recent commits (`569a2a2`, `d4ddbc7`, `deaf2ff`, …) harden Emagic IN demux, Matrix soak recovery, and macOS SysEx labs. No Auto-Start registration exists yet. Pattern to copy: English diagnostics, flag-gated lab paths, docs under `docs/tests/`, quality gate via `lint-touched.py`, artifacts under `builds/`.

### Latest tech information (Auto-Start on Win10/11)

- Per-user logon tasks with **InteractiveToken** can be registered **without Administrator** when the task does not require elevation and targets the current user ([Task Scheduler `ITaskFolder::RegisterTaskDefinition`](https://learn.microsoft.com/en-us/windows/win32/api/taskschd/nf-taskschd-itaskfolder-registertaskdefinition); logon type `TASK_LOGON_INTERACTIVE_TOKEN` = 3).
- Prefer **“Run only when user is logged on”** so VirtualMIDI and DAW clients share the interactive session (aligns with AD-20 rationale against Session-0).
- Do **not** enable “Run with highest privileges” for daily Auto-Start (would violate NFR-D2).
- `HKCU\...\Run` remains a valid simpler alternate if Task Scheduler COM surface is deferred — still user-session, still no admin.
- Win11 + Windows MIDI Services coexistence quirks (dynamic virtual ports) remain **AQ-4** ops notes — do not switch V1 backend; document if Auto-Start ordering exposes WMS quirks.

### Testing requirements

| Gate | Expectation |
|---|---|
| Unit / offline | Registration helpers: pure logic testable where practical; no hardware required for register/unregister against Task Scheduler/Run-key on a Windows box |
| Contributor smoke | `docs/tests/smoke-epic3-autostart-mt4.md` Pass/Fail rows; Win10 mandatory for closing the story’s lab claim |
| Regression | `--start-session` labs still spawn; mapper/port-name tests green; `lint-touched.py` on touched C++ |
| Negative | Missing VirtualMIDI → fail closed; elevated-only registration is a Fail |

### Project Structure Notes

- C++ sources under `src/` remain **PascalCase**; docs/scripts **kebab-case**.
- No French in sources or user-visible Bridge strings.
- Do not place Auto-Start logic under `src/Protocol/` or invent `src/Service/`.
- Builds only under `builds/` (reject repo-root `build/`).

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Epic 3 / Story 3.1]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — FR-3, NFR-D2, SM-5, UJ-1]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-9, AD-10, AD-12, AD-20, Structural Seed]
- [Source: `_bmad-output/specs/spec-unitor-win64-driver/SPEC.md` — CAP-3, user-session constraint]
- [Source: `src/App/Main.cpp`, `src/App/MidiSessionCli.cpp` — current flag-gated session host]
- [Source: `_bmad-output/implementation-artifacts/deferred-work.md` — CTRL_CLOSE orphan ports]
- [Source: `_bmad-output/implementation-artifacts/epic-2-context.md` — downstream Epic 3]

### Project context reference

No `project-context.md` is present in-repo. Follow `conventions.md` §3 quality gate, AD-20 user-session rule, and the Structural Seed paths above.

## Dev Agent Record

### Agent Model Used

Composer (Cursor agent)

### Debug Log References

- Task Scheduler `RegisterTaskDefinition` returned `hr=0x80070005` (ACCESS_DENIED) in this lab shell; register falls back to HKCU Run and reports the mechanism + scheduler error in stdout.
- Embedded manifest confirmed to contain `asInvoker` in `Bridge.exe`.

### Completion Notes List

- Implemented user-session Auto-Start: `--register-auto-start` / `--unregister-auto-start` / `--auto-session`.
- Registration tries Task Scheduler (interactive token, limited rights) then HKCU Run fallback; unregister clears both idempotently.
- `--auto-session` waits/rescan for project WinUSB GUID (2 s poll, 30 s progress, 900 s timeout) then reuses `runMt4MidiSession(false)`.
- Smoke guide + cross-links authored; offline contract unit tests added; `ctest`, `--test-mapper`, `--test-port-names`, `lint-touched.py` green.
- Operator hardware Pass/Fail rows in the smoke guide remain blank pending Guillaume’s Win10 lab run (honesty bar).

### File List

- `src/App/AutoStartRegistration.h`
- `src/App/AutoStartRegistration.cpp`
- `src/App/AutoStartTaskScheduler.h`
- `src/App/AutoStartTaskScheduler.cpp`
- `src/App/AutoStartRunKey.h`
- `src/App/AutoStartRunKey.cpp`
- `src/App/AutoStartWinUtil.h`
- `src/App/AutoStartWinUtil.cpp`
- `src/App/Mt4WinUsbPresence.h`
- `src/App/Mt4WinUsbPresence.cpp`
- `src/App/Bridge.manifest`
- `src/App/Main.cpp`
- `src/App/MidiSessionCli.h`
- `src/App/MidiSessionCli.cpp`
- `CMakeLists.txt`
- `tests/unit/AutoStartContractTests.cpp`
- `docs/tests/smoke-epic3-autostart-mt4.md`
- `docs/tests/smoke-epic2-mt4.md`
- `docs/dev/winusb-bind.md`
- `_bmad-output/implementation-artifacts/sprint-status.yaml`
- `_bmad-output/implementation-artifacts/3-1-auto-start-without-daily-administrator.md`

### Change Log

- 2026-08-10: Implemented Auto-Start runtime (register/unregister/auto-session), smoke guide, tests; status → review.
- 2026-08-10: Code review patches applied (dual-register/unregister, presence errors, cancel, WorkingDirectory, Run V1 docs, Epic 2 peel); status → done.

## Story completion status

- Status: **done**
- Note: Review patches applied; operator smoke matrix rows may still be blank until Guillaume’s Win10 lab run (honesty bar — blank ≠ Pass)
