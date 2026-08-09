---
baseline_commit: e26be0e
---

# Story 3.2: Hot-plug recovery without Windows reboot

Status: review

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a studio user who moves gear mid-session,
I want usable Virtual Ports to return after unplug/replug without rebooting Windows,
so that a rack move does not kill the whole PC session.

## Acceptance Criteria

1. **Given** a live session with Virtual Ports and a Validation Matrix host open (DAW and/or ShowMIDI)  
   **When** the MT4 is unplugged and then replugged  
   **Then** usable Virtual Ports return without requiring a Windows reboot — FR-11 / CAP-11 / NFR-R2

2. **And** recovery is a **new** DeviceSession that recreates ports under AD-6 identity; teardown destroys ports via `MidiBackend` (no orphan ports on the happy path) — AD-9

3. **And** host port rescan and/or supervised Bridge restart are allowed and documented; requiring reboot is a V1 failure — AD-10

4. **And** AQ-2 (silent recreate vs acknowledge restart) may be noted as UX preference only — lifecycle ownership stays AD-9 (do not invent a second port authority)

**Traces:** FR-11, CAP-11, AD-9, AD-10, SM-4; deferred AQ-2

## Tasks / Subtasks

- [x] Task 1: Lock hot-plug recovery contract (AC: 1, 2, 3, 4)
  - [x] Author operator smoke guide `docs/tests/smoke-epic3-hotplug-mt4.md` (kebab-case) with English Pass/Fail matrix
  - [x] State V1 recovery contract in that guide (see Dev Notes → Recommended shape)
  - [x] Explicit fences: multi-client policy → **3.3**; dual-MT4 ordinal persistence → **3.4**; polished `docs/user/` hot-plug chapter → **4.2**; Public Installer → **4.1**; latency harness → Epic **5**
  - [x] Note AQ-2 as UX preference only: V1 default = **silent in-process recreate** with English console diagnostics; no tray/GUI acknowledge dialog required
  - [x] Cite SSOT: epics Story 3.2; PRD FR-11 / NFR-R2 / SM-4 / UJ-4; AD-9, AD-10, AD-20; SPEC CAP-11

- [x] Task 2: Mid-session teardown + recreate host loop (AC: 1, 2, 3)
  - [x] Extend `src/App/MidiSessionCli.cpp` so `--auto-session` (product host) **survives** mid-session USB loss instead of exiting after one `waitForMidiSessionCancel` failure
  - [x] On pump failure / unexpected `!IsRunning()` while cancel is not requested: call `DeviceSession::Stop()` (must destroy ports via `MidiBackend`), emit English diagnostics, then wait/rescan for WinUSB GUID presence and start a **new** `DeviceSession` (same AD-6 name set for K=1)
  - [x] Reuse `queryMt4WinUsbPresence` / wait helpers from Story 3.1 — do **not** invent App-side `CreatePortSet` / `DestroyPortSet`
  - [x] Bound post-unplug wait (document timeout / poll cadence in smoke guide); fail closed with English diagnostics if device never returns — do not hang forever silently
  - [x] Keep `--start-session` / `--run-midi` usable for labs: either share the same recreate loop **or** document that lab one-shot exit remains for scripts while product recovery is `--auto-session` — do not break existing lab spawners that expect a process exit on unplug without updating those docs
  - [x] Prefer in-process recreate; supervised Bridge process restart remains an **allowed** documented escape (AD-10), not the only path
  - [x] Do **not** invent Session-0 Windows Service / SCM host (AD-20)

- [x] Task 3: Surprise-removal teardown robustness (AC: 2)
  - [x] Verify `DeviceSession::Stop` after abrupt USB removal still reaches `DestroyPortSet` then `transport_.Close()` (AD-9 destroy-on-teardown) — harden only where evidence shows Stop hangs or skips destroy
  - [x] Ensure a subsequent `Start` after successful `Stop` works on a freshly replugged GUID interface (no reboot)
  - [x] Leave `DeviceSessionManager` at V1 K=1 naming; do **not** implement dual-unit ordinal persistence (→ **3.4**)
  - [x] Do **not** implement “mark unavailable while keeping ports alive” (rejected by finalized AD-9)

