---
baseline_commit: 32e515a
---

# Story 4.4: Authenticode policy and SmartScreen honesty

Status: done

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a community downloader,
I want signed builds when possible, and clear SmartScreen guidance if a public build ships unsigned,
so that trust issues do not silently kill adoption.

## Acceptance Criteria

1. **Given** a public community build pipeline for the Bridge/installer  
   **When** Authenticode signing is available  
   **Then** signed builds use the Ten Square Software / chosen certificate path documented for the release — FR-15 / NFR-S1

2. **And** if a public build ships unsigned because the certificate lags, docs explain SmartScreen behavior and mitigation steps — AD-19

3. **And** Authenticode remains strongly recommended but **not** a hard V1 gate

4. **And** OQ-3 (personal vs org certificate) stays deferred to Guillaume before first tagged public community release — do not block this story

**Traces:** FR-15, NFR-S1, AD-19, SM-6; deferred OQ-3

## Tasks / Subtasks

- [x] Task 1: Lock Authenticode / SmartScreen contract + fences (AC: 1–4)
  - [x] Author operator smoke guide `docs/tests/smoke-epic4-authenticode-smartscreen-mt4.md` (kebab-case) with English Pass/Fail matrix proving FR-15 / NFR-S1 / AD-19. Prefer **docs-first** verification (no purchased certificate required for Pass). Blank lab rows ≠ Pass
  - [x] State product intent: a community downloader can understand (a) whether this release is signed, (b) what to do if Windows SmartScreen warns on an unsigned or low-reputation Setup, and (c) that signing is strongly recommended but not a V1 hard gate — without tribal knowledge from `_bmad-output/`
  - [x] Explicit fences:
    - Public Installer AD-12 UX / wizard redesign → **4.1** (string/messaging only if bind-fail honesty needs a clearer unsigned/SmartScreen pointer; no wizard redesign)
    - End-user UJ-1 / UJ-2 chapter structure under `docs/user/` → **4.2** (add SmartScreen section / troubleshooting; do **not** reopen single-file manual shape)
    - Three-way MIT ≠ virtualMIDI ≠ Windows MIDI Services → **4.3** (cross-link only; do not rewrite license page)
    - Tobias MSI **embed** redistributable clearance → **OQ-1** (never claim cleared under this story)
    - OQ-3 personal vs org certificate purchase / cost / timing → **Guillaume** before first tagged public community release (document the *policy*; do **not** decide the choice)
    - MIDI Path latency claims / harness → Epic **5**
    - Kernel / WHQL / Partner Center attestation signing → **never** (usermode Bridge + WinUSB INF catalog only)
  - [x] Honesty bar: this story may claim **FR-15 / NFR-S1** closed when its ACs Pass. Phrase **SM-6** as closable for the **Authenticode/SmartScreen slice** once smoke Passes — but do **not** claim full community-release honesty if OQ-1 MSI embed is still open or 4.1 hardware smoke rows remain blank (SM-5 / installer). Completion Notes must name remaining gates explicitly
  - [x] Cite SSOT: epics Story 4.4; PRD FR-15 / NFR-S1 / SM-6 / OQ-3; AD-19; SPEC Authenticode constraint (no CAP for FR-15 — do **not** invent CAP-15 for signing; CAP-15 is DeviceProfile)
  - [x] Cross-link: replace “→ 4.4” / “Authenticode / SmartScreen remains Story 4.4” placeholders in README, `docs/dev/license-and-backends.md`, `docs/dev/winusb-bind.md`, and Epic 4 smokes with pointers to the finished surfaces

