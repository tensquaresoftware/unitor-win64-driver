---
baseline_commit: 06981b3
---

# Story 2.5: Session longevity design for ~4h studio use

Status: done

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a studio user,
I want the Bridge designed so a ~4-hour session including SysEx activity does not require a mandatory restart for normal use,
so that long writing/editing days stay trustworthy.

## Acceptance Criteria

1. **Given** channel MIDI, clock/MTC, and SysEx paths from Stories 2.1–2.4  
   **When** the Bridge is run under a continuous studio/editor scenario including SysEx Sessions  
   **Then** the implementation is designed and documented for about **4 hours** continuous use without mandatory Bridge restart for normal operation — NFR-R1 / CAP-17 / AD-18

2. **And** a stability sample plan exists for at least Win10 x64 (matrix mandatory), including SysEx activity

3. **And** known leak/restart failure modes discovered in the sample are logged as defects (not accepted as “usermode limits”)

4. **And** this story does not invent final latency thresholds (those remain Epic 5 / OQ-2)

**Traces:** CAP-17, NFR-R1, AD-18, SM-3

## Tasks / Subtasks

- [x] Task 1: Lock longevity design notes (AC: 1, 4)
  - [x] Document the V1 longevity contract in English under `docs/tests/smoke-epic2-longevity-mt4.md` (kebab-case) and/or a short companion note in `docs/dev/` if the smoke guide would become unreadable — prefer **one** operator-facing soak guide
  - [x] State explicitly: ~4 h continuous studio/editor use including SysEx Sessions must **not** require a **mandatory** Bridge restart for normal operation (NFR-R1 / SM-3 / AD-18)
  - [x] Inventory bounded resources already in tree (do not redesign blindly): `HostOutboundQueue` **128** msgs / **128×400** bytes; `kMaxSysexHoldBytes = 1024`; teVirtualMIDI max SysEx **65535**; session bulk timeout **3000 ms**; one reader thread + CLI poll
  - [x] Inventory known restart / hang modes from `deferred-work.md` that become soak-relevant: queue overflow → pump fail; oversize/nested-F0 SysEx → pump fail; incomplete SysEx hold with no `F7` (no idle timeout); `usbIoMutex_` held across decode (stall under load); CTRL_CLOSE may kill before `Stop` (orphan ports); console heartbeat every **3 s** (~4800 lines / 4 h if redirected)
  - [x] Explicit fences: Matrix-Control minimum vectors → **2.4**; short SysEx pipe/burst → **2.3**; clock/MTC short smokes → **2.1** / **2.2**; Auto-Start / hot-plug → Epic **3**; latency/jitter Studio-Done numbers → Epic **5** / OQ-2
  - [x] Cite SSOT: PRD NFR-R1 / SM-3 / §10 stability sample; AD-18; epics Story 2.5; `_bmad-output/specs/spec-unitor-win64-driver/validation-matrix.md` (“Stability sample: ~4h…”)

- [x] Task 2: Author the Win10 stability sample plan (AC: 2)
  - [x] In `docs/tests/smoke-epic2-longevity-mt4.md`, define a reproducible ~**4 hour** sample with Pass/Fail rows for at least Win10 x64 (mandatory); Win11 x64 when available
  - [x] Sample **must include SysEx activity** (not notes-only): e.g. periodic Inquiry / patch-shaped / short Matrix-Control or MIDI-OX SysEx exchanges interleaved with ordinary notes/CC; clock and/or MTC welcome when those lab rows are green
  - [x] Prerequisites: Epic 1 notes/CC green; Story **2.3** pipe+buffer green (synthetic minimum); prefer Story **2.4** hard gates green or in-progress with honest note if Matrix-Control vectors still open — longevity sample may use any host that can send librarian-scale SysEx, but record which host
  - [x] Procedure vehicle: leave `Bridge --start-session` / `--run-midi` running (alias pair already shipped — **do not** invent a mandatory new soak CLI unless lab proves the 3 s heartbeat / cancel path is unusable for a 4 h run)
  - [x] Pass bar: session reaches ~4 h without **mandatory** Bridge restart for normal operation; Virtual Ports remain usable; no Bridge-induced silent MIDI death that forces a restart to recover
  - [x] Fail bar: Bridge process crash, pump failure that stops the session, forced restart to restore ports, unbounded growth / resource exhaustion attributable to the Bridge, or SysEx dumps that require restart mid-session under normal editor pacing
  - [x] Record Bridge build identity / commit, OS, start/end wall times, hosts used, and English notes on any anomaly (Port N / cable / direction when applicable)
  - [x] Cross-link from `docs/tests/smoke-epic2-sysex-mt4.md`, `smoke-epic2-matrix-control-mt4.md`, `smoke-epic2-clock-mt4.md`, `smoke-epic2-mtc-mt4.md` (replace bare “→ 2.5” fences with the new guide path)

