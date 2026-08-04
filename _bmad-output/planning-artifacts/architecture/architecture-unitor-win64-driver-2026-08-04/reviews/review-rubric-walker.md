# Rubric Walker Review — ARCHITECTURE-SPINE.md

**Target:** `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md`  
**Reviewer:** rubric walker (good-spine checklist)  
**Date:** 2026-08-04  
**Spine status at review:** `draft`

---

## Gate Verdict

**CONDITIONAL PASS** — The spine is a strong initiative-altitude build substrate: paradigm, module boundaries, multi-MT4 naming, VirtualMIDI backend, installer bar, CI loop, and measurement harness are well fixed. Two operational divergence points remain undecided (Bridge runtime/deployment model; SysEx buffering for NFR-R3), and the capability map omits a few PRD-driven areas. Safe to proceed to epics **after** those gaps are closed via one new AD, one open question, and a capability-map patch — or an explicit defer with a non-divergence constraint.

---

## Checklist Results

| Criterion | Result | Notes |
| --- | --- | --- |
| Fixes real divergence points; misses none important | **Partial** | Core pipeline, naming, sessions, backend, installer, CI, harness covered. Gaps: runtime deployment model, SysEx buffering. |
| Every AD Rule enforceable and prevents divergence | **Mostly** | AD-9 hot-plug escape hatch is soft; AD-2 diagram implies direct `Sess → Vm` wiring. |
| Deferred items cannot let units diverge incompatibly | **Pass** | WiX/Inno constrained by AD-12 checklist; CI minimum in AD-13; cousin profiles gated by AD-3. |
| Named tech verified-current or ASSUMPTION tagged | **Pass** | CMake 3.20+, GHA runner, harness loop method, AD-6 serial, VirtualMIDI pin deferred to AQ-3 — all tagged or scoped. |
| Greenfield OK | **Pass** | Explicit greenfield; no brownfield contradiction; seed tree matches conventions. |
| PRD capabilities via Capability map | **Partial** | FR-13 user docs, NFR-R1 stability, NFR-R3 buffering absent from map; FR-6/FR-7 implicit only. |
| Every owned dimension decided/deferred/open | **Partial** | Operational envelope: environments/CI/install covered; **runtime deployment model** silent. |
| No template HTML; mermaid valid; AD IDs unique | **Pass** | No `<!--` comments; three mermaid blocks syntactically valid; AD-1..AD-16 unique. `lint_spine.py`: 1 low false-positive on DeviceInterfaceGUID. |

---

## Findings

### Critical

None. No AD contradicts the PRD or introduces an incompatible fork.

### High

#### H-1 — Bridge runtime deployment model is undecided

**Checklist:** operational/environmental envelope; divergence points for epics below.

The seed names `App/ServiceHost/AutoStart` and AD-10 requires Auto-Start on Windows boot and/or USB arrival, but nothing decides:

- Windows Service (LocalSystem / LocalService) vs per-user process
- Registry Run key vs Task Scheduler vs service Control Manager
- Whether the Bridge runs in the installing user's session (required for some VirtualMIDI / DAW interaction patterns) vs machine-wide service

Two epics (`installer/`, `App/`) could implement incompatible auto-start and elevation models while both claiming AD-10 compliance.

**Recommendation:** Add **AD-17** (or amend AD-10) fixing the V1 runtime shape — e.g. *single usermode process, machine-wide or per-user session TBD with one chosen mechanism and documented elevation scope* — or add **AQ-4** with a revisit gate before installer epic starts, and list under Capability map.

---

#### H-2 — NFR-R3 SysEx buffering not architected

**Checklist:** divergence points; PRD capability coverage.

AD-16 binds transparent SysEx transport (no Oberheim framing rewrite) but does not fix buffering semantics for bursty/large dumps (275 B patch, 351 B master, optional ~100× bank stress per PRD §10). NFR-R3 is binding in frontmatter (`NFR-R1..R3`) but has no AD, deferred row, or open question.

Implementers of `Protocol/`, `Device/`, and `Usb/` could choose incompatible fixed-buffer sizes, drop policies, or back-pressure strategies — directly risking Matrix-Control acceptance.

**Recommendation:** Add **AD-17** rule: minimum buffering contract (e.g. per-direction queue depth ≥ largest PRD vector with headroom; no silent truncation; back-pressure or drop logged in English). Map in Capability table. Alternatively defer with explicit *minimum queue depth tied to PRD §10 largest frame* so deferral cannot diverge.

---

#### H-3 — Capability map gaps vs PRD

**Checklist:** PRD-driven capabilities.

| PRD item | Spine coverage | Risk |
| --- | --- | --- |
| FR-13 User documentation | Not in map; no AD | Doc epics may omit VirtualMIDI prerequisite / hot-plug recovery sections inconsistently |
| NFR-R1 ~4 h session stability | Not mapped | Threading/reconnect stories may lack shared acceptance hook |
| NFR-R3 SysEx buffering | Not mapped | See H-2 |
| FR-6 / FR-7 channel + clock + MTC | Bound only via AD-1 range FR-6..FR-8 | Low — mapper is single module; acceptable if H-2 fixed |

**Recommendation:** Extend Capability map rows for user docs (`docs/` + FR-13), session stability (cross-cutting AD or story acceptance tied to NFR-R1), and SysEx buffering (post H-2).

