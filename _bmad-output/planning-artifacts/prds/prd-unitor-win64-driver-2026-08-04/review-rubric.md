# PRD Quality Review — unitor-win64-driver

## Overall verdict

This PRD is decision-ready for Architecture and epic work on a community OSS hardware bridge. Locked decisions are stated plainly, trade-offs and rejections are honest, and scope omissions are explicit rather than inferred. The main risk is downstream acceptance friction: a few FRs still lean on subjective language or deferred validation detail (installer UX bar, SysEx pass vectors, MIDI clock message enumeration) that story authors will need to sharpen before ship — but these are acknowledged deferrals, not hidden gaps. Provisional timing anchors and the Studio-Done Gate are appropriately framed as planning gates, not false precision.

## Decision-readiness — strong

The PRD earns trust as a decision document. §11 Constraints and Guardrails lists locked product decisions with an explicit "do not reopen" instruction; §0 reinforces the same boundary. Trade-offs are named with what was given up: kernel driver rejected (§5, addendum "Rejected / deferred alternatives"), Windows MIDI Services deferred to Win11-only second backend (§3 Glossary, §11), VirtualMIDI redistribution as release gate not Architecture blocker (OQ-1, FR-12 consequences). Open Questions (§12) are genuinely open — eight items with owners, class labels (`Release gate`, `Studio-Done Gate`, `Deferred`), and next actions; none are rhetorical setups with immediate answers buried below. Counter-metrics SM-C1–SM-C4 explicitly guard against optimizing the wrong things. Risks table (§9) pairs impact with mitigation and named owners (e.g., VirtualMIDI outreach "sent; no reply yet; owner Guillaume").

The PRD does not smooth tensions to neutral: usermode latency/jitter is flagged as a studio-credibility risk with provisional ceilings and an explicit "jitter non-alibi" counter-metric (SM-C4, NFR-P2). Multi-MT4 validation honesty is built into UJ-3 resolution and FR-10 consequences ("If only one physical MT4 is available at ship time, release notes/docs state multi-instance validation status honestly").

### Findings

- **low** Authenticode deferral vs trust goals (§12 OQ-3, FR-15) — Signing path and timing are deferred without a decision trigger beyond "first public build." Acceptable per locked policy, but a decision-maker preparing community launch may want a firmer "when unsigned is OK vs not" checkpoint before marketing. *Fix:* Add a one-line decision trigger in OQ-3 (e.g., "revisit N weeks before tagged public release").

## Substance over theater — strong

Content is earned and product-specific. Vision (§1) names orphaned Emagic hardware, proprietary cable mapping, and Matrix-Control validation — not a generic "modernize legacy device" paragraph. Differentiation explicitly rejects invented moats: "honesty and operability for orphaned hardware… not invented feature moats." Four UJs (§2.3) each drive a distinct acceptance concern (install, SysEx, multi-MT4, hot-plug) with named protagonists and edge cases; no persona sprawl. NFRs carry product thresholds (~4 h stability NFR-R1, p99 latency/jitter bands NFR-P1/P2, SysEx buffering NFR-R3) rather than boilerplate scalability/security copy. Secondary audience (§2.1b) serves FR-16 extensibility without promising cousin devices — functional, not decorative.

### Findings

- **low** Installer UX aspiration (FR-12, addendum "Installer UX bar") — Phrases like "user-friendly," "polished macOS installer," and "minimal jargon" are product intent furniture, partially bounded by Assumptions Index entry for "macOS-class installer." Not theater given downstream Architecture ownership, but the subjective layer remains. *Fix:* No PRD change required pre-Architecture; UX/Architecture should translate to a short checklist (step count, prerequisite block behavior, success screen) when shaping stories.

## Strategic coherence — strong

