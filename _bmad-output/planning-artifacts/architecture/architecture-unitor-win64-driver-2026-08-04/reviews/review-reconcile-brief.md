---
title: "Architecture Spine ↔ Product Brief Reconciliation"
reviewed: 2026-08-04
brief: _bmad-output/planning-artifacts/briefs/brief-unitor-win64-driver-2026-08-04/brief.md
brief_addendum: _bmad-output/planning-artifacts/briefs/brief-unitor-win64-driver-2026-08-04/addendum.md
spine: _bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md
prd_reference: _bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md
scope: locked brief decisions + success-bar items only
method: miss or contradiction vs brief; PRD cited only when spine would fight both
---

# Architecture Spine ↔ Product Brief Reconciliation

## Verdict

**Gaps only — no contradictions.** The draft spine aligns with every locked brief decision it addresses (usermode WinUSB, VirtualMIDI V1, MT4 profile, multi-instance, port topology/naming, auto-start/hot-plug, installer UX bar, quality gate, MIT/no-GPL, VirtualMIDI clearance as release gate). Several locked decisions and success-bar items are **not reflected in spine rules** even though frontmatter `binds` claims some of them (e.g. `NFR-R1`). Nothing in the spine **fights** the brief; omissions are the issue.

---

## Findings (missed or under-specified)

### F-1 — MIDI clock and channel/system MIDI coverage (locked + success bar)

| Source | Requirement |
| --- | --- |
| Brief (locked) | Channel MIDI **and SysEx** (required); **MIDI clock in V1** |
| Brief (success bar) | Notes, CC, common channel/system messages; **MIDI clock**; SysEx |
| PRD | FR-6 (channel/system MIDI); FR-7 (MIDI clock, Start/Stop/Continue, **MTC**) |
| Spine | AD-16 covers SysEx/Matrix-Control transparency only; **no AD** for clock, transport realtime, MTC, or general channel MIDI |

The device-session pipeline implies byte transport, but the brief locks clock as a V1 product requirement and the success bar treats message coverage as testable. The spine should bind FR-6/FR-7 (or an AD stating the mapper/backend carry clock and channel traffic without Bridge-induced dropouts under session-stability conditions).

---

### F-2 — ~4-hour session stability (success bar)

| Source | Requirement |
| --- | --- |
| Brief (success bar) | Continuous studio/editor use of about **4 hours** without Bridge restart for normal use (including SysEx) |
| PRD | SM-3, **NFR-R1** (~4-hour continuous operation including SysEx Sessions) |
| Spine | Frontmatter lists `NFR-R1` in `binds`; **no invariant or AD** governs soak duration, restart policy, or architectural implications (buffering, session teardown, SysEx burst handling under long run) |

Claimed bind without a matching rule is a documentation gap builders cannot trace.

---

### F-3 — Ten Square Software public facade (locked)

| Source | Requirement |
| --- | --- |
| Brief (locked) | Public facade: **Ten Square Software** |
| Brief (success bar) | Public project identity under Ten Square Software |
| PRD | §6.1, FR-15 (signed builds use Ten Square / chosen certificate path) |
| Spine | **Not mentioned** in Stack, Consistency Conventions, Deferred, or any AD |

Product identity is locked in the brief; spine should at least cross-reference release/docs conventions (installer branding, README, certificate path) even if implementation is docs/installer-owned.

---

### F-4 — SmartScreen documentation when unsigned (locked Authenticode row + success bar)

| Source | Requirement |
| --- | --- |
| Brief (locked) | Authenticode strongly recommended; **not** a hard gate if certificate lags (**document SmartScreen**) |
| Brief (success bar) | Unsigned first public build OK with **SmartScreen documentation** if certificate lags |
| PRD | FR-15, NFR-S1, SM-6 (docs explain SmartScreen behavior and mitigation if unsigned) |
| Spine | Deferred table mentions Authenticode deferral only; **no SmartScreen doc requirement** |

Spine matches “not a hard gate” but drops the paired **documentation obligation** that brief and PRD lock together with Authenticode policy.

---

### F-5 — User documentation scope (success bar)

