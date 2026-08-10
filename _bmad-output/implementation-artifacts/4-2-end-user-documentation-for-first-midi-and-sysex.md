---
baseline_commit: 538ceea
---

# Story 4.2: End-user documentation for first MIDI and SysEx

Status: done

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a new MT4 owner on Windows,
I want shipped user docs that cover prerequisites, install, Auto-Start, first MIDI test, first SysEx test, troubleshooting, and works/does-not-work,
so that I can complete UJ-1 and UJ-2 without tribal knowledge.

## Acceptance Criteria

1. **Given** the Public Installer / Bridge from Story 4.1 and Epic 1–3 capabilities  
   **When** a new user follows only shipped `docs/user/` materials (plus named external prerequisites)  
   **Then** they can complete first MIDI (UJ-1) and first SysEx with Matrix-Control or documented equivalent path (UJ-2) — FR-13 / CAP-13 / AD-19

2. **And** docs cover VirtualMIDI prerequisites, install, Auto-Start, first MIDI test, first SysEx test, troubleshooting, and an explicit works / does-not-work list

3. **And** hot-plug recovery expectations (rescan / supervised restart OK; reboot = fail) are stated

4. **And** multi-MT4 validation honesty is stated if only one physical unit was proven

**Traces:** FR-13, CAP-13, AD-19, SM-5

## Tasks / Subtasks

- [x] Task 1: Lock end-user doc contract + fences (AC: 1–4)
  - [x] Create `docs/user/` (Architecture Structural Seed / AD-19) — directory does **not** exist today; this story owns it
  - [x] Author operator smoke guide `docs/tests/smoke-epic4-user-docs-mt4.md` (kebab-case) with English Pass/Fail matrix proving UJ-1 + UJ-2 can be completed **from `docs/user/` alone** (plus named external prerequisites). Blank lab rows ≠ Pass; Win10 x64 mandatory
  - [x] State product intent: a musician who never saw the lab smokes can reach first MIDI the same evening and first SysEx without contributor tribal knowledge
  - [x] Explicit fences:
    - Public Installer AD-12 UX / packaging → **4.1** (describe it; do **not** redesign the wizard)
    - three-way MIT vs VirtualMIDI vs Windows MIDI Services honesty polish + contributor dual-machine loop → **4.3**
    - Authenticode / SmartScreen honesty for unsigned public builds → **4.4**
    - Tobias MSI **embed** redistributable → **OQ-1** release gate only (docs name eval path: loopMIDI / rtpMIDI)
    - MIDI Path latency claims / harness → Epic **5** (never cite ASIO buffer size as MIDI proof)
    - Zadig primary path → **forbidden**; keep Zadig contributor-only in `docs/dev/winusb-bind.md`
  - [x] Honesty bar: do **not** claim SM-5 closed until this story’s user-doc smoke Passes; do **not** claim SM-6 / FR-14 closed under this story ID
  - [x] Cite SSOT: epics Story 4.2; PRD FR-13 / UJ-1 / UJ-2 / SM-5; AD-19 / AD-10 / AD-8; SPEC CAP-13
  - [x] Cross-link: replace “→ 4.2” fences in Epic 3 / Epic 4.1 smokes with pointers to the new `docs/user/` chapters (and this user-docs smoke guide)