- [x] Task 2: Ship end-user SmartScreen honesty (AC: 2–3)
  - [x] Language: musician-facing prose in **English** user guide + keep **French** peer in sync (`docs/user/unitor-mt4-bridge-user-guide.md` ↔ `docs/user/unitor-mt4-bridge-manuel-utilisateur.md`) for shared product facts (SmartScreen steps, unsigned honesty, Ten Square facade)
  - [x] Add a short Installation subsection and/or Troubleshooting entry covering Microsoft Defender SmartScreen when the public Setup is **unsigned** or **not yet reputation-trusted**:
    1. Expected UI: “Windows protected your PC” / unrecognized app (wording may vary by Windows version)
    2. Mitigation on consumer PCs where policy allows: **More info** → **Run anyway** (only after the user confirms they downloaded from the project’s official channel)
    3. Alternate mitigation: file Properties → **Unblock** when the Zone.Identifier mark-of-the-web is present
    4. Honesty: enterprise / managed PCs may **block override** entirely — docs must say so; do not promise “Run anyway always works”
    5. Honesty: even a **valid** Authenticode signature can still warn until SmartScreen reputation accumulates (Microsoft: OV/EV no longer buy instant trust). Do not claim “signed = never SmartScreen”
  - [x] State clearly: Authenticode is **strongly recommended** for public builds; V1 may ship unsigned if the certificate lags — **only** with this guidance present
  - [x] Keep facade **Ten Square Software**; do not invent a different publisher name in user prose
  - [x] Optional one-line discoverability pointer from `docs/user/README.md` once the section exists
  - [x] Do **not** tell users to disable SmartScreen globally, whitelist entire folders, or run random unsigned copies from third-party mirrors

- [x] Task 3: Ship contributor / release Authenticode + catalog policy (AC: 1, 3–4)
  - [x] Add kebab-case deep page (recommended): `docs/dev/authenticode-and-smartscreen.md` covering:
    1. **Two signing domains** (must not be conflated):
       - **A)** Authenticode on `Bridge.exe` / `UnitorMt4Bridge-Setup.exe` (SmartScreen / publisher trust)
       - **B)** WinUSB INF catalog (`.cat`) for clean-machine driver-package trust (`CatalogFile=mt4-winusb.cat` in `installer/mt4-winusb.inf`)
    2. **Lab vs public:** `installer/sign-lab-package.ps1` = self-signed lab catalog + LocalMachine Root/TrustedPublisher staging — **not** community trust; never describe it as public Authenticode
    3. **When a cert exists:** document the Ten Square Software / chosen certificate path for the release (Subject, how SignTool is invoked, timestamping expectation, which artifacts are signed). Leave OQ-3 (personal vs org) as **Deferred — Guillaume before first tagged public community release**
    4. **When cert lags:** unsigned public builds are allowed for V1 **only** with user SmartScreen docs (Task 2); strongly recommended to sign later
    5. **CI honesty:** merge CI (`.github/workflows/windows-build.yml`) stays compile-only; Authenticode packaging / `release.yml` remains omitted unless Guillaume explicitly adds a separate release workflow with secrets — do not invent merge-gate signing theater
    6. Pointers to user SmartScreen section + `docs/dev/windows-ci-toolchain.md` “omitted on purpose” table
  - [x] Replace the stub “Signing note” in `docs/dev/winusb-bind.md` with a short summary + link to the deep page (keep Zadig = contributor fallback only)
  - [x] Retarget `docs/dev/license-and-backends.md` fence from “Story 4.4 (not this page)” to the finished Authenticode/SmartScreen surface
  - [x] Update root `README.md` Status / Roadmap so Authenticode is no longer “remains 4.4”; link the policy docs; keep “strongly recommended / not hard gate” wording
  - [x] Do **not** commit private keys, PFX files, or CI secrets into the repo
  - [x] Do **not** treat gitignored lab `installer/*.cat` as a shipped production artifact without a defined public catalog path

- [x] Task 4: Optional packaging hooks + clean-machine catalog honesty (AC: 1, 3)
  - [x] Document (and optionally implement behind an env/cert gate) a **separate** public signing helper — e.g. `scripts/packaging/sign-public-artifacts.ps1` — that signs Setup/Bridge with a real code-signing certificate when available. Must be clearly distinct from `installer/sign-lab-package.ps1`
  - [x] If implementing the hook: call it from `scripts/packaging/build-public-installer.ps1` only when cert/env is present; unsigned build must still succeed (Authenticode is **not** a hard gate)
  - [x] Public INF catalog policy for clean machines (closes `deferred-work.md` fence):
    - Document current honesty: Public Installer ships `mt4-winusb.inf` **without** a production `.cat` today; clean-machine association may fail until a real catalog is produced
    - Lab mitigation remains `installer/sign-lab-package.ps1` (contributor/lab only)
    - If a **public** catalog signing path becomes available under the same chosen certificate, document how `.cat` is produced and whether it is packaged into the installer; do **not** ship the lab self-signed `.cat` as if it were Trusted Root community trust
    - Update installer bind-fail copy only if needed so users/operators are pointed at shipped docs rather than “Story 4.4” placeholders
  - [x] If `scripts/packaging/verify-installer-contract.py` is touched: assert presence of SmartScreen / unsigned honesty references on user or smoke surfaces — do not require a purchased cert for the contract to Pass
  - [x] Prefer docs + optional gated script over Bridge/MIDI C++ churn

