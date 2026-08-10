---
organization: Ten Square Software
project: unitor-win64-driver
title: Sprint Change Proposal — Community hobby pivot (no paid cert, no virtualMIDI community redistrib, Epic 5 then WMS Win11)
author: Guillaume DUPONT (Correct Course with BMad)
created: 2026-08-10
updated: 2026-08-10
status: implemented-planning-artifacts
updated: 2026-08-10
workflow: bmad-correct-course
---

# Sprint Change Proposal — 2026-08-10

## 1. Issue Summary

### Trigger

Guillaume is pivoting Unitor MT4 Bridge from an implied “commercial-grade signed community product” trajectory to an honest **hobby / personal + free GitHub community** posture:

- This project line **does not ship** an Authenticode / catalog certificate.
- Sources and community posture stay **free on GitHub** (hobby / open contribution).
- Musicians with little IT skill still get a **realistic** install path — without promising polished commercial same-evening success on a clean unsigned PC.
- Community Releases of VirtualMIDI-linked binaries are **out of scope**; the community binary path is Windows MIDI Services (Epic 6). Product strategy does **not** wait on third-party virtualMIDI redistribution clearance.

- Preferred long-term community MIDI backend: **Windows MIDI Services (WMS)** on **Windows 11 only**, accepting loss of Windows 10 as a community target. Until a Win11 lab machine is available, **Epic 5 runs first** on the current virtualMIDI lab stack.

### Evidence

| Evidence | Where |
|---|---|
| Clean Win10 Public Installer WinUSB association Fail `0xE000022F` (unsigned INF / no trusted catalog) | `docs/tests/smoke-epic4-public-installer-mt4.md` (2026-08-10) |
| Authenticode strongly recommended, not hard V1 gate; OQ-3 still framed as purchase choice | Story 4.4; `docs/dev/authenticode-and-smartscreen.md` |
| Lab self-sign ≠ community trust | `installer/sign-lab-package.ps1` |
| virtualMIDI SDK not freeware; VirtualMIDI-linked community Releases out of scope (OQ-1); MSI embed out of scope | `docs/dev/license-and-backends.md`; PRD OQ-1 |
| WMS already planned as future Win11-only adapter, not V1 | PRD §11; Architecture AD-7 / Deferred |

### Core problem statement

Two independent blockers were conflated under “community install”:

1. **MIDI backend rights (A):** Publishing ready-to-run binaries that use proprietary virtualMIDI without author clearance is not aligned with “give everything / owe nobody.” Third-party proprietary SDK clearance is not a controllable community gate.
2. **USB bind trust (B):** WinUSB *driver* is in-box Microsoft; associating a **non-class-compliant** Emagic MT4 via a third-party INF still needs a **trusted catalog** on clean PCs. No paid cert ⇒ Setup-alone WinUSB success remains **out of promise**. SmartScreen on unsigned Setup is a softer friction (document “Run anyway”).

WMS solves **A** (for community binaries) and does **not** solve **B**.

---

## 2. Impact Analysis

### Epic impact

| Epic | Impact |
|---|---|
| **1–3** | No rollback. Lab Bridge remains valid for personal/Matrix-Control use on virtualMIDI + Win10. |
| **4 Community Install and Trust** | Marked **done** for packaging/docs/policy delivery, but the **product promise** (“community polished commercial same evening on clean PC”) must be **rewritten**. Epic 4 value statement and FR-12 interpretation need honesty realignment — not a full re-implementation of 4.1–4.4. |
| **5 MIDI Path Proof** | **Unblocked and next.** Run on current stack while Win11 lab is prepared. Do **not** wait for WMS or a certificate. |
| **New Epic 6 (proposed)** | **Windows MIDI Services community backend (Win11-only)** — after Epic 5. Enables MIT + public GitHub binaries without a virtualMIDI SDK redistribution dependency. Revalidation of ports / multi-client / Matrix-Control on WMS. |