- [x] Task 3: Execute or honestly leave blank the Win10 sample (AC: 2, 3)
  - [x] When Win10 lab time allows: run the ~4 h sample; fill Pass/Fail; keep honesty bar (blank ≠ green)
  - [x] Do **not** claim SM-3 / NFR-R1 closed from synthetic-only work or a 20-minute smoke
  - [x] Optional shorter “confidence soak” (e.g. 30–60 min) may be recorded as interim evidence — it does **not** replace the ~4 h sample for AC closure
  - [x] Win11 rows: Skip/N/A when hardware absent; do not block Win10 Pass on Win11 absence

- [x] Task 4: Defect logging — no “usermode alibi” (AC: 3)
  - [x] Any leak, forced restart, pump fail, hang, or orphan-port mode discovered in the sample → file/append in `_bmad-output/implementation-artifacts/deferred-work.md` **or** fix in the same story if small and in-path
  - [x] Forbidden: accepting “usermode can’t do 4 h” / “VirtualMIDI limit” / “WinUSB always needs restart” without measurement and a logged defect
  - [x] Counter-metric SM-C4 still applies to **jitter** excuses (Epic 5); for **this** story the parallel rule is: restart/leak excuses are defects, not architecture destiny
  - [x] Raise priority of CTRL_CLOSE orphan-port teardown if the sample path is going to be shared publicly — still Epic-adjacent, but log honestly if hit

- [x] Task 5: Lab-gated hardening only (AC: 1, 3)
  - [x] Default assumption: Stories **2.1–2.3** already shipped transport + buffering; **2.4** owns Matrix-Control vectors — **do not** reinvent SysexEngine, second I/O pump, or unbounded queues
  - [x] If soak Fail points at Bridge: fix the **smallest** layer (`MidiSessionCli` heartbeat/logging, incomplete-SysEx idle policy, `HostOutboundQueue` / drain, `DeviceSession` mutex split, framer hold timeout) with evidence
  - [x] Incomplete SysEx idle hang (deferred from 2.3): add idle-timeout / English session failure **only if** soak or lab shows a real hang — do not invent a parser
  - [x] Heartbeat spam: if redirected logs become unusable over 4 h, throttle CLI heartbeat (e.g. 30–60 s) — keep counter prints on `send_fail` / first bulk / meaningful deltas
  - [x] Do **not** add Auto-Start, hot-plug recovery, multi-client policy, or MIDI Path harness work here

- [x] Task 6: Regression + quality (AC: all)
  - [x] `Bridge --test-mapper` still exit 0 (SysEx + realtime + MTC vectors)
  - [x] `ctest` / `BridgeTests` Pass when any C++ changed
  - [x] If any C++ changes: `python scripts/quality/lint-touched.py` exits 0; compile under `builds/`; no French in sources; Protocol/Profile free of VirtualMIDI/WinUSB
  - [x] Confirm CAP-8 still holds (no Matrix-Control link)
  - [x] Do **not** invent Studio-Done latency/jitter numbers; do not claim Epic 3 resilience done

### Review Findings

