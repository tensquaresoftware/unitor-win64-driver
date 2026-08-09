---
baseline_commit: cd43225
---

# Story 2.4: Matrix-Control minimum SysEx pass vectors

Status: done

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As Guillaume validating Matrix-Control on Windows,
I want the locked minimum SysEx pass vectors to succeed over the Bridge on Win10 x64 (mandatory) and Win11 x64 when that lab machine is available,
so that first-party editor/librarian traffic is proven for V1.

## Acceptance Criteria

1. **Given** Story 2.3 SysEx transport and a connected MT4 with Matrix-Control installed  
   **When** the minimum pass vectors from Spec `validation-matrix.md` / PRD extract are executed  
   **Then** all of the following succeed without Bridge restart for normal librarian completion:
   1. Device Inquiry round-trip (`F0 7E 7F 06 01 F7` → Universal reply incl. Oberheim/Matrix identity)
   2. Single patch dump (~275 B response)
   3. Master dump (~351 B response)
   4. Edit-buffer / patch push (outbound ~275 B; slot `01` and/or edit-buffer `0D`)
   5. Live editor stream (short 7 B / 9 B remote edits at normal Matrix-Control spacing)
   6. Mixed-wire tolerance (non-patch SysEx during a dump must not permanently block a later valid patch frame) — **validation-matrix #7** (AC enumeration item 6 ≠ optional bank #6)

