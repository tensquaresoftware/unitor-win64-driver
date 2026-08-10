---
baseline_commit: fc29bfbfe7b79d462ad686411361f4e1331d62ce
---

# Story 4.3: Technical docs and three-way license honesty

Status: done

<!-- Ultimate context engine analysis completed - comprehensive developer guide created -->

## Story

As a contributor or community evaluator,
I want technical docs and public messaging that keep MIT, VirtualMIDI, and future Windows MIDI Services claims honest,
so that the project stays credible and legally clear.

## Acceptance Criteria

1. **Given** the public repository and README/docs surfaces  
   **When** a reader checks license and backend claims  
   **Then** materials state clearly: MIT (this repo) ≠ VirtualMIDI (proprietary, separate) ≠ Windows MIDI Services (future Win11-only backend, not V1) — FR-14 / CAP-14 / AD-19

2. **And** no GPL Linux sources are vendored; `aaron1a12/virtual-midi` is cited as integration proof only, not a fork base

3. **And** contributor docs describe the dual-machine loop (edit on macOS / validate on Windows x64; Win10 mandatory in matrix) — NFR-D3 / AD-13

4. **And** public facade is **Ten Square Software**

**Traces:** FR-14, CAP-14, AD-13, AD-19, SM-6

## Tasks / Subtasks

- [x] Task 1: Lock tech-docs / license contract + fences (AC: 1–4)
  - [x] Author operator smoke guide `docs/tests/smoke-epic4-license-honesty-mt4.md` (kebab-case) with English Pass/Fail matrix proving FR-14 / CAP-14 surfaces. Prefer **docs-only** verification (no physical MT4 required for Pass). Blank lab rows ≠ Pass; Win10 x64 still listed when any hardware-adjacent claim is checked
  - [x] State product intent: a community reader or contributor can tell what this repo licenses, what VirtualMIDI requires separately, and that Windows MIDI Services is not the V1 backend — without tribal knowledge from planning artifacts
  - [x] Explicit fences:
    - Public Installer AD-12 UX / packaging → **4.1** (do **not** redesign the wizard; keep OQ-1 MSI-embed honesty as-is)
    - End-user UJ-1 / UJ-2 manuals under `docs/user/` → **4.2** (do **not** reopen chapter structure; light cross-links OK)
    - Authenticode / SmartScreen honesty for unsigned public builds → **4.4**
    - Tobias MSI **embed** redistributable clearance → **OQ-1** release gate only (never claim cleared under this story)
    - MIDI Path latency claims / harness → Epic **5**
    - Protocol reimplementation / Emagic mapper code → already done Epics 1–2; this story documents honesty, does **not** reopen MIDI code
  - [x] Honesty bar: do **not** claim SM-6 closed until this story’s smoke Passes **and** Story **4.4** SmartScreen/Authenticode messaging is addressed (SM-6 also covers SmartScreen if unsigned). This story may claim **FR-14 / CAP-14** closed when its ACs Pass; phrase SM-6 as **partially** met (license/backend honesty) without claiming the full community-release honesty gate
  - [x] Cite SSOT: epics Story 4.3; PRD FR-14 / NFR-D3 / NFR-Q2 / NFR-Q3 / SM-6; AD-7 / AD-13 / AD-14 / AD-19; SPEC CAP-14; PRD addendum VirtualMIDI licensing
  - [x] Cross-link: replace “→ 4.3” / “Full three-way honesty polish is Story 4.3” placeholders in README and Epic 4 smokes with pointers to the finished surfaces

