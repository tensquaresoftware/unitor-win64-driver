---
title: "Product Brief: unitor-win64-driver"
status: ready
created: 2026-08-04
updated: 2026-08-04
authenticode_policy: strongly-recommended-v1
project: unitor-win64-driver
organization: Ten Square Software
---

# Product Brief: unitor-win64-driver

## Executive Summary

**unitor-win64-driver** is a community-facing, MIT-licensed Windows 10/11 (64-bit) usermode bridge that restores usable MIDI I/O for Emagic Unitor-family USB interfaces — starting with the **MT4** — after decades without an official 64-bit vendor driver.

Today the hardware enumerates over USB, but DAWs see no usable MIDI ports because Windows’ generic class stack does not implement Emagic’s proprietary cable mapping. This project closes that gap with a studio-grade software bridge (WinUSB transport + original Emagic protocol reimplementation + virtual MIDI ports), published under the **Ten Square Software** public facade, with documentation and packaging quality expected of a serious open-source hardware-support project — not a throwaway MVP.

V1 is deliberately narrow in *which* hardware is promised (MT4 profile only as the guaranteed, validated product) but sets a high bar for engineering quality and community credibility: **SysEx-capable** studio MIDI (not notes-only), measurable timing discipline, long-session stability, auto-start, stable macOS-like port names, multi-client use, friendly install, honest licensing, and an architecture that can run **multiple MT4 units** and later absorb AMT8 / Unitor8 without a rewrite. Differentiation is solving a long-standing hardware orphan honestly — not invented feature moats.

A first-party motivation for SysEx in V1: enabling Windows testing of **Matrix-Control** against a real MT4 — the bridge must carry editor/librarian traffic, not only performance MIDI.

## The Problem

Musicians and small studios still own Emagic **MT4**, **AMT8**, and **Unitor8** interfaces. After moving to 64-bit Windows, they lose MIDI connectivity: the last official package targeted Windows XP (32-bit). The device appears in Device Manager; no DAW ports appear. Workarounds are painful. Forum threads (Mod Wiggler, Gearspace, Cockos) show multi-year demand with no durable Windows solution.

The gap is especially painful for **SysEx-heavy** workflows (hardware editors, librarians, dump/restore): without ports, those tools cannot talk to the interface at all.

Root cause: the protocol is **not** fully USB-MIDI class-compliant. Emagic cable multiplex/demultiplex must be implemented explicitly. Linux does this via `QUIRK_MIDI_EMAGIC`; Windows does not.

## The Solution

Ship a **usermode** bridge — not a custom kernel MIDI driver:

1. Bind the device to Microsoft-signed **WinUSB**.
2. Run a C++17 service/application that speaks Emagic cable framing (original implementation informed by public Linux references — **no GPL sources vendored**).
3. Expose named virtual MIDI ports to DAWs and MIDI utilities via **VirtualMIDI** (Tobias Erichsen) on Windows 10 and 11.

End-user outcome: **install once** (admin elevation OK at install time), **plug in the MT4**, and **the bridge starts** with Windows and/or when the device arrives. **Then** open the DAW or a SysEx editor, select stable **MT4 Port N** endpoints, and work. Advanced Unitor features (Patch, LTC/VITC, Fast Mode/AMT) and **Emagic-style cascaded multi-interface stacks** stay out of V1. Multiple independent MT4 units on one PC **are** in scope.

## Locked product decisions (do not reopen)

| Topic | Decision |
| --- | --- |
| Platforms | Windows **10 and 11**, 64-bit — **Win10 required** |
| Solution type | Usermode only (WinUSB + C++ app/service) — **no custom kernel driver in V1** |
| MIDI backend (V1) | **VirtualMIDI SDK** — Windows MIDI Services = v2 / second backend (Win11-only) |
| Project license | **MIT** (original reimplementation) |
| Hardware V1 | **MT4** profile (`VID 086A` / `PID 0003`) as the validated product; architecture multi-device / multi-instance from day one |
| MIDI content V1 | Channel MIDI **and SysEx** (required); MIDI clock in V1 |
| Port naming | Stable macOS-like names (e.g. **MT4 Port 1** …); distinguishable when multiple MT4s are present |
| Code quality | `conventions.md` + `scripts/quality/lint-touched.py` gate as soon as C++ lands |
| Public facade | **Ten Square Software** |
| Authenticode | Strongly recommended for public builds; **not** a hard gate if the certificate lags (document SmartScreen) |

