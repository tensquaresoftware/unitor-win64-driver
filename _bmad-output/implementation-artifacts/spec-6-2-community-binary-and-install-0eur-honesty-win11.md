---
title: '6.2 Dual community release — Win11 WMS + Win10 virtualMIDI (honesty + manuals)'
type: 'feature'
created: '2026-08-21'
status: 'done'
baseline_commit: 'ffb1554cf16d9b286b370044df465224dacea8fe'
review_loop_iteration: 0
context:
  - '{project-root}/_bmad-output/implementation-artifacts/epic-6-context.md'
  - '{project-root}/docs/dev/license-and-backends.md'
  - '{project-root}/docs/dev/authenticode-and-smartscreen.md'
  - '{project-root}/conventions.md'
---

<frozen-after-approval reason="human-owned intent — do not modify unless human renegotiates">

## Intent

**Problem:** Community materials still describe a single virtualMIDI-first install, while Story 6.1 already ships a WMS MidiBackend — so Guillaume cannot yet publish honest dual community Releases (Win11 comfort via Windows MIDI Services; Win10 parallel via user-installed virtualMIDI) with matching musician vs motivated manuals.

**Approach:** For each version, automate a Luthier-style release pipeline that stages both Bridge/Setup flavors, user manuals, README/honesty materials, checksums, and notes into a clean versioned folder layout, then creates the matching GitHub tag + Release with both flavors as assets; split end-user manuals into four path-specific guides (Win11 WMS FR+EN, Win10 virtualMIDI FR+EN) with a 20-second README router — never merge both stacks into one guide.

## Boundaries & Constraints

**Always:**
- Same semantic version on both flavors; artifact names distinguish `win11-wms` vs `win10-virtualmidi` (align with existing `UnitorMt4Bridge-Setup` / `Bridge.exe` branding).
- **Fully automated packaging in-repo** (inspire from sibling Luthier `publish/prepare-release.py` + tag-push CD): assemble manuals, executables/Setups, README/router materials, release notes, and checksums into a clean staged tree; publish one GitHub Release + tag that carries **both** flavors (and shared docs) for that version — no hand-zipping as the primary path.
- Tag ↔ version SSOT must match before publish; split local “create tag + push” from CI-safe “upload assets to existing tag/Release” where practical.
- Win11 / WMS flavor: community comfort path; **no** virtualMIDI install prerequisite; **no** redistributed virtualMIDI MSI/SDK/DLL; SmartScreen “Run anyway” / “Exécuter quand même”; guided WinUSB (Zadig) on clean PCs; Auto-Start; fail closed if WMS unavailable.
- Win10 / virtualMIDI flavor: parallel motivated path; user **self-installs** virtualMIDI (Tobias Erichsen links; verify `teVirtualMIDI.dll`); Bridge may require that DLL at install/runtime; **never** embed or redistribute virtualMIDI MSI/SDK/DLL from this project.
- Public honesty (README + `license-and-backends.md` + manuals): MIT ≠ virtualMIDI ≠ WMS; OQ-1 no redistribution clearance; OQ-3 no Authenticode cert in this hobby line; no Setup-alone WinUSB magic.
- Manuals: four files + `docs/user/README.md` aiguillage (Windows 11 vs Windows 10 in ~20 s). Win11 guides = musician, zero PowerShell/Admin as primary recovery. Win10 guides = motivated; PowerShell/service paths allowed. Win11 midisrv sticky case → close DAW → quit Bridge → reboot PC → one Bridge relaunch (no Admin terminal).
- Retire or redirect old single guides so there is one truth per path/language.
- Preserve Story 6.1 WMS behavior (`Midi1StreamAssembler`, SendToHost + BufferFull retry). English in technical artifacts; FR+EN user manuals; no French in C++ sources.

**Ask First:**
- Compile-time stripping VirtualMidiBackend from the Win11 flavor vs one Bridge binary with flavor-specific defaults/installer gates only.
- Separate Inno `AppId` / side-by-side install of both flavors on one PC.
- Changing public product/publisher display names away from Unitor MT4 Bridge / Ten Square Software.
- Tag naming prefix (`vX.Y.Z` vs bare `X.Y.Z`) if it conflicts with existing Unitor conventions.
- Treating midisrv GUI repair helper as in-scope for 6.2 (default = follow-up only).
- Publishing to GitHub from an agent session without Guillaume’s explicit go-ahead for that tag/Release.