- [x] Task 2: Ship three-way license + backend honesty on public surfaces (AC: 1–2, 4)
  - [x] Language: **English** for public OSS / contributor prose (`document_output_language`). Do **not** invent a second French legal manual; keep FR user manual peers from 4.2 aligned only if a short WMS / license cross-link needs a one-line sync
  - [x] Polish root `README.md` § License (and related Acknowledgments / status blurb) so the three-way split is explicit and self-contained:
    1. **MIT** — this repository’s own Bridge / installer scripts / docs as listed in `LICENSE` (copyright already: Guillaume DUPONT / Ten Square Software)
    2. **VirtualMIDI** — proprietary (Tobias Erichsen); separate from MIT; SDK not freeware; software linking the SDK **must not be distributed** without prior clearance; eval path = pre-installed loopMIDI / rtpMIDI; MSI/merge-module embed = **OQ-1** only
    3. **Windows MIDI Services** — future optional Win11-only `MidiBackend`; **not** V1; V1 backend remains VirtualMIDI
  - [x] Cite `https://github.com/aaron1a12/virtual-midi` on a public surface (README License/Acknowledgments or `docs/dev/` page linked from README) as **integration existence proof only** — GPL + vendored SDK; **do not fork** as project base
  - [x] State clearly: Linux `sound/usb/midi.c` + `quirks-table.h` (`QUIRK_MIDI_EMAGIC`) are **reference only**; no GPL Linux sources are vendored in this tree (AD-14 / NFR-Q2)
  - [x] Recommended thin technical page (merge into README only if a dedicated page would be thinner than a screen): `docs/dev/license-and-backends.md` covering the three-way table, aaron1a12 proof citation, Linux reference policy, OQ-1 redistribution honesty, pointer to `docs/user/` for install prerequisites
  - [x] Confirm facade **Ten Square Software** appears consistently on README, `LICENSE`, installer branding (already), and `docs/user/` — fix any public surface that still reads as anonymous / personal-only without the product facade
  - [x] Do **not** invent a `NOTICE` / `THIRD_PARTY` tree unless a concrete vendored redistributable appears in-tree (today: none). Catch2 via FetchContent stays a build-time test dep — optional one-line “test harness dependency” note is enough; do not pretend Catch2 is shipped to end users
  - [x] Do **not** vendor Tobias SDK binaries, GPL Linux trees, or fork aaron1a12 into the repo

- [x] Task 3: Contributor dual-machine loop docs (AC: 3)
  - [x] Expand `contributing.md` **and/or** add `docs/dev/contributor-dual-machine-loop.md` (kebab-case) so AD-13 / NFR-D3 is unmistakable:
    - Primary edit on **macOS** + Cursor
    - Build, USB, DAW, SysEx, and Studio-Done measurements on **Windows 10/11 x64**
    - **Win10 x64 mandatory** in the validation matrix (Win11 alone is not enough)
    - Artifacts under `builds/` (never root `build/`)
    - Windows CI compile is the minimum merge gate; hardware Pass rows remain lab-owned
    - Offline: macOS can author markdown / C++; Windows validation machine closes hardware / installer Pass
  - [x] Cross-link from root README Development environment table to the dual-machine contributor doc (keep the short table; deepen the narrative in contributor docs)
  - [x] Point contributors at existing `docs/dev/windows-ci-toolchain.md` and `docs/dev/winusb-bind.md` (Zadig = contributor fallback only) — do not duplicate full bind/CI content
  - [x] Keep `conventions.md` as quality SSOT; contributor docs summarize the loop, not re-litigate §3 limits

- [x] Task 4: Align existing user / installer honesty without scope creep (AC: 1, 4)
  - [x] Ensure `docs/user/` works/does-not-work (or equivalent) still states Windows MIDI Services is **not** V1; add a short “see also” to the polished license/backends surface if useful — do **not** rewrite the 4.2 single-file manual structure
  - [x] Keep installer VirtualMIDI missing / OQ-1 MSI messaging aligned with README (eval via loopMIDI/rtpMIDI; embed gated) — string tweaks only if a contradiction exists; no AD-12 redesign
  - [x] Port naming: if any user-facing string is touched, keep shipped Bridge names `MT4 In N` / `MT4 Out N` (and `MT4 #K …` for multi-unit). Planning shorthand `MT4 Port N` must not be reintroduced as the live name
  - [x] Update Epic 4 smoke fences that still say “three-way honesty → 4.3” to point at the finished README / `docs/dev/` surfaces + this story’s smoke guide