### Story impact

| Now | Later (post–Epic 5) |
|---|---|
| Optional small stories / quick-dev: rewrite user docs + README Status + smoke honesty for “hobby install” contract (can start after this proposal is approved, without blocking Epic 5) | Create-story for Epic 6 WMS; install-path stories (guided WinUSB / Zadig as primary clean-PC path; SmartScreen; drop third-party-clearance-wait language) |
| Do **not** invent production `.cat` without a shipped certificate | Do **not** ship public Bridge/Setup binaries that depend on virtualMIDI SDK until WMS replaces that dependency |

### Artifact conflicts

| Artifact | Conflict | Required change class |
|---|---|---|
| PRD | Win10 mandatory; virtualMIDI V1; OQ-1 framed as live gate; OQ-3 purchase framing; FR-12 clean-install optimism | **MVP / constraint rewrite** (staged: honesty now; WMS/Win11 as next community MVP) |
| Epics | Epic 4 value statement; no WMS epic; OQ-1 as live release gate | Amend Epic 4 narrative; add Epic 6 backlog; sequence Epic 5 → Epic 6 |
| Architecture | AD-7 VirtualMIDI V1; Deferred “WMS post-V1”; AQ-3 wait on third-party pin | Retarget: virtualMIDI = interim lab backend; WMS = next community backend; close AQ-3/OQ-1 as out of community scope |
| UX spec | None | N/A — user guides are the UX surface |
| `sprint-status.yaml` | NEXT still implies Epic 4 wrap / OQ-3 before public release | NEXT = Epic 5; OQ-1 out of community scope; OQ-3 no certificate in this line; Epic 6 backlog |
| `deferred-work.md` | Production `.cat` waits on OQ-3 purchase | Reframe: production catalog **out of scope hobby** unless a certificate is later shipped; clean-PC path = guided WinUSB |
| User docs EN+FR, README, Epic 4 smokes, license/authenticode pages | Still carry commercial-installer or “wait OQ-3” tones in places | Rewrite surfaces listed in §4 |

### Technical impact

| Area | Impact |
|---|---|
| Epic 5 harness / measurements | No cert dependency; use current Win10 + virtualMIDI lab |
| WMS Epic 6 | New `MidiBackend` implementation; Win11 machine required; drop Win10 as community claim |
| Public binary on GitHub | **After** WMS (recommended). Until then: sources + honesty; personal/lab binaries not marketed as cleared redistributables |
| Setup / WinUSB | Keep fail-closed honesty; elevate **guided Zadig (or equivalent)** as the supported clean-PC association path without paid cert; do not claim Setup-alone WinUSB success |
| SmartScreen | Docs-first mitigation (“More info → Run anyway”) — sufficient for hobby unsigned EXE |
| Bridge MIDI protocol / Epics 2–3 | Out of scope unless WMS port lifecycle breaks them |

---

## 3. Recommended Approach

**Selected path: Hybrid**

1. **Direct adjustment (now):** Correct Course artifacts + product-contract honesty (OQ-1 out of community scope, OQ-3 no certificate in this line, hobby install promise rewrite). Small doc/packaging follow-ups allowed in parallel with Epic 5.
2. **MVP review (next community shape):** Community-facing redistributable Bridge = **Win11 + WMS + unsigned-OK + guided WinUSB**, not “signed commercial installer on Win10+virtualMIDI.”
3. **No rollback** of Epics 1–4 implementation.

### Sequencing (approved 2026-08-10)

```text
1) Approve this Sprint Change Proposal
2) Apply planning artifact edits (PRD / epics / architecture / sprint-status / deferred-work)
3) Epic 5 (MIDI Path) on current lab stack
4) Prepare Win11 lab machine (parallel with or after Epic 5 start)
5) Epic 6 — WMS Win11 community backend + public binary policy
6) Hobby install doc/UX finish (SmartScreen + guided WinUSB as primary clean-PC story)
```