---

### Medium

#### M-1 — AD-9 allows divergent hot-plug semantics until AQ-2 closes

Rule ends with *"or marks them unavailable per documented hot-plug behavior"* without tying to AQ-2 (auto port recreate vs supervised restart acknowledgment). Two builders could ship different "unavailable" UX while passing AD-9 literally.

**Recommendation:** Cross-reference AQ-2 in AD-9 Rule; or narrow AD-9 to *destroy ports on disconnect; recreate on reconnect per AQ-2 outcome*.

---

#### M-2 — AD-2 diagram vs ports-and-adapters purity

Dependency diagram includes `Sess --> Vm` (solid edge to concrete adapter) alongside `Sess --> Midi`. Rule text is correct (interface-only in headers); diagram may encourage direct VirtualMIDI coupling in `DeviceSession`.

**Recommendation:** Remove solid `Sess --> Vm`; keep `Vm -.implements.-> Midi` and composition root in `App/` only. Cosmetic but improves enforceability in code review.

---

#### M-3 — `[ADOPTED]` tag inconsistency

AD-1, AD-3..AD-8, AD-10, AD-14..AD-16 tagged `[ADOPTED]`; AD-2, AD-6, AD-9, AD-11..AD-13 not. Memlog shows Guillaume adopted AD-5; others were Fast-path decisions. Not a divergence risk but weakens scanability for epic authors.

**Recommendation:** Tag all settled Fast-path decisions `[ADOPTED]` or drop tags uniformly.

---

### Low

#### L-1 — Spine still `status: draft`

Expected for Fast-path first review. Flip to `final` after H-1/H-2 resolution and reviewer gate sign-off.

#### L-2 — `lint_spine.py` GUID false positive

Line 159 DeviceInterfaceGUID `{aa209017-cf8a-49ad-a0e7-701187ff7e05}` flagged as placeholder token — content is intentional, not template leakage.

#### L-3 — FR-15 / Authenticode only in Deferred

Correct per PRD (strongly recommended, not hard gate). NFR-S1 satisfied by deferral + FR-15 consequences — no action needed.

---

## Positive Observations

1. **OQ-6 closed in AD-5** with exact multi-MT4 spelling — removes a common DAW recall divergence.
2. **OQ-7 closed in AD-8** with enforceable no-exclusive-lock rule and 8-client ceiling documentation duty.
3. **Deferred table is clean** — WiX/Inno, MSI terms, Authenticode, cousin profiles, and WMS backend all carry non-divergence guardrails.
4. **Stack seed** appropriately minimal with ASSUMPTION tags where pins are not yet known (CMake, GHA, VirtualMIDI SDK build, harness loop type).
5. **Greenfield alignment** with brief, PRD §11, and `conventions.md` (builds/, PascalCase src/, lint gate).
6. **Mermaid** — three diagrams, all valid flowchart syntax; structural seed + runtime topology are clear.

---

## PRD Traceability Snapshot

Frontmatter binds `FR-1..FR-16`, `NFR-P1..P3`, `NFR-R1..R3`, `NFR-D1..D3`, `SM-1..SM-9`.

| Area | AD / section | Gap? |
| --- | --- | --- |
| WinUSB + installer | AD-12 | — |
| Bridge session + mapper | AD-1, AD-14 | — |
| Auto-start + hot-plug | AD-10 | Runtime mechanism (H-1) |
| Port topology + naming | AD-4, AD-5, AD-6 | — |
| VirtualMIDI backend | AD-7, AD-8, AD-9 | — |
| Multi-MT4 | AD-4, AD-5, AD-6 | — |
| SysEx transparent | AD-16 | Buffering (H-2) |
| Channel / clock / MTC | AD-1 (implicit) | Acceptable at initiative |
| Measurement / timing | AD-11 | — |
| CI / dual-machine | AD-13 | — |
| Quality gate | AD-15 | — |
| DeviceProfile extensibility | AD-3 | — |
| User docs | — | H-3 |
| Authenticode | Deferred | OK |
| License honesty | Conventions + AD-14 | — |

---

## Recommended Actions (priority order)

1. **Decide Bridge runtime model** (H-1) — AD or AQ before installer/App epics.
2. **Add SysEx buffering invariant** (H-2) — tie to PRD §10 frame sizes and NFR-R3.
3. **Patch Capability map** (H-3) — FR-13, NFR-R1, buffering row.
4. **Tighten AD-9 ↔ AQ-2 link** (M-1).
5. **Polish AD-2 diagram and ADOPTED tags** (M-2, M-3) at finalize.

---

## Autofix vs Discuss

| Finding | Suggested disposition |
| --- | --- |
| H-1 Runtime model | **Discuss** — product choice (user-session vs service) affects VirtualMIDI and daily admin (NFR-D2) |
| H-2 SysEx buffering | **Autofix** — can derive minimum from PRD §10 without new product debate |
| H-3 Capability map | **Autofix** |
| M-1 AD-9 / AQ-2 | **Autofix** after AQ-2 answer or interim cross-ref |
| M-2 Diagram | **Autofix** |
| M-3 ADOPTED tags | **Autofix** at finalize |

---

*End of rubric walker review.*