- [x] Task 2: Ship `docs/user/` chapter set (AC: 1–2)
  - [x] Language: **English** shipped user prose (project `document_output_language` / public OSS bar). French operator smokes under `docs/tests/` stay operator-only — distill, do not ship as community manuals
  - [x] Create kebab-case chapters covering every FR-13 / AD-19 checklist item (recommended layout — merge only if a chapter would be thinner than a screen):

    | Path | Must cover |
    |---|---|
    | `docs/user/README.md` | Landing: start here → prerequisites → install → Auto-Start → first MIDI → first SysEx → troubleshooting → works/does-not-work |
    | `docs/user/prerequisites.md` | Win10/11 x64; physical MT4; VirtualMIDI driver present via **loopMIDI** or **rtpMIDI** (eval); name external hosts; OQ-1 MSI embed not required for this path |
    | `docs/user/install.md` | Public Installer walkthrough (Ten Square Software / Unitor MT4 Bridge); one-time admin OK; success only when gates pass; install dir expectation; **not** Zadig |
    | `docs/user/auto-start.md` | Logon / plug → ports without daily admin; no Session-0 service; what “Bridge is running” looks like for a non-developer |
    | `docs/user/first-midi.md` | UJ-1: ports appear as `MT4 Port N` (2 IN / 4 OUT); select in Ableton Live 12 **or** MIDI-OX; notes/CC round-trip on at least one IN and one OUT |
    | `docs/user/first-sysex.md` | UJ-2: Matrix-Control dump/restore **or** documented MIDI-OX / SysEx equivalent; no Bridge restart for normal librarian completion; Computer Mode wake honesty (channel MIDI / CC kick — SysEx alone does not wake Computer Mode) |
    | `docs/user/troubleshooting.md` | VirtualMIDI missing; WinUSB bind fail; empty ports; Computer Mode; host rescan; supervised Bridge restart; multi-client ≤8 ceiling pointer |
    | `docs/user/works-and-does-not-work.md` | Explicit V1 works vs non-goals (see Dev Notes) |
    | `docs/user/hot-plug.md` **or** section inside troubleshooting | AD-10: rescan / supervised restart OK; Windows reboot = fail |
    | `docs/user/multi-mt4.md` **or** section in works/does-not-work | Naming `MT4 Port N` vs `MT4 #K Port N` for `K≥2`; honesty if only one physical unit was proven |

  - [x] Keep tone AD-12-aligned: few steps, minimal jargon, clear success — user docs describe the same product story as the installer
  - [x] Name external prerequisites explicitly (VirtualMIDI via loopMIDI/rtpMIDI; Ableton / MIDI-OX / Matrix-Control as applicable) — never imply they are vendored in this repo
  - [x] Matrix-Control is a **validation target**, not a Bridge runtime dependency — say so once in first-sysex
  - [x] Multi-client: document VirtualMIDI ceiling of up to **8** clients per port (AD-8) in user-facing language (short note is enough; full quirks stay in Epic 3 smoke)
  - [x] Do **not** paste full Epic 2 Matrix-Control vector tables (#1–#7) into user docs — first SysEx = one clear happy path, not SM-2 lab closure

- [x] Task 3: Discoverability surfaces (AC: 1)
  - [x] Update root `README.md`: point community users to `docs/user/` as the start-here path; refresh status beyond “greenfield / early planning”; keep deep protocol / Linux quirk tables out of the user journey (contributor material can remain lower or move pointer to `docs/dev/`)
  - [x] Light license honesty only if needed for install trust — **full** three-way MIT ≠ VirtualMIDI ≠ Windows MIDI Services polish is **4.3**; do not claim 4.3 done
  - [x] Update Public Installer success / support messaging to point at `docs/user/` (GitHub path and/or packaged short “Getting started” link). Prefer a durable repo-relative URL under `docs/user/README.md`. Do **not** redesign AD-12 wizard flow
  - [x] Cross-link `docs/dev/winusb-bind.md`: community readers → `docs/user/install.md`; Zadig remains contributor fallback
  - [x] Update `docs/tests/smoke-epic4-public-installer-mt4.md` fence: polished user docs now live under `docs/user/` (SM-5 still needs this story’s user-docs smoke Pass)

- [x] Task 4: Distill lab knowledge — do not reinvent product facts (AC: 1–4)
  - [x] Reuse facts from (do **not** ship these as user manuals):
    - `docs/tests/smoke-epic4-public-installer-mt4.md` — installer path, gates, install dir
    - `docs/tests/smoke-epic3-autostart-mt4.md` — Auto-Start daily expectations
    - `docs/tests/smoke-epic1-mt4.md` — first notes/CC (strip build-from-source / `--dev-zadig`)
    - `docs/tests/checklists/smoke-epic2-matrix-control-mt4.md` + `smoke-epic2-sysex-mt4.md` — simplify to one librarian happy path
    - `docs/tests/smoke-epic3-hotplug-mt4.md` — hot-plug honesty
    - `docs/tests/smoke-epic3-dual-mt4-mt4.md` — multi-MT4 naming + validation honesty
    - `docs/tests/smoke-epic3-multiclient-mt4.md` — ≤8 clients; MIDI-OX as Validation Matrix utility
  - [x] Port naming must match shipped Bridge: unit 1 `MT4 Port N`; unit `K≥2` `MT4 #K Port N` (AD-5 / Story 3.4)
  - [x] Install path must match 4.1: `C:\Program Files\Ten Square Software\Unitor MT4 Bridge\` (or document the actual shipped path if it differs)
  - [x] USB identity in docs: VID/PID hex uppercase `086A:0003`; primary HWID `USB\VID_086A&PID_0003&MI_02`; DeviceInterfaceGUID `{aa209017-cf8a-49ad-a0e7-701187ff7e05}` only where a troubleshooting reader needs it — not in every chapter intro

- [x] Task 5: Operator verification + quality (AC: all)
  - [x] Fill `docs/tests/smoke-epic4-user-docs-mt4.md` Pass/Fail rows (Win10 x64 mandatory), minimum:
    1. Fresh reader follows `docs/user/` only → completes install + Auto-Start expectations (or confirms already installed from 4.1) without lab smoke tribal knowledge
    2. First MIDI (UJ-1): notes/CC visible in Ableton Live 12 **or** MIDI-OX on named `MT4 Port N`
    3. First SysEx (UJ-2): Matrix-Control **or** documented equivalent completes a dump/restore (or short SysEx exchange) without Bridge restart for normal completion
    4. Troubleshooting chapter matches at least one deliberate negative (e.g. VirtualMIDI missing messaging aligns with installer/docs)
    5. Works / does-not-work list present and consistent with PRD non-goals
    6. Hot-plug expectations stated (rescan / supervised restart OK; reboot = fail)
    7. Multi-MT4 honesty stated (proven dual-unit **or** explicit “single-unit proven” wording)
    8. Discoverability: README and installer support/success pointer reach `docs/user/`
  - [x] Prefer **documentation-only** change set. If touching installer strings / C++: `python scripts/quality/lint-touched.py` exits 0; no French in sources; do not pull docs concerns into Protocol/Profile/Midi
  - [x] Confirm no polished chapter claimed under **4.3** / **4.4** story IDs; no Tobias MSI embed; no Zadig-as-primary; no latency Studio-Done claims

## Dev Notes

### Soft dependencies

| Dependency | Status | How this story treats it |
|---|---|---|
| Epic 1–3 Bridge capabilities (ports, SysEx, Auto-Start, hot-plug, multi-client, dual-MT4) | **done** | Document behavior; do not reopen MIDI protocol work |
| Story **4.1** Public Installer | **in-progress** / near-complete packaging | Docs describe the shipped installer path; if 4.1 lab matrix is still blank, state validation honesty — do not block writing docs on blank 4.1 rows, but SM-5 closure needs both installer + these docs |
| VirtualMIDI eval path (loopMIDI / rtpMIDI) | **available** | Named external prerequisite |
| OQ-1 Tobias MSI clearance | **open** | Docs must not claim redistributable embed |
| Story **4.3** license / tech docs | backlog | Light pointers OK; full three-way honesty polish out of scope |
| Story **4.4** Authenticode / SmartScreen | backlog | Do not write SmartScreen policy chapter here |

### Scope fence

This story ships the **community end-user manual** under `docs/user/` so a new MT4 owner can finish UJ-1 and UJ-2 from shipped prose alone. It is **not** installer redesign, **not** license/legal polish, and **not** Authenticode policy.

| In scope | Out of scope (later / never) |
|---|---|
| Create + fill `docs/user/` per FR-13 / AD-19 | Redesigning AD-12 installer UX → **4.1** |
| UJ-1 first MIDI + UJ-2 first SysEx happy paths | Full SM-2 Matrix-Control vector lab tables |
| Troubleshooting + works/does-not-work | Three-way license honesty polish → **4.3** |
| Hot-plug + multi-MT4 honesty statements | SmartScreen / Authenticode docs → **4.4** |
| README + installer discoverability pointers | Embedding VirtualMIDI MSI → **OQ-1** |
| Operator smoke proving docs-alone journey | MIDI Path / latency Studio-Done → Epic **5** |
| English user prose | Shipping French `docs/tests/` operator guides as community manuals |
| | Claiming AMT8 / Unitor8 / cascade / Patch mode / LTC as V1 |

### Architecture compliance (must follow)

- **AD-19:** shipped **user** docs cover VirtualMIDI prerequisites, install, Auto-Start, first MIDI, first SysEx, troubleshooting, works/does-not-work; facade **Ten Square Software**; SmartScreen detail deferred to **4.4**
- **AD-12:** docs describe community INF/WinUSB + Public Installer path; Zadig is **not** primary
- **AD-10:** hot-plug = rescan / supervised restart OK; Windows reboot = fail — must be stated
- **AD-8:** ≤8 VirtualMIDI clients per port — short user-facing note
- **AD-5 / AD-6:** port naming and ordinal honesty for multi-MT4
- **AD-20:** Bridge is user-session process — docs must not instruct installing a Windows Service
- **AD-2:** documentation must not drag packaging/USB concerns into Protocol/Profile code
- **Structural seed:** `docs/user/` = end-user; `docs/dev/` = contributor; `docs/tests/` = operator smoke
- **Conventions:** kebab-case paths; no top-level `Documentation/`; English in shipped docs; no French in C++ if touched

### Works / does-not-work content (must appear)

**Works (V1 intent — phrase for users):**

- MT4 on Windows 10/11 x64 via Public Installer + WinUSB
- Stable Virtual Ports: 2 IN / 4 OUT as `MT4 Port N` (multi-unit `#K` when applicable)
- Notes, CC, clock/transport, MTC, transparent SysEx (librarian/editor scale)
- Auto-Start without daily Administrator
- Hot-plug recovery without Windows reboot (host rescan / supervised Bridge restart OK)
- Multi-client DAW + MIDI-OX within VirtualMIDI limits
- Dual MT4 when hardware exists — with honesty if only one unit was proven

**Does not work / not claimed in V1:**

- Patch mode, LTC/VITC, Fast Mode / AMT features from Unitor-family manuals
- Cascaded / stacked Emagic multi-interface topologies
- Guaranteed AMT8 / Unitor8 without validated hardware
- Windows MIDI Services as the V1 backend (future Win11-only option — detail in **4.3**)
- Custom kernel MIDI driver
- Zadig as the recommended community install path
- “Studio-done” latency/jitter numbers before Epic **5** measurement

### UJ mapping (write toward these journeys)

**UJ-1 (Alex):** installer → VirtualMIDI prerequisite → one admin elevation → plug MT4 → Auto-Start → Ableton (or MIDI-OX) → `MT4 Port N` → notes/CC/clock.

**UJ-2 (Sam):** Bridge running → Matrix-Control (or documented SysEx equivalent) on MT4 ports → dump/restore / editor session completes → no Bridge restart for normal librarian use; mid-dump unplug → recover via rescan/restart, not reboot.

### Recommended authoring shape

1. **Compose from lab facts, rewrite for musicians** — every `docs/user/` page should pass the clarity bar: usable in ~20 seconds without file-path piles.
2. **One happy path per chapter** — link deeper lab smokes only as “for contributors / lab operators,” never as required reading.
3. **First SysEx equivalent path:** if Matrix-Control is unavailable, document MIDI-OX (or another Validation Matrix–honest path) that still proves SysEx bytes traverse a Virtual Port end-to-end — label it as equivalent proof, not as Matrix-Control SM-2 closure.
4. **Computer Mode:** state clearly that waking Computer Mode needs channel MIDI activity (e.g. CC kick); SysEx alone may not wake the device — this is a top support footgun.
5. **Discoverability before polish theater** — README + installer pointer are part of AC1 (“shipped materials” must be findable).

### Previous story intelligence

- **4.1:** Inno Setup 6 Public Installer; VirtualMIDI fail-closed; WinUSB via `pnputil`; unelevated Auto-Start register; Ten Square branding; explicitly fenced polished `docs/user/` here. Installer currently points support URL at GitHub only — **harden to `docs/user/`**. Do not claim full SM-5 without this story. Lab smoke matrix for 4.1 may still be blank — honesty applies.
- **3.1–3.4:** Each fenced polished user chapters (Auto-Start, hot-plug, multi-client, multi-MT4) to **4.2**. Tech/smoke docs already hold the facts — distill, do not contradict.
- **3.3:** Validation Matrix multi-client utility is **MIDI-OX** (not ShowMIDI) after lab lock — user docs should say MIDI-OX when naming the utility.
- **2.4:** Matrix-Control minimum vectors are lab gates; user first-SysEx is a **simplified** happy path.
- **1.5:** VirtualMIDI driver must be present; Bridge creates ports programmatically — users need not manage ports solely via VirtualMIDI UI.

### Git intelligence

Recent commits (`538ceea`, `ea3e21d`, `83d9226`, `42e4314`, `6b95279`) delivered Epic 3 hardening and Story 4.1 Public Installer packaging. This story opens the greenfield `docs/user/` surface on a stable Bridge + installer spine — prefer markdown + light installer/README discoverability edits over C++ churn.

### Latest tech / product notes

- VirtualMIDI prerequisite messaging must stay aligned with installer English fix path (`loopMIDI` / `rtpMIDI`) and Bridge `kVirtualMidiMissingDriverFixPath` intent.
- Public Installer output: `builds/installer/UnitorMt4Bridge-Setup.exe` via `scripts/packaging/build-public-installer.ps1`.
- Do not document deprecated DIFx or Zadig as the community bind path.
- If linking to GitHub-hosted docs, use stable paths under `docs/user/` that will exist after merge.

### Project structure notes

```text
docs/user/           # NEW — end-user manual (this story, AD-19)
docs/tests/          # NEW smoke-epic4-user-docs-mt4.md + UPDATE Epic 3/4.1 fences
docs/dev/            # UPDATE cross-links only (winusb-bind.md)
README.md            # UPDATE community start-here → docs/user/
installer/           # OPTIONAL light success/support URL / finished-page pointer only
_bmad-output/.../4-2-*.md  # this story file
```

Do **not** create `Documentation/`. Do **not** invent a second user-doc tree.

### Testing requirements

- Operator smoke on Win10 x64 with physical MT4 for MIDI/SysEx rows.
- “Docs-alone” means: verifier uses `docs/user/` (+ named externals), not Epic 1–3 French operator smokes, as the instruction set.
- Matrix-Control preferred for UJ-2; equivalent SysEx path allowed if labeled honestly.
- Offline/macOS edit machine can author markdown; Windows validation machine closes Pass rows (dual-machine loop).
- Quality: English-only user docs; `lint-touched.py` only if C++ touched.

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Epic 4 / Story 4.2]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — FR-13, UJ-1, UJ-2, SM-5, §5 non-goals, Validation Matrix]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-19, AD-12, AD-10, AD-8, Structural Seed `docs/user/`]
- [Source: `_bmad-output/specs/spec-unitor-win64-driver/SPEC.md` — CAP-13]
- [Source: `_bmad-output/implementation-artifacts/4-1-public-installer-meeting-ad-12-ux-bar.md`]
- [Source: `_bmad-output/implementation-artifacts/3-1-auto-start-without-daily-administrator.md`]
- [Source: `_bmad-output/implementation-artifacts/3-2-hot-plug-recovery-without-windows-reboot.md`]
- [Source: `_bmad-output/implementation-artifacts/3-3-multi-client-daw-plus-showmidi.md`]
- [Source: `_bmad-output/implementation-artifacts/3-4-two-mt4-units-with-stable-distinguishable-names.md`]
- [Source: `docs/tests/smoke-epic4-public-installer-mt4.md`]
- [Source: `docs/tests/checklists/smoke-epic2-matrix-control-mt4.md`]
- [Source: `README.md` — current public facade (needs refresh)]

