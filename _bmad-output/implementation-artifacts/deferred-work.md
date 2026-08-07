# Deferred work

## Deferred from: story 2-5 session longevity design (2026-08-05)

Open soak risks for SM-3 / NFR-R1 (`docs/tests/checklists/smoke-epic2-longevity-mt4.md`). None measured Fail this turn (Win10 ~4 h sample still blank). Do **not** treat them as “usermode destiny” if the sample hits them.

- Incomplete SysEx under hold with no `F7` (no idle timeout) — observe during soak; add idle-timeout / English session failure only if hang is real.
- CTRL_CLOSE may kill before `Stop` → orphan Virtual Ports — prefer Ctrl+C for sample teardown; raise priority if the shared soak path uses window-close.
- `processBulkRead` holds `usbIoMutex_` across decode — observe under multi-hour clock/SysEx load; fix only with evidence.
- Console heartbeat every ~3 s (at least ~4800 lines / 4 h if redirected) — throttle only if redirected logs become unusable.
- Win10 x64 ~4 h sample matrix blank until lab wall-clock time — design + plan landed; blank ≠ Pass; optional 30–60 min interim does not close SM-3 alone.

Design note already captured in the longevity guide (not an open deferral): after pump fail, ports stay up until CLI ~50 ms poll `Stop` — expected Fail→Stop.

## Deferred from: code review of 2-4-matrix-control-minimum-sysex-pass-vectors.md (2026-08-05)

- Update `deferred-work.md` with open 2.3 Win10 SysEx blanks after the Matrix-Control lab (story Task 2+) — not required for Task 1 checklist-only delivery.
- Epic 1 smoke guide does not yet point operators to `docs/tests/checklists/smoke-epic2-matrix-control-mt4.md` — nice-to-have navigation; not a Task 1 acceptance gap.

## Deferred from: code review of 2-3-transparent-sysex-transport-with-burst-buffering.md (2026-08-05)

- Incomplete SysEx under hold cap with no `0xF7`: no idle-timeout / session failure in 2.3 — AC4 for this story = oversize, queue overflow, nested `0xF0` abandon; revisit if Win10 lab shows hangs.
- Win10 SysEx hardware matrix / bilan blank — deferred as manual lab gate (same honesty bar as 2.1/2.2); not a code blocker for this review.
- No DeviceSession pump-level burst integration test in 2.3 — AC2 accepted as queue unit tests + framer/mapper smokes + Win10 lab checklist.
- Trailing bytes after oversize SysEx `Reset()` in the same `Push` span are dropped without an extra reject count (framer cold-start after abort; corrupt-stream edge).
- After the first device→host `SendToHost` failure, remaining demuxed frames from the same bulk read are skipped once `stopPump_` is set (general pump-fail semantics; not unique to SysEx).

## Deferred from: quick-dev spec-epic1-in-mute-and-out1-f5.md (2026-08-05)

- source_spec: `_bmad-output/implementation-artifacts/spec-epic1-in-mute-and-out1-f5.md`
  summary: SysEx over 1024 bytes is dropped silently by MidiMessageFramer with no counter bump — **raised in story 2.3** (`ConsumeOversizeSysexRejectCount` + session English failure)
  evidence: Epic 1 notes/CC smoke only; full SysEx observability belongs with Epic 2; closed by 2.3 observability path
  status: resolved-by-2-3
- source_spec: `_bmad-output/implementation-artifacts/spec-epic1-in-mute-and-out1-f5.md`
  summary: Reader-thread device-host counter lines race CLI std::cout without a shared lock
  evidence: Surfaced in review; pre-existing multi-writer console pattern, not unique to counters
- source_spec: `_bmad-output/implementation-artifacts/spec-epic1-in-mute-and-out1-f5.md`
  summary: After framer Reset, bare running-status data bytes are dropped until a new status byte
  evidence: Standard MIDI cold-start behavior; DIN devices usually send status after reconnect

## Deferred from: code review (Epic 1 integration, 2026-08-05)

