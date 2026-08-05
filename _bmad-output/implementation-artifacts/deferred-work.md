# Deferred work

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