- [x] Task 5: Operator verification + quality (AC: all)
  - [x] Fill `docs/tests/smoke-epic4-authenticode-smartscreen-mt4.md` Pass/Fail rows, minimum:
    1. User docs (EN) explain SmartScreen behavior + mitigation if public Setup is unsigned / unrecognized (AD-19)
    2. FR user peer covers the same SmartScreen facts (not a stale EN-only island)
    3. Public/contributor surface states Authenticode **strongly recommended** but **not** a hard V1 gate (FR-15 / NFR-S1)
    4. When signing is documented as available: Ten Square Software / chosen certificate path is written down (without closing OQ-3)
    5. OQ-3 remains explicitly deferred to Guillaume before first tagged public community release
    6. Lab `sign-lab-package.ps1` is clearly labeled **not** public Authenticode; two domains (binary Authenticode vs INF catalog) are distinguished
    7. Discoverability: community reader can reach SmartScreen / signing honesty from README and/or `docs/user/` without opening `_bmad-output/`
    8. Scope fence: no AD-12 wizard redesign; no 4.3 three-way rewrite; no OQ-1 MSI embed; no kernel/WHQL attestation; no claim that signed == never SmartScreen; no secrets in repo
  - [x] Update Epic 4 smoke fences that still say “Authenticode → 4.4” / “do not claim SM-6 Authenticode closed” to point at finished surfaces + this smoke guide; adjust SM-6 wording carefully (see Honesty bar)
  - [x] Resolve or restate the unsigned INF / missing `.cat` item in `_bmad-output/implementation-artifacts/deferred-work.md` once the public catalog policy is documented
  - [x] Prefer **documentation-first** change set. If touching installer strings / PowerShell / C++: `python scripts/quality/lint-touched.py` exits 0 on touched C++; no French in sources; kebab-case paths; commits only on Guillaume’s ask
  - [x] Confirm no Tobias MSI embed; no latency Studio-Done claims; no Zadig-as-primary community path; no hard-gate on missing certificate

## Dev Notes

### Soft dependencies

| Dependency | Status | How this story treats it |
|---|---|---|
| Epic 1–3 Bridge + virtualMIDI backend | **done** | Do not reopen MIDI protocol or backend code |
| Story **4.1** Public Installer | **in-progress** | Messaging / optional sign hook / catalog honesty only; no AD-12 redesign; Win10 smoke matrix may still be blank (SM-5) |
| Story **4.2** End-user docs | **done** | Add SmartScreen section; preserve single-file EN+FR manuals and `MT4 In/Out N` names |
| Story **4.3** License honesty | **done** | Cross-link only; SM-6 license slice already Pass; this story owns trust/signing slice |
| OQ-1 Tobias MSI clearance | **open** | Must remain explicitly gated; never claim redistributable embed |
| OQ-3 Authenticode personal vs org | **open** | Document policy; do **not** decide or block |
| `project-context.md` | **absent** | Do not block on generating it; use this story + conventions + spine |

### Scope fence

This story ships **Authenticode / SmartScreen trust policy** and the **honesty docs** (plus optional gated signing helpers) so community downloaders are not silently blocked by Windows warnings, and so lab self-signing is never mistaken for public trust. It is **not** installer UX redesign, **not** license/backend rewrite, and **not** a forced certificate purchase.