**Never:**
- Story 6.3 full Validation Matrix as a 6.2 gate.
- Refactoring WMS teardown to “fix midisrv”; overnight restart-aggressive as ship gate.
- Embedding or redistributing teVirtualMIDI / virtualMIDI MSI/SDK.
- Merging Win11 and Win10 stacks into a single manual per language.
- Purchasing Authenticode / claiming a production catalog without a shipped cert.
- Soft-echo as community hardware proof.
- Manual-only zip/upload as the supported release procedure (automation must be the primary path).

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Win11 happy path | Win11; WMS available; Win11 Setup/Bridge | Install succeeds without virtualMIDI; ports via WMS; manuals describe musician path | N/A |
| Win11 no WMS | Win11 flavor; WMS unavailable | Fail closed; no empty “success” ports | Clear WMS prerequisite messaging |
| Win10 happy path | Win10; user-installed virtualMIDI DLL present; Win10 Setup/Bridge | Install/session uses virtualMIDI; manuals describe self-install | N/A |
| Win10 missing DLL | Win10 flavor; no `teVirtualMIDI.dll` | Install or session fails closed | Point to Tobias self-install; never embed DLL |
| Clean-PC WinUSB | Either flavor; MI_02 unbound | Guided Zadig path documented; not Setup-alone success | Manuals + honesty materials |
| SmartScreen | Unsigned/low-reputation Setup | Documented “Run anyway” / “Exécuter quand même” | No pretend cert |
| Wrong guide | User on Win10 opens WMS guide (or reverse) | README + “what works / does not” point to the other path | Cross-links only |
| Sticky MIDI after stop/restart | Win11 after Bridge stop/relaunch | Non-geek recovery: close DAW → quit Bridge → reboot → one relaunch | No PowerShell Admin in Win11 guide |
| Automated release | Version bumped; pack → finalize → publish(+ci) | Staged tree holds both flavors + manuals/docs + SHA256 + notes; GitHub tag/Release lists both assets | Fail if tag ≠ version SSOT or required assets missing |

</frozen-after-approval>

## Code Map

- `installer/public-installer.iss` -- single Setup today (`OutputBaseFilename=UnitorMt4Bridge-Setup`); Welcome/gates assume virtualMIDI (`VirtualMidiPresent`, `AllInstallGatesPassed` ~L91–119). Split or parameterize into win11-wms (drop virtualMIDI gate) vs win10-virtualmidi (keep self-install gate, no MSI embed).
- `scripts/packaging/build-public-installer.ps1` -- builds Setup from Bridge output + version; extend for dual artifact names/outputs; feed staged release tree.
- **Create** `scripts/packaging/prepare-release.py` (Luthier-inspired) -- stages pack/finalize/verify/publish(+ci): dual Setup (+ optional Bridge bins), docs zip or folder of four manuals + README, per-flavor README snippets, `RELEASE_NOTES`, `SHA256SUMS`; gitignored staging under `_local/releases/{version}/` (or repo-equivalent).
- **Create** `.github/workflows/release.yml` (or extend `windows-build.yml`) -- tag-push CD: validate tag↔version, build both flavors, finalize, `gh release` upload; keep publish-ci distinct from local tag-create.
- `scripts/packaging/verify-installer-contract.py` -- still asserts teVirtualMIDI three-gate contract; must diverge per flavor; optionally invoked from prepare-release verify.
- `installer/check-virtualmidi.ps1` -- operator helper for Win10 flavor only.
- External pattern (read-only inspiration): sibling Luthier `publish/prepare-release.py`, `publish/templates/*`, `.github/workflows/release.yml` — reuse staged CLI + one-Release-many-assets; do not copy PyInstaller/3-OS matrix.
- `src/Midi/MidiBackendSelect.{h,cpp}` -- resolve CLI/env → default WMS (`MidiBackendKind::Wms`); Win10 flavor needs virtualMIDI default (CLI/env/`--midi-backend` already exist).
- `src/Midi/MidiBackendFactory.cpp` -- constructs WMS vs VirtualMidiBackend.
- `src/App/Main.cpp` (~L79–104, help ~396–399) -- CLI/env wiring; flavor default must match packaging story.
- `src/App/MidiSessionMultiHost.cpp` (~L81) -- creates backend at live session start.
- `src/Midi/VirtualMidiBackend.cpp` -- runtime `LoadLibraryEx` of `teVirtualMIDI.dll` from System32; keep for Win10/lab; do not ship DLL.
- `src/Midi/WmsMidiBackend.*` + `Midi1StreamAssembler` -- **read-only preserve** 6.1 SendToHost / BufferFull retry behavior.
- `CMakeLists.txt` + `cmake/FetchMicrosoftMidiAppSdk.cmake` -- both backends compile today; flavor policy may stay runtime or Ask-First compile strip.
- `README.md` (License table) + `docs/dev/license-and-backends.md` -- three-way honesty still says WMS “next / not shipping” and Win10 not community; update to dual shipping paths (WMS=Win11 community; virtualMIDI=Win10 community **self-install** + lab).
- `docs/user/README.md` + `docs/user/unitor-mt4-bridge-user-guide.md` + `docs/user/unitor-mt4-bridge-guide-utilisateur.md` -- single bilingual pair, virtualMIDI-first; replace with four guides + OS aiguillage; redirect/retire old filenames.
- `docs/dev/authenticode-and-smartscreen.md` -- reuse SmartScreen honesty (OQ-3).
- `_bmad-output/implementation-artifacts/spec-6-1-windows-midi-services-midibackend-win11.md` -- continuity; packaging/docs were explicitly deferred to 6.2.
- `_bmad-output/implementation-artifacts/epic-6-context.md` -- still says Win10 drops as community claim; this story **expands** to parallel Win10 self-install offer (record in Design Notes / honesty, do not silently revert).
- Read-only reuse: Epic 4.1–4.4 installer/docs/license/SmartScreen smokes under `docs/tests/smoke-epic4-*.md` — update callers that deep-link old user-guide paths.

