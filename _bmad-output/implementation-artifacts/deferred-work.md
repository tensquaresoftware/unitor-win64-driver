# Deferred work

## Deferred from: code review (Epic 1 integration, 2026-08-05)

- MIDI demux→host framer still absent; 1.6 pump now escalates incomplete spans via `recordPumpFailure` / session stop — still open, aggravated by integration (was 1-6).
- CTRL_CLOSE_EVENT still only sets cancel flag; Windows may kill before `Stop` finishes port/USB teardown — still open (was 1-6); raise priority before any public/shareable session path.
- Epic 1 integration CR (2026-08-05) **patched**: host→device WriteBulk under `usbIoMutex_`, VirtualMIDI sink mutex, bulk OUT `PIPE_TRANSFER_TIMEOUT`, atomic `running_`, Zadig multi-match refuse, bind docs `--start-session`/`--run-midi`. Remaining lifecycle edge: orphan ports after hard crash / CTRL_CLOSE kill (was 1-5 / 1-6).
- After `recordPumpFailure`, Virtual Ports stay up until CLI ~50 ms poll calls `Stop`; host→device encode silently no-ops in that window.
- `processBulkRead` holds `usbIoMutex_` across full `DecodeFromDevice` + allocations — busy IN can stall host→device encode (latency under load; not a wrong-cable bug once WriteBulk is locked).
- Zadig fallback opens the first hardware-ID match without counting multiple MT4s — ambiguous multi-unit open (Epic 3 / multi-device; primary GUID path already refuses `matchCount != 1`).
- Partial `CreatePortSet` errors omit which `MT4 Port N` / IN·OUT failed.
- First host→device encode on Port 1 may omit F5 when mapper `currentOutCable_` starts at 0 (Port 1 == cable 0); **hardware confirmed 2026-08-05** (Boot Camp): first notes on Out 1 lit all four Out LEDs; later Out 1 OK after other ports used; CC7 Out 1 was OK.
- **P0 lab 2026-08-05**: device DIN In 1/2 LEDs blink (notes+CC from Mac) but Ableton on PC sees no traffic on virtual `MT4 Port 1/2` IN — bulk IN → demux → `SendToHost` path (and/or Live monitor setup) not proven.
- Hardware notes/CC **full** round-trip on all 2 IN + 4 OUT — still open (was 1-6); PC→device largely OK; device→host missing.
- Identical IN/OUT `MT4 Port N` teVirtualMIDI collision — **patched** in lab session (merged bidirectional create); keep an eye on DAW IN/OUT pairing.
- Shared `MidiBackend` across concurrent `DeviceSession` instances not rejected — still open (was 1-5 / Epic 3).

## Deferred from: code review of 1-6-notes-and-cc-round-trip-on-all-ports.md (2026-08-05)

- Hardware AC1/AC2 round-trip proof (notes+CC on all 2 IN + 4 OUT via ShowMIDI/DAW) — deferred: infra OK for story close; Windows smoke remains a manual checklist.
- CTRL_CLOSE_EVENT may terminate the process before `DeviceSession::Stop()` finishes closing Virtual Ports and WinUSB.
- Device→host path can forward raw Emagic demux spans that are not complete MIDI messages (no message framer in 1.6).
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