| Source | Requirement |
| --- | --- |
| Brief (success bar) | User docs: VirtualMIDI prerequisites, install, auto-start, first MIDI and SysEx test, troubleshooting, explicit works / does-not-work list |
| PRD | FR-13 (testable user-doc coverage for UJ-1/UJ-2) |
| Spine | AD-12 installer UX checklist; AD-8 multi-client ceiling in user/tech docs; **no AD or convention** for full user-doc bar |

Installer UX is partially covered; the broader user-documentation success bar is absent from spine governance.

---

### F-6 — Multi-MT4 honest validation status at ship (success bar)

| Source | Requirement |
| --- | --- |
| Brief (success bar) | Two MT4 units supported in V1 design; **if only one unit available at ship, document validation status honestly** |
| PRD | FR-10 consequence; SM-8 |
| Spine | AD-4 supports two concurrent sessions; **no rule** for release-note/doc honesty when dual-unit hardware proof is unavailable |

Design support is present; the success-bar **honesty obligation** is not carried into spine.

---

## Locked decisions and success-bar items — confirmed aligned

No action required for these; listed to bound scope and avoid false positives.

| Brief item | Spine coverage |
| --- | --- |
| Windows 10/11 x64 (Win10 mandatory) | Stack, AD-13 |
| Usermode only; no custom kernel driver | AD-1 |
| VirtualMIDI V1; WMS post-V1 second backend | AD-2, AD-7, Stack |
| MIT; no GPL Linux sources vendored | AD-14, Consistency Conventions |
| MT4 `086A:0003` validated; multi-DeviceProfile / multi-instance | AD-3, AD-4 |
| 2 IN + 4 OUT port topology; macOS-like naming; multi-MT4 disambiguation | AD-5, AD-6 |
| SysEx required (Matrix-Control validation path) | AD-16 |
| Multi-client (DAW + utility concurrently) | AD-8 |
| Auto-start; hot-plug without Windows reboot | AD-10 |
| Quality gate (`conventions.md`, `lint-touched.py`) | AD-15 |
| VirtualMIDI author clearance before redistributable public installer | AD-7, Deferred |
| Installer: not Zadig-primary; macOS-class UX bar; one-time admin | AD-12 |
| MIDI Path measurement (not ASIO buffer); provisional timing anchors | AD-11 |
| Dual-machine dev / Win10 validation matrix | AD-13 |
| Authenticode strongly recommended, not hard V1 gate | Deferred (minus SmartScreen docs — see F-4) |

---

## Contradictions checked — none

| Candidate | Result |
| --- | --- |
| Port naming (`MT4 Port N`, multi-unit `#K`) | AD-5 matches brief/addendum intent |
| 2 IN / 4 OUT vs per-channel flood | AD-5 explicit |
| Emagic cascade out; two independent MT4s in | AD-4 explicit |
| VirtualMIDI eval vs redistributable MSI | AD-7 + Deferred align with brief release gate |
| Zadig developer-only vs installer-primary | AD-12 aligns with addendum |
| PRD-expanded FR-7 (MTC, Start/Stop/Continue) vs brief “MIDI clock” | Spine silent on both — **miss**, not fight |

---

## Recommended spine patches (minimal)

1. Add **AD-17** (or extend AD-16): bind FR-6, FR-7 — transparent carry of channel/system MIDI, MIDI clock (0xF8), Start/Stop/Continue, MTC; no Bridge-induced dropouts under NFR-R1 scenario.
2. Add **AD-18** or NFR bind note: session soak **~4 h** (NFR-R1 / SM-3) — architectural expectations (no mandatory restart; SysEx burst buffering ties to AD-16 / NFR-R3).
3. Consistency Conventions row: **public facade Ten Square Software** (release/docs/installer branding).
4. Extend Deferred or AD-12: **SmartScreen documentation** when unsigned (FR-15 / NFR-S1 / SM-6).
5. AD-12 or docs convention: **FR-13 user-doc checklist** (mirror installer UX pattern).
6. AD-4 footnote: **honest multi-MT4 validation status** in release notes when dual hardware unavailable (FR-10 / SM-8).

---

## Out of scope for this review

- PRD-only expansions not in brief locked table (e.g. MTC detail beyond brief “MIDI clock”) — reported under F-1 because brief still locks clock and PRD agrees; spine misses both.
- Assumptions, open questions, post-MVP cousin profiles — correctly deferred in spine unless they were locked brief decisions.
- Prodikeys64 pattern — brief architecture orientation only, not locked table.