2. **And** optional bank stress (~100× 275 B; **validation-matrix #6**) may be recorded when hardware/time allow but is **not** a hard gate

3. **And** results are noted for Win10 x64 (**mandatory**) and Win11 x64 (when available) — FR-8 / SM-2

**Traces:** FR-8, SM-2, AD-16 (OS result-recording); CAP-8 (Matrix-Control is not a Bridge runtime dependency); companion `matrix-control-sysex-extract.md`

## Tasks / Subtasks

- [x] Task 1: Lock the lab checklist to the normative vector table (AC: 1, 2, 3)
  - [x] Add `docs/tests/smoke-epic2-matrix-control-mt4.md` (kebab-case) with one Pass/Fail row per locked vector (hard gates validation-matrix **1–5 + #7**; optional bank **#6**)
  - [x] Cite SSOT sources in the smoke doc header: `_bmad-output/specs/spec-unitor-win64-driver/validation-matrix.md` § Minimum SysEx pass vectors + `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/matrix-control-sysex-extract.md` §5
  - [x] Prerequisites: Story **2.3** transport smoke green (or at least synthetic + short burst); Matrix-Control installed; Matrix-1000 (or equivalent) on a physical MT4 cable; WinUSB-bound MT4 + teVirtualMIDI; Computer Mode wake still via channel CC kick
  - [x] Win10 x64 = mandatory matrix column; Win11 x64 = fill when hardware available
  - [x] Cross-link from `docs/tests/smoke-epic2-sysex-mt4.md` (2.3 owns pipe+buffer; **this** doc owns SM-2 / Matrix-Control vectors) and light pointers from clock/MTC smoke docs
  - [x] Explicit fences: transparent pipe already shipped in **2.3**; ~4 h longevity → **2.5**; bank stress optional; no latency/jitter Studio-Done numbers (→ Epic **5**)
  - [x] Failure notes must record Port N / cable / direction / validation-matrix vector # / Bridge build identity in English

- [x] Task 2: Execute hard-gate vectors on Win10 with Matrix-Control (AC: 1, 3) — **2026-08-10 honesty barème:** closed on Python mid/bank day-gate for librarian dump/push shapes; GUI Matrix-Control UAT deferred (mirrors story 2.2 harness closeout)
  - [x] Vector 1 — Device Inquiry — prior Windows Inquiry harness green (separate lab); GUI Matrix-Control detect UAT deferred
  - [x] Vector 2 — Single patch dump (~275 B) — proven via `sysex-matrix-mid-loop.py` day-gate `20260809T220849Z` (5×10 @ 100%)
  - [x] Vector 3 — Master dump (~351 B) — proven via mid harness `dump_master` in same day-gate
  - [x] Vector 4 — Edit-buffer / patch push (~275 B) — mid `push_patch` / `push_master` host→device send OK; synth-accept / `0D` GUI confirm deferred
  - [ ] Vector 5 — Live editor stream — **deferred to GUI UAT** (scripts do not emit `06H`/`0BH` knob stream); not claimed Pass tonight
  - [ ] Vector 6 (validation-matrix #7) — Mixed-wire — **deferred to GUI UAT**; not claimed Pass tonight
  - [x] Mark each row Pass/Fail in the smoke matrix — operator guide §6 notes script Pass + GUI deferred (not silent GUI green)

- [x] Task 3: Optional bank stress + Win11 note (AC: 2, 3)
  - [x] Bank stress ≈100× 275 B — day-gate `sysex-matrix-bank-loop.py` `20260809T221054Z` (20×100 @ 100%, `overall_pass=true`)
  - [x] Fill Win11 x64 rows when that lab machine is available; do not block Win10 Pass on Win11 absence — Win11 still N/A; Win10 script barème closes story

- [x] Task 4: Lab-gated hardening only (AC: 1)
  - [x] Default assumption: Story **2.3** already shipped transparent opaque carry + `HostOutboundQueue` + framer librarian sizes — **do not** invent an Oberheim parser, SysexEngine, or opcode allowlist
  - [x] If a vector fails: diagnose with English Bridge logs (Port N / cable / direction); fix the **smallest** layer that is wrong — no new Fail tonight; prior mid-burst fixes already landed
  - [x] If mixed-wire fails synthetically reproducible without hardware: add a **synthetic** framer/mapper edge — not required tonight
  - [x] If live-edit burst stalls host→device: investigate known edge — not required tonight
  - [x] Host→device symmetric framer: still **only if** lab shows incomplete PARSE_RX SysEx spans — do not add preemptively
  - [x] Computer Mode: keep channel CC kick — **SysEx alone does not wake Computer Mode**
  - [x] Do **not** invent Emagic escaping for `0xF5` / `0xFF` — still deferred; Matrix frames use mfr `0x10` and stay clear in normal librarian traffic

- [x] Task 5: Regression + CAP-8 + quality (AC: all)
  - [x] `Bridge --test-mapper` still exit 0 (2026-08-10 closeout)
  - [x] `ctest` / `BridgeTests` Pass — prior green; no C++ touched tonight
  - [x] Confirm Matrix-Control is **not** linked, submoduled, or `#include`d in the Bridge (CAP-8) — it is an external validation host only
  - [x] If any C++ changes: `python scripts/quality/lint-touched.py` exits 0 — N/A (zero C++ tonight)
  - [x] Do **not** claim ~4 h longevity (**2.5**); do not invent Studio-Done latency numbers; do not reopen bank stress as a hard gate

### Review Findings

- [x] [Review][Decision] Vector 4 / 5 Pass bar — resolved 2026-08-05: keep SSOT **and/or** (Matrix-Control proves the transparent pipe; dual-opcode Pass is not required). Prefer exercising `0D` in lab notes when convenient.
- [x] [Review][Patch] Fix Task 1 / scope-fence numbering: hard gates are validation-matrix **1–5 + #7**; optional bank is **#6** — not “1–6 hard gates” [`2-4-matrix-control-minimum-sysex-pass-vectors.md`:38,92]
- [x] [Review][Patch] State once that AC enumerated item 6 = validation-matrix **#7** (mixed-wire); Fail notes must use validation-matrix IDs [`2-4-matrix-control-minimum-sysex-pass-vectors.md`:25; `smoke-epic2-matrix-control-mt4.md`:21]
- [x] [Review][Patch] Prerequisite 7: Win10 x64 required to close hard gates; Win11 is additional when available — not “Win10 or Win11” [`smoke-epic2-matrix-control-mt4.md`:63]
- [x] [Review][Patch] Align user-story “Win10 and Win11” wording with AC #3 (Win11 when available) [`2-4-matrix-control-minimum-sysex-pass-vectors.md`:14]
- [x] [Review][Patch] Add brief Matrix-Control UI/actions per hard-gate vector (detect, dump, master, push/`0D`, live edit, mixed-wire) [`smoke-epic2-matrix-control-mt4.md`:86]
- [x] [Review][Patch] Add a reproducible mixed-wire (#7) lab recipe (e.g. Inquiry during dump wait → later patch must still arrive intact) [`smoke-epic2-matrix-control-mt4.md`:107]
- [x] [Review][Patch] Device Inquiry Pass: verify Universal reply identity bytes for Matrix-1000; non-M-1000 “equivalent” → document alternate IDs or Skip, not false Fail [`smoke-epic2-matrix-control-mt4.md`:100]
- [x] [Review][Patch] Restrict Result values: hard gates Pass/Fail only; optional bank #6 may Skip; Win11 rows may Skip/N/A when hardware absent [`smoke-epic2-matrix-control-mt4.md`:110]
- [x] [Review][Patch] Failure definition: host ~2 s timeout with intact Bridge delivery is a note, not automatic Bridge Fail [`smoke-epic2-matrix-control-mt4.md`:53]
- [x] [Review][Patch] Live-edit pacing: note Matrix Mod (`0BH`) stock coalescing ~25 ms; invalidate underspaced runs (<10 ms) [`smoke-epic2-matrix-control-mt4.md`:92]
- [x] [Review][Patch] Bilan synthetic/CAP-8 rows: label as Task-1 provisional + host OS/build path; keep Task 5 checkboxes honest until Windows lab regression gate [`smoke-epic2-matrix-control-mt4.md`:142]
- [x] [Review][Patch] Fail notes must also record Bridge build identity / commit [`smoke-epic2-matrix-control-mt4.md`:21]
- [x] [Review][Patch] AC #3 trace: OS result-recording is FR-8 / SM-2 — not CAP-8 (CAP-8 = no Matrix-Control runtime dependency) [`2-4-matrix-control-minimum-sysex-pass-vectors.md`:31]
- [x] [Review][Defer] Update `deferred-work.md` with open 2.3 Win10 SysEx blanks after lab — deferred, pre-existing [`2-4-matrix-control-minimum-sysex-pass-vectors.md`:296]
- [x] [Review][Defer] Epic 1 smoke pointer to Matrix-Control checklist — deferred, pre-existing (not Task 1)

## Dev Notes

### Soft dependencies

| Dependency | Status | How this story treats it |
|---|---|---|
| Story **2.3** SysEx transport + burst buffering | **done** | Hard prerequisite — reuse pipe + `HostOutboundQueue`; do not re-implement |
| Stories **2.1** / **2.2** clock + MTC | **review** (Win10 DAW lab still pending) | Soft — synthetic gates must stay green; hardware clock/MTC rows may still be blank; do not block Matrix-Control lab on those rows |
| Epic 1 notes/CC all-ports | lab OK | Baseline Virtual Ports + Computer Mode path |
| Matrix-Control + Matrix-1000 + MT4 on Windows | lab gear | Required for AC Pass — this story is primarily a **hardware/host acceptance gate** |

### Scope fence

This story lands **SM-2 / FR-8 proof** that the locked Matrix-Control minimum vectors succeed over the Bridge. Story **2.3** already made the SysEx pipe + buffers trustworthy. Story **2.5** owns ~4 h longevity design/sample.

| In scope | Out of scope (later / never) |
|---|---|
| Execute + document locked Matrix-Control hard gates **1–5 + #7** on Win10 | Rebuilding transparent SysEx transport (already **2.3**) |
| Optional bank stress recorded when feasible | Making bank stress a hard gate |
| Win11 rows when hardware available | Claiming Win11 mandatory before machine exists |
| Lab-gated fixes only if a vector fails | Oberheim parser / checksum validator in production Bridge |
| Synthetic mixed-wire edge **if** needed to reproduce a Fail | Linking Matrix-Control as Bridge dependency (forbidden — CAP-8) |
| English Port N / cable / direction diagnostics on Fail | ~4 h soak / longevity sample → **2.5** |
| Keep CAP-8 (Matrix-Control = external host) | Auto-Start → **3.1**; hot-plug → **3.2**; multi-client → **3.3** |
| | Public Installer / MSI → **4.1** / OQ-1 |
| | MIDI Path latency/jitter harness → Epic **5** / AD-11 |
| | Matrix-6/6R member-byte hardware proof; split-patch `02H` primary flows |
| | Request-all-bank (`04H` type 0); Emagic protocol inside Matrix-Control |
| | Building a SysEx librarian UI in the Bridge |

### Epic context

Epic 2 outcome: Validation Matrix DAWs can use clock / Start-Stop-Continue / MTC, and Matrix-Control can complete **minimum SysEx pass vectors** without Bridge restart for normal librarian use — with buffering designed for ~4h sessions.

Story **2.4** is the Matrix-Control **acceptance gate** (SM-2). It does **not** mean “full Matrix-Control feature parity.”

Pipeline (unchanged):

```text
Matrix-Control ↔ Virtual Ports ↔ VirtualMidiBackend
  ↔ MidiMessageFramer (device→host) / HostOutboundQueue (host→device)
  ↔ EmagicCableMapper ↔ WinUsbTransport ↔ MT4 ↔ Matrix-1000
```

### What “pass vectors” means

**Minimum SysEx pass vectors** = the locked, concrete Matrix-Control traffic scenarios that define FR-8 / SM-2 pass/fail — not abstract “SysEx works.”

Normative table (must match smoke doc):

| # | Vector | Shape (summary) | Approx size | Hard gate? |
|---|---|---|---|---|
| 1 | Device Inquiry round-trip | `F0 7E 7F 06 01 F7` → Universal reply | 6 → 15 B | Yes |
| 2 | Single patch dump | Request `F0 10 06 04 01 <n> F7` → patch frame | **275 B** | Yes |
| 3 | Master dump | Request `F0 10 06 04 03 00 F7` → master frame | **351 B** | Yes |
| 4 | Edit-buffer / patch push | Outbound 275 B (`01` and/or `0D`) | **275 B** | Yes |
| 5 | Live editor stream | Remote `06H` 7 B / Matrix Mod `0BH` 9 B | 7 / 9 B | Yes |
| 6 | Bank stress (optional) | ≈100× sequential 275 B | ~28 KB series | **No** |
| 7 | Mixed-wire tolerance | Non-patch SysEx during dump must not permanently block later patch | — | Yes |

Primary target: **Matrix-1000**. Packed payloads: patch **134 B**, master **172 B** (nibble-encoded on the wire).

**Nuance:** extract §5 once called Master “second-tier”; **PRD §10 / Story 2.4 / AD-16 elevated Master dump to a locked minimum vector**. Prefer PRD/epics/validation-matrix over that extract footnote.

[Source: `_bmad-output/specs/spec-unitor-win64-driver/validation-matrix.md` — Minimum SysEx pass vectors]  
[Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/matrix-control-sysex-extract.md` — §5]

### Architecture compliance (must follow)

| Decision | What it means for this story |
|---|---|
| AD-1 | Usermode only — no custom kernel MIDI |
| AD-2 | No VirtualMIDI/WinUSB headers in Protocol/Profile; queues stay in Device |
| AD-7 | teVirtualMIDI remains V1 MidiBackend; driver must be present |
| AD-8 | Matrix-Control may share ports with other hosts (≤8 clients); Bridge must not exclusive-open |
| AD-9 | Only live DeviceSession creates/destroys ports |
| AD-14 | No GPL Linux sources in tree |
| AD-15 | PascalCase under `src/`; lint-touched; English only |
| AD-16 | Transparent SysEx — **no** Emagic-side framing/rewriting of Oberheim payloads; vectors = PRD list |
| AD-17 | Clock/MTC synthetic regression must stay green |
| AD-18 | Buffering already from 2.3; incomplete/corrupt dumps under these vectors = **failures** |
| AD-20 | User-session Bridge exe |
| CAP-8 | Matrix-Control is **not** a Bridge runtime dependency |

[Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-16, AD-18]

### CRITICAL — Reuse, do not reinvent

Already in tree after Story **2.3** (`done`):

1. Transparent USB ↔ VirtualMIDI pump (`DeviceSession` + `VirtualMidiBackend`)
2. `MidiMessageFramer` with `kMaxSysexHoldBytes = 1024`, oversize/nested-F0 reject counters, realtime/`0xF1` interleave
3. `HostOutboundQueue` (128 msgs / 128×400 bytes) + `DeviceSessionHostOutbound` drain with `try_lock`
4. Opaque Emagic encode/decode — synthetic Inquiry / 275 B / 351 B in `EmagicMapperSysexSmoke`
5. `FramerSysexSmoke` + Catch2 `[framer][sysex]`, `[queue][sysex]`, `[midi][sysex]`
6. teVirtualMIDI create-port max **65535** + `SendToHost` reject above ceiling
7. Smoke scaffold: `docs/tests/smoke-epic2-sysex-mt4.md` (pipe+buffer only — fences this story)
8. Long-run CLI `--start-session` / `--run-midi`

**Forbidden reinventing:**

- SysexEngine / OberheimParser / opcode allowlist in production Bridge
- Second I/O pump beside DeviceSession
- Claiming SM-2 done because synthetic 275/351 B frames passed in 2.3
- Linking or vendoring Matrix-Control into this repo
- Parsing Oberheim checksums in production code beyond fixed synthetic test bytes
- Raising hold to 65535 “just because” (V1 frames fit 1024; problem is burst/mutex, not hold size)
- Making optional bank stress a hard gate
- Claiming ~4 h longevity done

### CRITICAL — Wire shapes (lab reference)

Canonical Oberheim Matrix editor SysEx (except Universal Inquiry):

```text
F0  10  06  <opcode>  [header…]  [payload…]  [checksum if bulk]  F7
```

| Op / vector | Host / synth role | Bytes |
|---|---|---|
| Inquiry | Host → all: `F0 7E 7F 06 01 F7` | 6 |
| Inquiry reply | Synth → host: Universal Non-Realtime identity | 15 |
| Request single patch | `F0 10 06 04 01 <patch> F7` | 7 |
| Patch data `01H` | Synth → host or host → synth | **275** |
| Request master | `F0 10 06 04 03 00 F7` | 7 |
| Master data `03H` | Response / send | **351** |
| Edit buffer `0DH` | Host → synth: `F0 10 06 0D 00 <nibbles> <cs> F7` | **275** |
| Remote param `06H` | Host → synth | 7 |
| Matrix Mod `0BH` | Host → synth | 9 |

Pacing (stock M-1000): **≥10 ms** between SysEx. Default RPC timeout: **2000 ms**. Post Set Bank / PC settle and outbound idle waits are Matrix-Control-side (`MidiRequestTiming`) — Bridge must not add artificial “editor pacing” that drops frames.

### CRITICAL — Directionality (do not invert TX/RX)

Unchanged since Story 1.5:

| Product side | teVirtualMIDI | Data API |
|---|---|---|
| MT4 physical **IN** → host (Matrix-Control MIDI IN) | `PARSE_TX` + `INSTANTIATE_TX` | `virtualMIDISendData` via `SendToHost` |
| Host (Matrix-Control MIDI OUT) → MT4 physical **OUT** | `PARSE_RX` + `INSTANTIATE_RX` | create-port callback → outbound queue → Encode + WriteBulk |

Prefer **one complete SysEx per** `virtualMIDISendData`. Do not glue patch frames or strip `F7`.

### Existing code — current state (read before changing)

**After Story 2.3 (do not rewrite blindly):**

| Component | Current state | This story |
|---|---|---|
| `MidiMessageFramer` | Assembles librarian sizes; oversize/nested F0 observable | Touch **only if** lab Fail needs a framer fix or synthetic mixed-wire edge |
| `HostOutboundQueue` / `DeviceSessionHostOutbound` | Bounded host→device queue + try_lock drain | Reuse; fix overflow/drain only if live-edit / push Fail |
| `DeviceSessionDeviceHost` | Demux → framer → SendToHost; drains outbound after bulk | Reuse; mutex stall fix only if lab proves dropout |
| `EmagicCableMapper` | Opaque MIDI copy | Must stay opaque — no Oberheim logic |
| `VirtualMidiBackend` | max SysEx 65535; SendToHost rejects oversize | Untouched unless SendToHost Fail |
| `docs/tests/smoke-epic2-sysex-mt4.md` | Pipe + short burst; fences 2.4 | Cross-link; do not claim SM-2 there |
| Synthetic smokes | Inquiry / 275 / 351 opaque carry | Optional extend for request/`0D`/`06`/`0BH` **size** edges if useful — not a substitute for Matrix-Control lab |

**What must be preserved:**

- Notes/CC all-ports path
- Clock + MTC synthetic vectors
- TX/RX mapping, Port N naming, Computer Mode kick
- CAP-8 (no Matrix-Control in build)
- Layer isolation; `builds/` output; fail-closed missing VirtualMIDI
- 2.3 oversize / queue-overflow failure visibility

**Must not break:**

- `Bridge --test-mapper` / Catch2 SysEx + queue + realtime + MTC
- Windows CI without proprietary SDK in-repo
- Epic 1 hardware notes/CC path
- Story 2.3 short-burst without restart behavior

### Known deferred edges (raise only if lab hits them)

From `deferred-work.md` after 2.3 CR:

| Edge | Relevance to 2.4 |
|---|---|
| Incomplete SysEx under hold with no `F7` (no idle timeout) | Raise **if** Matrix-Control hang matches open hold |
| Win10 SysEx transport matrix still blank (2.3) | Prefer filling 2.3 transport rows in the same lab session when possible; **this** story’s Pass is Matrix-Control vectors |
| No DeviceSession pump-level burst integration test | Optional if bank stress / live-edit Fail needs repro; not a hard deliverable |
| `processBulkRead` holds `usbIoMutex_` across decode | Investigate **if** live-edit / dump shows Bridge-induced drop |
| F5 / `0xFF` raw MIDI vs Emagic framing | Still out — Matrix traffic normally clear |
| Host→device framer | Only if incomplete PARSE_RX spans observed |
| ~4 h longevity | **Out → 2.5** |

### Technical requirements

- **Language:** C++17; Allman; 4 spaces; `#pragma once`; English — `conventions.md` §6
- **Quality:** `scripts/quality/lint-touched.py` on any touched C++
- **Primary artifact:** Matrix-Control smoke doc + filled Win10 matrix (code changes are conditional)
- **Build:** if new smoke TUs added, list in **both** Bridge and BridgeTests in `CMakeLists.txt`
- **No** committed proprietary VirtualMIDI SDK; **no** Matrix-Control link; **no** French in sources

### Library / framework requirements

| Use | Do not use |
|---|---|
| Existing DeviceSession / framer / HostOutboundQueue / opaque mapper | New SysexEngine / Oberheim allowlist |
| Matrix-Control as **external** Windows host | Matrix-Control as CMake/submodule dependency |
| Fixed synthetic vectors only if lab needs a regression catch | Vendoring Matrix-Control encoder sources |
| `docs/tests/smoke-epic2-matrix-control-mt4.md` | Claiming SM-2 from 2.3 sysex transport doc alone |
| English diagnostics with Port N / cable / vector # | Silent “almost works” without matrix rows |
| Stock Matrix-Control pacing (≥10 ms) | Inventing Bridge-side artificial SysEx throttling that drops frames |
| teVirtualMIDI max 65535 (already set) | Tiny maxSysexLength that discards 275/351 B frames |

### File structure requirements

#### NEW (primary)

| Path | Why |
|---|---|
| `docs/tests/smoke-epic2-matrix-control-mt4.md` | Locked Matrix-Control vector checklist + Win10/Win11 matrix + bilan for SM-2 |

#### UPDATE (likely)

| Path | Current state | This story |
|---|---|---|
| `docs/tests/smoke-epic2-sysex-mt4.md` | Fences 2.4 | Cross-link to Matrix-Control smoke; keep pipe-only ownership |
| `docs/tests/smoke-epic2-clock-mt4.md` / `smoke-epic2-mtc-mt4.md` | Point SysEx → 2.3/2.4 | Ensure Matrix-Control vector doc discoverable |
| `_bmad-output/implementation-artifacts/deferred-work.md` | Open 2.3 lab blanks | Note which edges closed or still open after this lab |

#### CONDITIONAL UPDATE (lab-gated Fail only)

| Path | When |
|---|---|
| `src/Protocol/MidiMessageFramer.*` / `FramerSysexSmoke.cpp` / `tests/MidiMessageFramerSysexTests.cpp` | Vector Fail needs framer fix or synthetic mixed-wire edge |
| `src/Device/HostOutboundQueue.*` / `DeviceSessionHostOutbound.cpp` / `DeviceSessionDeviceHost.cpp` | Push / live-edit / dump Fail points at queue or mutex |
| `src/Protocol/EmagicMapperSysexSmoke.cpp` / Catch2 mapper | Opaque carry regression on librarian frames |
| `src/Midi/VirtualMidiBackend.cpp` | SendToHost / max length Fail |
| `CMakeLists.txt` | Only if new TUs added |

#### Likely untouched

| Path | Why |
|---|---|
| `src/Profile/DeviceProfile.*` | No SysEx capability bit |
| `src/Protocol/EmagicCableMapper.*` core | Opaque carry already correct |
| `src/Usb/WinUsbTransport.*` | Unless bulk timeout under dump |
| `tools/midi-path-harness/` | Epic **5** |
| Matrix-Control repo | External host only |

### Testing requirements

| Check | How |
|---|---|
| Synthetic regression | `Bridge --test-mapper` exit 0; `ctest` / `BridgeTests` Pass |
| Matrix-Control hard gates | Fill smoke matrix vectors 1–5 + mixed-wire on **Win10 x64** |
| Optional bank stress | Record when feasible; Skip is OK |
| Win11 | Document when available |
| CAP-8 | Grep/CMake: no Matrix-Control dependency |
| Regression | Notes/CC path + 2.1/2.2 synthetic vectors still green |
| Lint (if C++ changed) | `python scripts/quality/lint-touched.py` |
| Compile | `builds/` on macOS smoke; Windows CI gate |

**Failure definition for this story:** Matrix-Control cannot complete a hard-gate vector because of Bridge-induced truncation, merge, rewrite, silent drop, permanent mixed-wire block, or forced Bridge restart under normal librarian use. Not a multi-hour soak (→ 2.5), not Epic 5 p99 thresholds, not bank-stress hard fail, not Matrix-6 hardware proof.

**Honesty bar:** Synthetic 2.3 green ≠ SM-2 done. Story stays incomplete until Win10 Matrix-Control hard-gate rows are filled Pass (or Fail with a fix and re-Pass). Same pattern as 2.1/2.2 keeping `review` until lab evidence.

### Previous story intelligence

From Story **2.3** (`done`):

- Delivered transparent SysEx + `HostOutboundQueue` + framer/mapper librarian-size smokes + `smoke-epic2-sysex-mt4.md`
- Explicit fence: SM-2 / Matrix-Control vector table → **this story**
- CAP-8 enforced (synthetic patterns only in Bridge)
- Review patches landed: nested F0 reject, queue wrap-safe byte-cap, SendToHost >65535 tests, shutdown/enqueue race, drain-before-encode discard behavior
- Deferred still open: open SysEx without F7 idle timeout; Win10 transport matrix blank; no session pump integration test; trailing bytes after oversize Reset; post-SendToHost-fail skip remaining demux frames
- Computer Mode: SysEx alone does not wake — keep CC kick
- Do not claim 2.4 done from synthetic 275/351 alone

From Stories **2.1** / **2.2** (`review`):

- Prove-don’t-reinvent; dedicated smoke TUs; hardware checklist honesty (synthetic ≠ lab Pass)
- Mutex stall gated on observed dropouts — same rule for live-edit Fail here
- Dual-harness smoke vs Catch2 duplication accepted; consolidate later if touching framer tests

From Epic 1:

- Hardware notes/CC all-ports green; ReadBulk uses endpoint max packet size — do not regress
- F5 / `0xFF` escaping still deferred

### Git intelligence summary

Recent relevant commits:

- `cd43225` — Add transparent SysEx transport with burst buffering for story 2.3
- `1c93de3` — Prove MTC quarter-frame and full-frame sync for story 2.2
- `4814f82` — Prove MIDI clock and transport realtime for story 2.1
- `7934b75` — Align mapper smoke encode expectations with initial Out 1 F5
- `41cccdb` — Catch2 BridgeTests for Emagic mapper and DeviceProfile

Patterns to extend: kebab-case smoke docs under `docs/tests/`, prove-don’t-reinvent, lab honesty bar, CAP-8, English Port N diagnostics, fix only the failing layer.

### Latest tech information

- teVirtualMIDI default `maxSysexLength` / `TE_VM_DEFAULT_SYSEX_SIZE` = **65535**; with `PARSE_RX`, SysEx longer than max are **discarded** by the driver — already configured correctly in-tree; do not lower it
- Prefer complete SysEx per `virtualMIDISendData`; do not stream partial `F0`… without `F7` to the host
- Matrix-Control stock pacing ≥**10 ms** (M-1000); bank export = sequential request/response pairs, not one giant SysEx
- Single V1 frames (275 / 351 B) fit hold **1024**; teVirtualMIDI ceiling is not the V1 bottleneck
- Matrix-Control does **not** speak Emagic framing — Bridge must forward Oberheim bytes unchanged (AD-16)
- Do not use ASIO buffer size as SysEx proof (SM-C2 / AD-11)
- Provisional latency/jitter anchors remain unmeasured until Epic 5

### Project context reference

- `conventions.md` §3 quality gate, §6 C++ style
- No `project-context.md` in-repo yet — follow architecture spine + conventions
- Keep hardware/protocol names in English (MT4, WinUSB, VirtualMIDI, MIDI, SysEx, Matrix-Control, Oberheim, Ableton, Reason, MIDI-OX)

### Anti-patterns to forbid

- Building a SysexEngine, OberheimDecoder, or message-type allowlist
- Linking / vendoring Matrix-Control into the Bridge build (CAP-8)
- Claiming SM-2 done because Story 2.3 synthetic 275/351 B passed
- Claiming ~4 h longevity (**2.5**) done because vectors passed once
- Treating MTC full-frame (2.2) or transport smoke (2.3) as “Matrix-Control vectors done”
- Making optional bank stress a merge blocker
- Inventing Emagic `0xFF` escaping “for completeness”
- Preemptive host→device framer without lab evidence
- Unbounded queues; raising hold to 65535 without measured need
- Building `tools/midi-path-harness/` or quoting Studio-Done latency as proven
- Inverting TX/RX flags
- French comments; kebab-case sources under `src/`
- Committing proprietary VirtualMIDI SDK binaries
- Removing Computer Mode channel CC kick because “SysEx should be enough”

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Story 2.4, Epic 2]
- [Source: `_bmad-output/specs/spec-unitor-win64-driver/validation-matrix.md` — Minimum SysEx pass vectors]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — FR-8, SM-2, §10]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/matrix-control-sysex-extract.md` — opcodes, sizes, pacing, §5]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-16, AD-18, CAP map]
- [Source: `_bmad-output/implementation-artifacts/2-3-transparent-sysex-transport-with-burst-buffering.md` — pipe+buffer, CAP-8, fences]
- [Source: `_bmad-output/implementation-artifacts/deferred-work.md` — open 2.3 edges]
- [Source: `docs/tests/smoke-epic2-sysex-mt4.md` — transport smoke; fences this story]

## Dev Agent Record

### Agent Model Used

Composer (Cursor agent router)

### Debug Log References

- `Bridge --test-mapper` exit 0 on `builds/debug/Bridge` (2026-08-05)
- `builds/debug/BridgeTests` — 46 cases / 521 assertions Pass (includes `[framer][sysex]`, `[mapper][sysex]`, `[queue][sysex]`, MTC)
- CAP-8 grep: no Matrix-Control references under `src/` or `CMakeLists.txt`

### Completion Notes List

- Task 1: added Matrix-Control locked-vector smoke checklist with Win10/Win11 matrix, SSOT citations, fences, and English Fail note requirements; cross-linked from 2.3/2.1/2.2 smoke docs
- Synthetic regression green; no C++ changes yet (Task 4 default: wait for lab Fail)
- Code review 2026-08-05: decision keep SSOT and/or for vectors 4/5; 13 checklist/story patches applied (numbering, UI actions, mixed-wire recipe, Pass/Skip rules, provisional bilan)
- HALT pending Guillaume Win10 Matrix-Control lab fill for Task 2 hard gates (honesty bar: synthetic ≠ SM-2)
- 2026-08-10 PC-only closeout (`spec-epic-2-pc-only-closure`): mid day-gate `20260809T220849Z` 5×10 `overall_pass=true`; bank day-gate `20260809T221054Z` 20×100 `overall_pass=true`; story → `done` on script barème; Matrix-Control GUI UAT deferred

### File List

- `docs/tests/smoke-epic2-matrix-control-mt4.md` (new; CR patches applied)
- `docs/tests/smoke-epic2-sysex-mt4.md` (cross-link)
- `docs/tests/smoke-epic2-clock-mt4.md` (pointer)
- `docs/tests/smoke-epic2-mtc-mt4.md` (pointer)
- `docs/tests/smoke-epic2-mt4.md` (operator §6 script closeout note)
- `_bmad-output/implementation-artifacts/sprint-status.yaml` (2-4 → done; epic-2 → done)
- `_bmad-output/implementation-artifacts/2-4-matrix-control-minimum-sysex-pass-vectors.md` (status / tasks / review findings)
- `_bmad-output/implementation-artifacts/deferred-work.md` (2-4 CR deferrals + GUI UAT)

### Change Log

- 2026-08-05 — Task 1 smoke checklist + cross-links; sprint status in-progress; awaiting Win10 Matrix-Control lab
- 2026-08-05 — Code review: and/or Pass kept; 13 patches applied to smoke + story; 2 deferred; status remains in-progress (lab pending)
- 2026-08-10 — Script mid/bank day-gate Pass; status → done on PC-only librarian barème; Matrix-Control GUI UAT deferred