### Effort / risk / timeline

| Item | Effort | Risk | Notes |
|---|---|---|---|
| Planning/doc contract rewrite | Low–Medium | Low | Unblocks honest communication |
| Epic 5 | Medium (as planned) | Low vs this pivot | Independent of cert/Tobias |
| Win11 machine purchase | Hardware / time | Low | owner-provided |
| Epic 6 WMS | High | Medium (API/runtime maturity, DAW behavior) | Solves binary redistribution vs virtualMIDI SDK redistrib |
| Clean-PC WinUSB without cert | Medium UX | Medium (musician friction) | Cannot be eliminated without cert; must be honest |

### Rationale

- Depending on third-party virtualMIDI clearance fails Guillaume’s “owe nobody / publish freely” goal.
- Paying for Authenticode is explicitly rejected.
- WMS addresses the legal/redistribution knot better than perpetual OQ-1 limbo.
- Epic 5 first preserves momentum and uses existing hardware before Win11 arrives.
- SmartScreen is the easy friction; WinUSB bind trust is the hard one — name it honestly.

---

## 4. Detailed Change Proposals

Approved incremental proposals A–D (2026-08-10) are expanded below.

### 4.1 PRD (`prd-unitor-win64-driver-2026-08-04/prd.md` + addendum as needed)

#### OQ-1

**Section:** §12 Open Questions

**OLD:** Wait for Tobias reply; release gate for Public Installer redistributable / MSI terms; eval path for development.

**NEW:** **Out of community scope.** No community Releases of Bridge/Setup binaries that use the virtualMIDI SDK. virtualMIDI remains acceptable for **personal/lab** use until Windows MIDI Services replaces it for community binaries. MSI embed of virtualMIDI remains out of scope under this hobby posture.

**Rationale:** Hobby posture prefers a Microsoft/open stack for public binaries; do not make community redistribution depend on third-party proprietary SDK clearance.

#### OQ-3

**Section:** §12 Open Questions

**OLD:** Personal vs org certificate path/cost before first tagged public community release.

**NEW:** **Out of scope hobby / no certificate purchase.** No Authenticode or production INF catalog certificate purchase is planned. Unsigned public builds (when binaries exist) require SmartScreen user docs. Clean-PC WinUSB association via Setup-alone without a trusted catalog is **not** a promised outcome.

**Rationale:** avoids fake “decision pending purchase.”

#### §11 Constraints (staged wording)

**OLD:** Platforms Win10+Win11 mandatory Win10; MIDI backend V1 = VirtualMIDI; WMS = optional later.

**NEW (community target — after Epic 6):** Platforms **Windows 11 x64** for community claims; MIDI backend community = **Windows MIDI Services**; Win10 + virtualMIDI retained as **interim lab / personal** stack through Epic 5.

**Transition note:** Do not delete Win10 lab validity overnight; mark community MVP retarget clearly with effective epic (Epic 6).

#### FR-12 / SM-5 / SM-6 interpretation

**OLD implication:** Public Installer + docs → first MIDI same evening including clean WinUSB success path toward signed trust.

**NEW:** Public Installer may install Bridge + Auto-Start and gate prerequisites; **clean-PC WinUSB** success without paid catalog is via **guided association** (Zadig or documented equivalent), not Setup-alone. License honesty becomes MIT vs interim virtualMIDI (lab) vs WMS (community target). Magazines / polished commercial marketing are **non-goals** for this hobby phase.

### 4.2 Epics (`epics-unitor-win64-driver-2026-08-04/epics.md`)

#### Epic 4 value statement

**OLD:** New community user installs via Public Installer UX bar… community-ready (commercial-installer reading).

**NEW:** Epic 4 delivered installer packaging, user/tech docs, license honesty, and Authenticode/SmartScreen **policy**. Community “first MIDI on clean PC without paid cert” is **redefined** under the hobby install contract (guided WinUSB + SmartScreen). Full redistributable binary community launch waits on Epic 6 (WMS).