- MIDI demux→host framer still absent; 1.6 pump now escalates incomplete spans via `recordPumpFailure` / session stop — **patched** 2026-08-05 evening (`MidiMessageFramer` before `SendToHost`; lab notes/CC OK).
- CTRL_CLOSE_EVENT still only sets cancel flag; Windows may kill before `Stop` finishes port/USB teardown — still open (was 1-6); raise priority before any public/shareable session path.
- Epic 1 integration CR (2026-08-05) **patched**: host→device WriteBulk under `usbIoMutex_`, VirtualMIDI sink mutex, bulk OUT `PIPE_TRANSFER_TIMEOUT`, atomic `running_`, Zadig multi-match refuse, bind docs `--start-session`/`--run-midi`. Remaining lifecycle edge: orphan ports after hard crash / CTRL_CLOSE kill (was 1-5 / 1-6).
- After `recordPumpFailure`, Virtual Ports stay up until CLI ~50 ms poll calls `Stop`; host→device encode silently no-ops in that window.
- `processBulkRead` holds `usbIoMutex_` across full `DecodeFromDevice` + allocations — busy IN can stall host→device encode (latency under load; not a wrong-cable bug once WriteBulk is locked).
- Zadig fallback opens the first hardware-ID match without counting multiple MT4s — ambiguous multi-unit open (Epic 3 / multi-device; primary GUID path already refuses `matchCount != 1`).
- Partial `CreatePortSet` errors omit which `MT4 Port N` / IN·OUT failed.
- First host→device encode on Port 1 may omit F5 when mapper `currentOutCable_` starts at 0 (Port 1 == cable 0); **patched** 2026-08-05 evening (`currentOutCable_=0xFF` sentinel); **hardware retest OK** (first Out 1 notes no longer fan all Out LEDs).
- **P0 lab 2026-08-05**: device DIN In 1/2 LEDs blink but Ableton silent on virtual IN — **patched** evening. Root cause: WinUSB `ReadBulk` used a 512-byte buffer while Emagic sends full `wMaxPacketSize` packets (lab: 32) padded with `0xFF`, so reads timed out with 0 bytes; fix reads at endpoint max packet size. Also: init IN drain before further OUT, Set Computer Mode + channel CC kick, `MidiMessageFramer` before `SendToHost`. **Hardware OK**: notes+CC on In 1/2 in Ableton (C3, CC7).
- Hardware notes/CC **full** round-trip on all 2 IN + 4 OUT — **lab OK** 2026-08-05 evening (notes then CC); keep section 6.3 parallel multi-OUT optional.
- Identical IN/OUT `MT4 Port N` teVirtualMIDI collision — **patched** in lab session (merged bidirectional create); keep an eye on DAW IN/OUT pairing.
- Shared `MidiBackend` across concurrent `DeviceSession` instances not rejected — still open (was 1-5 / Epic 3).
- Device-host counter lines were invisible in PowerShell during successful retest (reader-thread `cout` buffering) — **patched** to `cerr` + flush + Start hint (re-confirm next session).

## Deferred from: code review of 1-6-notes-and-cc-round-trip-on-all-ports.md (2026-08-05)

- Hardware AC1/AC2 round-trip proof (notes+CC on all 2 IN + 4 OUT via ShowMIDI/DAW) — deferred: infra OK for story close; Windows smoke remains a manual checklist.
- CTRL_CLOSE_EVENT may terminate the process before `DeviceSession::Stop()` finishes closing Virtual Ports and WinUSB.
- Device→host path can forward raw Emagic demux spans that are not complete MIDI messages (no message framer in 1.6) — **patched** 2026-08-05 (`MidiMessageFramer`).
- `SendToHost` does not reject payloads above teVirtualMIDI default max Sysex length — **raised in story 2.3** (reject above 65535 with English diagnostic).

## Deferred from: code review of 1-5-virtualmidi-backend-and-stable-mt4-port-names.md (2026-08-05)

- No recovery for stale Virtual Ports left after a crashed Bridge — revisit when multi-unit / reconnect lifecycle is hardened.
- Same AD-5 display names on IN and OUT need Windows collision proof with teVirtualMIDI (exact spelling vs `#2` suffixes).
- `--start-session` prints expected names but does not enumerate live Windows MIDI endpoints — strengthen when hardware Validation Matrix automation exists.
- Hand-rolled teVirtualMIDI flag constants unpinned to an SDK version/checksum (AQ-3).
- Shared `MidiBackend` across concurrent `DeviceSession` instances is not rejected — Epic 3 multi-unit ownership.