- [x] Task 4: Operator smoke + contributor cross-links (AC: 1, 3, 4)
  - [x] Smoke matrix rows (Win10 x64 mandatory; Win11 when available):
    1. Live session + ShowMIDI and/or Validation Matrix DAW open → Virtual Ports usable
    2. Unplug MT4 → ports tear down (no orphan names on happy path / Ctrl+C-class stop); Bridge process stays alive for product `--auto-session` path (or supervised restart is documented and used)
    3. Replug MT4 → usable Virtual Ports return **without Windows reboot**
    4. Host rescan (or documented supervised Bridge restart) restores host visibility if needed
    5. Confirm recovery used a **new** session recreate under AD-5/AD-6 names (`MT4 Port N` for single unit)
    6. Negative: requiring Windows reboot to regain ports = **Fail**
  - [x] Cross-link from `docs/tests/smoke-epic3-autostart-mt4.md` fence, `docs/tests/checklists/smoke-epic2-longevity-mt4.md` ownership table, and Epic 2 smoke “→ 3.2” fences where helpful
  - [x] Honesty bar: blank lab rows ≠ Pass; mid-dump unplug during SysEx may need rescan/restart (PRD UJ-2 edge) — document; do not claim Matrix GUI UAT closed here

- [x] Task 5: Regression + quality (AC: all)
  - [x] `--register-auto-start` / `--unregister-auto-start` / first-availability wait from 3.1 still work
  - [x] `Bridge --test-mapper` and `Bridge --test-port-names` still exit 0
  - [x] `ctest` / `BridgeTests` Pass when C++ changed
  - [x] If C++ changed: `python scripts/quality/lint-touched.py` exits 0; compile under `builds/`; no French in sources; Protocol/Profile free of VirtualMIDI/WinUSB
  - [x] Confirm no second port authority, no Session-0 service, no tray GUI invent for AQ-2, no dual-MT4 work under this story ID

## Dev Notes

### Soft dependencies

| Dependency | Status | How this story treats it |
|---|---|---|
| Epic 1 DeviceSession + VirtualMIDI Create/Destroy | **done** | Hard — recreate must call existing Start/Stop spine |
| Epic 2 transport / SysEx / longevity design | **done** | Soft — do not reopen clock/MTC/SysEx; longevity soak ≠ hot-plug |
| Story **3.1** Auto-Start | **done** | Hard host prerequisite — extend `--auto-session` survival; leave register/unregister alone unless a tiny shared-constant move is required |
| Story **3.3** multi-client | backlog | Do not add exclusive-open policy; hosts may already be open during the drill |
| Story **3.4** dual-MT4 | backlog | Keep K=1; AD-6 identity for the unit under test only |
| Story **4.2** user docs | backlog | Operator smoke + contributor cross-links in 3.2; polished `docs/user/` later |
| CTRL_CLOSE orphan ports | deferred | Document Ctrl+C; do not invent a second lifecycle owner to “fix” orphans unless lab proves recreate fails on stale names |

### Scope fence

This story lands **FR-11 / CAP-11 / SM-4 hot-plug recovery** so unplug/replug restores usable Virtual Ports without a Windows reboot. It is **not** multi-client policy, dual-MT4 naming, or end-user docs polish.

| In scope | Out of scope (later / never) |
|---|---|
| Mid-session unplug → Stop/destroy → wait → new DeviceSession Start | Session-0 Windows Service (**forbidden** — AD-20) |
| Product `--auto-session` survives USB loss | “Mark unavailable / keep ports alive” (**rejected** — AD-9) |
| Document host rescan + supervised Bridge restart | AQ-2 GUI acknowledge dialog / tray app |
| SM-4 operator smoke guide | Dual-MT4 ordinal persistence → **3.4** |
| English hot-plug diagnostics | Multi-client exclusivity claims → **3.3** |
| Surprise-removal Stop robustness (evidence-based) | Public Installer / `docs/user/` polish → **4.x** |
| | Windows MIDI Services backend switch |
| | AMT8 / Unitor8 product claims |

### Epic context

Epic 3 (“Daily Studio Resilience”) makes daily studio use survivable: no hand-launch (3.1), hot-plug without reboot (**3.2**), multi-client (3.3), two-unit naming (3.4). Story **3.2** is the unplug/replug slice — the missing recreate loop after 3.1’s first-availability wait.