- [x] [Review][Patch] Power policy for multi-hour soak: disable sleep / USB selective suspend; void or Fail the sample if the host slept [`docs/tests/smoke-epic2-longevity-mt4.md`:104]
- [x] [Review][Patch] Fix `sprint-status.yaml` `last_updated` moved backward (`23:10:00Z` → `21:45:00Z`) [`_bmad-output/implementation-artifacts/sprint-status.yaml`:2,54]
- [x] [Review][Patch] Bilan honesty: do not mark “Soak defects logged” Pass without a measured sample Fail; add an open row that SM-3 / NFR-R1 closes only when Win10 ~4 h sample Pass [`docs/tests/smoke-epic2-longevity-mt4.md`:210]
- [x] [Review][Patch] Activity mix reproducibility: minimum SysEx + notes/CC cadence over ~4 h, mid-soak usability checks, and soft pacing floor (e.g. Matrix-Control ≥10 ms / not continuous flood) [`docs/tests/smoke-epic2-longevity-mt4.md`:149]
- [x] [Review][Patch] Pass/Fail hardening: minimum wall-clock floor under “~4 h”; Record fields required before Matrix Pass; Win10 mandatory to close SM-3 (Win11-only cannot); CTRL_CLOSE voids/Fails the run; any Fail before a later Pass needs `deferred-work.md`; incomplete-SysEx Bridge-vs-host classification; redirected-log / disk-full void; how to observe resource exhaustion [`docs/tests/smoke-epic2-longevity-mt4.md`:131]
- [x] [Review][Patch] Fix bounds markdown so `kMaxSysexHoldBytes = 1024` does not render literal asterisks [`docs/tests/smoke-epic2-longevity-mt4.md`:38]
- [x] [Review][Patch] Make `winusb-bind.md` `--start-session` note evergreen (path to longevity guide, no transient story **2.5** ID) [`docs/dev/winusb-bind.md`:98]
- [x] [Review][Patch] Deduplicate “Out of scope” vs “Explicit fences” into one ownership table [`docs/tests/smoke-epic2-longevity-mt4.md`:88]
- [x] [Review][Patch] Story File Structure still says sprint-status moves `backlog → ready-for-dev` — stale vs actual `review` [`2-5-session-longevity-design-for-4h-studio-use.md`:273]
- [x] [Review][Patch] `deferred-work.md` 2.5 section: keep open soak risks separate from the “document Fail→Stop (done)” note [`_bmad-output/implementation-artifacts/deferred-work.md`:3]

## Dev Notes

### Soft dependencies

| Dependency | Status | How this story treats it |
|---|---|---|
| Story **2.3** SysEx transport + burst buffering | **done** | Hard prerequisite for SysEx portion of design/sample — reuse pipe + `HostOutboundQueue` |
| Story **2.4** Matrix-Control minimum vectors | **in-progress** (lab) | Soft — prefer hard gates green before claiming SM-3 “studio editor day”; sample may use MIDI-OX/SysEx tools if Matrix-Control lab still open; record honesty |
| Stories **2.1** / **2.2** clock + MTC | **review** (Win10 DAW lab pending) | Soft — synthetic gates must stay green; include clock/MTC in soak when hardware rows exist; do not block longevity **design** on DAW lab blanks |
| Epic 1 notes/CC all-ports | lab OK | Baseline Virtual Ports + Computer Mode path |
| Win10 x64 lab time (~4 h wall clock) | scheduling | Required to **close** the sample matrix; design + plan can land first with blank rows |

### Scope fence

This story lands **SM-3 / NFR-R1 / CAP-17 design + soakable sample criteria** (and lab evidence when run). It is **not** a new MIDI message class and **not** Epic 5 timing proof.

| In scope | Out of scope (later / never) |
|---|---|
| Longevity design documentation (restart policy, bounds, failure modes) | Rebuilding transparent SysEx / clock / MTC paths |
| Win10 ~4 h sample **plan** including SysEx activity | Making bank stress (~100× 275 B) a hard gate (still optional / 2.4) |
| Running the sample when lab allows; honest blank rows | Claiming SM-3 from a short smoke or synthetic-only |
| Logging soak defects as defects | Accepting forced restarts as “usermode limits” |
| Lab-gated minimal fixes if soak Fail is Bridge-induced | Oberheim parser / SysexEngine / opcode allowlist |
| Optional CLI heartbeat throttle for long runs | New mandatory `--soak` flag (only if proven needed) |
| Cross-links from Epic 2 smoke guides | Auto-Start → **3.1**; hot-plug → **3.2**; multi-client → **3.3** |
| | Public Installer / MSI → **4.1** / OQ-1 |
| | MIDI Path latency/jitter harness / Studio-Done numbers → Epic **5** / AD-11 / OQ-2 |
| | Matrix-Control as Bridge dependency (forbidden — CAP-8) |