- [x] Task 5: Operator verification + quality (AC: all)
  - [x] Fill `docs/tests/smoke-epic4-license-honesty-mt4.md` Pass/Fail rows, minimum:
    1. README (or linked `docs/dev/license-and-backends.md`) states MIT (this repo) ≠ VirtualMIDI (proprietary) ≠ Windows MIDI Services (future Win11-only, not V1) in plain language
    2. `aaron1a12/virtual-midi` cited as integration proof only / not a fork base
    3. Explicit “no GPL Linux sources vendored” (or equivalent) on a public/contributor surface; spot-check tree: no vendored `midi.c` / `quirks-table.h` / aaron1a12 fork
    4. Contributor dual-machine loop documented (macOS edit / Windows x64 validate; Win10 mandatory)
    5. Public facade **Ten Square Software** visible on README + LICENSE (and not contradicted by installer / user docs)
    6. OQ-1 honesty preserved: no claim that VirtualMIDI MSI embed is cleared / redistributable in the Public Installer
    7. Discoverability: community reader can reach the three-way explanation from README without opening `_bmad-output/`
    8. Scope fence: no SmartScreen/Authenticode chapter claimed under this story ID (→ 4.4); no 4.2 manual rewrite; no installer UX redesign
  - [x] Prefer **documentation-only** change set. If touching installer strings / C++: `python scripts/quality/lint-touched.py` exits 0; no French in sources; do not pull docs concerns into Protocol/Profile/Midi
  - [x] Confirm no polished chapter claimed under **4.4**; no Tobias MSI embed; no latency Studio-Done claims; no claim that SM-6 is fully closed without 4.4

## Dev Notes

### Soft dependencies

| Dependency | Status | How this story treats it |
|---|---|---|
| Epic 1–3 Bridge + VirtualMIDI backend | **done** | Document honesty; do not reopen MIDI protocol or backend code |
| Story **4.1** Public Installer | **in-progress** | Keep OQ-1 / VirtualMIDI gate messaging aligned; no wizard redesign |
| Story **4.2** End-user docs | **done** | Light cross-links only; preserve single-file EN+FR manuals and `MT4 In/Out N` names |
| OQ-1 Tobias MSI clearance | **open** | Must remain explicitly gated; never claim redistributable embed |
| Story **4.4** Authenticode / SmartScreen | backlog | Out of scope; SM-6 partially overlaps — do not write SmartScreen policy here |
| `project-context.md` | **absent** | Do not block on generating it; use this story + conventions + spine |

### Scope fence

This story ships **technical / contributor docs** and **public license–backend honesty** so community evaluators and contributors stay legally and product-clear. It is **not** end-user UJ rewrite, **not** installer redesign, and **not** Authenticode policy.

| In scope | Out of scope (later / never) |
|---|---|
| Three-way MIT ≠ VirtualMIDI ≠ WMS on README / `docs/dev/` | Redesigning AD-12 installer UX → **4.1** |
| aaron1a12 proof-only citation + no-GPL-vendored statement | Rewriting `docs/user/` UJ manuals → **4.2** (done) |
| Contributor dual-machine loop (AD-13 / NFR-D3) | SmartScreen / Authenticode docs → **4.4** |
| Ten Square Software facade consistency check | Embedding VirtualMIDI MSI → **OQ-1** |
| Operator smoke proving docs/license honesty | MIDI Path / latency Studio-Done → Epic **5** |
| English public / contributor prose | Shipping French legal twin of LICENSE |
| Optional thin `docs/dev/license-and-backends.md` | Vendoring SDK / GPL trees / forking aaron1a12 |
| | Claiming AMT8 / Unitor8 / cascade / Patch mode / LTC as V1 |
| | Inventing NOTICE/THIRD_PARTY without in-tree redistributables |

### Architecture compliance (must follow)

- **AD-19:** public facade **Ten Square Software**; FR-14 honesty on docs surfaces; SmartScreen detail deferred to **4.4**
- **AD-14:** MIT original Emagic reimplementation; Linux quirks **reference only**; **no GPL sources in the repository**
- **AD-13:** document dual-machine loop in **contributor** docs; Win10 mandatory in matrix; artifacts under `builds/`
- **AD-7:** VirtualMIDI is V1 `MidiBackend`; do not fork `aaron1a12/virtual-midi`; MSI embed only after clearance (OQ-1)
- **Consistency Conventions:** License messaging row — MIT (this repo) ≠ VirtualMIDI (proprietary) ≠ Windows MIDI Services (future backend)
- **Structural seed:** `docs/user/` = end-user (owned by 4.2); `docs/dev/` = contributor / protocol / this story’s tech honesty; `docs/tests/` = operator smoke
- **Conventions:** kebab-case paths; no top-level `Documentation/`; English in shipped public docs; no French in C++ if touched; commits only on Guillaume’s ask

### Required messaging content (must appear)