### Architecture compliance (must follow)

| Decision | Rule for this story |
|---|---|
| **AD-9** | Only a live `DeviceSession` creates/destroys Virtual Ports via `MidiBackend`. Teardown (including unplug) **destroys** ports. Recovery = **new** session that recreates ports under AD-6 identity. App/hot-plug code must **not** call Create/DestroyPortSet. |
| **AD-10** | Usable ports return without Windows reboot; host rescan and/or supervised Bridge restart allowed and must be documented; reboot required = V1 failure. |
| **AD-20** | Bridge stays a user-session process; no SCM service for recovery. |
| **AD-6** | Recreate under stable unit identity (V1: single unit K=1 / `MT4 Port N`). Do not reshuffle names casually; dual-unit proof is **3.4**. |
| **AD-4** | One DeviceSession per MT4; teardown/recreate per unit — not a multiplexed cascade. |
| Structural Seed | Hot-plug detection/host loop may live in `src/App/` and/or thin helpers; session ownership stays in `src/Device/`. Capability table: Auto-start / hot-plug → `App/`, `Device/` under AD-10, AD-20. |

**Notification ownership (resolve AD-10 soft gap):** App owns **arrival/removal detection** (poll and/or `CM_Register_Notification`). Detection only signals the host to Stop/Start a `DeviceSession`. Ports still only via DeviceSession → MidiBackend. Do not put CreatePortSet in a watcher callback.

### Current code baseline (UPDATE files)

**`src/App/MidiSessionCli.cpp` — today**
- `--start-session`: one `DeviceSession::Start`, then `waitForMidiSessionCancel`.
- On `TakePumpFailure` / `!IsRunning()`: print error, `session.Stop()`, **return exit 1** — process ends.
- `--auto-session`: wait/poll GUID (900 s / 2 s / 30 s) → same one-shot session path (`allowZadig=false`).
- **No** mid-session recreate loop after live USB loss.

**What this story changes**
- Product host loop: after unexpected session death (unplug), Stop → wait for presence → Start again (while cancel not requested).
- English diagnostics for hot-plug teardown and recreate.
- Operator smoke guide documenting rescan / supervised restart / reboot=Fail.

**What must be preserved**
- `DeviceSession::Start` order: WinUSB open → Emagic init → `MidiBackend::CreatePortSet` → bulk-IN pump.
- `DeviceSession::Stop` order: join pump → clear sink → `DestroyPortSet` → finish magic → `Close`.
- GUID-first WinUSB open (`{aa209017-cf8a-49ad-a0e7-701187ff7e05}`); product Auto-Start / recovery uses GUID path only (no `--dev-zadig`).
- VirtualMIDI fail-closed English fix path.
- 3.1 Auto-Start registration (Task Scheduler → HKCU Run), `asInvoker` manifest, non-admin daily ops.
- Binary output under `builds/` only.

**`src/App/Mt4WinUsbPresence.*` — today**
- Poll-only `SetupDi` present-check for project GUID (no open claim).
- Hard SetupDi errors ≠ Absent (3.1 review patch) — preserve.
- Sufficient for V1 post-unplug wait; event-driven CM notifications are optional enhancement, not required if poll meets SM-4.

**`src/Device/DeviceSession.*` — today**
- Start calls `Stop()` first; supports Start-after-Stop on the same object.
- Unplug typically surfaces as ReadBulk/async failure → `TakePumpFailure`.
- Host never restarts a second session in-process today.

**`src/Device/DeviceSessionManager.*` — today**
- Naming only (`unitOrdinalK_ = 1`). Not a PnP owner. Leave dual-unit work to **3.4**.

**Likely NEW**
- `docs/tests/smoke-epic3-hotplug-mt4.md`
- Optional thin PascalCase helper under `src/App/` for “wait for replug” / recovery loop constants (prefer reuse of 3.1 wait constants or a clearly named twin — avoid magic numbers)

**Likely UPDATE**
- `src/App/MidiSessionCli.cpp` / `.h` — recreate loop for product (and lab policy as chosen)
- `src/Device/DeviceSession.*` / `src/Usb/WinUsbTransport.*` — only if Stop/Close after surprise removal is unsafe
- Cross-link smoke / longevity ownership tables