## Tasks & Acceptance

**Execution:**
- [x] `installer/public-installer.iss` (+ sibling or defines) -- dual Setup flavors: win11-wms without virtualMIDI gate; win10-virtualmidi with DLL presence gate and Tobias self-install messaging; never embed MSI/DLL.
- [x] `scripts/packaging/build-public-installer.ps1` -- produce clearly named dual artifacts for the same version; outputs consumable by prepare-release staging.
- [x] `scripts/packaging/prepare-release.py` (+ templates under `scripts/packaging/templates/` as needed) -- Luthier-style pack/finalize/verify/publish(+ci): stage both flavors, manuals/README, notes, SHA256 into `_local/releases/{version}/`; create/upload GitHub Release + tag.
- [x] `.github/workflows/release.yml` (or equivalent) -- tag-push automation: tag↔version check, build dual flavors, finalize, publish-ci asset upload.
- [x] `docs/dev/release-guide.md` -- short operator runbook (bump version → tag → automated Release); document dry-run/verify without publishing.
- [x] `scripts/packaging/verify-installer-contract.py` -- verify each flavor’s honesty gates (WMS path ≠ virtualMIDI three-gate; Win10 keeps self-install gate, no embed).
- [x] `src/App/Main.cpp` + `src/Midi/MidiBackendSelect.*` (+ packaging shortcut/params if needed) -- Win11 flavor defaults WMS; Win10 flavor defaults virtualMIDI; keep CLI/env override for lab.
- [x] `README.md` + `docs/dev/license-and-backends.md` -- refresh three-way table: WMS = community Win11 shipping path; virtualMIDI = community Win10 self-install + lab; OQ-1/OQ-3 unchanged.
- [x] `docs/user/unitor-mt4-bridge-win11-wms-user-guide.md` + `docs/user/unitor-mt4-bridge-win11-wms-guide-utilisateur.md` -- musician manuals (prereqs, Setup+SmartScreen+Zadig, Auto-Start, first MIDI/SysEx, non-geek MIDI recovery, what works/does not).
- [x] `docs/user/unitor-mt4-bridge-win10-virtualmidi-user-guide.md` + `docs/user/unitor-mt4-bridge-win10-virtualmidi-guide-utilisateur.md` -- motivated manuals (self-install virtualMIDI + why, Setup, first use, technical troubleshooting allowed, cross-link Win11 comfort path).
- [x] `docs/user/README.md` -- 20-second Windows 11 vs Windows 10 choice + language links; update SupportURL targets if needed.
- [x] `docs/user/unitor-mt4-bridge-user-guide.md` + `docs/user/unitor-mt4-bridge-guide-utilisateur.md` -- removed (obsolete single-path guides; router is `docs/user/README.md`).
- [x] Smoke / link callers (`docs/tests/smoke-epic4-user-docs-mt4.md`, license/SmartScreen smokes, root README) -- point at new paths.
- [x] `scripts/quality/lint-touched.py` -- run if C++ touched; green before review.

