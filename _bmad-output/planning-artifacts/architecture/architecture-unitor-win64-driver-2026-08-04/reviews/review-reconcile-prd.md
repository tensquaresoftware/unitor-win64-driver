# PRD ↔ Architecture Spine Reconciliation Review

**Date:** 2026-08-04  
**Reviewer:** BMad reconcile pass (PRD final vs spine draft)  
**Inputs:**
- `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md` (status: final)
- `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/addendum.md` (hardware, SysEx, VirtualMIDI skim)
- `_bmad-output/planning-artifacts/architecture/architecture-unitor-win64-driver-2026-08-04/ARCHITECTURE-SPINE.md` (status: draft)

**Scope:** List **quiet gaps only** — PRD requirements, tones, or constraints that did **not** land in the spine. Items explicitly **Deferred** in the spine that match PRD deferrals (Tobias/VirtualMIDI redistribution, Authenticode cert path, final latency/jitter numbers) are **excluded**. No new product decisions invented.

---

## Verdict

**gaps**

The spine captures the structural substrate well (pipeline, DeviceProfile, VirtualMIDI, installer INF path, harness location, SysEx transparency, multi-MT4 sessions). Several **normative PRD acceptance bars** and **locked product tones** remain implicit or absent — they are not loudly contradicted, but a builder reading only the spine could miss them.

---

## Coverage Summary (what landed)

| PRD area | Spine landing | Notes |
| --- | --- | --- |
| FR-1 WinUSB bind (INF primary, Zadig fallback) | AD-12 | GUID + INF rule present |
| FR-2 C++17 usermode Bridge, no kernel driver | AD-1, AD-2, stack | ✓ |
| FR-3 Auto-Start, no daily admin | AD-10, AD-12 checklist | ✓ |
| FR-4 2 IN / 4 OUT topology | AD-5 | ✓ |
| FR-5 Stable macOS-like names, multi-MT4 | AD-5, AD-6 | Spelling resolved (OQ-6) |
| FR-8 SysEx transparent, Matrix vectors | AD-16 | Vectors listed; mixed-wire ✓ |
| FR-9 Multi-client | AD-8 | OQ-7 resolved in spine |
| FR-10 Multi-MT4 two sessions | AD-4 | ✓ |
| FR-11 Hot-plug without reboot | AD-10 | ✓ |
| FR-12 Installer UX bar | AD-12 checklist | ✓ |
| FR-14 License honesty (MIT ≠ VirtualMIDI ≠ WMS) | Consistency conventions, AD-14 | Partial — see gap G-4 |
| FR-15 / NFR-S1 Authenticode | Deferred | Matches PRD — excluded |
| FR-16 DeviceProfile extensibility | AD-3 | ✓ |
| NFR-P1–P3 provisional timing + harness | AD-11 | Provisional numbers ✓; decision gate partial — G-6 |
| NFR-D3 dual-machine + Windows CI minimum | AD-13 | ✓ |
| NFR-Q1 lint-touched.py | AD-15 | ✓ |
| NFR-Q3 backend abstraction | AD-2 | ✓ |
| OQ-1 VirtualMIDI redistribution | Deferred | Matches PRD — excluded |
| OQ-2 final latency thresholds | Deferred | Matches PRD — excluded |
| OQ-3 Authenticode cert path | Deferred | Matches PRD — excluded |
| Hardware masks (MT4) | AD-3, addendum aligned | ✓ |
| VirtualMIDI SDK programmatic ports | AD-7 | ✓ |
| Non-goals: cascade, kernel driver, GPL vendoring | AD-1, AD-4, AD-14 | ✓ |

---

## Quiet Gaps

### G-1 — FR-7 / SM-1: MIDI clock, transport realtime, and MTC not explicit

**PRD says:** FR-7 requires MIDI clock (`0xF8`), Start / Stop / Continue, and **MTC** (quarter-frame and full-frame) as **required V1 transport**, carried without Bridge-induced dropouts under the session-stability scenario. SM-1 explicitly validates clock **including Start/Stop/Continue and MTC** on both OS targets. §11 locks “clock + Start/Stop/Continue + MTC + SysEx” as a product decision.

**Spine says:** AD-1 binds `FR-6..FR-8` generically (“MIDI bytes flow”) but no invariant names clock, transport realtime, or MTC. AD-16 covers SysEx only. Validation Matrix §10 pass rules (DAW smoke with MTC) are not mirrored.

**Gap type:** Functional requirement + locked constraint  
**Risk:** Implementer treats MTC/clock as implicit channel traffic with no soak or pass-vector anchor; Studio-Done / validation drift.

