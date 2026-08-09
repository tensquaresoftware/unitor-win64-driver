# Epic 2 Context: Studio Transport and SysEx

<!-- Compiled from planning artifacts. Edit freely. Regenerate with compile-epic-context if planning docs change. -->

## Goal

After this epic, Validation Matrix DAWs can use MIDI clock, Start/Stop/Continue, and MTC through the Bridge, and Matrix-Control can complete the minimum SysEx pass vectors without a Bridge restart for normal librarian use — with buffering and session design aimed at about four hours of continuous studio/editor work. This epic deepens Epic 1’s notes/CC path into full studio transport and first-class SysEx, completing the MIDI-depth and SysEx editor journeys for V1.

## Stories

- Story 2.1: MIDI clock and transport realtime
- Story 2.2: MTC quarter-frame and full-frame
- Story 2.3: Transparent SysEx transport with burst buffering
- Story 2.4: Matrix-Control minimum SysEx pass vectors
- Story 2.5: Session longevity design for ~4h studio use

## Requirements & Constraints

- V1 transport must carry channel MIDI plus MIDI clock (`0xF8`), Start / Stop / Continue, and MTC (quarter-frame and full-frame used for sync). Dropping any of these classes is a V1 defect, not optional polish.
- Under normal short sequencing smoke in Ableton Live 12 or Reason Studios 12, the Bridge must not induce dropouts of clock, transport realtime, or MTC on MT4 Virtual Ports.
- SysEx is a required V1 capability sized for real editor/librarian traffic. The Bridge carries SysEx (including Oberheim Matrix / Matrix-Control frames) transparently between Virtual Ports and MT4 cables — no Emagic-side framing or rewriting of Oberheim payloads.
- Matrix-Control is a first-party validation target only; it must not be linked or bundled as a Bridge runtime dependency.
- Minimum SysEx pass vectors (Win10 x64 mandatory; Win11 x64 also) must succeed without Bridge restart for normal librarian completion: Device Inquiry round-trip; single patch dump (~275 B response); master dump (~351 B response); edit-buffer / patch push (outbound ~275 B; slot `01` and/or edit-buffer `0D`); live editor stream (short 7 B / 9 B remote edits at normal Matrix-Control spacing); mixed-wire tolerance (non-patch SysEx during a dump must not permanently block a later valid patch frame). Optional bank stress (~100× 275 B) may be recorded when hardware/time allow but is not a hard gate.
- SysEx bursts must be buffered/queued so librarian-scale dumps complete; incomplete or corrupt dumps under normal test conditions are failures.
- Design and document continuous studio/editor use for about **4 hours** (including SysEx Sessions) without mandatory Bridge restart for normal operation. A stability sample plan is required for at least Win10 x64; leak/restart failure modes found in samples are defects, not accepted as “usermode limits.”
- This epic does not invent final latency/jitter thresholds (those remain Epic 5 / Studio-Done Gate).
- Diagnostics for transport/MTC failures should identify the Virtual Port / cable in English.
- Validation hosts for this epic: Ableton Live 12, Reason Studios 12, Matrix-Control on Win10 x64 and Win11 x64 (Win10 mandatory in matrix).

## Technical Decisions

- Emagic cable multiplex/demultiplex stays in the C++17 usermode Bridge; no custom kernel MIDI driver. Channel / clock / MTC and SysEx paths sit on the existing `WinUsbTransport` ↔ `EmagicCableMapper` ↔ `MidiBackend` (VirtualMIDI) spine.
- Transparent SysEx: carry frames as-is; do not interpret or reframe Oberheim Matrix payloads on the Emagic side.
- Session reliability: buffer/queue SysEx bursts for Matrix-Control librarian vectors; design for ~4h continuous use without mandatory restart.
- English diagnostic logging must be enough to diagnose SysEx burst failures (and session lifecycle context shared with the wider Bridge).
- Dual-machine loop remains: primary edit may be macOS; USB / DAW / SysEx validation runs on Windows x64.

## Cross-Story Dependencies

- Depends on Epic 1: working notes/CC path on at least one IN and one OUT Virtual Port and a live DeviceSession with Virtual Ports before clock/MTC/SysEx work lands.
- Within the epic: 2.1 → 2.2 (MTC builds on clock/transport path); 2.3 → 2.4 (pass vectors need transparent buffered SysEx); 2.5 assumes channel MIDI, clock/MTC, and SysEx paths from 2.1–2.4.
- Downstream: Epic 3 (resilience / Auto-Start / hot-plug) and Epic 4 (community install / first SysEx docs) build on this epic’s transport and SysEx depth; Epic 5 owns final MIDI Path timing proof — do not conflate ~4h stability with latency thresholds.
