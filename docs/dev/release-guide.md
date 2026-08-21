---
organization: Ten Square Software
project: unitor-win64-driver
title: Release guide — dual community Setups
author: Guillaume DUPONT
created: 2026-08-21
updated: 2026-08-21
---

# Release guide (operators)

Primary path for publishing **Unitor MT4 Bridge** community binaries: in-repo automation that stages **both** flavors for one semantic version, then attaches them to one GitHub tag/Release. Hand-zipping is not the supported procedure.

## One primary publish path per tag (important)

Pick **one** vehicle for a given tag — do **not** race both:

| Path | When to use |
|---|---|
| **Tag-push CI** (`.github/workflows/release.yml`) | Preferred: push annotated tag `X.Y.Z` → CI builds dual Setups, finalizes, creates/ensures the Release shell, then `publish-ci` **upload-only** |
| **Local `prepare-release.py publish`** | Only when you intentionally create the tag + Release from a clean machine (Ask First / `--yes`) |

Running local `publish` **and** tag-push CI for the same tag can race (duplicate uploads, conflicting Release edits). Prefer CI after a deliberate tag push, **or** local publish alone — not both.

`publish-ci` never creates tags and never creates a missing Release (upload-only). CI creates an empty Release shell first when needed.

## Flavors (same version)

| Flavor | Setup name pattern | Default MIDI backend | virtualMIDI gate |
|---|---|---|---|
| **win11-wms** | `UnitorMt4Bridge-Setup-win11-wms-{version}.exe` | Windows MIDI Services (`--midi-backend=wms`) | No (Win11 + midisrv required) |
| **win10-virtualmidi** | `UnitorMt4Bridge-Setup-win10-virtualmidi-{version}.exe` | virtualMIDI (`--midi-backend=virtualmidi`) | Yes — user must already have `teVirtualMIDI.dll` |

Never embed or redistribute teVirtualMIDI / virtualMIDI MSI/SDK (OQ-1). No Authenticode certificate purchase in this hobby line (OQ-3) — document SmartScreen.

## Version SSOT

1. Bump `project(… VERSION x.y.z)` in `CMakeLists.txt` (feeds `bridge-version.txt` and `Bridge.exe --version`).
2. Tag name is **bare** `X.Y.Z` (optional prerelease suffix `X.Y.Z-rc1`). Do not invent a `v` prefix unless product policy changes.
3. Tag base must match CMake `VERSION` before publish (CI enforces this).

## Local dry-run (no GitHub publish)

```powershell
# 0) Offline contract
python scripts/packaging/verify-installer-contract.py

# 1) Build Bridge (Release)
cmake -S . -B builds/release -A x64
cmake --build builds/release --config Release

# 2) Build both Setups
.\scripts\packaging\build-public-installer.ps1 -Flavor both

# 3) Stage under _local/releases/{version}/ (gitignored)
python scripts/packaging/prepare-release.py pack
python scripts/packaging/prepare-release.py finalize
python scripts/packaging/prepare-release.py verify
python scripts/packaging/prepare-release.py status
```

`verify` checks both Setups, the docs zip (four manuals + README + `license-and-backends.md` + `authenticode-and-smartscreen.md`), `RELEASE_NOTES.md`, and that **every** distributable appears in `SHA256SUMS.txt`. It does **not** create tags or call `gh`.

## Installer contract (offline)

```powershell
python scripts/packaging/verify-installer-contract.py
```

Expects dual-flavor honesty in `installer/public-installer.iss` and the packaging scripts.

## Publish gates (Ask First)

| Command | What it does | Gate |
|---|---|---|
| `prepare-release.py publish` | Annotated tag + optional push + `gh release` create/upload | Interactive prompt unless `--yes`; agents must not push without Guillaume’s explicit go-ahead for that tag |
| `prepare-release.py publish-ci` | Upload assets to an **existing** tag **and** Release | Used by `.github/workflows/release.yml` with `--yes` after Release shell exists |
| `publish --skip-tag-push` | Local tag only (no remote) | Still creates a local tag — use only when intentional |

## Tag-push CD

Pushing tag `X.Y.Z` runs `.github/workflows/release.yml`:

1. Validate tag ↔ CMake version + `verify-installer-contract.py`
2. Build Bridge Release + both Setups
3. `pack` → `finalize` → `verify`
4. Ensure GitHub Release shell exists for the tag
5. `publish-ci --yes` (upload-only)

## Related

- User router: [`docs/user/README.md`](../user/README.md)
- License honesty: [`license-and-backends.md`](license-and-backends.md)
- SmartScreen: [`authenticode-and-smartscreen.md`](authenticode-and-smartscreen.md)