| In scope | Out of scope (later / never) |
|---|---|
| User SmartScreen guidance if unsigned / low reputation (AD-19) | Redesigning AD-12 installer wizard → **4.1** |
| Contributor Authenticode + INF catalog policy (lab vs public) | Rewriting UJ manuals’ overall structure → **4.2** |
| Document Ten Square / chosen cert path when signing exists | Reopening three-way MIT ≠ virtualMIDI ≠ WMS → **4.3** |
| Keep OQ-3 deferred; strongly recommended ≠ hard gate | Deciding personal vs org cert / buying cert → **OQ-3 / Guillaume** |
| Optional env-gated public SignTool helper (distinct from lab) | Embedding virtualMIDI MSI → **OQ-1** |
| Operator smoke proving trust honesty | MIDI Path / latency Studio-Done → Epic **5** |
| Close deferred-work unsigned INF/`.cat` **policy** honesty | Kernel / WHQL / attestation signing |
| EN + FR user peer sync for SmartScreen facts | Disabling SmartScreen globally / unsafe trust advice |
| | Committing PFX / secrets / treating lab `.cat` as public Root trust |
| | Making merge CI fail without a signing certificate |

### Architecture compliance (must follow)

- **AD-19:** if a public build ships unsigned, **user** docs explain SmartScreen behavior and mitigation; facade **Ten Square Software**; personal-vs-org cert remains Deferred for Guillaume
- **AD-12:** INF / packaging under `installer/`; do not reopen WiX vs Inno or wizard UX under this story
- **AD-13:** Windows CI compile is the minimum merge gate; Authenticode packaging is **not** required by AD-13; artifacts under `builds/`
- **NFR-S1 / FR-15:** Authenticode strongly recommended; unsigned public build allowed **only** with SmartScreen documentation
- **NFR-S2 / non-goal:** no custom kernel → no Microsoft attestation / Partner Center kernel signing
- **Structural seed:** `docs/user/` = end-user SmartScreen home; `docs/dev/` = contributor/release signing runbook; `docs/tests/` = operator smoke; `installer/` = INF + lab helper; `builds/` = outputs only
- **Conventions:** kebab-case paths; no top-level `Documentation/`; English in shipped public/contributor docs; no French in C++/scripts if touched; commits only on Guillaume’s ask; Ten Square Software org identity

### Required messaging content (must appear)

**For community users (plain language):**

1. Windows may show SmartScreen (“Windows protected your PC”) for an unrecognized or unsigned Setup — that does **not** automatically mean the file is malware.
2. Only continue if the download came from the project’s official channel; then use **More info → Run anyway** when the OS allows it, or Properties → **Unblock**.
3. Managed PCs may block the override — contact the PC admin / use a personal machine for community eval.
4. Signing (Authenticode) is planned/recommended under **Ten Square Software**; if this build is unsigned, that is an honesty-documented V1 allowance, not a silent omission.

**For contributors / releasers:**

1. Lab catalog script ≠ public Authenticode.
2. Binary signing and INF `.cat` are related but separate trust problems.
3. OQ-3 is still open; do not pretend a cert choice was made.
4. Strongly recommended ≠ hard gate; do not fail the Public Installer build solely because SignTool secrets are missing.
5. Even signed files can SmartScreen-warn until reputation accumulates (Microsoft Learn: OV/EV no longer bypass; self-signed ≈ unsigned for SmartScreen).

### Current baseline (do not regress)

Already present (incomplete vs ACs — polish / extend, do not delete):

- `installer/sign-lab-package.ps1` — lab-only self-signed `mt4-winusb.cat`; header already says Story 4.4 is separate
- `installer/mt4-winusb.inf` — `CatalogFile = mt4-winusb.cat`; Provider Ten Square Software; `.cat` gitignored / not in Public Installer `[Files]`
- `installer/public-installer.iss` — ships INF without `.cat`; bind-fail text still mentions Story 4.4 / lab script
- `scripts/packaging/build-public-installer.ps1` — builds Setup to `builds/installer/`; **no** SignTool step
- `.github/workflows/windows-build.yml` — compile + tests only
- `docs/dev/windows-ci-toolchain.md` — lists `release.yml` / Authenticode packaging as **omitted on purpose**
- `docs/dev/winusb-bind.md` — stub Signing note → Story 4.4
- `docs/dev/license-and-backends.md` — fence “Authenticode / SmartScreen … Story 4.4”
- `docs/user/*` — Installation has no SmartScreen chapter yet
- `README.md` — Status still says Authenticode / SmartScreen remains Story 4.4
- `_bmad-output/implementation-artifacts/deferred-work.md` — unsigned INF / missing `.cat` fenced to 4.4