---

### G-2 — NFR-R1 / SM-3: ~4-hour session stability not architecture-bound

**PRD says:** ~**4 hours** continuous studio/editor use **including SysEx Session activity** without mandatory Bridge restart for normal use (NFR-R1, SM-3). §10 stability sample: “~4h including SysEx activity on at least Win10 x64.” Counter-metric SM-C4: jitter must not be excused as usermode alibi (partially in AD-11).

**Spine says:** AD-10 binds SM-4/SM-5 (hot-plug, install) but **no rule or harness note** for long-session lifecycle (thread leaks, SysEx burst over hours, Virtual Port handle churn, session manager durability). SM-3 appears in front-matter `binds` list only.

**Gap type:** Non-functional acceptance bar  
**Risk:** Architecture reviews pass without a longevity/soak expectation; “works for 20 minutes” could ship against PRD.

---

### G-3 — NFR-R3: SysEx burst buffering requirement absent

**PRD says:** “SysEx bursts **buffered sufficiently** for Matrix-Control librarian dumps” (NFR-R3). FR-8: bursty/large SysEx must not require Bridge restart for normal librarian completion. §10 optional bank stress (~100× 275 B frames).

**Spine says:** AD-16 requires **transparent transport** and lists minimum pass vectors; **no buffering, queue depth, or backpressure rule** between Virtual Ports and WinUSB bulk path.

**Gap type:** Functional / reliability requirement  
**Risk:** Minimal pass-through design fails master dump or bank stress under real Matrix-Control pacing; gap discovered only at hardware validation.

---

### G-4 — FR-13: End-user documentation scope not in spine

**PRD says:** User docs must cover VirtualMIDI prerequisites, install, Auto-Start, **first MIDI test**, **first SysEx test**, **troubleshooting**, and an explicit **works / does-not-work** list. Consequence: new user completes UJ-1 and UJ-2 using only shipped docs.

**Spine says:** AD-13 documents **contributor** dual-machine loop and CI; AD-12 mentions prerequisite messaging in installer. **No structural seed or invariant** for end-user doc deliverables (`docs/user/` or equivalent), SysEx-first-test path, or works/does-not-work matrix.

**Gap type:** Functional requirement (docs deliverable)  
**Risk:** Contributor docs ship; acceptance FR-13 / SM-5 user-doc bar has no architecture hook for epics or release checklist.

---

### G-5 — §10 Validation Matrix: locked hosts and pass rules not consolidated

**PRD says:** §10 locks **Ableton Live 12**, **Reason Studios 12**, **Matrix-Control**, **ShowMIDI** on Win10 x64 (mandatory) and Win11 x64, with explicit pass rules (DAW notes/CC/clock+MTC, Matrix minimum vectors, ShowMIDI+DAW concurrent, ~4h stability sample, hot-plug drill).

**Spine says:** Hosts appear ad hoc (AD-8: Ableton/Reason + ShowMIDI; diagram lists Matrix-Control). **No single acceptance anchor** in spine tying implementation milestones to §10 pass rules. Reason Studios 12 only implied; Matrix-Control validation lives only via AD-16 vector list without “Validation Matrix” label.

**Gap type:** Acceptance / traceability constraint  
**Risk:** Epics and test design lack a spine-level pointer to §10 as the V1 definition of done for host proof.

---

### G-6 — NFR-P1: “Ship above ceiling” requires explicit product decision

**PRD says:** Provisional do-not-ship-worse ceiling ~**8–10 ms p99**; **shipping above that ceiling requires an explicit product decision** (NFR-P1, SM-C4 tone).

**Spine says:** AD-11 repeats provisional anchors including ceiling numbers but **does not state the decision gate** — only measurement and publish rules.

**Gap type:** Product guardrail / tone  
**Risk:** Measured regression above ceiling treated as “document and ship” without product sign-off.

---

### G-7 — FR-10: Honest multi-MT4 validation disclosure

**PRD says:** If only one physical MT4 is available at ship time, **release notes/docs state multi-instance validation status honestly** (FR-10 consequence, SM-8).

**Spine says:** AD-4 supports two sessions; **no doc/release honesty rule** when dual-unit hardware proof is incomplete.

**Gap type:** Acceptance / community honesty tone  
**Risk:** Marketing or README implies dual-unit proof without spine reminder.

---

### G-8 — §11 locked decisions: Ten Square Software public facade

**PRD says:** Public facade **Ten Square Software** (§11, addendum naming/trademarks).

**Spine says:** Not referenced in stack, conventions, or installer rules.

