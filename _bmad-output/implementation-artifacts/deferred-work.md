# Deferred work

## Deferred from: code review of 1-4-devicesession-and-emagic-cable-mapper-usermode.md (2026-08-05)

- Synchronous `WriteBulk` / `ReadBulk` use blocking WinUSB calls with no `PIPE_TRANSFER_TIMEOUT` — address when a continuous MIDI I/O pump lands (Story 1.5+).
- No synchronization between transport `Close` and in-flight bulk I/O — needed once a reader thread shares the session handle.
- Emagic wire protocol: raw MIDI bytes `0xF5` / `0xFF` are indistinguishable from port-switch / end-of-valid-data framing (same limitation as Linux quirk reference); no V1 escaping layer.

## Deferred from: code review of 1-1-scaffold-bridge-project-and-windows-build-gate.md (2026-08-04)

- Fill the Observed versions table in `docs/dev/windows-ci-toolchain.md` with CMake and Visual Studio values from the first green `windows-2022` Actions log (workflow already prints them).