## Dev Agent Record

### Agent Model Used

Composer (Cursor agent)

### Debug Log References

### Implementation Plan

- Authored English + French `docs/user/` manuals from Epic 1–4 lab facts; used live Bridge names `MT4 In N` / `MT4 Out N` (planning shorthand `MT4 Port N` called out once).
- Added operator smoke `smoke-epic4-user-docs-mt4.md` with content-verified Pass on rows 4–8; rows 1–3 left blank for Win10 physical lab (blank ≠ Pass; SM-5 not claimed closed).
- Discoverability: `docs/user/README.md` landing, root README start-here, installer `AppSupportURL` / FinishedLabel / success text → GitHub `docs/user/README.md`, winusb-bind community pointer, Epic 3/4.1 fence updates.

### Completion Notes List

- Shipped AD-19 user manuals as single-file EN + FR peers under `docs/user/`, plus thin `docs/user/README.md` landing.
- User prose covers UJ-1 / UJ-2, troubleshooting, hot-plug (rescan / supervised restart OK; Windows reboot = fail), multi-MT4 validation honesty (single-unit proven path), and explicit works / does-not-work.
- Port names match Bridge: `MT4 In/Out N` and `MT4 #K In/Out N`.
- Computer Mode: SysEx alone does not wake; Auto-Start: user-session Bridge, not a Windows Service.
- Installer support / finished / success pointers use GitHub `docs/user/README.md` without redesigning AD-12 wizard flow.
- `verify-installer-contract.py` OK after pointer update.
- Bridge version CMake / `--version` work remains on disk but is **owned by Story 4.1** (not claimed as 4.2 docs delivery).
- Honesty: SM-5 not closed — hardware smoke rows 1–3 in `smoke-epic4-user-docs-mt4.md` still blank pending Win10 + MT4 lab.

