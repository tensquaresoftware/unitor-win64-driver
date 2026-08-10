# Input Reconciliation — Passation + Finalize memlog vs PRD draft

**Date:** 2026-08-04  
**Inputs compared:**
- Passation: `docs/dev/passation-bmad-prd-2026-08-04.md`
- Finalize memlog decisions: `.memlog.md` (post-audit entries)
- PRD draft: `prd.md`
- PRD addendum: `addendum.md`

**Method:** Extract-only — report what is present, missing, vague, or contradictory. No invention.

---

## Executive summary

The PRD draft and addendum already carry most passation-level product decisions (validation matrix, Studio-Done Gate, VirtualMIDI release gate concept, open-question list, backend abstraction). **Eight memlog decisions logged during Finalize audit are not yet reflected in `prd.md` / `addendum.md`**, plus open-question triage metadata expected by passation §2 and Finalize step 4.

**Overall:** `gaps-need-fix` — not blocking Architecture *if* triage is understood from memlog/passation, but **Finalize polish should patch PRD/addendum before `status: final`**.

No hard contradiction with locked brief decisions. One **input-source conflict** (passation vs memlog on Tobias contact status).

---

## Coverage matrix (locked new facts)

| # | Locked fact (memlog / passation) | PRD | Addendum | Verdict |
| --- | --- | --- | --- | --- |
| 1 | VirtualMIDI = SDK **programmatic port create/destroy**; driver prerequisite (eval via loopMIDI/rtpMIDI or licensed MSI) | Glossary: "SDK/driver stack" only. FR-12 mentions prerequisite messaging, not SDK integration model. | §VirtualMIDI licensing covers eval path (loopMIDI/rtpMIDI), MSI, redistribution gate. Does **not** state programmatic create/destroy. | **Partial** — addendum closer; PRD body gap |
| 2 | VirtualMIDI redistribution / community binary scope (owner Guillaume) — historically release-gate discussion; Correct Course 2026-08-10 sets VirtualMIDI-linked community Releases **out of scope** | OQ-1 scope historically incomplete | Author clearance / redistrib terms | **Superseded** by Correct Course 2026-08-10 |
| 3 | Explicit provisional latency: healthy bridge-added **≤4–5 ms p99**; jitter **≤1–2 ms p99**; **do-not-ship-worse ~8–10 ms p99**; MIDI Path not ASIO; Studio-Done Gate | NFR-P1/P2: "low single-digit ms" / "sub-ms to low-ms" (vague). No ship ceiling. MIDI Path + Studio-Done Gate present. | Same vague anchors in §Latency planning context. | **Gap** — numeric anchors locked in memlog not in artifacts |
| 4 | Home-grown kernel VirtualMIDI **Plan B deferred** V1; independence = backend abstraction + possible Windows MIDI Services later | Non-goals: custom kernel driver rejected. NFR-Q3: abstractable backend. Windows MIDI Services as future backend. | Rejected: custom KMDF driver. Backend abstraction noted. | **Partial** — generic kernel rejection covers spirit; **Plan B** not named |
| 5 | `github.com/aaron1a12/virtual-midi` = integration proof only, **NOT fork base** (GPL) | Not mentioned. GPL vendoring rejected generically (FR-14, NFR-Q2). | Not mentioned. | **Missing** |
| 6 | Open-question triage expectations (Tobias open OK; latency provisional; Authenticode defer; Emagic defer Arch; CI/CD defer; port spelling defer; multi-client defer; Matrix-Control SysEx owner Guillaume) | §12 lists 8 OQs without owner / blocker class / revisit condition. | N/A | **Gap** — list present, triage metadata absent |
| 7 | (Optional) Reason Studios 12 vs Reason 12 label consistency | Consistent **Reason Studios 12** + assumption on SKU. | Consistent. | **Covered** (memlog typo "Reason Studio 12" is memlog-only) |

---

## Gaps by severity

### High — should patch before `status: final`

| Gap | Source | What's missing |
| --- | --- | --- |
| **H-1 Explicit latency numbers + ship ceiling** | Memlog decision | NFR-P1/P2 and addendum §Latency still use vague "low single-digit ms". Memlog locks: healthy ≤4–5 ms p99 bridge-added, jitter ≤1–2 ms p99, do-not-ship-worse ~8–10 ms p99. Ceiling entirely absent. |
| **H-2 VirtualMIDI SDK integration model in PRD** | Memlog decision | Product requirement that Bridge creates/destroys ports via SDK API (not relying on end-user manually running loopMIDI UI alone). Eval vs licensed MSI paths belong in FR/glossary or addendum cross-ref. |

