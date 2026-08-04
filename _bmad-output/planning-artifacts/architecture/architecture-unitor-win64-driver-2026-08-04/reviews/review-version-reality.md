# Architecture Spine — Version & Reality Review

**Document reviewed:** `ARCHITECTURE-SPINE.md` (unitor-win64-driver V1, draft 2026-08-04)  
**Review date:** 2026-08-04  
**Lens:** Verify every committed technology claim against web sources, the existing repo, or current starter defaults — flag unconfirmed or stale assertions.

---

## Verdict

**CONDITIONAL PASS** — The product-locked core stack (C++17 usermode, WinUSB custom INF, VirtualMIDI SDK V1, ports-and-adapters pipeline) remains architecturally sound and aligns with project docs. Several **acceptance-test host names**, **Win11 + VirtualMIDI operational assumptions**, and **CI runner defaults** are stale or underspecified for August 2026 and should be refreshed before implementation stories treat them as facts. No blocker invalidates the spine; three items are **medium risk** for V1 validation on Win11.

---

## Method

| Source | Use |
| --- | --- |
| Web (Aug 2026) | DAW versions, Windows MIDI Services rollout, GitHub Actions images, vendor pages, Linux kernel quirks |
| Repo | PRD/brief addendum, `conventions.md`, `docs/dev/prompt-demarrage-projet-bmad.md`, absence of CMake scaffold |
| Author pre-check (accepted) | VirtualMIDI 8-client limit, WinUSB via custom INF, C++17/WinUSB/VirtualMIDI product lock |

---

## Verified — no action required

| Claim (spine) | Check | Result |
| --- | --- | --- |
| AD-1 / Stack: C++17 usermode, no custom kernel MIDI | Product lock + conventions | Consistent across repo |
| AD-12: WinUSB via custom INF for `USB\VID_086A&PID_0003`; OS descriptors not assumed | Microsoft Learn WinUSB installation docs (still current); project brief | Valid pattern; INF + `DeviceInterfaceGUIDs` AddReg remains documented |
| AD-3 MT4 profile: `086A:0003`, `in_cables=0x8003`, `out_cables=0x800f`, `ifnum=2` | Linux `quirks-table.h` (`USB_DEVICE(0x086a, 0x0003)`, `QUIRK_DATA_MIDI_EMAGIC(2)`) | **Exact match** in upstream kernel |
| AD-14: `QUIRK_MIDI_EMAGIC` in `sound/usb/midi.c` + `quirks-table.h` | torvalds/linux master | Still present; Emagic F5 port-switch protocol documented |
| AD-7: Do not fork `aaron1a12/virtual-midi` (GPL) | github.com/aaron1a12/virtual-midi | Exists; **GPL-3.0**; uses VirtualMIDI SDK — spine correct |
| AD-8: ShowMIDI as concurrent monitor | github.com/gbevin/ShowMIDI | Active; Windows builds available |
| AD-8: Ableton Live 12 | ableton.com release notes | Current line (12.4.x, July 2026) — valid acceptance host |
| AD-12 dev fallback: Zadig | zadig.akeo.ie | **v2.9** (2024-06-13); WinUSB install still supported |
| AD-8: VirtualMIDI multi-client up to 8 | Author docs (pre-checked) | Accepted per review brief |
| Deferred: Windows MIDI Services post-V1, Win11-only | Microsoft GA blog Feb 2026; known-issues post Apr 2026 | **Confirmed Win11-only** (24H2/25H2/26H1 retail); not on Win10 — spine deferral still correct |
| AD-11: `QueryPerformanceCounter` for harness timing | Windows API | Still standard high-resolution clock |
| Build output `builds/` | conventions + project rules | Repo convention; no conflicting `CMakeLists.txt` yet |

---

## Findings (by severity)

### F1 — MEDIUM: AD-8 names **Reason Studios 12** for V1 multi-client acceptance

**Spine:** AD-8 requires Ableton Live 12 or **Reason Studios 12** concurrently with ShowMIDI.

**Reality (web, May 2026):** Reason **14** is GA worldwide (Reason Studios press, 2026-05-11). Reason 12 remains installable but is **three major versions behind** the current perpetual/subscription line.

**Risk:** Acceptance matrix anchored to a legacy DAW version may miss Win11 MIDI stack quirks visible only in current hosts; reviewers may install Reason 14 by default.

**Recommendation:** Update acceptance wording to **Reason 14** (or “current Reason perpetual / Reason+”) while keeping ShowMIDI + Ableton Live 12. Note Reason 14 supports Win10+ per vendor specs.