The PRD has a clear thesis: restore **usable, trustworthy MIDI** (performance + SysEx) for forgotten Emagic MT4 hardware on modern Windows through a serious OSS community project — not a ports-sometimes-appear MVP. Features follow that arc: WinUSB bind → Bridge session → Virtual Port topology/naming → MIDI coverage (channel, clock, SysEx) → multi-client/multi-instance → hot-plug → community packaging/docs/licensing → extensibility without scope creep. Success Metrics validate the thesis (SM-1–SM-6 operability, stability, honesty) with counter-metrics that block feature-breadth or ASIO-buffer theater (SM-C1–SM-C4). MVP scope (§6) is problem-solving kind: MT4-validated, SysEx-required, studio stability, community release honesty. Non-Goals (§5) and §6.2 reinforce the same arc by cutting Patch/LTC, cascade topology, kernel driver, and cousin-device guarantees.

No finding that the document reads as a backlog with section headings — FR groupings map to user journeys and SMs.

## Done-ness clarity — adequate

This is the PRD's strongest structural habit and its main residual gap. All sixteen FRs include **Consequences (testable)** blocks — excellent downstream fuel. Many consequences are concrete: FR-4's "2 IN + 4 OUT endpoints per MT4," FR-11's "Requiring a Windows reboot… is a V1 failure," NFR-P1/P2's p99 bands with do-not-ship-worse ceiling. Validation Matrix pass rules (§10) further operationalize SM-1–SM-4.

However, several acceptance hooks remain adjective-driven or deferred by design, which story creation must not treat as done:

### Findings

- **high** SysEx pass criteria undefined (FR-8, SM-2, NFR-R3, §10, OQ-8) — FR-8 requires Matrix-Control to complete "representative dump/restore or editor exchanges"; NFR-R3 says "normal test vectors"; §10 Matrix-Control row says "representative SysEx exchange"; OQ-8 defers "Representative Matrix-Control SysEx test vectors defining 'pass'" to Guillaume. Until vectors exist, SM-2 is not independently verifiable — two reviewers could disagree on pass/fail. *Fix:* Before epic/story breakdown for SysEx, specify minimum vectors (e.g., single large dump size, burst count, corrupt/retry behavior) in OQ-8 resolution or a validation appendix; reference from §10 pass rules.

- **medium** MIDI clock message scope unspecified (FR-7) — FR-7 says "MIDI clock/transport-related timing messages **required for sequencing use**" without enumerating Start/Stop/Continue, Song Position Pointer, MTC, etc. Consequence ties to "without Bridge-induced dropouts under the session-stability scenario" but not to message coverage. *Fix:* Architecture or a one-line FR-7 amendment listing in-scope system realtime messages; defer MTC explicitly if out.

- **medium** Installer acceptance remains subjective (FR-12) — Consequences cover admin elevation, prerequisite messaging, and clearance gate, but "feels closer to a polished macOS installer" and "Packaging and docs quality match a serious hardware-support project" are not falsifiable without a checklist. Assumptions Index captures intent ("few steps, clear progress…") but not thresholds. *Fix:* Derive 3–5 installer acceptance checks in UX/Architecture (blocked install path, success state, prerequisite failure UX) and link from FR-12 stories.

- **low** WinUSB bind verification slightly circular (FR-1) — "Device Manager shows the MT4 bound **as intended by the install docs**" delegates truth to docs not yet written. Acceptable at PRD stage. *Fix:* Architecture/install spec should name the expected Device Manager node/class string FR-1 testers will look for.

Provisional latency (NFR-P1/P2) and Studio-Done Gate are **not** flagged — numbers are explicitly provisional with measurement gate and Assumptions Index roundtrip; aligned with calibration.

## Scope honesty — strong

Omissions are explicit and repeated: §5 Non-Goals, §2.2 Non-Users, §6.2 Out of Scope, FR-10 "Out of Scope" callout for cascade topology, addendum rejected-alternatives table. `[ASSUMPTION]` tags mark inferences (FR-5 IN/OUT endpoints, FR-9 VirtualMIDI multi-client, NFR-P1/P2 planning anchors). `[NOTE FOR PM]` pattern appears via OQ class labels and FR-12 release-gate wording rather than inline PM notes — functionally equivalent. De-scoping is honest: UJ-3 and FR-10 admit single-MT4 hardware at ship; VirtualMIDI clearance blocks redistributable installer only (OQ-1, FR-12); Authenticode not a hard gate (FR-15). Open-items density (8 OQs, 6 assumptions) is high but proportionate to a public OSS launch with proprietary SDK dependency and hardware validation constraints — not incompleteness theater given owners and deferral classes.