### File List

- `docs/user/README.md` (new — landing)
- `docs/user/unitor-mt4-bridge-user-guide.md` (new — English single-file manual)
- `docs/user/unitor-mt4-bridge-manuel-utilisateur.md` (new — French single-file manual peer)
- `docs/tests/smoke-epic4-user-docs-mt4.md` (new)
- `docs/tests/smoke-epic4-public-installer-mt4.md` (modified)
- `docs/tests/smoke-epic3-autostart-mt4.md` (modified)
- `docs/tests/smoke-epic3-hotplug-mt4.md` (modified)
- `docs/tests/smoke-epic3-dual-mt4-mt4.md` (modified)
- `docs/tests/smoke-epic3-multiclient-mt4.md` (modified)
- `docs/dev/winusb-bind.md` (modified)
- `README.md` (modified)
- `installer/public-installer.iss` (modified — Getting started / AppSupportURL → `docs/user/README.md`)
- `_bmad-output/implementation-artifacts/4-2-end-user-documentation-for-first-midi-and-sysex.md` (modified)
- `_bmad-output/implementation-artifacts/sprint-status.yaml` (modified)

### Change Log

- 2026-08-10: Story 4.2 — shipped `docs/user/` manual, user-docs smoke guide, README/installer discoverability, Epic 3/4.1 fence pointers; status → review.
- 2026-08-10: Consolidated user manual into single `docs/user/unitor-mt4-bridge-user-guide.md` (YAML header, multi-H1, TOC); removed per-chapter files.
- 2026-08-10: Rewrote EN/FR user manuals: plain user-manual tone, positive “what to do” framing, FR gender « la MT4 », virtualMIDI branding aligned.
- 2026-08-10: Code review — applied docs/smoke/discoverability patches; attributed Bridge versioning to 4.1; status → done (SM-5 hardware rows 1–3 still blank).

