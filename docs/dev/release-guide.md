---
organization: Ten Square Software
project: unitor-win64-driver
title: Release guide — dual Setups
author: Guillaume DUPONT
created: 2026-08-21
updated: 2026-08-21
---

# Release guide (operators)

Primary path for publishing **Unitor MT4 Bridge** binaries: in-repo automation that stages **both** editions for one semantic version, then attaches them to one GitHub tag/Release. Hand-zipping is not the supported procedure.

Layout (inspired by Luthier):

| Folder | Role | Git? |
|---|---|---|
| `installer/` | Inno Setup **sources** (`.iss`, INF helpers) | Versioned |
| `builds/` | CMake build trees (`Bridge.exe`, tests, …) | Ignored |
| `dist/` | Compiled Setup EXEs ready to ship (Inno output) | Ignored |
| `_local/releases/X.Y.Z/` | Staged Release folder (Setups + docs zip + notes + SHA256) | Ignored |

There is **no** root `installers/` folder. Do not confuse `installer/` (sources) with `dist/` (binaries) or old `builds/installer/` paths.

## Naming (Luthier-style)

| Item | Pattern | Example |
|---|---|---|
| GitHub Release title | `Unitor MT4 Bridge X.Y.Z` | `Unitor MT4 Bridge 0.1.0` |
| Tag | bare `X.Y.Z` | `0.1.0` |
| Win11 Setup | `Unitor-MT4-Bridge-{ver}-win11-wms-setup.exe` | `Unitor-MT4-Bridge-0.1.0-win11-wms-setup.exe` |
| Win10 Setup | `Unitor-MT4-Bridge-{ver}-win10-virtualmidi-setup.exe` | `Unitor-MT4-Bridge-0.1.0-win10-virtualmidi-setup.exe` |
| Docs zip | `Unitor-MT4-Bridge-{ver}-docs.zip` | `Unitor-MT4-Bridge-0.1.0-docs.zip` |

## One primary publish path per tag (important)

Pick **one** vehicle for a given tag — do **not** race both:

| Path | When to use |
|---|---|
| **Tag-push CI** (`.github/workflows/release.yml`) | Preferred: push annotated tag `X.Y.Z` → CI builds dual Setups, finalizes, creates/ensures the Release shell, then `publish-ci` **upload-only** |
| **Local `prepare-release.py publish`** | Only when you intentionally create the tag + Release from a clean machine (Ask First / `--yes`) |

## Editions (same version)

| Flavor | Setup name pattern | Default MIDI backend | virtualMIDI gate |
|---|---|---|---|
| **win11-wms** | `Unitor-MT4-Bridge-{version}-win11-wms-setup.exe` | Windows MIDI Services | No |
| **win10-virtualmidi** | `Unitor-MT4-Bridge-{version}-win10-virtualmidi-setup.exe` | virtualMIDI | Yes — user must already have `teVirtualMIDI.dll` |

Never embed or redistribute teVirtualMIDI / virtualMIDI MSI/SDK (OQ-1). No Authenticode certificate purchase in this hobby line (OQ-3).

## Version SSOT

1. Bump `project(… VERSION x.y.z)` in `CMakeLists.txt`.
2. Tag name is **bare** `X.Y.Z` (optional prerelease `X.Y.Z-rc1`).
3. Tag base must match CMake `VERSION` before publish (CI enforces this).

## Local dry-run (no GitHub publish)

```powershell
python scripts/packaging/verify-installer-contract.py

cmake -S . -B builds/release -A x64
cmake --build builds/release --config Release

.\scripts\packaging\build-public-installer.ps1 -Flavor both
# → dist/Unitor-MT4-Bridge-*-setup.exe

python scripts/packaging/prepare-release.py pack
python scripts/packaging/prepare-release.py finalize
python scripts/packaging/prepare-release.py verify
python scripts/packaging/prepare-release.py status
```

Staging lives under `_local/releases/{version}/` (gitignored), same idea as Luthier.

## Related

- User router: [`docs/user/README.md`](../user/README.md)
- License honesty: [`license-and-backends.md`](license-and-backends.md)
- SmartScreen: [`authenticode-and-smartscreen.md`](authenticode-and-smartscreen.md)
