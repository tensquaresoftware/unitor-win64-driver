# Input Reconciliation — Product Brief → PRD

**Input name:** `brief-unitor-win64-driver-2026-08-04`  
**Compared against:** `prd-unitor-win64-driver-2026-08-04` (`prd.md` + `addendum.md`)  
**Reconciliation date:** 2026-08-04  
**Method:** Extract-only — gaps are brief evidence not adequately preserved in PRD/addendum; no invented requirements.

---

## Coverage summary (what transferred well)

### Locked product decisions
All ten locked decisions from `brief.md` § « Locked product decisions » appear in PRD §11 (Constraints and Guardrails) with equivalent substance: Win10/11 x64 (Win10 mandatory), usermode WinUSB + C++ only, VirtualMIDI as V1 backend, MIT license, MT4-validated scope with multi-DeviceProfile / multi-instance from day one, channel MIDI + clock + SysEx required, stable macOS-like port naming, `conventions.md` + `lint-touched.py` quality gate, Ten Square Software public facade, Authenticode strongly recommended but not a hard gate.

### Scope, non-goals, and MVP boundary
In-scope V1 items (MT4 2 IN / 4 OUT, SysEx, clock, auto-start, multi-client, multi-MT4, installer + docs, Matrix-Control validation path) and explicit outs (Patch, LTC/VITC, Fast Mode/AMT, cascaded stacks, guaranteed cousin devices, WMS-only V1, kernel driver, MIDI 2.0) map cleanly to PRD §5–§6 and FR-16.

### Success bar — operability
~4 h session stability, hot-plug without Windows reboot, multi-MT4 design with honest validation status, SysEx as V1 requirement (Matrix-Control), port topology (2+4 not channel flood), stable naming, multi-client, auto-start, and admin-at-install-only are all testable in PRD FRs, NFRs, and success metrics (SM-1–SM-8).

### Brief addendum → PRD addendum
Hardware/cable mask table, Linux quirk reference, architecture orientation bullets, VirtualMIDI licensing snapshot, rejected alternatives table, community forum URLs, naming/trademark note, dev/validation environment table, latency planning context, and installer UX bar transferred with high fidelity. PRD addendum adds useful enrichment (VirtualMIDI URLs, loopMIDI/rtpMIDI eval note, MIDI-OX lock rationale, Windows MIDI Services landscape note, Validation Matrix lock record).

### Problem → requirements traceability
Core problem (USB enumerates, no usable MIDI ports; non-class-compliant Emagic cable mapping; orphaned hardware) and solution shape (WinUSB + original protocol reimplementation + VirtualMIDI virtual ports) are preserved in PRD §1 Vision and FR-1/FR-2. Differentiation as honesty for orphaned hardware — not invented feature moats — appears verbatim in PRD §1.

### Open questions — mostly carried
Seven of eight brief open questions appear in PRD §12. Brief open question #3 (validation matrix hosts) was intentionally deferred to PRD and is now **resolved** in PRD §10 (Ableton Live 12, Reason Studios 12, Matrix-Control, MIDI-OX on Win10+11 x64).

### Risks and external dependencies
Brief risks table maps to PRD §9 with equivalent mitigations (VirtualMIDI clearance, timing harness, SysEx buffering, protocol reference strategy, SmartScreen, dual-machine validation, multi-MT4 naming, cousin-device scope control).

### User intent and journeys
Primary persona (MT4 owner needing DAW + SysEx), first-party Matrix-Control motivation, secondary AMT8/Unitor8 watchers (not promised V1), emotional “no Driver Hell” trust, and end-user install→plug→auto-start→work flow are expressed in PRD §2 JTBD and UJ-1–UJ-4.

---

## Gaps list

Each gap: **brief evidence → PRD status → severity → suggested action**.

### G-01 — Usermode jitter is not an excuse