**Three-way honesty (phrase for readers, not lawyers-only):**

1. **This repo (MIT)** — Unitor MT4 Bridge sources and project docs under `LICENSE`.
2. **VirtualMIDI (proprietary, separate)** — Tobias Erichsen; not covered by this repo’s MIT; eval via loopMIDI/rtpMIDI; distributing software that links the SDK needs prior clearance; Public Installer does **not** embed the driver MSI until OQ-1.
3. **Windows MIDI Services (future)** — optional later Win11-only backend behind `MidiBackend`; **not** what V1 ships or claims.

**Proof / non-goals:**

- `aaron1a12/virtual-midi` = existence proof that VirtualMIDI SDK integration works; GPL + vendored SDK → **not** a fork base.
- Linux Emagic quirk sources = read-only reference; never copied into this tree.
- Prodikeys64-style WinUSB + virtual MIDI pattern may stay in Acknowledgments as inspiration — not a license substitute for VirtualMIDI terms.

### Current baseline (do not regress)

Already present (incomplete vs ACs — polish, do not delete):

- Root `LICENSE` — MIT; Copyright `(c) 2026 Guillaume DUPONT / Ten Square Software`
- `README.md` § License — MIT + light VirtualMIDI / WMS caveats; still says “Full three-way honesty polish is Story **4.3**” → **remove that deferral once polished**
- `docs/user/` — VirtualMIDI prerequisite + WMS “not V1” in works/does-not-work
- Installer — VirtualMIDI presence gate; OQ-1 MSI embed not shipped
- `contributing.md` — thin quality/commit rules; **missing** explicit dual-machine AD-13 narrative
- **No** `NOTICE*` / `THIRD_PARTY*` / aaron1a12 citation on public README surfaces yet

### Recommended authoring shape

1. **One scannable three-way block** on README (table or three short bullets) + optional deep page under `docs/dev/`.
2. **Contributor loop** as a short “how we work” section — musicians reading README stay on user docs; contributors get `contributing.md` / `docs/dev/`.
3. **Reuse Tobias’s published terms** (SDK page: not freeware; no distribution without clearance; MSI for licensees) — paraphrase honestly; link the vendor page; do not invent softer terms.
4. **Keep OQ-1 loud** — community trust dies if the installer looks like it redistributes VirtualMIDI when clearance is still open.
5. **SM-6 honesty** — license/backend part closes here; SmartScreen/Authenticode remains **4.4**; do not mark SM-6 fully done in Completion Notes.

### Previous story intelligence

- **4.2 (done):** Shipped single-file EN + FR user manuals under `docs/user/` (not the multi-chapter tree originally sketched). Explicitly fenced full three-way + dual-machine loop here. Discoverability via README + installer → `docs/user/README.md`. Port names live as `MT4 In/Out N`. Do **not** claim FR-14 / SM-6 closed from 4.2’s light WMS bullet alone. Review lesson: keep FR user peer in sync only when touching shared product facts; attribute packaging/version C++ to **4.1**.
- **4.1 (in-progress):** Inno Setup Public Installer; VirtualMIDI fail-closed; Ten Square branding; fenced three-way honesty → **4.3**. OQ-1 MSI embed remains release gate. Unsigned INF / SmartScreen → **4.4**. Do not contradict installer English fix path (`loopMIDI` / `rtpMIDI`).
- **1.5 / AD-7:** aaron1a12 proof-only already enforced in implementation stories — 4.3 makes the **public citation** explicit.
- **Epic 3 smokes:** still may say “polished user docs → 4.2”; update only license/4.3 fences, not user-journey ownership.

### Git intelligence

Recent commits (`fc29bfb` Story 4.2 docs, `1d23869` Bridge version for packaging, `538ceea` Story 4.1 installer) show Epic 4 is docs + packaging heavy. Prefer markdown + light README/`contributing.md`/smoke edits over C++ churn. No license-bundling script exists today — do not invent installer `LicenseFile` theater unless it clearly improves honesty without claiming VirtualMIDI terms are MIT.

### Latest tech / product notes (as of 2026-08)