### Recommended authoring shape

1. **User first:** short SmartScreen block in Installation + a Troubleshooting bullet (EN+FR).
2. **One contributor deep page** for policy / lab vs public / optional SignTool runbook — README links it; do not dump SignTool flags into the musician manual.
3. **Optional gated script** only if it clearly helps a future tagged release; docs alone can Pass FR-15 when no cert exists yet.
4. **Keep lab script:** document and keep; do not “upgrade” it into fake public trust by installing self-signed roots on user machines.
5. **SM-6 phrasing:** after Pass, say the Authenticode/SmartScreen honesty slice is met; call out remaining SM-6 / release gates (OQ-1, any open 4.1 smoke) explicitly.

### Previous story intelligence

- **4.3 (done):** Docs-first Epic 4 pattern — smoke matrix, fences, English contributor prose, light EN↔FR user sync only for shared facts. SM-6 marked **partially** met (license/backend); this story owns the remaining trust slice. Review lessons: keep honesty precise (do not overclaim); mark English/technical deep links in FR manual when linking contributor pages; do not invent NOTICE/THIRD_PARTY without redistributables.
- **4.2 (done):** Single-file EN + FR manuals (not multi-chapter tree). Explicitly fenced SmartScreen here. Port names live as `MT4 In/Out N`. Add a section; do not reopen chapter tree.
- **4.1 (in-progress):** Inno Public Installer; unsigned INF / missing `.cat` Review[Defer] → 4.4; lab mitigation `sign-lab-package.ps1`. Do not redesign wizard; string clarity OK.
- **1.3:** Introduced lab INF signing helper (`06b6fe1`); unsigned INF OK for contributor bind; public catalog → 4.4.

### Git intelligence

Recent commits (`5e294ba` / `12eff9a` Story 4.3, `fc29bfb` Story 4.2, `538ceea` Story 4.1, `0d4ce20` virtualMIDI installer spelling) show Epic 4 is **docs + packaging** heavy. Production Authenticode has **not** started beyond the lab catalog helper. Prefer markdown + optional packaging PowerShell over Bridge/MIDI C++ churn. No CI secrets or SignTool-for-Setup commits exist — keep it that way unless Guillaume supplies a real cert workflow.

### Latest tech notes (as of 2026-08)

- Microsoft Defender SmartScreen is **reputation-based**, not “signed = silent.” Per Microsoft Learn (`smartscreen-reputation`): unsigned → “Windows protected your PC” / More info → Run anyway (unless enterprise policy blocks); valid OV/EV still warn until reputation accumulates; **EV no longer bypasses** SmartScreen; self-signed behaves like unsigned for SmartScreen.
- User-facing mitigation vocabulary to document: **More info** / **Run anyway**; Properties **Unblock** (mark-of-the-web); honesty about managed-PC lockouts.
- SignTool + timestamping remain the standard usermode Authenticode path; Windows Kit tools already located by `sign-lab-package.ps1`’s `Find-KitTool` pattern — reuse discovery ideas, do not conflate lab cert creation with public signing.
- Do not recommend paying EV premium solely to “skip SmartScreen” — outdated advice; consistency of publisher identity + real downloads build reputation.
- Reference for implementers: https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/smartscreen-reputation

### Project structure notes

