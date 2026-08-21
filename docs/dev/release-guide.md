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
| Tag / folder | bare `X.Y.Z` (optional `X.Y.Z-rc1`) | `0.1.0` / `0.1.0-rc1` |
| Win11 Setup | `unitor-mt4-bridge-{cmake-ver}-win11-wms-setup.exe` | `unitor-mt4-bridge-0.1.0-win11-wms-setup.exe` |
| Win10 Setup | `unitor-mt4-bridge-{cmake-ver}-win10-virtualmidi-setup.exe` | `unitor-mt4-bridge-0.1.0-win10-virtualmidi-setup.exe` |
| Docs zip | `unitor-mt4-bridge-{tag}-docs.zip` | `unitor-mt4-bridge-0.1.0-docs.zip` |
| Notes | `RELEASE_NOTES.md` (Release asset body source) | staged under `_local/releases/{tag}/` |
| Checksums | `SHA256SUMS.txt` | staged + uploaded with the Release |

**Prerelease note:** Setup EXE names always use the bare CMake `project(VERSION)` (no `-rc1` suffix). Tag, staging folder, docs zip, and notes title use the full tag (`0.1.0-rc1`). Templates use `{{ARTIFACT_VERSION}}` for Setup names and `{{VERSION}}` for the tag.

## One primary publish path per tag (important)

Pick **one** vehicle for a given tag — do **not** race both:

| Path | When to use |
|---|---|
| **Tag-push CI** (`.github/workflows/release.yml`) | Preferred: push annotated tag `X.Y.Z` → CI builds dual Setups, finalizes, creates/ensures the Release shell, then `publish-ci` **upload-only** |
| **Local `prepare-release.py publish`** | Only when you intentionally create the tag + Release from a clean machine (Ask First / `--yes`) |

If both run, the second upload can clobber assets or waste a build. Prefer tag-push CI; for local publish use `--skip-tag-push` when the tag is already on origin, or avoid pushing a tag that would also fire CI.

## Editions (same version)

| Flavor | Setup name pattern | Default MIDI backend | virtualMIDI gate |
|---|---|---|---|
| **win11-wms** | `unitor-mt4-bridge-{version}-win11-wms-setup.exe` | Windows MIDI Services | No |
| **win10-virtualmidi** | `unitor-mt4-bridge-{version}-win10-virtualmidi-setup.exe` | virtualMIDI | Yes — user must already have `teVirtualMIDI.dll` |

Both flavors share **one** Windows product identity (`AppId`): installing one Setup **replaces** the other under Program Files and Auto-Start. There is no side-by-side install.

Never embed or redistribute teVirtualMIDI / virtualMIDI MSI/SDK (OQ-1). No Authenticode certificate purchase in this hobby line (OQ-3). Optional local signing via `scripts/packaging/sign-public-artifacts.ps1` (when present) is **not** a hard gate — unsigned builds remain the community default.

## Version SSOT

1. Bump `project(… VERSION x.y.z)` in `CMakeLists.txt`.
2. Tag name is **bare** `X.Y.Z` (optional prerelease `X.Y.Z-rc1`).
3. Tag base must match CMake `VERSION` before publish (CI enforces this).

## Local dry-run (no GitHub publish)

Default `-Flavor` for the installer script is **`both`**. Pass `-BridgeDir` when Bridge is not under the script’s default search path (CI always passes it).

```powershell
python scripts/packaging/verify-installer-contract.py

cmake -S . -B builds/release -A x64
cmake --build builds/release --config Release

.\scripts\packaging\build-public-installer.ps1 -BridgeDir builds\release\Release -Flavor both
# → dist/unitor-mt4-bridge-*-setup.exe

python scripts/packaging/prepare-release.py pack
python scripts/packaging/prepare-release.py finalize
python scripts/packaging/prepare-release.py verify
python scripts/packaging/prepare-release.py status
```

Staging lives under `_local/releases/{version}/` (gitignored), same idea as Luthier.

### Publish flags (local)

| Command | Useful flags |
|---|---|
| `publish` | `--yes`, `--prerelease`, `--skip-tag-push` |
| `publish-ci` | `--yes`, `--prerelease`, `--stable` (upload-only; never creates the tag) |

## Related

- User router: [`docs/user/README.md`](../user/README.md)
- License honesty: [`license-and-backends.md`](license-and-backends.md)
- SmartScreen: [`authenticode-and-smartscreen.md`](authenticode-and-smartscreen.md)