- **Brief evidence:** Success Criteria — Timing: *« Excessive jitter is not an alibi for the usermode path. »* (`brief.md` L103). Memlog reinforces studio credibility bar tied to timing (` .memlog.md` L9).
- **PRD status:** NFR-P1/P2 set provisional targets and Studio-Done Gate requires measurement, but no explicit guardrail that poor jitter cannot be dismissed because the stack is usermode. Counter-metrics (SM-C2) reject ASIO-as-proof only.
- **Severity:** **high**
- **Suggested PRD/addendum action:** Add a normative sentence to NFR-P2 or §7 counter-metrics: usermode architecture does not waive jitter acceptance; failure to meet provisional/revised jitter targets blocks “studio-done” regardless of implementation choice.

### G-02 — C++17 language standard lock

- **Brief evidence:** Solution step 2: *« Run a C++17 service/application »* (`brief.md` L36). Locked stack orientation implies C++17 usermode.
- **PRD status:** PRD references *« C++ usermode Bridge »* (§1, FR-2, §6) but never locks **C++17** as a product/implementation constraint in §11 or NFRs.
- **Severity:** **medium**
- **Suggested PRD/addendum action:** Add C++17 to §11 locked decisions or NFR-Q1 companion constraint; note in addendum that this matches project conventions and brief stack lock.

### G-03 — “Serious open-source hardware-support project — not a throwaway MVP” bar

- **Brief evidence:** Executive Summary: *« documentation and packaging quality expected of a serious open-source hardware-support project — not a throwaway MVP »* (`brief.md` L17). Memlog: *« Public facade… V1 aims user-friendly installer (macOS-class simplicity) »* (`.memlog.md` L10).
- **PRD status:** FR-12/FR-13/FR-14 encode installer and docs requirements functionally; SM-6 covers license honesty. The **anti-MVP / serious OSS hardware-support** positioning is not stated as an acceptance bar — requirements read as checklist items without the brief’s quality ceiling narrative.
- **Severity:** **medium**
- **Suggested PRD/addendum action:** Add a short « Community release bar » subsection under §7 or a vision-adjacent note: V1 is rejected as success if packaging/docs feel like a throwaway MVP despite passing minimal FR checks. Optionally reference in SM-6 consequences.

### G-04 — Windows MIDI Services Win11-only qualifier in locked table

- **Brief evidence:** Locked decisions: *« Windows MIDI Services = v2 / second backend (Win11-only) »* (`brief.md` L47).
- **PRD status:** PRD §11 locked table says *« Windows MIDI Services = v2 / second backend »* without **Win11-only**. PRD addendum § « Windows MIDI Services landscape note » captures Win11 focus but is not in the authoritative locked-decisions table.
- **Severity:** **medium**
- **Suggested PRD/addendum action:** Restore *« (Win11-only) »* or equivalent in PRD §11 MIDI backend row to match brief lock and prevent Win10 WMS scope creep.

### G-05 — Contributor / tester audience persona

- **Brief evidence:** Who This Serves — *« Contributors / testers: Community members who can validate cousin hardware post-MVP »* (`brief.md` L62–62). Success for them implied via technical docs and honest cousin path.
- **PRD status:** FR-14 enables contributor technical docs; FR-16 promises non-blocking architecture for cousins. PRD §2 lists end-user JTBD and non-users only — **no explicit contributor/tester persona** or success criteria for post-MVP hardware validation participation.
- **Severity:** **medium**
- **Suggested PRD/addendum action:** Add §2.4 « Contributors (post-MVP validation) » with JTBD (*validate cousin hardware when available, contribute without GPL vendoring*) and optional secondary metric SM for contributor doc completeness / hardware-gated story clarity.

### G-06 — Post-MVP north star: trusted family bridge + predictable releases

- **Brief evidence:** Post-MVP: *« Longer term: trusted Windows bridge for the Unitor protocol family, with clear docs and predictable releases under Ten Square Software »* (`brief.md` L91).
- **PRD status:** §6.2 and FR-16 mention post-MVP DeviceProfiles and WMS backend; no **predictable releases** or **trusted family bridge** north-star statement. Release cadence/trust under Ten Square Software absent.
- **Severity:** **low**
- **Suggested PRD/addendum action:** Add brief « Post-MVP orientation (non-binding) » paragraph to PRD addendum mirroring brief L89–91; keeps FR scope intact while preserving product horizon for Architecture/epics.

### G-07 — Historical problem anchor (XP 32-bit, multi-decade orphan)