### Epic context

Epic 2 outcome: Validation Matrix DAWs can use clock / Start-Stop-Continue / MTC, and Matrix-Control can complete **minimum SysEx pass vectors** without Bridge restart for normal librarian use — **with buffering designed for ~4h sessions**.

Story **2.5** closes the epic’s longevity commitment (SM-3). Implementation readiness note: physical 4 h proof is **acceptance evidence**; the story must deliver design + soakable criteria so “works for 20 minutes” cannot ship as NFR-R1.

Pipeline (unchanged):

```text
DAW / MIDI-OX / Matrix-Control ↔ Virtual Ports ↔ VirtualMidiBackend
  ↔ MidiMessageFramer (device→host) / HostOutboundQueue (host→device)
  ↔ EmagicCableMapper ↔ WinUsbTransport ↔ MT4
```

Long-run entrypoint (unchanged):

```text
Bridge --start-session   # alias: --run-midi
  → MidiSessionCli::runMt4MidiSession
  → DeviceSession::Start → reader thread + 50 ms CLI poll + 3 s heartbeat
  → Ctrl+C / cancel → Stop (join reader, DestroyPortSet, Close)
```

### What “~4h longevity” means

**~4 hours** = about one long writing/editing block (PRD NFR-R1 / SM-3 / §10), **including SysEx Session activity**, without a **mandatory** Bridge restart for normal operation.

| Term | Meaning for Pass/Fail |
|---|---|
| Mandatory restart | Operator must kill/relaunch Bridge to keep using Virtual Ports under normal studio/editor traffic |
| Normal operation | Notes/CC, clock/MTC when in use, Matrix-Control / librarian SysEx at stock pacing — not intentional abuse (e.g. continuous nested-F0 flood) |
| Supervised restart | Allowed for **hot-plug** recovery stories (Epic 3 / AD-10) — **not** an excuse for steady-state longevity Fail |
| Sample plan | Written procedure + Pass/Fail matrix an operator can execute without inventing steps |
| Defect | Crash, leak, pump fail, hang, or forced restart attributable to Bridge — logged, not hand-waved |

**Nuance:** AD-18 already binds buffering (NFR-R3) from Story 2.3. This story binds the **time horizon** and **sample evidence**, not a second buffer redesign.

[Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — NFR-R1, SM-3, §10]  
[Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-18]  
[Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Story 2.5]

### Architecture compliance (must follow)

| Decision | What it means for this story |
|---|---|
| AD-1 | Usermode only — no custom kernel MIDI |
| AD-2 | No VirtualMIDI/WinUSB headers in Protocol/Profile |
| AD-7 | teVirtualMIDI remains V1 MidiBackend |
| AD-9 | Only live DeviceSession creates/destroys ports — soak must not leave orphan ports on clean Ctrl+C Stop |
| AD-11 | Do not invent latency/jitter Studio-Done numbers here |
| AD-14 | No GPL Linux sources in tree |
| AD-15 | PascalCase under `src/`; lint-touched; English only |
| AD-16 | Transparent SysEx preserved — soak must not introduce Oberheim rewrite |
| AD-17 | Clock/MTC classes remain first-class if exercised in the sample |
| AD-18 | ~4 h without mandatory restart; librarian dumps buffered (already 2.3); incomplete/corrupt dumps = failures |
| AD-20 | User-session Bridge exe (not Session-0 service) |
| CAP-8 | Matrix-Control is external validation host only |
| CAP-17 | Same longevity + buffering capability as NFR-R1/R3 |

