# Epic 6 Context: Community MIDI backend on Windows MIDI Services (Win11-only)

<!-- Compiled from planning artifacts. Edit freely. Regenerate with compile-epic-context if planning docs change. -->

## Goal

After this epic, community users on Windows 11 get a Bridge that creates and destroys virtual MIDI ports through Windows MIDI Services, so the project can publish ready-to-run MIT Bridge/Setup binaries on GitHub without depending on proprietary virtualMIDI SDK redistribution clearance. Windows 10 drops as a community claim (Win10 + virtualMIDI remain valid only as interim lab / personal history through Epic 5). This epic is the vehicle for honest public binaries under the hobby posture.

## Stories

- Story 6.1: Windows MIDI Services MidiBackend (Win11)
- Story 6.2: Community binary + hobby install honesty on Win11
- Story 6.3: Win11 Validation Matrix soak for WMS path

## Requirements & Constraints

- Community redistributable path is Win11 + Windows MIDI Services only. Do not ship community Releases of Bridge/Setup binaries that link the virtualMIDI SDK; virtualMIDI MSI embed stays out of scope.
- Keep three-way license honesty: MIT covers this repo’s Bridge/Setup sources and docs; virtualMIDI is proprietary and separate (lab/personal only until cutover); Windows MIDI Services is the intended community backend — not already shipping as that path, and not a Win10 community claim.
- No paid Authenticode / no certificate purchase in this hobby line. Unsigned public builds must document SmartScreen (“Run anyway”); do not invent a production catalog without a shipped certificate.
- Hobby install contract for clean PCs: guided WinUSB association (Zadig or documented equivalent) is the primary path without a trusted catalog; materials must not promise Setup-alone WinUSB success.
- Community Validation Matrix claims after cutover are Win11-only: notes/CC plus Matrix-Control minimum SysEx must pass (or Fail documented honestly). Multi-client expectations must be documented for the WMS path (replace VirtualMIDI-only client ceilings where they no longer apply).
- Out of scope here: custom kernel MIDI driver; implementing Epic 5 measurement work; Magazines / polished commercial marketing.

## Technical Decisions

- Realize the `MidiBackend` abstraction so a Windows MIDI Services backend can land on Win11 without rewriting the Emagic cable-mapping core.
- New WMS implementation must follow the same module boundary: cable mapper and device profile stay free of WMS and WinUSB headers; only the backend talks to the MIDI stack.
- Port display names stay session-owned: unit ordinal `K` is assigned by the session manager and passed into the backend as ready-made names (`MT4 Port N` / `MT4 #K Port N`); backends must not invent or re-derive `K`. Topology remains 2 IN + 4 OUT per unit.
- Only a live device session may create or destroy that unit’s virtual port set, exclusively through `MidiBackend` APIs (create on session start; destroy on teardown / unplug; replug = new session).
- Sequencing: Epic 5 first (measurement method on interim virtualMIDI + Win10 lab); Epic 6 after Epic 5 (or at least published measurement method) and when a Win11 lab machine is available. Do not block Epic 5 on WMS.
- Protocol / Epics 2–3 MIDI depth stays out of scope unless WMS port lifecycle breaks it.

## UX & Interaction Patterns

- Community-facing install docs match the hobby contract: SmartScreen honesty if unsigned; guided WinUSB for clean-PC association; Auto-Start remains a user-session Bridge (not a Session-0 service).
- Fail closed with obvious messaging when WinUSB bind or MIDI backend prerequisites are missing — never present empty port lists as success.

## Cross-Story Dependencies

- Depends on Epic 5 complete (or measurement method published) and a Win11 development/validation PC.
- Within the epic: 6.1 (WMS backend) unlocks 6.2 (public MIT binaries + install honesty); 6.3 soaks the community path on Validation Matrix hosts.
- Upstream: Epics 1–4 delivered Bridge spine, install packaging, and license/policy honesty; Epic 4’s community binary launch wait is resolved here.
- Downstream: after Epic 6, ready-to-run community binaries that do not depend on virtualMIDI SDK redistribution are allowed; Win10 is no longer a community support claim.