- **Brief evidence:** Problem: *« last official package targeted Windows XP (32-bit) »*; forum *« multi-year demand »* (`brief.md` L25–25, L27). Community URLs in brief addendum.
- **PRD status:** PRD §1 states no official 64-bit driver; addendum lists forum links. **XP/32-bit anchor and multi-year community demand** not in PRD vision/problem narrative (only in addendum links table).
- **Severity:** **low**
- **Suggested PRD/addendum action:** Optional one-sentence problem context in PRD §1 or addendum « Community evidence » intro tying links to *decades-long* demand; supports honesty/differentiation tone without new FRs.

### G-08 — « Studio-grade » / « engineering quality and community credibility » paired bar

- **Brief evidence:** Executive Summary: *« studio-grade software bridge »*, *« high bar for engineering quality and community credibility »* (`brief.md` L17–19).
- **PRD status:** « Studio operability » appears in metrics (SM-1, Studio-Done Gate); « community credibility » appears once in FR-14 trace. The **paired engineering + community credibility** success theme from the brief is fragmented across NFR-Q1 and SM-6 without explicit coupling.
- **Severity:** **low**
- **Suggested PRD/addendum action:** Add qualitative acceptance note under §7: studio-grade operability **and** community credibility are jointly required for V1 ship (not either/or).

### G-09 — Linux QUIRK_MIDI_EMAGIC contrast in main PRD narrative

- **Brief evidence:** Root cause: *« Linux does this via QUIRK_MIDI_EMAGIC; Windows does not »* (`brief.md` L29). Addendum expands quirk table.
- **PRD status:** PRD §1 explains generic class stack gap; Linux quirk reference lives only in **addendum**, not `prd.md` problem statement. Not wrong, but brief’s cross-platform proof-of-solvability is weakened in the primary doc.
- **Severity:** **low**
- **Suggested PRD/addendum action:** One clause in PRD §1 Vision: public Linux kernel quirk proves protocol is implementable; Windows lacks equivalent — motivates project without vendoring GPL.

### G-10 — Prodikeys64 pattern in authoritative PRD body

- **Brief evidence:** Locked architecture orientation: *« Pattern inspiration: Prodikeys64 (WinUSB + virtual MIDI) »* (`brief.md` L56). Brief addendum § Architecture notes.
- **PRD status:** Present in PRD `addendum.md` only; absent from `prd.md` §11 or architecture pointer in §0.
- **Severity:** **low**
- **Suggested PRD/addendum action:** Optional single bullet under §0 « Mechanism depth belongs in Architecture » citing Prodikeys64 as **pattern reference, not dependency** — aligns brief lock without expanding FR scope.

### G-11 — Emagic manual reference for out-of-scope feature context

- **Brief evidence:** Brief addendum links Emagic Unitor8/AMT8 functional manual for features mostly out of V1 scope (`addendum.md` L51–52).
- **PRD status:** Same URL in PRD addendum § Community evidence. **No PRD pointer** that manual documents Patch/LTC/Fast Mode/etc. for future DeviceProfile capability flags.
- **Severity:** **low**
- **Suggested PRD/addendum action:** Add one line in PRD addendum Architecture orientation: manual is reference for deferred capability flags, not V1 acceptance input.

### G-12 — Validation matrix expansion beyond brief minimum (informational, not a gap)

- **Brief evidence:** *« at least one DAW + one SysEx path (Matrix-Control); exact hosts locked in PRD »* (`brief.md` L102).
- **PRD status:** PRD §10 locks **two** DAWs (Ableton Live 12, Reason Studios 12) plus MIDI-OX utility — stricter than brief minimum. Documented in PRD addendum Validation Matrix lock.
- **Severity:** **low** (expansion, not regression)
- **Suggested PRD/addendum action:** None required; optionally note in addendum that dual-DAW matrix exceeds brief minimum deliberately for studio credibility.

---

## Qualitative ideas that FR structure might drop

These are brief tone, voice, and feel elements that functional requirements and success metrics may not preserve unless explicitly carried in vision, counter-metrics, or release bar prose.