#### Add Epic 6 (backlog)

**Working title:** Community MIDI backend on Windows MIDI Services (Win11-only)

**After this epic:** Bridge creates/destroys virtual ports via WMS; community may distribute MIT Bridge/Setup binaries without virtualMIDI SDK clearance; Validation Matrix community claims are Win11; Win10 community support dropped; install docs match hobby install contract.

**Depends on:** Epic 5 complete (or at least measurement method published); Win11 lab machine available.

**Does not include:** Paid Authenticode; Tobias MSI; kernel driver.

#### Epic 5

No scope change. Explicit note: **not blocked** by OQ-1/OQ-3/WMS.

### 4.3 Architecture (`ARCHITECTURE-SPINE.md`)

| Item | Change |
|---|---|
| AD-7 | Split: **Interim lab backend** = VirtualMIDI SDK; **Next community backend** = Windows MIDI Services (Win11). Do not treat Tobias MSI clearance as a live architecture gate. |
| Deferred table | Tobias/MSI → **Abandoned**. Authenticode purchase → **No certificate purchase / hobby**. WMS → **Epic 6 community**, not vague post-V1. |
| AQ-3 | Close or reclassify: SDK pin for lab-only if needed; redistributable terms **out of community scope**. |
| AQ-4 | Remains useful for Win11 soak; expand to WMS-primary notes under Epic 6. |

### 4.4 Sprint status & deferred work

#### `sprint-status.yaml`

- `NEXT:` Epic 5 backlog → in progress when first 5.x story is created.
- Comment block: OQ-1 **abandoned/out of community scope**; OQ-3 **no certificate purchase / out of scope hobby**; Epic 6 WMS backlog after Epic 5.
- Add `epic-6` + placeholder stories once epics.md lists them (or a single epic-6 backlog line until create-story).

#### `deferred-work.md`

- Production `.cat` / OQ-3 certificate items → **out of scope for hobby posture** (revisit only if this line later ships a certificate).
- Add pointer to guided WinUSB as supported clean-PC path.
- Keep unrelated technical deferrals untouched.

### 4.5 Documentation surfaces to rewrite (implementation follow-up)

Priority order after proposal approval (docs can trail Epic 5 start):

| Surface | Change intent |
|---|---|
| `README.md` Status + License | Hobby pivot; no VirtualMIDI-linked community Releases; no cert purchase; binary public after WMS; WinUSB clean honesty |
| `docs/user/unitor-mt4-bridge-user-guide.md` | Hobby install steps; SmartScreen; guided WinUSB primary for clean PC; don’t oversell Setup-alone |
| `docs/user/unitor-mt4-bridge-guide-utilisateur.md` | Same in FR |
| `docs/user/README.md` | Index / promise alignment |
| `docs/dev/license-and-backends.md` | OQ-1 out of community scope; WMS = next community backend |
| `docs/dev/authenticode-and-smartscreen.md` | OQ-3 no certificate in this line; policy remains “unsigned + docs” |
| `docs/tests/smoke-epic4-public-installer-mt4.md` | Expected Fail Setup-alone WinUSB without catalog = known contract, not “until OQ-3 buy” |
| `docs/dev/revue-code-transverse-epic-4-community-install.md` | Align remaining open items with pivot |
| `docs/dev/prompt-demarrage-projet-bmad.md` | Optional brief honesty (Win10/virtualMIDI interim vs WMS community) |

### 4.6 Code / packaging (minimal, later)

| Change | When |
|---|---|
| User-facing Setup strings / finished-page pointing to guided WinUSB when bind fails | With hobby install doc stories |
| Do **not** embed virtualMIDI MSI | Permanent under this posture |
| Do **not** treat `sign-lab-package.ps1` as community trust | Already policy — reinforce in docs |
| WMS backend implementation | Epic 6 only |
| No Bridge MIDI protocol rewrite in this change | Out of scope |