### Medium — polish / Architecture handoff clarity

| Gap | Source | What's missing |
| --- | --- | --- |
| **M-1 VirtualMIDI redistrib / community binary scope** | Memlog + passation §2 row 1 | Document owner Guillaume; community VirtualMIDI-linked Releases **out of scope** (Correct Course 2026-08-10); Epic 6 WMS path |
| **M-2 Open Questions triage table** | Passation §2 + Finalize step 4 | §12 needs per-item: owner, blocker class (release / Architecture / docs), defer OK?, next action. Especially OQ-1 (Guillaume), OQ-8 (Guillaume, SysEx vectors). |
| **M-3 aaron1a12/virtual-midi reference policy** | Memlog decision | Addendum "Reference / community evidence" or Architecture orientation: proof-of-integration only; GPL — not fork base. |
| **M-4 Home-grown kernel VirtualMIDI Plan B deferred** | Memlog decision | Name explicitly in addendum rejected/deferred table (distinct from generic KMDF rejection). |

### Low — optional / already sufficient at draft level

| Gap | Source | What's missing |
| --- | --- | --- |
| **L-1 Reason SKU string** | Passation §3 optional | PRD assumption already covers; confirm at doc-writing time. |
| **L-2 Memlog typo "Reason Studio 12"** | `.memlog.md` line 8 | PRD is correct; fix memlog at polish if desired (out of reconcile scope for PRD). |

---

## Contradictions

### C-1 Passation vs memlog (input sources — not PRD internal)

| Source | Statement |
| --- | --- |
| Passation line 120 | "Pas de contact Tobias encore documenté comme fait" |
| Memlog (Finalize audit) | VirtualMIDI redistrib / clearance tracked as owner concern; later Correct Course sets community Releases out of scope |

**Resolution for Finalize:** Keep third-party contact status out of public docs. Planning wording states product scope (out of community scope / Epic 6 WMS), not private contact narrative.

### C-2 OQ-1 wording ambiguity (soft — not a true conflict)

- OQ-1 labels VirtualMIDI terms as "**blocker for Public Installer**" — correct per locked decisions.
- Missing explicit "**not** a blocker for PRD finalization or Architecture" could cause misread during Finalize triage.
- **Not a product contradiction** — clarity gap only.

### C-3 No PRD contradiction with locked brief decisions

Passation explicitly forbids reopening: usermode, VirtualMIDI V1, MIT, MT4-only validated, SysEx V1, Win10 mandatory. PRD §11 aligns. **No conflict detected.**

---

## Passation open-question triage — expected vs PRD §12

| # | Passation action | PRD §12 present? | Owner / defer in PRD? |
| --- | --- | --- | --- |
| 1 | VirtualMIDI eval/redist scope — release gate historically; open OK; note owner/action | OQ-1 (partial scope) | **Missing** owner Guillaume, community-scope framing, non-blocker for Arch |
| 2 | Latency — OK provisional + Studio-Done Gate | OQ-2 + NFR-P1–P3 | Covered (numbers should tighten per H-1) |
| 3 | Authenticode — defer with owner | OQ-3 + FR-15 | Owner missing |
| 4 | Emagic docs — defer Architecture | OQ-4 | Covered |
| 5 | CI/CD — min Windows build CI; detail Architecture | OQ-5 + NFR-D3 | Covered |
| 6 | Port spelling — Architecture/UX | OQ-6 | Covered |
| 7 | Multi-client — Architecture confirm | OQ-7 + FR-9 assumption | Covered |
| 8 | Matrix-Control SysEx vectors — Guillaume when possible | OQ-8 | **Missing** owner Guillaume |

---

## What is already well covered (no patch required)