| Brief qualitative idea | Where it lives in brief | PRD preservation | Risk if omitted |
| --- | --- | --- | --- |
| **Honesty over feature moats** — differentiation is solving a hardware orphan honestly, not inventing moats | Exec summary L19; echoed in PRD §1 | **Well preserved** verbatim | Low |
| **Anti-throwaway-MVP / serious OSS hardware-support** — packaging and docs at credible community project standard | Exec summary L17 | **Weakened** — FR checklist without ceiling narrative (G-03) | Medium — ship could pass FRs with mediocre UX |
| **Driver Hell / shady unsigned stacks emotional relief** | Who This Serves; PRD JTBD emotional | **Preserved** in §2.1 emotional JTBD | Low |
| **SysEx-heavy workflows especially painful** — editors/librarians blocked entirely without ports | Problem L27–28 | **Partial** — covered in UJ-2/SM-2 but problem statement less emphatic | Low |
| **Jitter discipline — usermode is not an alibi** | Success timing L103 | **Missing** as philosophy (G-01) | High — undermines studio credibility stance |
| **Honest validation status** — e.g. single MT4 at ship for dual-unit design | Success L106; UJ-3 | **Well preserved** in FR-10, UJ-3, SM-8 | Low |
| **Three-way license clarity** (MIT repo / VirtualMIDI proprietary / WMS future) | Success release bar L112 | **Well preserved** FR-14, SM-6 | Low |
| **macOS-class installer feel** — few steps, progress, success state, minimal jargon | Assumptions L147; addendum Installer UX | **Preserved** FR-12 + assumption index | Low — but « macOS-class » is feel, not just steps |
| **Community evidence / multi-year forum demand** — social proof of problem | Problem L25; addendum URLs | **Relegated to addendum links** (G-07) | Low for engineering; medium for public narrative |
| **Ten Square Software as trusted public facade** | Throughout | **Preserved** | Low |
| **Matrix-Control as motivation, not dependency** | Exec L21; addendum | **Well preserved** FR-8, glossary, non-goals | Low |
| **Predictable releases / long-term family bridge trust** | Post-MVP L91 | **Absent** (G-06) | Low for V1; medium for community retention narrative |
| **Contributor welcome path** — cousin hardware validation post-MVP | Who This Serves L62 | **Implicit** in FR-14/16 only (G-05) | Medium — contributors may feel unspecified |
| **Measurable timing discipline** (brand phrase) | Exec L19 | **Split** into NFR-P1/P2 + Studio-Done Gate — mechanism yes, phrase no | Low |
| **Decades without 64-bit vendor driver** — historical weight | Exec L15 | **Shortened** to « no official 64-bit vendor driver » | Low |
| **Excessive jitter / latency credibility tied to studio clock feel** | Addendum latency context L72–73 | **Partial** — jitter vs latency emphasis in addendum; not normative in PRD body | Medium |

---

## Summary assessment

| Dimension | Verdict |
| --- | --- |
| Locked decisions | **Strong transfer** — one wording gap (WMS Win11-only in §11) |
| Scope / non-goals | **Complete** |
| Operability success bar | **Strong** — jitter « no alibi » philosophy is the main miss |
| Brief addendum technical depth | **Complete** in PRD addendum |
| Qualitative voice (honesty, community, anti-MVP) | **Partial** — functionalized but ceiling narrative and contributor persona diluted |
| Open questions | **Mostly closed or carried** — validation matrix resolved in PRD |

**Overall reconciliation verdict:** **gaps-need-fix** — no blocking scope inversion, but **one high-severity philosophy gap** (G-01 jitter alibi) and several **medium** items (C++17 lock, anti-MVP bar, WMS Win11-only in §11, contributor persona) should be patched in PRD/addendum before finalize sign-off.

---

## Recommended patch priority (for PRD Finalize)

1. **G-01** — Add usermode jitter non-alibi guardrail (high).
2. **G-04** — Restore Win11-only on WMS second backend in §11 (medium, lock integrity).
3. **G-02** — Lock C++17 in constraints (medium).
4. **G-03** — Anti-throwaway-MVP release bar sentence (medium, qualitative).
5. **G-05** — Contributor persona stub in §2 (medium).
6. **G-06, G-07, G-08, G-09, G-10, G-11** — Addendum or light §1/§7 prose (low).