### Review Findings

- [x] [Review][Decision] French community manual under `docs/user/` — **Resolved: keep FR as shipped peer** (align FR with EN on every docs patch).
- [x] [Review][Decision] C++ / CMake Bridge versioning in a docs story — **Resolved: attribute to Story 4.1 packaging; do not revert.** Keep `BridgeVersion*.in` / `--version` / CMake generation on disk; remove ownership from 4.2 File List / Completion Notes; finish wiring / lint under 4.1 (alongside existing `MyAppVersion` build-script work).
- [x] [Review][Patch] Add explicit works / does-not-work (V1 non-goals) to user guide(s) [`docs/user/unitor-mt4-bridge-user-guide.md`]
- [x] [Review][Patch] Restore smoke row 5 to works/does-not-work contract (undo “What you can do only” dilution) [`docs/tests/smoke-epic4-user-docs-mt4.md`]
- [x] [Review][Patch] State hot-plug honesty: rescan / supervised restart OK; Windows reboot = fail [`docs/user/unitor-mt4-bridge-user-guide.md` ~# Unplug]
- [x] [Review][Patch] State multi-MT4 validation honesty (single-unit proven / dual not claimed closed) in user prose [`docs/user/unitor-mt4-bridge-user-guide.md` ~# Two MT4]
- [x] [Review][Patch] Align smoke rows 6–7 Pass notes with actual guide wording after prose fixes [`docs/tests/smoke-epic4-user-docs-mt4.md`]
- [x] [Review][Patch] Fix broken deep-link fragments (`#hot-plug`, `#two-mt4-units`, `#install`) to real H1 anchors [`docs/tests/smoke-epic3-hotplug-mt4.md`, `docs/tests/smoke-epic3-dual-mt4-mt4.md`, `docs/dev/winusb-bind.md`]
- [x] [Review][Patch] Replace installer FinishedLabel repo-relative path with actionable GitHub Getting started URL [`installer/public-installer.iss`]
- [x] [Review][Patch] Add thin `docs/user/README.md` landing (start-here → EN guide; durable support URL) [`docs/user/README.md`]
- [x] [Review][Patch] Auto-Start: say Bridge is a user-session process, not a Windows Service / Session-0 [`docs/user/unitor-mt4-bridge-user-guide.md` ~# Auto-Start]
- [x] [Review][Patch] First SysEx / Computer Mode: state clearly that SysEx alone does not wake Computer Mode [`docs/user/unitor-mt4-bridge-user-guide.md`]
- [x] [Review][Patch] Soften README status so community install does not read as fully lab-closed while 4.1/4.2 hardware rows pending [`README.md`]
- [x] [Review][Patch] Remove BMad marketing footer from end-user manuals [`docs/user/unitor-mt4-bridge-user-guide.md`]
- [x] [Review][Patch] Fix French typo `rescannnez` → `rescanner` [`docs/user/unitor-mt4-bridge-manuel-utilisateur.md`]
- [x] [Review][Patch] Correct Completion Notes (C++ present vs “no C++”; lint honesty) [`_bmad-output/implementation-artifacts/4-2-end-user-documentation-for-first-midi-and-sysex.md`]
- [x] [Review][Patch] Point multiclient smoke fence at a real user-guide fragment once anchors exist [`docs/tests/smoke-epic3-multiclient-mt4.md`]
- [x] [Review][Defer] Bridge CMake `project(VERSION)` vs installer `MyAppVersion` dual sources [`CMakeLists.txt` / `installer/public-installer.iss`] — deferred, pre-existing packaging concern (Story 4.1 already tracks wiring `MyAppVersion` from build)