- Validation matrix lock (Ableton Live 12, Reason Studios 12, Matrix-Control, MIDI-OX, Win10 mandatory + Win11).
- Studio-Done Gate + MIDI Path (not ASIO) measurement policy.
- VirtualMIDI redistribution as Public Installer release gate (conceptual).
- MIDI-OX as V1 multi-client utility (ShowMIDI / MidiView retired); multi-client FR-9 with Architecture assumption.
- Backend abstraction for future Windows MIDI Services (NFR-Q3, addendum).
- Authenticode strongly recommended, not hard gate V1.
- Brief locked decisions in §11; traceability §14.
- Addendum hardware masks, licensing research snapshot, rejected alternatives (KMDF, WMS-only V1, GPL vendoring).

---

## Suggested patches (for Finalize polish — do not apply in reconciliation pass)

### Patch 1 — `prd.md` glossary + FR-12 (VirtualMIDI SDK model)

**Target:** §3 Glossary `VirtualMIDI` entry; optionally FR-12 consequences.

**Add essence:**
- Bridge uses VirtualMIDI **SDK** to programmatically create/destroy virtual ports at runtime.
- A VirtualMIDI **driver must be present** on the machine: evaluation via user-preinstalled loopMIDI/rtpMIDI, or redistributable installer via licensed MSI after author clearance.

### Patch 2 — `prd.md` §12 OQ-1 + risks table (Tobias status)

**Target:** §12 item 1; optionally §9 VirtualMIDI row.

**Add essence:**
- Community Releases of VirtualMIDI-linked binaries are **out of scope** as of Correct Course 2026-08-10 (see `prd.md` OQ-1); Epic 6 WMS is the community binary path.
- **Owner:** Guillaume.
- **Blocks:** redistributable Public Installer only (historical framing).
- **Does not block:** PRD finalization or Architecture.

### Patch 3 — `prd.md` NFR-P1, NFR-P2, assumptions index + `addendum.md` §Latency

**Target:** §8 NFR-P1/P2; §13 assumptions; addendum §Latency planning context.

**Replace vague anchors with memlog numbers (keep provisional + Studio-Done Gate):**
- **Healthy target:** bridge-added latency ≤ **4–5 ms p99** on MIDI Path.
- **Healthy target:** jitter ≤ **1–2 ms p99**.
- **Do-not-ship-worse ceiling:** ~ **8–10 ms p99** bridge-added until revised by harness.
- Reiterate: measure MIDI Path, not ASIO; Studio-Done Gate still applies.

### Patch 4 — `addendum.md` rejected/deferred + reference policy

**Target:** §Rejected / deferred alternatives table; new short §Reference integrations.

**Add rows/notes:**
- Home-grown kernel VirtualMIDI (Plan B) — **deferred V1** (effort); independence via backend abstraction + optional Windows MIDI Services later.
- `github.com/aaron1a12/virtual-midi` — **integration proof only**; GPL / vendored SDK — **not** a fork or code base for this repo.

### Patch 5 — `prd.md` §12 triage metadata

**Target:** §12 Open Questions (table or sub-bullets per item).

**Add columns/fields:** Owner | Blocks | Defer OK | Next action/revisit.

Example for OQ-1: Guillaume | Public Installer redistribution | Yes (PRD/Arch proceed) | Keep VirtualMIDI-linked community Releases out of scope; evaluate loopMIDI/rtpMIDI for lab vs Epic 6 WMS community path.

Example for OQ-8: Guillaume | Studio-Done SysEx acceptance definition | Yes until vectors defined | Define representative Matrix-Control dump/restore pass set.

### Patch 6 — (Optional) memlog consistency

Fix `.memlog.md` line 8 typo "Reason Studio 12" → "Reason Studios 12" when polishing memlog.

---

## Reconciliation verdict

| Dimension | Assessment |
| --- | --- |
| Passation product decisions in PRD | **Mostly covered** |
| Finalize memlog new decisions in PRD/addendum | **5/7 substantive gaps** (latency numbers, SDK model, Tobias status, aaron1a12, Plan B naming; triage metadata) |
| Contradictions with locked brief | **None** |
| Blocking Architecture? | **No** (per passation: open items may remain with explicit triage) |
| Blocking PRD `status: final`? | **Soft yes** — polish should close H-1/H-2 and M-1/M-2 before final |

**Recommended next Finalize steps:** Apply patches 1–5 during polish (step 5); run open-question triage (step 4) using passation table + memlog owners; then reviewer gate.

---

*Generated by BMad PRD Finalize input reconciliation (passation slug).*