**Architecture orientation:** deferred to Architecture (single cable-mapping core, `DeviceProfile` per PID, multi-instance sessions, pluggable virtual-MIDI backend). Pattern inspiration: Prodikeys64 (WinUSB + virtual MIDI).

## Who This Serves

- **Primary:** Windows studio users who own an **MT4** and need reliable **2 IN / 4 OUT** ports for DAWs **and** SysEx editors/librarians (including Ten Square’s **Matrix-Control** validation on Windows).
- **Secondary:** Owners of AMT8 / Unitor8 watching for a credible path — they are **not** promised V1 support, but must not be blocked by structural debt.
- **Contributors / testers:** Community members who can validate cousin hardware post-MVP.

Success for them: install without Driver Hell, ports appear with clear stable names, MIDI (including SysEx) is stable through real sessions, the bridge is running when they open their tools, docs state plainly what works / what does not / what depends on VirtualMIDI’s separate license.

## Scope

### In for V1

- MT4 (`086A:0003`), **2 IN / 4 OUT**, channel MIDI + **MIDI clock** + **SysEx**.
- WinUSB + C++ usermode bridge + VirtualMIDI-backed ports.
- Stable macOS-like port naming; multi-client access; auto-start.
- Multi-`DeviceProfile` architecture; **multi-instance MT4** supported.
- Friendly installer + serious user/technical documentation.
- Public project identity under Ten Square Software.
- First-party SysEx validation path for **Matrix-Control**.

### Explicitly out of V1

- Patch mode, LTC/VITC, Fast Mode / AMT.
- Emagic-style **cascaded / stacked multi-interface** topologies (fragile even under Linux/ALSA) — distinct from “two independent MT4s.”
- Guaranteed AMT8 / Unitor8 / Unitor8 mk2 support without real test hardware.
- “Windows MIDI Services only” as the V1 target.
- Custom kernel driver.
- MIDI 2.0 as a V1 claim (era hardware remains MIDI 1.0 framing).

### Post-MVP (anticipated demand)

- Cousin DeviceProfiles (AMT8 / Unitor8 / Unitor8 mk2) as **hardware-validation** workstreams with explicit external hardware dependency.
- Optional second backend: Windows MIDI Services (Win11), behind the same abstraction.
- Longer term: trusted Windows bridge for the Unitor protocol family, with clear docs and predictable releases under Ten Square Software.

## Success Criteria (high bar)

### Studio operability (V1)

- **Ports:** **2 input + 4 output** virtual ports per MT4 (physical I/O), not a flood of channel endpoints.
- **Port names:** stable across launches and replugs; macOS-like **MT4 Port N**; when two MT4s are connected, names remain stable and unambiguously distinguishable per unit (exact disambiguation in Architecture/UX).
- **Message coverage:** notes, CC, common channel/system messages; **MIDI clock**; **SysEx** large enough for real editor/librarian use (including Matrix-Control). SysEx is a **V1 requirement**.
- **Multi-client:** DAW and MIDI utility concurrently without exclusive-lock dead ends.
- **Auto-start:** bridge starts with Windows and/or on MT4 USB arrival — no manual launch before every session.
- **Validation matrix:** at least one DAW + one SysEx path (**Matrix-Control**); exact hosts locked in PRD.
- **Timing:** PRD defines numeric latency/jitter thresholds and a reproducible **MIDI-path** measurement method (not ASIO buffer size). Planning anchors in Assumptions until locked. Excessive jitter is not an alibi for the usermode path.
- **Session stability:** continuous studio/editor use of about **4 hours** without requiring a bridge restart for normal use (including SysEx sessions).
- **Hot-plug:** replug restores usable ports without a Windows reboot; DAW/editor port rescan or supervised bridge restart is acceptable; a Windows reboot is a failure.
- **Multi-MT4:** two MT4 units on one PC are a supported V1 design. If only one unit is available at ship time, document validation status honestly.