- Tobias Erichsen **virtualMIDI SDK** page still states: SDK is **not** freeware; software linking the SDK **may not be distributed** without prior clearance; eval expects loopMIDI/rtpMIDI; licensees can obtain MSI/merge module for installer embedding. Link: `https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html`
- Vendor marketing pages still list “Windows 7 up to Windows 10” for some VirtualMIDI products — project Validation Matrix still includes Win11 x64; do not silently drop Win11, but do not pretend vendor marketing equals Matrix policy. Win10 remains mandatory.
- Windows MIDI Services is a Microsoft OS component path (Win11-oriented); keep it labeled **future backend**, never “also supported in V1.”
- `aaron1a12/virtual-midi` remains GPL-3.0 with vendored SDK — citation must keep “proof only / do not fork.”

### Project structure notes

```text
README.md                              # UPDATE — three-way License + remove 4.3 deferral stub
LICENSE                                # VERIFY — Ten Square facade already present
contributing.md                        # UPDATE — dual-machine loop (+ quality pointers)
docs/dev/license-and-backends.md       # NEW (recommended) — deep three-way + aaron1a12 + OQ-1
docs/dev/contributor-dual-machine-loop.md  # NEW (optional if contributing.md stays thin)
docs/dev/winusb-bind.md                # OPTIONAL cross-link only
docs/dev/windows-ci-toolchain.md       # OPTIONAL cross-link only
docs/user/*                            # LIGHT touch — WMS / see-also only; no manual rewrite
docs/tests/smoke-epic4-license-honesty-mt4.md  # NEW — FR-14 / CAP-14 matrix
docs/tests/smoke-epic4-*.md            # UPDATE fences away from “→ 4.3”
installer/*                            # ONLY if contradiction with OQ-1 / facade wording
_bmad-output/.../4-3-*.md              # this story file
```

Do **not** create `Documentation/`. Do **not** invent a second user-doc tree. Do **not** add `third_party/` GPL drops.

### Testing requirements

- Docs/license smoke can Pass on content verification alone (no MT4 required for FR-14 rows).
- Still list Win10 when claiming matrix policy in contributor docs.
- Offline/macOS edit machine can author markdown; any installer string change still validates on Windows if `verify-installer-contract.py` is affected.
- Quality: English public docs; `lint-touched.py` only if C++ touched.
- Anti-lies: grepping the tree for vendored GPL filenames / aaron1a12 subtree is part of AC2 verification — document the check in the smoke guide.

### References

