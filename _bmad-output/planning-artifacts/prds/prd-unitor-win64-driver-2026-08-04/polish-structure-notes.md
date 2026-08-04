# Structural polish notes — PRD Finalize (2026-08-04)

Content-sacrosanct structural pass on `prd.md` and `addendum.md`. No product decisions, IDs, SysEx vectors, MTC scope, latency numbers, OQ table, or VirtualMIDI SDK model changed.

## Changes applied

### `prd.md`

1. **§0 Document Purpose** — Replaced inline enumeration of locked decisions with a single pointer to §11 (one source of truth).
2. **§3 Glossary (VirtualMIDI)** — Kept SDK programmatic-port model; moved licensing/eval/redistribution detail to `addendum.md` + OQ-1 cross-reference.
3. **FR-12 consequences** — Removed bullet that duplicated §1 Vision and SM-C3 (“not a throwaway MVP”).
4. **§6.2 Out of Scope for MVP** — Replaced six near-duplicate bullets with cross-reference to §5 Non-Goals plus cousin-profile nuance.

### `addendum.md`

5. **Architecture orientation** — Merged redundant multi-instance and port-naming bullets into one cross-reference to `prd.md` FR-5/FR-10/§11.
6. **VirtualMIDI licensing** — Removed duplicate contact-status block; points to `prd.md` OQ-1 for owner/status.
7. **Latency / Installer / Validation Matrix sections** — Removed second copies of provisional timing anchors, installer UX prose, and host/OS table; retained addendum-unique context (USB frame background, harness pointer) with normative pointers to `prd.md`.
8. **Matrix-Control SysEx extract** — Added explicit normative-vs-extract boundary note at section top; vector table and sizes preserved.

## Intentional non-changes

- All FR/SM/OQ/NFR IDs and acceptance consequences unchanged.
- §10 SysEx pass vectors (all seven items), MTC in FR-7/SM-1, and NFR-P1/P2 latency/jitter numbers untouched.
- §12 Open Questions triage table left intact (canonical OQ home).
- §5 Non-Goals, §6.1 In Scope, §11 Constraints — kept as separate spine layers (negative scope, positive MVP checklist, locked decisions).
- User journeys, Success Metrics, Risks table, Traceability, Assumptions Index — no cuts (cross-layer traceability, not identical redundancy).
- Rejected-alternatives table, hardware masks, community links, Windows MIDI Services note — preserved as addendum-only depth.