**Acceptance Criteria:**
- Given Story 6.1 WMS works, when Guillaume runs the in-repo release automation for a bumped version, then a clean staged tree and a GitHub Release/tag are produced with **both** win11-wms and win10-virtualmidi artifacts (plus manuals/docs and checksums) for that same version — without hand-assembling zips as the primary path.
- Given Win11 community materials, when a musician follows the Win11 guide, then install does not require virtualMIDI, documents SmartScreen + guided WinUSB, and recovers sticky MIDI without PowerShell Admin (reboot path).
- Given Win10 community materials, when a motivated user follows the Win10 guide, then they self-install virtualMIDI from Tobias Erichsen, verify `teVirtualMIDI.dll`, and never receive an embedded MSI/DLL from this project.
- Given public honesty pages, when an evaluator reads README + license-and-backends, then MIT ≠ virtualMIDI ≠ WMS is current, OQ-1/OQ-3 hold, and Win10 is an assumed parallel offer — not the comfort community promise.
- Given `docs/user/README.md`, when a new user lands, then they can choose Windows 11 vs Windows 10 in about 20 seconds and open the matching FR or EN guide.
- Given old single-guide filenames, when opened, then they redirect (or equivalent) so they cannot contradict the four path-specific manuals.

## Spec Change Log

## Design Notes

**Course expansion vs Epic 6 context:** Correct Course / `epic-6-context.md` framed Win10 as dropped community claim. Guillaume’s 6.2 Build expands to a **dual community line**: Win11 WMS = comfort; Win10 virtualMIDI = parallel self-install (not comfort). Do not “fix” docs back to Win10-drop wording.

**Packaging preference (unless Ask First flips it):** Prefer flavor-specific **defaults + installer gates + artifact names** over compile-time stripping VirtualMidiBackend on day one — both backends already coexist via CLI/env. Win11 Setup must stop requiring `teVirtualMIDI.dll`. Win10 Setup may keep the DLL presence gate and must keep “user installs driver” honesty.

**Release automation (Luthier pattern):** One semver tag → one GitHub Release → multiple named assets (`win11-wms` Setup, `win10-virtualmidi` Setup, docs package, `SHA256SUMS`, notes). Stages: pack → finalize → verify → publish-ci (CI) vs publish (local tag create, Ask First before agent pushes). Skip Luthier’s PyInstaller/3-OS matrix; keep Inno Setups as delivery. Staging stays gitignored.

**Manuals:** Migrate current virtualMIDI-first body into Win10 guides; author Win11 guides from the same USB/SmartScreen/Auto-Start/MIDI-SysEx spine with WMS prereqs and non-geek recovery. Do not invent a midisrv GUI helper here. Finalize step must pull the four guides + README into the release docs asset.

**Artifact naming example:** `UnitorMt4Bridge-Setup-win11-wms-{version}.exe` and `UnitorMt4Bridge-Setup-win10-virtualmidi-{version}.exe` (exact token order may match repo conventions if already present).

## Verification

**Commands:**
- `python scripts/packaging/verify-installer-contract.py` -- expected: both flavor contracts pass (or documented dual invocation).
- `python scripts/packaging/prepare-release.py verify` (or equivalent) -- expected: staged tree complete for both flavors + docs + checksums; dry-run OK without GitHub publish.
- `python scripts/quality/lint-touched.py` -- expected: green if C++ changed.
- Build Bridge + both Setups via packaging scripts -- expected: two distinct outputs, same version string.

**Manual checks:**
- Spot-read four manuals + README aiguillage for OS choice, OQ-1/OQ-3, SmartScreen, Zadig, and Win11 no-PowerShell recovery.
- Confirm old guide paths redirect and root README License table matches `license-and-backends.md`.
- Confirm `docs/dev/release-guide.md` describes the automated path as primary (not hand upload).

## Suggested Review Order

**Dual Setup flavors**