### Release and community bar

- User docs: VirtualMIDI prerequisites, install (one-time admin OK), auto-start, first MIDI and SysEx test, troubleshooting, explicit works / does-not-work list.
- Technical docs for contributors without shipping GPL Linux sources.
- Honest licensing messaging: MIT for this repo; VirtualMIDI proprietary and separate; Windows MIDI Services is a future backend, not the V1 claim.
- Installer: guided WinUSB + bridge + auto-start + VirtualMIDI prerequisite handling — not Zadig as the primary end-user path; daily use without Administrator.
- C++ passes the project quality gate; architecture supports DeviceProfiles and multi-instance without rewrite.
- **VirtualMIDI author clearance** before shipping a redistributable public installer.
- Authenticode strongly recommended; unsigned first public build OK with SmartScreen documentation if the certificate lags.

## Risks and external dependencies

| Risk / dependency | Why it matters | Mitigation direction |
| --- | --- | --- |
| Usermode MIDI jitter/latency | Studio credibility | Mandatory PRD thresholds + MIDI-path measurement |
| Large / bursty SysEx | Editor workflows (Matrix-Control) fail | Explicit SysEx acceptance tests; buffering for librarian dumps |
| Emagic protocol docs scarce | Implementation risk | Use Linux `midi.c` / quirks as reference (no copy); USB captures if needed |
| VirtualMIDI redistribution | Blocks public installers | Early author contact; document terms; keep backend abstracted |
| SmartScreen / trust | Users abandon unsigned downloads | Authenticode strongly recommended; document if deferred |
| Dual-machine dev (macOS edit / Win10 validate) | CI and hardware test gaps | Explicit Win10 x64 validation loop; CI for build at minimum |
| Multi-MT4 naming / identity | DAW recalls wrong ports | Stable per-instance naming rules in Architecture |
| Cousin-device pressure post-launch | Scope creep / rewrite risk | DeviceProfile + non-goals; hardware-gated stories only |
| Hardware access | V1 and multi-instance proof | ≥1 MT4 required; second MT4 when available; cousins only when physical |
| WinUSB / Windows install | Binding and privileges | INF/installer; one-time admin install |

## Open questions

1. Exact VirtualMIDI evaluation vs redistribution / installer bundling terms after author contact (**blocker for public installer**).
2. Locked numeric latency/jitter thresholds and MIDI-path measurement harness.
3. Final V1 validation matrix (DAWs + Matrix-Control + MIDI utility).
4. Authenticode certificate path/cost (personal vs org Ten Square Software) and timing relative to first public build.
5. Availability of original Emagic protocol documentation vs reference+capture fallback only.
6. CI strategy across macOS-primary development and Windows 10 x64 build/USB/DAW/SysEx validation.
7. Exact multi-MT4 port-name disambiguation scheme (product rule locked; spelling in Architecture/UX).
8. Confirm VirtualMIDI multi-client behavior meets the concurrent DAW + utility requirement (Architecture).

## Assumptions (planning only)

- `[ASSUMPTION]` Timing anchors for PRD drafting (replace with measured targets): USB full-speed MIDI historically aligns with ~**1 ms** frame granularity; a healthy usermode bridge should aim for **low single-digit milliseconds** of *additional* end-to-end latency beyond the host USB path at p99, and **sub-millisecond to low-millisecond** jitter suitable for studio clock/sequencing.
- `[ASSUMPTION]` “macOS-class installer” means few steps, clear progress, obvious success state, and minimal jargon — tooling is an Architecture choice.
- `[ASSUMPTION]` Port name strings follow macOS-like **MT4 Port N**; IN vs OUT appear as separate selectable endpoints as Windows UI requires.

## Non-goals for this document

This brief does **not** replace the Architecture decision record or the PRD. It locks product intent, scope, bar, and already-chosen orientation so those documents can proceed without re-litigating platform or backend choice.