**Likely leave alone**
- `src/App/AutoStartRegistration*` (register/unregister)
- `src/Protocol/*`, `src/Profile/*`, Epic 2 transport behavior
- VirtualMIDI backend internals (call Create/Destroy via session only)
- Public Installer / WiX / Inno

### Recommended implementation shape (not a second product decision)

1. **Detect loss:** Existing pump failure / `!IsRunning()` while `g_cancelRequested` is false = treat as hot-plug (or USB fault) candidate.
2. **Teardown:** `session.Stop()` → AD-9 destroy ports; English log e.g. `MT4 disconnected; waiting for replug...`.
3. **Wait:** Reuse presence poll (2 s cadence; document timeout — may reuse 900 s or a shorter hot-plug-specific bound documented in smoke).
4. **Recreate:** Build PortNameSet via `DeviceSessionManager` (K=1), `session.Start(...)` again (or a fresh `DeviceSession` instance — either OK if Stop completed), print started banner.
5. **Host visibility:** Document that Ableton/Reason/ShowMIDI may need a MIDI device rescan; supervised Bridge restart is allowed if in-process recreate is insufficient.
6. **Cancel:** Ctrl+C still exits cleanly (prefer over console close — CTRL_CLOSE orphan risk remains deferred).

**Anti-patterns (do not)**
- App calling `VirtualMidiBackend::CreatePortSet` from a PnP callback
- Keeping Virtual Ports alive while WinUSB is gone (“mark unavailable”)
- Exiting `--auto-session` on first unplug and calling that “recovery” because Task Scheduler will start again at next logon
- Inventing a Windows Service to “watch USB”
- Claiming dual-MT4 hot-plug stability under this story ID

### Previous story intelligence (3.1 → 3.2)

- 3.1 explicitly fenced mid-session unplug/replug to **3.2** — first USB wait is **not** hot-plug recovery.
- Patterns to copy: English diagnostics, kebab-case smoke under `docs/tests/`, honesty bar (blank ≠ Pass), `lint-touched.py`, builds under `builds/`, AD-9 “App starts a session; never a second port authority”.
- Review lessons to carry: SetupDi hard errors ≠ Absent; honor Ctrl+C across wait → Start; do not install console Ctrl handler twice; offline unit tests may be thin contracts — hardware smoke is the real gate.
- Deferred: CTRL_CLOSE orphan ports — Auto-Start increased exposure; hot-plug recreate will hit stale-name failures if Stop skipped — document clean stop; harden only with evidence.
- Task Scheduler ACCESS_DENIED → HKCU Run is acceptable V1 for registration — irrelevant to recreate loop but do not regress unregister/register.

### Git intelligence summary

- `58bc5f4` — Auto-Start (`MidiSessionCli`, `Mt4WinUsbPresence`, registration stack).
- Recent Epic 2 closure commits (`e26be0e`, `a325c3d`, `9a6e29a`) — do not reopen; hot-plug is App/Device lifecycle, not transport demux.
- Pattern: flag-gated lab paths stay intact; product path is `--auto-session`.

### Latest tech information (PnP notifications on Win10/11)

- Preferred modern API for interface arrival/removal: `CM_Register_Notification` with `CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE` filtered to the project WinUSB DeviceInterfaceGUID (`Cfgmgr32.lib`). Does **not** enumerate already-present interfaces — register first, then enumerate/list (or keep using `SetupDi` / existing presence helper).
- Callbacks must stay non-blocking; defer Stop/Start to the host thread (Microsoft guidance).
- Optional per-open-handle `CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE` helps query-remove / surprise-removal close — useful if Stop races with Windows remove; not mandatory if pump-failure + poll already Pass SM-4.
- Legacy `WM_DEVICECHANGE` / `RegisterDeviceNotification` is acceptable but heavier for a console host; prefer CM API or reuse 3.1 polling for V1.
- Win11 + Windows MIDI Services quirks with dynamic VirtualMIDI ports remain **AQ-4** ops notes — do not switch V1 backend.

### Testing requirements