[Source: `ARCHITECTURE-SPINE.md` — AD-18 binds NFR-R1, NFR-R3, SM-3]

### CRITICAL — Reuse, do not reinvent

Already in tree after Stories **2.1–2.3** (and 2.4 checklist):

1. Transparent USB ↔ VirtualMIDI pump (`DeviceSession` + `VirtualMidiBackend`)
2. `MidiMessageFramer` with `kMaxSysexHoldBytes = 1024`, oversize/nested-F0 reject counters, realtime/`0xF1` interleave
3. `HostOutboundQueue` (128 msgs / 128×400 bytes) + `DeviceSessionHostOutbound` drain with `try_lock`
4. Long-run CLI `--start-session` / `--run-midi` (`MidiSessionCli`) — **this is the soak vehicle**
5. Epic 2 smoke scaffolds: clock / MTC / sysex / matrix-control docs (fence longevity here)
6. Bounded failure modes: queue overflow and oversize SysEx already force English pump failure (visible Fail, not silent)

**Forbidden reinventing:**

- Second I/O pump or “LongevityManager” service beside DeviceSession
- Unbounded queues “so 4 h never overflows”
- SysexEngine / Oberheim parser / opcode allowlist
- Claiming SM-3 done because Story 2.3 short burst passed or 2.4 vectors passed once
- Building `tools/midi-path-harness/` or publishing Studio-Done latency numbers
- Auto-Start / hot-plug / multi-client work under this story ID
- Linking Matrix-Control into the Bridge build

### CRITICAL — Existing code current state (read before changing)

| Component | Current state | This story |
|---|---|---|
| `src/App/MidiSessionCli.cpp` | 50 ms poll; **3 s** heartbeat always; Ctrl+C/Break/Close sets cancel; clean path calls `Stop` | Primary soak vehicle; throttle heartbeat **only if** needed; harden cancel path **only if** soak proves CTRL_CLOSE orphan issue in-scope |
| `DeviceSession` | One reader thread; Start/Stop join; clear queue on Stop; destructor Stop | Preserve lifecycle; fix only on soak Fail |
| `HostOutboundQueue` | Bounded 128 / 51200 bytes; overflow → pump fail | Keep bounded — overflow during normal soak = defect to diagnose, not “raise to millions” |
| `MidiMessageFramer` | Hold 1024; no incomplete-SysEx idle timeout | Idle-timeout only if lab hang |
| `WinUsbTransport` session timeout | `kSessionBulkTimeoutMs = 3000` (nonzero PIPE_TRANSFER_TIMEOUT — good for responsive long runs) | Do not set timeout to 0 (indefinite hang risk) without evidence |
| `docs/tests/smoke-epic2-*.md` | Longevity fenced to 2.5 | Point at new longevity guide |
| Epic 5 harness | Not started | Untouched |

**What must be preserved:**

- Notes/CC all-ports path
- Clock + MTC + SysEx synthetic vectors
- TX/RX mapping, Port N naming, Computer Mode kick
- CAP-8; layer isolation; `builds/` output
- Fail-closed missing VirtualMIDI / WinUSB bind
- Visible Fail on queue overflow / oversize SysEx (do not silence them to “pass” a soak)

**Must not break:**

- `Bridge --test-mapper` / Catch2 suite
- Windows CI without proprietary SDK in-repo
- Epic 1 hardware notes/CC path
- Story 2.3 short-burst without restart behavior
- Story 2.4 Matrix-Control checklist semantics

### Known deferred edges (soak-relevant)

From `deferred-work.md` — treat as **watch list** during design + sample:

| Edge | Relevance to 2.5 |
|---|---|
| Incomplete SysEx under hold with no `F7` (no idle timeout) | **In scope to observe**; fix if soak hangs |
| CTRL_CLOSE kills before `Stop` → orphan Virtual Ports | **Observe**; log defect; fix if sample path requires clean window-close |
| `processBulkRead` holds `usbIoMutex_` across decode | **Observe** under long clock/SysEx load; fix only with evidence |
| After pump fail, ports stay up until CLI ~50 ms poll `Stop` | Document expected Fail→Stop behavior in soak guide |
| Console heartbeat every 3 s | **Ops risk** for 4 h redirected logs — throttle if needed |
| Shared MidiBackend multi-session | Out → Epic **3** |
| F5 / `0xFF` raw MIDI escaping | Still out |
| Hot-plug without reboot | Out → **3.2** |

