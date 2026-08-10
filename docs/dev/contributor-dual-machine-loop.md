---
organization: Ten Square Software
project: unitor-win64-driver
title: Contributor dual-machine loop
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
---

# Contributor dual-machine loop

How this project is developed day to day (architecture AD-13 / NFR-D3).

## Roles

| Role | Machine | Typical work |
|---|---|---|
| Primary edit | **macOS** + Cursor | Author markdown and C++, review diffs, run agent workflows |
| Validate | **Windows 10 / 11 x64** | Build, USB / WinUSB, DAW, SysEx, installer, Studio-Done measurements |

**Windows 10 x64 is mandatory** in the validation matrix. Windows 11 alone is **not** enough to close hardware or installer claims that require the matrix.

## Artifacts and merge gate

- Build outputs live under `builds/` (for example `builds/debug`, `builds/ci`) — never a root `build/` tree as the expected layout
- Windows CI compile is the **minimum merge gate** — workflow: [`.github/workflows/windows-build.yml`](../../.github/workflows/windows-build.yml) (see also [`windows-ci-toolchain.md`](windows-ci-toolchain.md))
- Hardware Pass rows in operator smokes remain **lab-owned** (blank ≠ Pass)

## Offline / split machines

macOS can author markdown and C++ offline. The Windows validation machine closes hardware, WinUSB bind, DAW / SysEx, and installer Pass rows.

## Pointers (do not duplicate here)

| Topic | Doc |
|---|---|
| Quality SSOT and §3 limits | [`conventions.md`](../../conventions.md) |
| Commit / quality gate habit | [`contributing.md`](../../contributing.md) |
| Windows CI toolchain | [`windows-ci-toolchain.md`](windows-ci-toolchain.md) |
| WinUSB bind (Zadig = contributor fallback only) | [`winusb-bind.md`](winusb-bind.md) |
| License / backend honesty | [`license-and-backends.md`](license-and-backends.md) |
| End-user install path | [`docs/user/README.md`](../user/README.md) |