- [Source: `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md` — Epic 4 / Story 4.3]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` — FR-14, NFR-D3, NFR-Q2, NFR-Q3, SM-6, §5 non-goals, §11 license]
- [Source: `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/addendum.md` — VirtualMIDI licensing; aaron1a12 proof-only]
- [Source: `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` — AD-7, AD-13, AD-14, AD-19, Consistency Conventions license row, Structural Seed `docs/dev/`]
- [Source: `_bmad-output/specs/spec-unitor-win64-driver/SPEC.md` — CAP-14]
- [Source: `_bmad-output/implementation-artifacts/4-2-end-user-documentation-for-first-midi-and-sysex.md`]
- [Source: `_bmad-output/implementation-artifacts/4-1-public-installer-meeting-ad-12-ux-bar.md`]
- [Source: `README.md` — License section (partial; self-labels polish as 4.3)]
- [Source: `LICENSE`]
- [Source: `contributing.md`]
- [Source: `docs/user/unitor-mt4-bridge-user-guide.md`]
- [Source: `docs/tests/smoke-epic4-public-installer-mt4.md`]
- [Source: `docs/tests/smoke-epic4-user-docs-mt4.md`]
- [Source: https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html — SDK redistribution terms]
- [Source: https://github.com/aaron1a12/virtual-midi — integration proof only]

## Dev Agent Record

### Agent Model Used

Composer (Cursor agent router)

### Debug Log References

- Tree spot-check 2026-08-10: no `sound/usb/midi.c`, no `quirks-table.h`, no `third_party/`, no aaron1a12 / virtual-midi subtree
- Docs-only change set; `lint-touched.py` not required (no C++ / installer string edits)

### Completion Notes List

- Shipped three-way MIT ≠ VirtualMIDI ≠ Windows MIDI Services honesty on README + `docs/dev/license-and-backends.md`; removed Story 4.3 deferral stub
- Cited aaron1a12 as integration proof only; stated no GPL Linux sources vendored; OQ-1 MSI embed remains gated
- Documented contributor dual-machine loop (macOS edit / Win10+11 x64 validate; Win10 mandatory) in `contributing.md` + dedicated deep page
- Light EN/FR user-guide see-also links; Epic 4 smoke fences now point at finished surfaces
- Operator smoke `docs/tests/smoke-epic4-license-honesty-mt4.md` rows 1–8 Pass (docs-only)
- FR-14 / CAP-14 closable from this Pass; SM-6 only **partially** met (license/backend) — Authenticode/SmartScreen remains Story 4.4
- Ten Square Software facade verified on LICENSE / README / existing installer and user docs; no NOTICE/THIRD_PARTY invented

### File List

- `README.md`
- `contributing.md`
- `docs/dev/license-and-backends.md`
- `docs/dev/contributor-dual-machine-loop.md`
- `docs/tests/smoke-epic4-license-honesty-mt4.md`
- `docs/tests/smoke-epic4-public-installer-mt4.md`
- `docs/tests/smoke-epic4-user-docs-mt4.md`
- `docs/user/unitor-mt4-bridge-user-guide.md`
- `docs/user/unitor-mt4-bridge-manuel-utilisateur.md`
- `_bmad-output/implementation-artifacts/4-3-technical-docs-and-three-way-license-honesty.md`
- `_bmad-output/implementation-artifacts/sprint-status.yaml`

### Change Log

- 2026-08-10: Implemented Story 4.3 docs — three-way license honesty, dual-machine contributor loop, Epic 4 smoke fence updates; status → review
- 2026-08-10: Code review patches applied (GPL consequence, WMS Microsoft note, OQ-1/Bridge redistribution clarity, smoke honesty, EN markers); status → done

### Review Findings

- [x] [Review][Patch] Mark user-manual see-also to license page as English/technical [docs/user/unitor-mt4-bridge-user-guide.md:238] [docs/user/unitor-mt4-bridge-manuel-utilisateur.md:238] — decision: keep deep link; add clear English/technical marker (Guillaume chose option 1)
- [x] [Review][Patch] Restore GPL-copy consequence on README License [README.md:License] — prior text warned copying Linux `midi.c` / quirks would force GPL; current text only says “not vendored.”
- [x] [Review][Patch] Restore Microsoft licensing note for Windows MIDI Services [README.md:License] — WinUSB note kept; WMS dropped from the prior “WinUSB / Windows MIDI Services” Microsoft caveat.
- [x] [Review][Patch] Align Win11-only vs Win11-oriented wording [docs/dev/license-and-backends.md:20-36]
- [x] [Review][Patch] Clarify Bridge/Setup redistribution vs OQ-1 MSI-embed gate [docs/dev/license-and-backends.md:22-29] — readers may treat MSI-only OQ-1 as full Tobias clearance.
- [x] [Review][Patch] State any `teVirtualMIDI.dll` provider qualifies; loopMIDI/rtpMIDI are eval examples [docs/dev/license-and-backends.md:19]
- [x] [Review][Patch] Gloss `OQ-1` and `MidiBackend` on first public README use [README.md:License]
- [x] [Review][Patch] Docs-only smoke OS columns — allow N/A or “any checkout” path [docs/tests/smoke-epic4-license-honesty-mt4.md:52-63]
- [x] [Review][Patch] Smoke row 3 spot-check excludes `builds/**/_deps` [docs/tests/smoke-epic4-license-honesty-mt4.md:58]
- [x] [Review][Patch] Smoke row 6 notes: verify existing installer strings, no new installer edit claim [docs/tests/smoke-epic4-license-honesty-mt4.md:61]
- [x] [Review][Patch] Rename Catch2 section to Catch2-only (drop “other deps”) [docs/dev/license-and-backends.md:46-48]
- [x] [Review][Patch] Link Windows CI workflow/job from dual-machine merge-gate claim [docs/dev/contributor-dual-machine-loop.md:26]
- [x] [Review][Defer] loopMIDI/rtpMIDI product names hyperlink to SDK page [README.md:License] — deferred, pre-existing
- [x] [Review][Defer] “Public Installer” naming vs open OQ-1 redistributable clearance — deferred, pre-existing
- [x] [Review][Defer] Vendor virtualMIDI OS marketing (Win7–Win10) vs project Win10+Win11 matrix — deferred, pre-existing / optional honesty note