**Evidence:** [Reason 14 press release](https://www.reasonstudios.com/press/reason-14-available-worldwide), [Sound on Sound](https://www.soundonsound.com/news/reason-14-now-available)

---

### F2 — MEDIUM: VirtualMIDI / loopMIDI **Win11 support not documented**; WMS rollout adds runtime quirks

**Spine:** AD-7, AD-8, AD-10 assume VirtualMIDI SDK + loopMIDI/rtpMIDI eval path on **Windows 10 + 11** validation matrix. Tobias Erichsen marketing still says **“Windows 7 up to Windows 10”** only ([virtualMIDI page](https://www.tobias-erichsen.de/software/virtualmidi.html), fetched 2026-08-04).

**Reality (web, Jan–Aug 2026):**

- Windows **MIDI Services** GA rollout (Feb 2026) broke dynamic virtual ports (loopMIDI, teVirtualMIDI/VirtualMIDI, rtpMIDI) unless created before service start — Microsoft issue [#835](https://github.com/microsoft/MIDI/issues/835), closed fixed-awaiting-public-release.
- Fix rolling via **KB5083631** / Apr 2026 CFR; may require **Windows MIDI Service restart** after Bridge creates ports dynamically ([known issues blog](https://devblogs.microsoft.com/windows-music-dev/windows-midi-services-rollout-known-issues-and-workarounds/), updated 2026-04-30).
- Bridge **creates/destroys ports at session lifecycle** (AD-9) — exactly the dynamic pattern affected.

**Risk:** V1 hot-plug and multi-client acceptance on **Win11** may fail intermittently without documented service-restart or ordering workarounds. Spine does not mention WMS coexistence.

**Recommendation:** Add an **assumption or open question** for Win11: validate VirtualMIDI SDK against WMS-enabled 24H2+; document service-restart / port-creation ordering in AD-10 or contributor docs; track Microsoft transport fix status at Studio-Done Gate.

**Not a product-lock violation** — VirtualMIDI V1 remains correct; this is **operational validation debt**.

---

### F3 — LOW–MEDIUM: AD-13 `[ASSUMPTION: GitHub Actions windows-latest]` underspecified for Aug 2026

**Spine:** Assumes `windows-latest` (or equivalent) without pinning toolchain.

**Reality (web, June–Aug 2026):** `windows-latest` migrated to **Windows Server 2025 + Visual Studio 2026** (rollout completed ~2026-06-15). Preinstalled **CMake 4.4.x** on VS2026 images ([runner release 20260803](https://github.com/actions/runner-images/releases/tag/win25-vs2026/20260803.193)).

**Risk:** First scaffold may hit VS2026/CMake policy surprises if developers assume VS2022 locally. O3DE and others note **VS2026 project generation needs CMake ≥ 4.1**.

**Recommendation:** When CI lands, either pin `runs-on: windows-2022` explicitly for stability, or document VS2026 + CMake 4.x as the tested matrix. Revisit Stack table `[ASSUMPTION: CMake 3.20+]` — still valid minimum but may need **4.1+** if using VS2026 generator.

**Evidence:** [actions/runner-images#14017](https://github.com/actions/runner-images/issues/14017)

---

### F4 — LOW: Stack `CMake 3.20+` — conservative, not starter-verified

**Spine:** `[ASSUMPTION: CMake 3.20+]` — no repo scaffold exists (`CMakeLists.txt` absent).

**Reality:** 3.20 remains a safe floor (Modern CMake guidance). Current GitHub Windows runners ship **CMake 4.4.x**. No greenfield starter was applied — assumption is **plausible but unverified against an actual template**.

**Recommendation:** Set `cmake_minimum_required` when scaffolding; test against both local macOS edit loop and pinned Windows CI image. Consider `3.20...4.4` range syntax per CMake 4.x policy docs.

---

### F5 — LOW: Deferred Windows MIDI Services context evolved (informational)

**Spine:** WMS backend correctly deferred post-V1, Win11-only (AD-2, Deferred table).

**Reality (Feb–Apr 2026):** WMS now **GA** with built-in virtual loopback endpoints and multi-client MIDI 1.0/2.0 ([Windows Experience Blog](https://blogs.windows.com/windowsexperience/2026/02/17/making-music-with-midi-just-got-a-real-boost-in-windows-11/)). Does **not** invalidate VirtualMIDI V1 choice; strengthens long-term adapter story.

**Recommendation:** Optional one-line note in Deferred: “WMS GA 2026 confirms Win11-only second adapter; built-in loopback is not a V1 substitute for Emagic-specific Bridge ports.”

---

## Explicitly not flagged (confirmed or pre-checked)

| Item | Reason |
| --- | --- |
| WinUSB + custom INF primary path | Pre-checked + Microsoft Learn |
| VirtualMIDI 8 clients per port | Pre-checked (Tobias Erichsen docs) |
| C++17 / PascalCase / `builds/` | Repo conventions |
| DeviceInterfaceGUID `{aa209017-cf8a-49ad-a0e7-701187ff7e05}` | Project-specific; no external stale claim |
| Matrix-Control SysEx vectors (AD-16) | Product/PRD lock; Oberheim Matrix still valid validation target — not version-sensitive |
| WiX vs Inno installer choice | Correctly left open under AD-12 checklist |
| MT4 USB serial assumption (AD-6) | Already marked `[ASSUMPTION]` — hardware gate AQ-1 |

---

## Summary table

| ID | Area | Severity | Stale / unverified? | Action |
| --- | --- | --- | --- | --- |
| F1 | AD-8 Reason Studios 12 | Medium | **Stale** (Reason 14 GA) | Update acceptance hosts |
| F2 | VirtualMIDI on Win11 + WMS | Medium | **Unverified ops path** | Win11 soak + WMS workaround docs |
| F3 | AD-13 windows-latest | Low–Medium | **Underspecified** | Pin runner / document VS2026 |
| F4 | CMake 3.20+ assumption | Low | **Unverified** (no scaffold) | Confirm at first CMake commit |
| F5 | WMS deferred note | Low | Context evolved | Optional deferred blurb |

---

## Recommended spine edits (minimal)

1. **AD-8:** Replace “Reason Studios 12” → “Reason 14 (or current Reason perpetual / Reason+)”.
2. **AD-7 or AD-10:** Add Win11 note: VirtualMIDI vendor pages list Win10; validate on WMS-enabled Win11 24H2+; document MIDI service restart if dynamic ports fail to enumerate (link Microsoft known-issues).
3. **AD-13 / Stack:** When CI scaffold lands, pin runner label and record tested VS/CMake versions.
4. **AQ-4 (new, optional):** Win11 + VirtualMIDI + WMS dynamic port enumeration — revisit at first Win11 dual-DAW acceptance run.

---

## Reviewer sign-off

Core architecture decisions are **not** undermined by August 2026 technology drift. The spine should pass implementation readiness **after** refreshing host-app names and adding Win11 VirtualMIDI operational validation — not a paradigm change.