## Deferred from: code review of 1-4-devicesession-and-emagic-cable-mapper-usermode.md (2026-08-05)

- Synchronous `WriteBulk` / `ReadBulk` use blocking WinUSB calls with no `PIPE_TRANSFER_TIMEOUT` — address when a continuous MIDI I/O pump lands (Story 1.5+).
- No synchronization between transport `Close` and in-flight bulk I/O — needed once a reader thread shares the session handle.
- Emagic wire protocol: raw MIDI bytes `0xF5` / `0xFF` are indistinguishable from port-switch / end-of-valid-data framing (same limitation as Linux quirk reference); no V1 escaping layer.

## Deferred from: code review of 1-1-scaffold-bridge-project-and-windows-build-gate.md (2026-08-04)

- Fill the Observed versions table in `docs/dev/windows-ci-toolchain.md` with CMake and Visual Studio values from the first green `windows-2022` Actions log (workflow already prints them).

## Deferred from: spec-ci-cd-windows-pipeline.md (2026-08-05)

- source_spec: `_bmad-output/implementation-artifacts/spec-ci-cd-windows-pipeline.md`
  summary: Add lean C++ unit tests without hardware (EmagicCableMapper, DeviceProfile, MapperSmoke) plus a CMake Tests target and CI job.
  evidence: CI/CD ticket deliberately deferred the test factory — no Tests target today; keep the merge gate lean until the pure-logic surface is wired.

## Deferred from: spec-unit-tests-without-hardware.md (2026-08-05)

- source_spec: `_bmad-output/implementation-artifacts/spec-unit-tests-without-hardware.md`
  summary: Cache Catch2 / `_deps` across Windows CI runs to cut configure+compile time on clean runners.
  evidence: Every clean `windows-2022` job re-fetches and rebuilds Catch2; not required for the lean merge gate but adds flaky network time.

- source_spec: `_bmad-output/implementation-artifacts/spec-unit-tests-without-hardware.md`
  summary: Add a unit test for encode when buffer fits MIDI but has no room for trailing 0xFF pad.
  evidence: `appendTrailingPad` silently omits the end marker when capacity is exhausted; current smoke always uses a large buffer that always expects 0xFF.

## Deferred from: code review of 2-1-midi-clock-and-transport-realtime.md (2026-08-05)

- Dense-clock / transport path may stall when `processBulkRead` holds `usbIoMutex_` across decode while host→device Encode+WriteBulk needs the same lock — revisit only if short DAW clock smoke shows Bridge-induced dropouts (story 2.1 AC); already noted in Epic 1 CR deferred-work.

## Deferred from: code review of 2-2-mtc-quarter-frame-and-full-frame.md (2026-08-05)

- Duplicated MTC quarter-frame / full-frame vectors in `FramerMtcSmoke.cpp` vs `tests/MidiMessageFramerTests.cpp` can drift; same dual-harness pattern as story 2.1 realtime — consolidate shared helpers when touching the framer test factory again.

## Deferred from: spec-mt4-separate-virtual-ports.md (2026-08-06)

- Identical IN/OUT `MT4 Port N` teVirtualMIDI collision — previously **patched** with merged bidirectional create — **superseded** by directional `MT4 Input N` / `MT4 Output Y` separate faces (no shared handle). Re-verify in lab: Device Inquiry on Output must not appear on Input; update any remaining historical notes that still cite the merged workaround as current.
- Partial `CreatePortSet` errors still omit which directional face failed — unchanged; improve diagnostics when touching VirtualMIDI create again.
- Lab soak (Matrix-Control presence 2–5 min + MIDI-OX no-echo check) remains manual on Windows hardware.
- source_spec: `_bmad-output/implementation-artifacts/spec-mt4-separate-virtual-ports.md`
  summary: Amend planning SSOT (AD-5 / product SPEC) from undirected `MT4 Port N` to directional `MT4 Input` / `MT4 Output` naming.
  evidence: Code and operator smokes already ship directional names; architecture/epics still document the old shared-label contract.
- source_spec: `_bmad-output/implementation-artifacts/spec-mt4-separate-virtual-ports.md`
  summary: Harden uniqueness checks against wide/case-folded teVirtualMIDI alias collisions beyond exact UTF-8 string equality.
  evidence: Current validation compares UTF-8 strings only; driver alias rules may be wider.