```text
docs/user/unitor-mt4-bridge-user-guide.md              # UPDATE — SmartScreen install/troubleshoot
docs/user/unitor-mt4-bridge-manuel-utilisateur.md      # UPDATE — FR peer sync
docs/user/README.md                                    # OPTIONAL pointer
docs/dev/authenticode-and-smartscreen.md               # NEW (recommended) — lab vs public policy
docs/dev/winusb-bind.md                                # UPDATE — replace Signing stub
docs/dev/license-and-backends.md                       # UPDATE — retarget 4.4 fence
docs/dev/windows-ci-toolchain.md                       # OPTIONAL cross-link
README.md                                              # UPDATE — remove “remains 4.4”; link policy
docs/tests/smoke-epic4-authenticode-smartscreen-mt4.md # NEW — FR-15 / AD-19 matrix
docs/tests/smoke-epic4-*.md                            # UPDATE fences / SM-6 wording
installer/sign-lab-package.ps1                         # KEEP — document as lab-only
installer/public-installer.iss                         # OPTIONAL copy clarity (no UX redesign)
scripts/packaging/build-public-installer.ps1           # OPTIONAL env-gated SignTool hook
scripts/packaging/sign-public-artifacts.ps1            # NEW optional — distinct from lab
scripts/packaging/verify-installer-contract.py         # OPTIONAL contract asserts
_bmad-output/.../deferred-work.md                      # UPDATE — resolve/restate .cat fence
_bmad-output/.../4-4-*.md                              # this story file
```

Do **not** create `Documentation/`. Do **not** invent a second user-doc tree. Do **not** add merge-CI signing jobs that fail without secrets.

### Testing requirements

- SmartScreen / policy smoke can Pass on **content verification** alone (no purchased cert, no SmartScreen UI capture required for docs rows).
- Optional: on a Windows lab machine, screenshot or note SmartScreen UI only if an unsigned Setup is exercised — nice-to-have, not a hard Pass gate for FR-15 docs rows.
- Still list Win10 when any hardware/bind catalog claim is checked; Win10 mandatory in matrix policy elsewhere.
- Offline/macOS edit machine can author markdown; any installer/script change validates on Windows if packaging contracts are affected.
- Quality: English public/contributor docs; FR user peer sync for SmartScreen; `lint-touched.py` only if C++ touched; no French in sources/scripts.
- Anti-lies: grepping for committed `.pfx` / private keys; confirming lab script is still labeled lab-only; confirming docs do not claim EV bypasses SmartScreen.

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Epic 4 / Story 4.4]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — FR-15, NFR-S1, SM-6, OQ-3, §11 Authenticode]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-19, Deferred Authenticode personal vs org]
- [Source: `_bmad-output/specs/spec-unitor-win64-driver/SPEC.md` — Authenticode constraint; OQ-3]
- [Source: `_bmad-output/implementation-artifacts/4-3-technical-docs-and-three-way-license-honesty.md`]
- [Source: `_bmad-output/implementation-artifacts/4-2-end-user-documentation-for-first-midi-and-sysex.md`]
- [Source: `_bmad-output/implementation-artifacts/4-1-public-installer-meeting-ad-12-ux-bar.md`]
- [Source: `_bmad-output/implementation-artifacts/deferred-work.md` — unsigned INF / missing `.cat`]
- [Source: `installer/sign-lab-package.ps1`]
- [Source: `installer/mt4-winusb.inf`]
- [Source: `docs/dev/winusb-bind.md` — Signing note stub]
- [Source: `docs/dev/license-and-backends.md`]
- [Source: `docs/dev/windows-ci-toolchain.md`]
- [Source: `docs/user/unitor-mt4-bridge-user-guide.md` — Installation]
- [Source: `README.md` — Status / Roadmap]
- [Source: https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/smartscreen-reputation]

## Dev Agent Record

### Agent Model Used

Composer (Cursor agent router)

### Debug Log References

- `python scripts/packaging/verify-installer-contract.py` → OK
- `powershell -File scripts/packaging/sign-public-artifacts.ps1` without cert → SKIP exit 0 (ASCII-only strings; em-dash broke Windows PowerShell parse)

### Completion Notes List

- Shipped docs-first Authenticode / SmartScreen honesty: EN+FR user SmartScreen install + troubleshooting; contributor deep page distinguishing binary Authenticode vs INF catalog and lab vs public.
- Optional gated `sign-public-artifacts.ps1` hooked from `build-public-installer.ps1` via `UNITOR_CODE_SIGNING_CERT_SUBJECT` — unsigned packaging still succeeds.
- FR-15 / NFR-S1 closable from docs smoke Pass. **SM-6 Authenticode/SmartScreen slice** met; remaining community-release gates: **OQ-1** (Tobias MSI embed), Story **4.1** blank hardware smoke rows (SM-5), **OQ-3** cert personal vs org (policy documented, not decided).
- No PFX/secrets committed; no wizard redesign; no three-way license rewrite; no MSI embed; no kernel/WHQL claims; no “signed = never SmartScreen”.