| Gate | Expectation |
|---|---|
| Contributor smoke | `docs/tests/smoke-epic3-hotplug-mt4.md` Pass/Fail; Win10 mandatory for lab claim |
| Lifecycle | Unplug → DestroyPortSet; replug → CreatePortSet; names `MT4 Port N` (single unit) |
| Negative | Windows reboot required = Fail; App-created ports outside session = Fail |
| Regression | 3.1 Auto-Start register/wait; `--start-session` lab policy preserved or docs updated; mapper/port-name tests; `lint-touched.py` |
| Longevity distinction | Supervised restart OK for hot-plug; **not** an excuse for Epic 2 ~4h steady-state Fail |

### Project Structure Notes

- C++ under `src/` = **PascalCase**; docs/scripts = **kebab-case**.
- No French in sources or Bridge user-visible strings.
- Do not place hot-plug port create under `src/Protocol/` or invent `src/Service/`.
- Builds only under `builds/` (reject repo-root `build/`).

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Epic 3 / Story 3.2]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — FR-11, NFR-R2, SM-4, UJ-4, Hot-Plug Recovery glossary]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-9, AD-10, AD-20, AQ-2, Structural Seed]
- [Source: `_bmad-output/specs/spec-unitor-win64-driver/SPEC.md` — CAP-11]
- [Source: `_bmad-output/implementation-artifacts/3-1-auto-start-without-daily-administrator.md` — host patterns + soft fence]
- [Source: `_bmad-output/implementation-artifacts/deferred-work.md` — CTRL_CLOSE orphan ports]
- [Source: `src/App/MidiSessionCli.cpp` — one-shot session host to extend]
- [Source: `src/Device/DeviceSession.cpp` — Start/Stop destroy-on-teardown]
- [Source: `docs/tests/smoke-epic3-autostart-mt4.md` — fence → 3.2]
- [Source: Microsoft Learn — `CM_Register_Notification` / device interface arrival-removal]

### Project context reference

No `project-context.md` is present in-repo. Follow `conventions.md` §3 quality gate, AD-9 lifecycle, AD-20 user-session rule, and the Structural Seed paths above.

## Dev Agent Record

### Agent Model Used

Cursor Grok 4.5

### Debug Log References

- `lint-touched.py` initially flagged `runMt4AutoSession` length/complexity; extracted `Mt4PresenceWait` + `runAutoSessionHotPlugLoop` / `prepareMt4PortNames`.
- Build: `builds/debug` Debug — Bridge + BridgeTests green; `ctest` BridgeTests Pass; `--test-mapper` / `--test-port-names` exit 0.

### Completion Notes List

- Product `--auto-session` now survives mid-session USB loss: Stop (AD-9 destroy ports) → wait/replug poll (900 s / 2 s / 30 s) → new `DeviceSession::Start` under same K=1 names; silent in-process recreate (AQ-2 preference).
- Lab `--start-session` / `--run-midi` remain one-shot exit on disconnect so existing spawners keep working.
- `DeviceSession::Stop` verified destroy-then-Close order; comments clarify surprise-removal safety — no behavioral rewrite (no hang evidence).
- Operator smoke `docs/tests/smoke-epic3-hotplug-mt4.md` + cross-links from 3.1 / longevity / Epic 2 smoke.
- Offline contract test for hot-plug wait constants; no second port authority, no Session-0 service, no dual-MT4.

### File List

- `CMakeLists.txt`
- `_bmad-output/implementation-artifacts/3-2-hot-plug-recovery-without-windows-reboot.md`
- `_bmad-output/implementation-artifacts/sprint-status.yaml`
- `docs/tests/checklists/smoke-epic2-longevity-mt4.md`
- `docs/tests/smoke-epic2-mt4.md`
- `docs/tests/smoke-epic3-autostart-mt4.md`
- `docs/tests/smoke-epic3-hotplug-mt4.md`
- `src/App/AutoStartRegistration.h`
- `src/App/MidiSessionCli.cpp`
- `src/App/MidiSessionCli.h`
- `src/App/Mt4PresenceWait.cpp`
- `src/App/Mt4PresenceWait.h`
- `src/Device/DeviceSession.cpp`
- `tests/unit/HotPlugContractTests.cpp`

### Change Log

- 2026-08-10: Implemented Story 3.2 hot-plug recovery (in-process recreate on `--auto-session`), smoke guide, presence-wait helper, contract tests; status → review.

## Story completion status

- Status: **review**
- Note: Implementation complete; hardware SM-4 Pass/Fail rows remain for operator lab on Win10 x64.