## Deferred from: spec-sysex-matrix-mid-roundtrip.md (2026-08-07)

- source_spec: `_bmad-output/implementation-artifacts/spec-sysex-matrix-mid-roundtrip.md`
  summary: Mid-stream USB/reorder drops still exist; size-reject + one dump-request retry masks them for palier-1 gate — root cause at harvest/reorder remains open.
  evidence: Lab stamps `185403Z` (retry on len=319) and earlier `discard` wrong lengths before guards; Apple driver 100 % on same MT4.
- source_spec: `_bmad-output/implementation-artifacts/spec-sysex-matrix-mid-roundtrip.md`
  summary: Leading-F0 repair keys on first product byte `0x10` during dump expect — Matrix-shaped for this lab; false-positive risk if stray `0x10` arrives in the same window.
  evidence: Review; intentional narrow guard for Emagic Matrix dump body after lost `F0`.
- source_spec: `_bmad-output/implementation-artifacts/spec-sysex-matrix-mid-roundtrip.md`
  summary: Dump-request retry `WriteBulk` runs while reader may hold `usbIoMutex_` — sync OUT under contention.
  evidence: Edge review; revisit if retry latency or OUT stall shows in longer librarian runs.
- source_spec: `_bmad-output/implementation-artifacts/spec-sysex-matrix-mid-roundtrip.md`
  summary: Temporary first-burst diagnostic stderr (`first-burst IN`, leading-F0 repair lines) still enabled — gate or remove after palier credibility is locked.
  evidence: Intentional lab instrumentation during Quick Dev close.
- source_spec: `_bmad-output/implementation-artifacts/spec-sysex-matrix-mid-roundtrip.md`
  summary: Post-start calm is best-effort (opens librarian gate on 500 ms timeout even if pending/queued not ideal).
  evidence: Edge review; calm alone did not close the hole; keep for hygiene only.

## Deferred from: spec-mt4-device-inquiry-alternate-drop.md (2026-08-07)

- source_spec: `_bmad-output/implementation-artifacts/spec-mt4-device-inquiry-alternate-drop.md`
  summary: `usbIoMutex_` held across demux and SendToHost on the async IN path may delay host OUT WriteBulk under try_lock.
  evidence: Review BH-07; pre-existing async IN design; lab Identity path stays ~46 ms — revisit only with jitter evidence.
- source_spec: `_bmad-output/implementation-artifacts/spec-mt4-device-inquiry-alternate-drop.md`
  summary: Ungated stderr/stdout Inquiry and Identity lab lines on every message.
  evidence: Review BH-09; intentional while closing the first-shot gate — gate or throttle after V1 credibility lab is done.
- source_spec: `_bmad-output/implementation-artifacts/spec-mt4-device-inquiry-alternate-drop.md`
  summary: `BulkOutMaxPacketSize` is discovered but unused by encode/WriteBulk.
  evidence: Review BH-11; leftover from discarded full-packet pad experiments.
- source_spec: `_bmad-output/implementation-artifacts/spec-mt4-device-inquiry-alternate-drop.md`
  summary: Post-kick IN drain hard-caps at 8 packets (same as init drains).
  evidence: Edge review; unlikely after kick with idle bus; change only if lab shows residual >8.

## Deferred from: quick-dev spec-sysex-matrix-bank-burst-2.md (2026-08-07)

- source_spec: _bmad-output/implementation-artifacts/spec-sysex-matrix-bank-burst-2.md
  summary: Bank harness can discard a wrong-size SysEx then Pass on a later 275 B match within the same trial timeout.
  evidence: Edge review; pre-existing _wait_matched_sysex behavior shared with macOS gate - tighten only if a lab shows silent wrong-size frames.
- source_spec: _bmad-output/implementation-artifacts/spec-sysex-matrix-bank-burst-2.md
  summary: Patch-dump Pass does not bind reply slot/program identity to the requested slot.
  evidence: Blind/Edge review; _is_patch_dump checks 275 B + prefix/suffix only - same as accepted macOS control.
- source_spec: _bmad-output/implementation-artifacts/spec-sysex-matrix-bank-burst-2.md
  summary: Update macOS paliers lab report Windows narrative now that Bridge bank gate is closed.
  evidence: Blind review; report still frames early Windows Bridge first-dump losses - do not reopen hardware control, but refresh Windows status later.