### File List

- `docs/dev/authenticode-and-smartscreen.md` (new)
- `docs/tests/smoke-epic4-authenticode-smartscreen-mt4.md` (new)
- `scripts/packaging/sign-public-artifacts.ps1` (new)
- `docs/user/unitor-mt4-bridge-user-guide.md`
- `docs/user/unitor-mt4-bridge-manuel-utilisateur.md`
- `docs/user/README.md`
- `docs/dev/winusb-bind.md`
- `docs/dev/license-and-backends.md`
- `docs/dev/windows-ci-toolchain.md`
- `docs/tests/smoke-epic4-public-installer-mt4.md`
- `docs/tests/smoke-epic4-user-docs-mt4.md`
- `docs/tests/smoke-epic4-license-honesty-mt4.md`
- `README.md`
- `installer/public-installer.iss`
- `installer/sign-lab-package.ps1`
- `scripts/packaging/build-public-installer.ps1`
- `scripts/packaging/verify-installer-contract.py`
- `_bmad-output/implementation-artifacts/deferred-work.md`
- `_bmad-output/implementation-artifacts/sprint-status.yaml`
- `_bmad-output/implementation-artifacts/4-4-authenticode-policy-and-smartscreen-honesty.md`

### Review Findings

- [x] [Review][Decision] Name the “official channel” for Setup downloads — Resolved: use generic “project download page / Releases” wording (no fixed URL until first public tag); harden URL at release (see deferred-work).
- [x] [Review][Patch] User docs: official channel = project download page / Releases (generic, no fixed URL yet) [`docs/user/unitor-mt4-bridge-user-guide.md`:85]
- [x] [Review][Patch] Sign Bridge.exe before packaging when cert env is set [`scripts/packaging/build-public-installer.ps1`:133]
- [x] [Review][Patch] Fail closed if cert env is set but sign helper is missing [`scripts/packaging/build-public-installer.ps1`:136]
- [x] [Review][Patch] Reject empty Paths when signing is requested [`scripts/packaging/sign-public-artifacts.ps1`:52]
- [x] [Review][Patch] Avoid leaving a mixed signed/unsigned set if a later Path fails [`scripts/packaging/sign-public-artifacts.ps1`:52]
- [x] [Review][Patch] Discover SignTool under x64 or arm64 Kit layouts; prefer a current kit [`scripts/packaging/sign-public-artifacts.ps1`:42]
- [x] [Review][Patch] After SignTool, verify the signature matches the intended Subject [`scripts/packaging/sign-public-artifacts.ps1`:66]
- [x] [Review][Patch] Assert FR SmartScreen mitigation strings in the installer contract [`scripts/packaging/verify-installer-contract.py`:160]
- [x] [Review][Patch] Stabilize FR SmartScreen heading anchors (ASCII-safe fragment) [`docs/user/unitor-mt4-bridge-manuel-utilisateur.md`:81]
- [x] [Review][Patch] Tell users how to check Digital Signatures / publisher on Setup [`docs/user/unitor-mt4-bridge-user-guide.md`:81]
- [x] [Review][Defer] Production INF `.cat` still not produced/packaged [`docs/dev/authenticode-and-smartscreen.md`] — deferred, pre-existing (waits on OQ-3 cert path; policy already honest)
- [x] [Review][Defer] SignTool `/d` `/du` (and dual-sign) metadata not in helper [`scripts/packaging/sign-public-artifacts.ps1`:66] — deferred, pre-existing
- [x] [Review][Defer] Harden SmartScreen “official channel” to a concrete download URL at first tagged public release [`docs/user/`] — deferred, intentional until release
### Change Log

- 2026-08-10: Story 4.4 implementation — Authenticode/SmartScreen honesty docs, optional gated SignTool helper, Epic 4 smoke fence retargets; status → review
- 2026-08-10: Code review triage — 1 decision-needed, 9 patch, 2 defer
- 2026-08-10: Code review patches applied (signing helper guards, EN/FR SmartScreen honesty, contract asserts); status → done