No findings — intentional deferrals are labeled, not smuggled as completeness.

## Downstream usability — strong

The PRD is extractable for UX, Architecture, and story workflows. Glossary (§3) defines Bridge, Virtual Port, DeviceProfile, Studio-Done Gate, Validation Matrix, and other domain nouns used consistently in FRs and UJs. ID schemes are contiguous and unique: FR-1–FR-16, UJ-1–UJ-4, SM-1–SM-9 plus SM-C1–SM-C4, OQ-1–OQ-8, NFR-* families. Cross-references resolve: FRs cite UJs ("Realizes UJ-1"), SMs cite FRs ("Validates FR-4, FR-6, FR-7"), §14 traces to brief. Each UJ has a named protagonist (Alex, Sam/Guillaume, Jordan, Riley) carrying context inline. Addendum cleanly separates architecture orientation, hardware tables, and rejected alternatives without restating requirements — appropriate chain-top shape for a PRD feeding Architecture → epics.

### Findings

- **low** FR → NFR trace incomplete for some reliability items — NFR-R3 (SysEx buffering) is referenced in SM-2/SM-3 implicitly but not tagged on FR-8 consequences; story authors may miss the link. *Fix:* Add "See NFR-R3" to FR-8 consequences or SM-2 definition line.

## Shape fit — strong

Product shape is a community-facing technical capability with meaningful installer/docs UX — not a pure internal tool, not a consumer app with heavy UI. Four UJs with protagonists are load-bearing (install, SysEx, multi-MT4, hot-plug) without over-formalizing a solo-dev workflow: community contributors and Validation Matrix hosts justify the journey density. SMs mix user-facing operability (SM-1–SM-5) with operational/release honesty (SM-6, SM-9 Studio-Done Gate) appropriately. Brownfield references (Linux quirks in addendum, no GPL vendoring) are accurate pointers, not false existing-code claims. §0 and addendum header correctly partition PRD (WHAT/acceptance) from Architecture (HOW) — neither over- nor under-formalized for Ten Square Software public OSS launch stakes.

No findings.

## Mechanical notes

**Assumptions Index roundtrip — partial gap.** Three Assumptions Index entries (§13) lack inline `[ASSUMPTION: …]` tags in the body:
- "macOS-class installer" (installer intent only in FR-12 prose and addendum)
- "Reason Studios 12" SKU string (no inline tag)
- MIDI-OX availability/substitute (no inline tag)

Conversely, all inline `[ASSUMPTION]` tags in FR-5, FR-9, NFR-P1, NFR-P2 appear in the index. *Fix:* Add inline tags at first use or drop index-only entries into a "Document assumptions" subsection — prefer inline tags for extractor tooling.

**Glossary drift — none material.** "Virtual Port" vs "Virtual Ports," "Port Name" vs "Port Names" are consistent pluralization, not synonym drift. "Reason Studios 12" vs "Reason 12 DAW product line" is flagged in Assumptions Index.

**ID continuity — clean.** No gaps or duplicates in FR, UJ, SM, OQ sequences. NFR prefixes (P/R/D/S/Q) are stable.

**Cross-references — resolve.** "see §7 / §10" (§6.1), "see NFR-R1" (FR-7), Studio-Done Gate defined in Glossary and used in §7/§8/§12 — all valid.

**UJ protagonists — complete.** UJ-1 Alex, UJ-2 Sam (or Guillaume validating), UJ-3 Jordan, UJ-4 Riley — each named with entry/climax/resolution/edge case structure.

**Required sections for stakes — present.** Vision, users, glossary, features/FRs, non-goals, MVP scope, success metrics with counter-metrics, NFRs, risks, validation matrix, constraints, open questions, assumptions index, brief traceability; addendum for technical depth. Appropriate for public OSS community launch feeding Architecture and epics.