**Gap type:** Locked product constraint (packaging/branding)  
**Risk:** Low for build substrate; visible in installer/about strings and release materials without architecture pointer.

---

### G-9 — FR-8: Matrix-Control is not a Bridge runtime dependency

**PRD says:** Matrix-Control is first-party **validation target**, **not** bundled or required at Bridge runtime (FR-8, §5 non-goals).

**Spine says:** Matrix-Control appears in diagram as client only; **no explicit boundary rule** (Bridge build/runtime must not depend on Matrix-Control artifacts).

**Gap type:** Scope guardrail  
**Risk:** Low if obvious; worth one line in consistency or deferred non-goals echo.

---

### G-10 — SM-C1 / SM-C3: Counter-metric product tones absent

**PRD says:** Do not optimize **feature breadth over MT4 reliability** (SM-C1). **Partial enumeration** without stable SysEx + Auto-Start + docs is **not success** (SM-C3).

**Spine says:** Reliability implied by AD-9/AD-10/AD-16; **counter-metrics not stated**. Spine “fail closed on missing VirtualMIDI” (consistency) partially overlaps SM-C3 but not the full bar.

**Gap type:** Product tone / acceptance guardrail  
**Risk:** Scope creep stories (cousin profiles, polish) prioritized over MT4 SysEx/stability without spine pushback language.

---

### G-11 — FR-1 consequence: Device Manager verification string

**PRD says:** After install, Device Manager shows MT4 associated with WinUSB **per install specification**; exact **Device Manager node/class string named in Architecture / install docs** (FR-1 testable consequence).

**Spine says:** AD-12 specifies INF, GUID, WinUSB association — **does not name the expected Device Manager display string** for acceptance testing.

**Gap type:** Testable install consequence (documentation detail)  
**Risk:** Install QA ambiguous on Win10/Win11 node naming.

---

## Explicitly Excluded (matched deferrals — not gaps)

| Item | PRD | Spine Deferred |
| --- | --- | --- |
| VirtualMIDI redistribution / Tobias clearance | OQ-1, FR-12 release gate | Deferred table |
| Final latency/jitter after harness | OQ-2, NFR-P1/P2 Studio-Done | Deferred table |
| Authenticode personal vs org cert | OQ-3, FR-15 | Deferred table |
| Exact multi-MT4 port spelling | OQ-6 | Resolved in AD-5 (not a gap) |
| VirtualMIDI multi-client confirmation | OQ-7 | Resolved in AD-8 (not a gap) |

---

## Tones / Vision (informational — low spine obligation)

These PRD elements are real product intent but typically live in PRD/UX/release materials rather than build substrate. Listed for completeness; **not counted as blocking gaps** for spine pass/fail:

- §1 “serious open-source hardware-support project, not throwaway MVP”
- §2 emotional/social JTBD (trust, community under Ten Square Software)
- Addendum community evidence URLs
- Prodikeys64 as pattern inspiration (addendum orientation, not requirement)

---

## Recommended Spine Touchpoints (orientation only — not new decisions)

If closing gaps in a spine revision, prefer **binding references** over duplicating PRD prose:

1. **AD-1 or new AD-17:** Explicit carry rule for MIDI clock, Start/Stop/Continue, MTC — binds FR-7, SM-1, §10 DAW row.
2. **AD-10 or AD-16 companion line:** Session longevity expectation (~4h + SysEx) — binds NFR-R1, SM-3, §10 stability sample.
3. **AD-16 extension:** SysEx path buffering/backpressure minimum for NFR-R3 / bank stress vectors.
4. **Structural seed or AD-12/AD-13:** `docs/user/` (or equivalent) scope checklist mirroring FR-13.
5. **New short section or table:** “Validation Matrix (PRD §10)” — hosts, OS, pass-rule pointers; no re-locking hosts in spine.
6. **AD-11 one line:** Ceiling breach → explicit product decision before ship (NFR-P1).
7. **Consistency conventions:** Ten Square Software facade; Matrix-Control validation-only, not runtime dep.

---

## Sign-off

| Criterion | Result |
| --- | --- |
| Structural FR/NFR substrate | Strong |
| Quiet acceptance/guardrail gaps | **11 noted** (G-1–G-11); **5 blocking-ish** for epic/test authors (G-1, G-2, G-3, G-4, G-5) |
| Deferred items aligned with PRD | Yes |
| New product decisions introduced in this review | None |

**Final verdict:** **gaps** — proceed to epics with PRD §10 / FR-7 / NFR-R1/R3 / FR-13 as explicit spine follow-ups, or accept gaps as PRD-only until spine patch.