### Technical requirements

- **Language:** C++17; Allman; 4 spaces; `#pragma once`; English — `conventions.md` §6
- **Quality:** `scripts/quality/lint-touched.py` on any touched C++
- **Primary artifacts:** longevity smoke/sample guide + design notes sufficient for SM-3; code changes are **conditional**
- **Build:** if new smoke TUs added (unlikely), list in **both** Bridge and BridgeTests in `CMakeLists.txt`
- **No** committed proprietary VirtualMIDI SDK; **no** Matrix-Control link; **no** French in sources
- **CI:** long soak is **hardware-only / not CI** (`docs/dev/windows-ci-toolchain.md` already frames this)

### Library / framework requirements

| Use | Do not use |
|---|---|
| Existing `MidiSessionCli` / `--start-session` | New Session-0 Windows Service “for longevity” |
| Existing bounded `HostOutboundQueue` + framer | Unbounded memory growth “just in case” |
| Existing Epic 2 smoke doc pattern | Claiming SM-3 from Matrix-Control vector Pass alone |
| English diagnostics + defect log in `deferred-work.md` | Silent restart loops |
| Win10 x64 Validation Matrix row | Skipping Win10 because Win11 is “newer” |
| MIDI-OX / DAW / Matrix-Control as external hosts | Embedding Matrix-Control (CAP-8) |
| Nonzero WinUSB `PIPE_TRANSFER_TIMEOUT` (already 3000 ms) | Timeout 0 for bulk pipes on long sessions without rationale |

### File structure requirements

#### NEW (primary)

| Path | Why |
|---|---|
| `docs/tests/smoke-epic2-longevity-mt4.md` | Stability sample plan + Pass/Fail matrix for ~4 h including SysEx; design contract for operators |
| Optional short `docs/dev/` note | Only if design depth would drown the smoke guide — prefer keeping one operator doc |

#### UPDATE

| Path | Current state | This story |
|---|---|---|
| `docs/tests/smoke-epic2-sysex-mt4.md` | Longevity → 2.5 (bare) | Link concrete longevity guide |
| `docs/tests/smoke-epic2-matrix-control-mt4.md` | Longevity → 2.5 | Same |
| `docs/tests/smoke-epic2-clock-mt4.md` | Longevity → 2.5 | Same |
| `docs/tests/smoke-epic2-mtc-mt4.md` | Longevity → 2.5 | Same |
| `docs/dev/windows-ci-toolchain.md` | Mentions long soak not in CI | Confirm / clarify soak = lab-only evidence for 2.5 |
| `docs/dev/winusb-bind.md` | Flags table | Optional one-liner: leave `--start-session` up for longevity sample |
| `_bmad-output/implementation-artifacts/deferred-work.md` | Watch-list edges | Append soak-discovered defects |
| `_bmad-output/implementation-artifacts/sprint-status.yaml` | was backlog / ready-for-dev during create-story | Dev → `review` then `done` after code review |

#### Conditional UPDATE (lab-gated)

| Path | When |
|---|---|
| `src/App/MidiSessionCli.cpp` | Heartbeat too noisy for 4 h; or cancel/Stop race on window close |
| `src/Protocol/MidiMessageFramer.*` / DeviceSession device-host | Incomplete-SysEx idle hang observed |
| `src/Device/DeviceSessionDeviceHost.cpp` / HostOutbound | Mutex stall or overflow under realistic multi-hour load |
| `src/Usb/WinUsbTransportInit.cpp` | Timeout policy proven wrong for long runs (unlikely — keep nonzero) |

#### Likely untouched

| Path | Why |
|---|---|
| `src/Protocol/EmagicCableMapper.*` | Opaque carry already correct |
| `src/Profile/DeviceProfile.*` | No longevity capability bit |
| Matrix-Control repo | External host only |
| `tools/midi-path-harness/` | Epic **5** |