- Flavor defines, consistency `#error`, and named Setup output.
  [`public-installer.iss:24`](../../installer/public-installer.iss#L24)

- Win11 prereq fail-closed vs Win10 `teVirtualMIDI.dll` gate.
  [`public-installer.iss:247`](../../installer/public-installer.iss#L247)

- Build both flavors from one script without stale sibling Setups.
  [`build-public-installer.ps1:126`](../../scripts/packaging/build-public-installer.ps1#L126)

**Release automation (Luthier-style)**

- Pack → finalize → verify → publish-ci upload-only pipeline.
  [`prepare-release.py:101`](../../scripts/packaging/prepare-release.py#L101)

- Tag-push CD with contract check before assets.
  [`release.yml:1`](../../.github/workflows/release.yml#L1)

- Operator primary path: tag-push CI vs local publish (not both).
  [`release-guide.md:1`](../../docs/dev/release-guide.md#L1)

**Backend defaults / Auto-Start**

- Auto-Start bakes `--midi-backend` so logon matches Setup flavor.
  [`AutoStartRegistration.cpp:18`](../../src/App/AutoStartRegistration.cpp#L18)

- Catch2 locks WMS vs virtualMIDI Auto-Start argument strings.
  [`AutoStartContractTests.cpp:1`](../../tests/unit/AutoStartContractTests.cpp#L1)

**Honesty + manuals**

- Twenty-second Win11 vs Win10 router and same-AppId overwrite note.
  [`README.md:5`](../../docs/user/README.md#L5)

- Three-way MIT ≠ virtualMIDI ≠ WMS shipping claims refreshed.
  [`license-and-backends.md:16`](../../docs/dev/license-and-backends.md#L16)

- Musician Win11 path (no PowerShell primary recovery).
  [`unitor-mt4-bridge-win11-wms-user-guide.md:1`](../../docs/user/unitor-mt4-bridge-win11-wms-user-guide.md#L1)

- Motivated Win10 self-install virtualMIDI path.
  [`unitor-mt4-bridge-win10-virtualmidi-user-guide.md:1`](../../docs/user/unitor-mt4-bridge-win10-virtualmidi-user-guide.md#L1)

**Contract gate**

- Offline dual-flavor honesty needles for ISS + manuals.
  [`verify-installer-contract.py:1`](../../scripts/packaging/verify-installer-contract.py#L1)

### Review Findings

*(Code review chunk 1 — packaging / Setup / CI / release guide, 2026-08-21)*

- [x] [Review][Patch] WMS gate must require midisrv RUNNING (not mere registration) — decided: option 2 [`installer/public-installer.iss:188`]
- [x] [Review][Patch] Prerelease notes/docs interpolate full tag into Setup filenames while EXEs use CMake bare version [`scripts/packaging/prepare-release.py:195`]
- [x] [Review][Patch] release.yml pack/finalize/verify does not fail-fast on non-zero Python exit [`/.github/workflows/release.yml:98`]
- [x] [Review][Patch] Release shell create omits `--prerelease` for RC tags (stable until publish-ci edit) [`/.github/workflows/release.yml:113`]
- [x] [Review][Patch] UTF-8 BOM on packaging entrypoints breaks Unix shebang [`scripts/packaging/prepare-release.py:1`]
- [x] [Review][Patch] Hardcoded `tensquaresoftware/unitor-win64-driver` instead of `github.repository` [`/.github/workflows/release.yml:110`]
- [x] [Review][Patch] Missing `RELEASE_NOTES.md` existence check before `gh release create` [`/.github/workflows/release.yml:109`]
- [x] [Review][Patch] Release guide incomplete: `-BridgeDir`, publish flags, SHA256SUMS/notes assets, AppId overwrite, optional Authenticode, prerelease naming split [`docs/dev/release-guide.md:1`]
- [x] [Review][Patch] ISS header still says Ask First unanswered for shared AppId [`installer/public-installer.iss:6`]
- [x] [Review][Patch] Existing Release path never refreshes body from staged `RELEASE_NOTES.md` [`scripts/packaging/prepare-release.py:368`]
- [x] [Review][Patch] `git ls-remote` / `gh release view` treat infra failures like “missing” [`scripts/packaging/prepare-release.py:298`]
- [x] [Review][Patch] Partial docs zip not removed if ZipFile write fails [`scripts/packaging/prepare-release.py:181`]
- [x] [Review][Patch] Sign failure after ISCC leaves orphan Setup EXEs in dist/ [`scripts/packaging/build-public-installer.ps1:343`]
- [x] [Review][Defer] Dual-flavor installer gates are needle-checked only, never executed — deferred, pre-existing offline-contract design
- [x] [Review][Defer] prepare-release pack/verify has no merge-gate unit execution — deferred, follow-up test harness
- [x] [Review][Defer] Smoke/operator guides still teach old Setup path/names — deferred to lot 2 (docs/smoke review)
- [x] [Review][Defer] CI `choco install innosetup` unpinned — deferred, pre-existing CI drift risk
