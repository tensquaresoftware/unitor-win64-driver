# Deferred work

## Deferred from: quick-dev spec-epic1-in-mute-and-out1-f5.md (2026-08-05)

- source_spec: `_bmad-output/implementation-artifacts/spec-epic1-in-mute-and-out1-f5.md`
  summary: SysEx over 1024 bytes is dropped silently by MidiMessageFramer with no counter bump
  evidence: Epic 1 notes/CC smoke only; full SysEx observability belongs with Epic 2
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
- `SendToHost` does not reject payloads above teVirtualMIDI default max Sysex length — revisit with Epic 2 SysEx.

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