### Testing requirements

| Check | How |
|---|---|
| Design completeness | Smoke guide states restart policy, bounds, Fail modes, SysEx-included sample steps |
| Sample plan | Win10 x64 row exists; SysEx activity required; Pass/Fail columns; build identity recorded |
| Optional interim soak | 30–60 min may be noted; does not close AC alone |
| Full sample (lab) | ~4 h wall clock; no mandatory Bridge restart; ports usable at end |
| Defect logging | Any Fail → `deferred-work.md` entry or same-story fix |
| Synthetic regression | `Bridge --test-mapper` exit 0 if C++ touched |
| Catch2 | `ctest` / `BridgeTests` if C++ touched |
| Lint | `python scripts/quality/lint-touched.py` on touched C++ |
| Isolation | Grep Protocol/Profile: no VirtualMIDI / WinUSB |
| CAP-8 | No Matrix-Control in CMake / submodules / `#include` |
| Latency claims | None invented |

**Failure definition for this story:** Bridge-induced forced restart, crash, hang, leak, or unusable Virtual Ports before ~4 h under a normal studio/editor scenario that includes SysEx activity. Not Epic 5 p99 thresholds, not optional bank-stress hard fail, not hot-plug (→ 3.2), not “DAW host crashed so Bridge must restart” without Bridge fault evidence.

### Previous story intelligence

From Story **2.4** (`in-progress`, checklist landed):

- Pattern: operator smoke guide under `docs/tests/` with Pass/Fail matrix, explicit fences, English Fail notes with Port N / cable / Bridge build identity
- Longevity explicitly fenced here — do not claim ~4 h because Matrix-Control vectors passed once
- Soft dep on 2.3 pipe; CAP-8 strict
- Review culture: honest Skip/blank rows; Win10 mandatory

From Story **2.3** (`done`):

- Delivered buffering (AD-18 / NFR-R3) + oversize observability + `HostOutboundQueue`
- Explicitly deferred ~4 h soak to **this** story
- Incomplete-SysEx idle hang still open — primary longevity watch item
- Prove-don’t-reinvent; bounded queues with visible overflow Fail

From Stories **2.1** / **2.2** (`review`):

- Short smokes ≠ multi-hour soak
- Mutex stall under dense clock remains deferred until evidence
- Synthetic green + blank Win10 DAW rows honesty bar carries forward

From Epic 1 / deferred-work:

- CTRL_CLOSE orphan ports still open — raise visibility if soak uses window-close
- `--start-session` / `--run-midi` are the established long-run path

### Git intelligence summary

Recent relevant commits:

- `06981b3` — Add Matrix-Control SysEx pass-vector smoke checklist for story 2.4
- `cd43225` — Add transparent SysEx transport with burst buffering for story 2.3
- `1c93de3` — Prove MTC quarter-frame and full-frame sync for story 2.2
- `4814f82` — Prove MIDI clock and transport realtime for story 2.1

Patterns to extend: Epic 2 `docs/tests/smoke-epic2-*.md` scaffolds, explicit out-of-scope fences, lab honesty (blank ≠ Pass), conditional C++ only with evidence, raise deferred items when the owning story arrives (**now** for soak-visible edges).

### Latest tech information

- WinUSB bulk pipes: prefer **nonzero** `PIPE_TRANSFER_TIMEOUT` so stalled transfers cancel rather than blocking the reader forever; this repo already uses **3000 ms** for session bulk — keep that posture unless soak proves otherwise ([Microsoft WinUSB pipe policy docs](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-functions-for-pipe-policy-modification))
- teVirtualMIDI: complete SysEx per send; frames longer than create-port `maxSysexLength` are discarded by the driver — V1 uses **65535**
- Matrix-Control stock pacing remains **≥10 ms** between SysEx; bank export is many complete frames, not one multi-megabyte message — longevity stress is **time + repeated activity**, not one giant hold
- Do not use ASIO buffer size as MIDI longevity or timing proof (SM-C2 / AD-11)
- Provisional latency/jitter anchors remain **unmeasured** until Epic 5 — do not quote them as proven in the soak bilan