### 4.7 Hobby install musician contract (product text to adopt)

**We promise (hobby community):**

- Free MIT sources on GitHub.
- Clear EN+FR guides.
- SmartScreen “Run anyway” instructions for unsigned Setup (when binaries are published).
- A **step-by-step** clean-PC path that includes **guided WinUSB association** (Zadig or equivalent) when Setup cannot trust the INF.
- After Epic 6: ready-to-run binaries that do **not** depend on virtualMIDI SDK redistribution.

**We no longer promise:**

- Paid-certificate polished commercial install on a brand-new PC.
- Setup-alone WinUSB success on clean Windows without a trusted catalog.
- That third-party virtualMIDI redistribution clearance is required for community Releases.
- Windows 10 as a long-term **community** target after the WMS cutover.
- Magazine “commercial product” positioning as a V1 goal.

---

## 5. Implementation Handoff

### Scope classification: **Moderate → Major (staged)**

| Stage | Class | Who |
|---|---|---|
| Apply this proposal’s planning edits | **Moderate** | PO / Dev (Correct Course follow-through or `bmad-correct-course` completion + manual edits) |
| Epic 5 implementation | **Minor–Moderate** (existing backlog) | Developer agent (`bmad-dev-story` / create-story 5.1) |
| Epic 6 WMS + community binary | **Major** | PM/Architect update already sketched here; Dev implements after Win11 lab exists |
| Doc/hobby install finish | **Minor–Moderate** | Dev / tech writer after or parallel to Epic 5 |

### Success criteria

1. Guillaume can state in one sentence what is promised to musicians **without** a shipped certificate.
2. OQ-1 and OQ-3 read as product scope (out of community scope / no certificate in this line), not as waiting on third parties.
3. Epic 5 is the active NEXT without cert/WMS blockers.
4. Epic 6 exists in backlog as the vehicle for public `.exe` freedom.
5. Docs no longer imply Setup-alone clean WinUSB success is imminent via a certificate.

### Immediate next skills (after approval)

1. Apply artifact edits from §4 (or a dedicated quick-dev/docs pass).
2. `bmad-create-story` / `bmad-dev-story` for **5.1** (Epic 5).
3. After Epic 5 + Win11 machine: `bmad-create-story` for Epic 6 (or `bmad-create-epics-and-stories` slice for WMS).

### Out of scope (reaffirmed)

- Certificate shopping or quote comparison.
- Implementing Epic 5 or WMS inside Correct Course itself.
- Claiming that clean Setup WinUSB works today when lab evidence says otherwise.
- Expanding Epic 2/3 protocol work unless install/WMS breaks them.

---

## Approval record

| Decision | Status |
|---|---|
| Incremental mode | Chosen |
| Proposals A–D (OQ-1 out of community scope, OQ-3 no certificate in this line, Epic 5 then WMS, hobby install contract) | **Approved** by Guillaume 2026-08-10 |
| Epic 5 before WMS Win11 epic (Win11 lab prepared afterward) | **Approved** 2026-08-10 |
| Full Sprint Change Proposal (content Continu) | **Approved** by Guillaume 2026-08-10 |
| Implementation of artifact edits | **Approved and applied** 2026-08-10 |
| Handoff | Epic 5 next (`bmad-create-story` / `bmad-dev-story` 5.1); Epic 6 after Win11 lab; docs hobby install follow-up optional parallel |

---

## Checklist status (Correct Course)

| Section | Status |
|---|---|
| 1 Trigger & context | Done |
| 2 Epic impact | Done |
| 3 Artifact conflicts | Done |
| 4 Path forward | Done (Hybrid) |
| 5 Proposal components | Done (this document) |
| 6 Final approval & sprint-status apply | **Done** — planning artifacts updated 2026-08-10 |
