# Prose polish notes — PRD Finalize step 2 (2026-08-04)

Content-sacrosanct editorial pass on `prd.md` and `addendum.md`. No product decisions, IDs, SysEx vectors, MTC scope, latency numbers, OQ table, or VirtualMIDI SDK model changed. No restructuring.

## Fixes applied

| Original Text | Revised Text | Changes |
|---------------|--------------|---------|
| `over the same bridge community users will run` (`prd.md` §2.1) | `over the same bridge that community users will run` | Added relative pronoun; clarifies which bridge |
| `end users are not expected to manage ports only through the VirtualMIDI end-user UI` (`prd.md` §3 Glossary) | `end users need not manage Virtual Ports solely through the VirtualMIDI end-user UI` | Resolved ambiguous `not … only through` parsing; aligns SDK-first model with addendum |
| `as designed by VirtualMIDI multi-client behavior` (`prd.md` FR-9) | `per VirtualMIDI multi-client semantics` | Removed awkward agent (`behavior` does not design); matches FR-9 assumption wording |
| `Realizes secondary-user promise.` / `Realizes secondary-user path.` (`prd.md` FR-16) | `Realizes the §2.1b secondary-audience promise.` / `… path.` | Linked vague antecedent to §2.1b heading |
| `when hardware available` (`prd.md` SM-8) | `when hardware is available` | Added missing copula |
| `when hardware/time available` (`prd.md` §10 pass rule 6) | `when hardware and time are available` | Completed conditional clause |
| `commercial licence` (`addendum.md` VirtualMIDI licensing) | `commercial license` | Spelling consistency with adjacent `Licensees` / project US-English usage |
| `This section is grounded extract detail` (`addendum.md` Matrix-Control) | `This section provides grounded extract detail` | Fixed incomplete sentence (missing verb) |

## Intentional non-changes

- User-journey telegraphic style (`→` paths, “Realizes UJ-N”) — deliberate BMad traceability pattern.
- `[ASSUMPTION]` blocks, FR/SM/OQ/NFR IDs, SysEx hex, byte sizes, latency anchors — untouched.
- Tables (hardware masks, risks, validation matrix, rejected alternatives) — no cell edits.
- `SysEx-as-required` shorthand in Assumptions Index — domain term; clear in PRD context.
- British possessive `Windows'` — acceptable; not a comprehension blocker.

## Queries (no change applied)

- Consider: `Guillaume may refine sizes/timeouts without reopening SysEx-as-required` → explicit “without reopening the SysEx requirement in V1”? Left as-is to preserve locked shorthand.