### Project context reference

- `conventions.md` §3 quality gate, §6 C++ style
- No `project-context.md` in-repo yet — follow architecture spine + conventions
- Keep hardware/protocol names in English (MT4, WinUSB, VirtualMIDI, MIDI, SysEx, Matrix-Control, Ableton, Reason, MIDI-OX)

### Anti-patterns to forbid

- Claiming SM-3 / NFR-R1 done from a short smoke or synthetic-only run
- Accepting forced Bridge restarts as inevitable “usermode limits”
- Unbounded queues or raising hold to 65535 “for longevity” without measured need
- Building a LongevityEngine / second pump / Session-0 service
- Linking Matrix-Control (CAP-8)
- Inventing Studio-Done latency/jitter numbers
- Silently swallowing queue overflow / oversize rejects so the soak “looks green”
- Doing Auto-Start / hot-plug / multi-client under this story
- French comments; kebab-case sources under `src/`
- Committing proprietary VirtualMIDI SDK binaries

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Epic 2 / Story 2.5]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — NFR-R1, SM-3, §10 stability sample]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-18]
- [Source: `_bmad-output/planning-artifacts/implementation-readiness-report-2026-08-04.md` — Story 2.5 = design + soakable criteria]
- [Source: `_bmad-output/specs/spec-unitor-win64-driver/validation-matrix.md` — Stability sample line]
- [Source: `_bmad-output/implementation-artifacts/2-3-transparent-sysex-transport-with-burst-buffering.md`]
- [Source: `_bmad-output/implementation-artifacts/2-4-matrix-control-minimum-sysex-pass-vectors.md`]
- [Source: `_bmad-output/implementation-artifacts/deferred-work.md`]
- [Source: `src/App/MidiSessionCli.cpp`, `src/Device/HostOutboundQueue.h`, `src/Protocol/MidiMessageFramer.h`]

## Dev Agent Record

### Agent Model Used

Composer (Cursor agent router)

### Debug Log References

- `builds/debug/Bridge --test-mapper` → exit 0 (“Mapper synthetic tests passed”)
- `ctest --test-dir builds/debug` → BridgeTests Passed
- No C++ sources modified; lint-touched N/A for this turn
- CAP-8: no Matrix-Control references in CMakeLists.txt

### Completion Notes List

- Landed single operator-facing longevity guide with V1 contract, bounds inventory, soak watch list, Pass/Fail matrix (Win10 mandatory + SysEx required), and explicit fences to 2.1–2.4 / Epic 3 / Epic 5.
- Cross-linked Epic 2 smoke guides + clarified CI soak = lab-only + `--start-session` one-liner in winusb-bind.
- Win10 ~4 h sample rows left **blank** (honesty bar); did **not** claim SM-3 closed from synthetic-only.
- Elevated soak-relevant defect watch list in `deferred-work.md`; no measured soak Fail this turn → no lab-gated C++ hardening.
- No Studio-Done latency/jitter numbers invented; no Auto-Start / hot-plug / multi-client / LongevityManager.

### File List

- `docs/tests/smoke-epic2-longevity-mt4.md` (new)
- `docs/tests/smoke-epic2-sysex-mt4.md`
- `docs/tests/smoke-epic2-matrix-control-mt4.md`
- `docs/tests/smoke-epic2-clock-mt4.md`
- `docs/tests/smoke-epic2-mtc-mt4.md`
- `docs/dev/windows-ci-toolchain.md`
- `docs/dev/winusb-bind.md`
- `_bmad-output/implementation-artifacts/deferred-work.md`
- `_bmad-output/implementation-artifacts/sprint-status.yaml`
- `_bmad-output/implementation-artifacts/2-5-session-longevity-design-for-4h-studio-use.md`

### Change Log

- 2026-08-05: Story 2.5 — longevity design + Win10 soak plan landed; sample matrix blank pending lab; soak watch list elevated; status → review
- 2026-08-05: Code review — applied 10 doc patches (soak Pass/Fail hardening, bilan honesty, evergreen bind note); status → done
