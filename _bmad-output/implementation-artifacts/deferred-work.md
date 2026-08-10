# Deferred work

## Correct Course 2026-08-10 — hobby pivot (applied)

Source: `_bmad-output/planning-artifacts/sprint-change-proposal-2026-08-10.md` (approved for implementation).

| Item | New status |
|---|---|
| OQ-1 Tobias VirtualMIDI clearance / MSI embed / community VirtualMIDI-linked binaries | **Out of community scope** — community binaries after Epic 6 (WMS) |
| OQ-3 Authenticode / production INF `.cat` certificate purchase | **Out of scope hobby / no certificate purchase** |
| Production `.cat` packaging waiting on OQ-3 purchase | **Out of scope for hobby posture** (revisit only if this line later ships a certificate) |
| Clean-PC WinUSB without trusted catalog | **Supported via guided association** (Zadig or equivalent) — not Setup-alone commercial-installer promise |
| Epic sequencing | **Epic 5 next** → then Epic 6 WMS Win11 (after Win11 lab PC) |
| User docs / README / Epic 4 smokes / license / authenticode / winusb-bind hobby install rewrite | **Done 2026-08-10** (README, docs/user EN+FR + index, license-and-backends, authenticode-and-smartscreen, winusb-bind, Epic 4 smokes, revue Epic 4, SPEC constraints, prompt-demarrage banner). Optional later: deeper Zadig screenshots / operator FR smoke polish. |

## Deferred from: code review of 4-4-authenticode-policy-and-smartscreen-honesty.md (2026-08-10)

- ~~Production INF `.cat` still not produced/packaged for Public Installer — waits on OQ-3 certificate path~~ — **superseded 2026-08-10:** no certificate in this line; guided WinUSB is the clean-PC path.
- Optional SignTool description/URL metadata (`/d`, `/du`) and dual-sign guidance not wired in `sign-public-artifacts.ps1` — nice-to-have only if a cert ever appears; not required under the no-certificate hobby posture.
- SmartScreen “official channel” / “canal officiel” in EN+FR user guides stays generic (“project download page / Releases”) until the first tagged public community release — then replace with the concrete download URL (e.g. GitHub Releases owner/repo or project site) so musicians can verify the source without tribal knowledge.

## Deferred from: code review of 4-3-technical-docs-and-three-way-license-honesty.md (2026-08-10)

- loopMIDI/rtpMIDI product names hyperlink to the virtualMIDI SDK page (not end-user download pages) — pre-existing README pattern; not introduced by 4.3.
- ~~“Public Installer” product naming while OQ-1 redistributable MSI clearance remains open~~ — **superseded 2026-08-10:** OQ-1 out of community scope; rename/honesty pass under hobby install docs follow-up / Epic 6.
- Vendor virtualMIDI marketing OS claims (often Win7–Win10) vs project Validation Matrix (Win10+Win11 mandatory Win10) — optional honesty note; story Dev Notes already warn authors; not an AC miss. (Community target becomes Win11 under Epic 6.)

## Deferred from: code review of 4-2-end-user-documentation-for-first-midi-and-sysex.md (2026-08-10)

- ~~Bridge CMake `project(VERSION)` vs installer `MyAppVersion` dual sources~~ — **resolved 2026-08-10 under Story 4.1**: packaging resolves `MyAppVersion` / `MyAppVersionInfo` from `bridge-version.txt` (CMake) or `CMakeLists.txt` `project(VERSION)`; same SSOT as `Bridge --version`.

## Deferred from: code review of 4-1-public-installer-meeting-ad-12-ux-bar.md (2026-08-10)

- Unsigned INF / missing production `.cat` in Public Installer payload — **policy:** no certificate purchase cert (Correct Course 2026-08-10). Lab confirmed Fail `0xE000022F` on clean Win10. Community path = guided WinUSB; lab mitigation remains `installer/sign-lab-package.ps1` (not public trust).
- No timeout if `pnputil` or Bridge hangs under Inno `Exec` / `ExecAsOriginalUser` — installer wizard can block indefinitely; Inno limitation deferred unless a reusable timeout wrapper is added later.
- ~~Blank Win10 Public Installer smoke matrix~~ — **filled 2026-08-10** (`smoke-epic4-public-installer-mt4.md`); row 5 Fail (unsigned INF); rows 6/8/9 N/A after rollback.

## Deferred from: story 2-5 session longevity design (2026-08-05)

Open soak risks for SM-3 / NFR-R1 (`docs/tests/checklists/smoke-epic2-longevity-mt4.md`). None measured Fail this turn (Win10 ~4 h sample still blank). Do **not** treat them as “usermode destiny” if the sample hits them.

- Incomplete SysEx under hold with no `F7` (no idle timeout) — observe during soak; add idle-timeout / English session failure only if hang is real.
- CTRL_CLOSE may kill before `Stop` → orphan Virtual Ports — prefer Ctrl+C for sample teardown; raise priority if the shared soak path uses window-close.
- `processBulkRead` holds `usbIoMutex_` across decode — observe under multi-hour clock/SysEx load; fix only with evidence.
- Console heartbeat every ~3 s (at least ~4800 lines / 4 h if redirected) — throttle only if redirected logs become unusable.
- Win10 x64 ~4 h sample matrix blank until lab wall-clock time — design + plan landed; blank ≠ Pass; optional 30–60 min interim does not close SM-3 alone.

Design note already captured in the longevity guide (not an open deferral): after pump fail, ports stay up until CLI ~50 ms poll `Stop` — expected Fail→Stop.

## Deferred from: code review of 2-4-matrix-control-minimum-sysex-pass-vectors.md (2026-08-05)

- Update `deferred-work.md` with open 2.3 Win10 SysEx blanks after the Matrix-Control lab (story Task 2+) — not required for Task 1 checklist-only delivery.
- Epic 1 smoke guide does not yet point operators to `docs/tests/checklists/smoke-epic2-matrix-control-mt4.md` — nice-to-have navigation; not a Task 1 acceptance gap.

## Deferred from: code review of 2-3-transparent-sysex-transport-with-burst-buffering.md (2026-08-05)

- Incomplete SysEx under hold cap with no `0xF7`: no idle-timeout / session failure in 2.3 — AC4 for this story = oversize, queue overflow, nested `0xF0` abandon; revisit if Win10 lab shows hangs.
- Win10 SysEx hardware matrix / bilan blank — deferred as manual lab gate (same honesty bar as 2.1/2.2); not a code blocker for this review.
- No DeviceSession pump-level burst integration test in 2.3 — AC2 accepted as queue unit tests + framer/mapper smokes + Win10 lab checklist.
- Trailing bytes after oversize SysEx `Reset()` in the same `Push` span are dropped without an extra reject count (framer cold-start after abort; corrupt-stream edge).
- After the first device→host `SendToHost` failure, remaining demuxed frames from the same bulk read are skipped once `stopPump_` is set (general pump-fail semantics; not unique to SysEx).

## Deferred from: quick-dev spec-epic1-in-mute-and-out1-f5.md (2026-08-05)

- source_spec: `_bmad-output/implementation-artifacts/spec-epic1-in-mute-and-out1-f5.md`
  summary: SysEx over 1024 bytes is dropped silently by MidiMessageFramer with no counter bump — **raised in story 2.3** (`ConsumeOversizeSysexRejectCount` + session English failure)
  evidence: Epic 1 notes/CC smoke only; full SysEx observability belongs with Epic 2; closed by 2.3 observability path
  status: resolved-by-2-3
- source_spec: `_bmad-output/implementation-artifacts/spec-epic1-in-mute-and-out1-f5.md`
  summary: Reader-thread device-host counter lines race CLI std::cout without a shared lock
  evidence: Surfaced in review; pre-existing multi-writer console pattern, not unique to counters
- source_spec: `_bmad-output/implementation-artifacts/spec-epic1-in-mute-and-out1-f5.md`
  summary: After framer Reset, bare running-status data bytes are dropped until a new status byte
  evidence: Standard MIDI cold-start behavior; DIN devices usually send status after reconnect

## Deferred from: code review (Epic 1 integration, 2026-08-05)

- MIDI demux→host framer still absent; 1.6 pump now escalates incomplete spans via `recordPumpFailure` / session stop — **patched** 2026-08-05 evening (`MidiMessageFramer` before `SendToHost`; lab notes/CC OK).
- CTRL_CLOSE_EVENT still only sets cancel flag; Windows may kill before `Stop` finishes port/USB teardown — still open (was 1-6); raise priority before any public/shareable session path.
- Epic 1 integration CR (2026-08-05) **patched**: host→device WriteBulk under `usbIoMutex_`, VirtualMIDI sink mutex, bulk OUT `PIPE_TRANSFER_TIMEOUT`, atomic `running_`, Zadig multi-match refuse, bind docs `--start-session`/`--run-midi`. Remaining lifecycle edge: orphan ports after hard crash / CTRL_CLOSE kill (was 1-5 / 1-6).
- After `recordPumpFailure`, Virtual Ports stay up until CLI ~50 ms poll calls `Stop`; host→device encode silently no-ops in that window.
- `processBulkRead` holds `usbIoMutex_` across full `DecodeFromDevice` + allocations — busy IN can stall host→device encode (latency under load; not a wrong-cable bug once WriteBulk is locked).
- Zadig fallback opens the first hardware-ID match without counting multiple MT4s — ambiguous multi-unit open (Epic 3 / multi-device; primary GUID path already refuses `matchCount != 1`). — **partially closed by story 3.4**: product GUID path has selected-path open + multi-unit enumerate/open-by-identity. **`--dev-zadig` remains single-unit lab only** (GUID listing; Zadig open ignores selected path) — reopen if dual Zadig-bound units must be hosted.
- Partial `CreatePortSet` errors omit which `MT4 Port N` / IN·OUT failed.
- First host→device encode on Port 1 may omit F5 when mapper `currentOutCable_` starts at 0 (Port 1 == cable 0); **patched** 2026-08-05 evening (`currentOutCable_=0xFF` sentinel); **hardware retest OK** (first Out 1 notes no longer fan all Out LEDs).
- **P0 lab 2026-08-05**: device DIN In 1/2 LEDs blink but Ableton silent on virtual IN — **patched** evening. Root cause: WinUSB `ReadBulk` used a 512-byte buffer while Emagic sends full `wMaxPacketSize` packets (lab: 32) padded with `0xFF`, so reads timed out with 0 bytes; fix reads at endpoint max packet size. Also: init IN drain before further OUT, Set Computer Mode + channel CC kick, `MidiMessageFramer` before `SendToHost`. **Hardware OK**: notes+CC on In 1/2 in Ableton (C3, CC7).
- Hardware notes/CC **full** round-trip on all 2 IN + 4 OUT — **lab OK** 2026-08-05 evening (notes then CC); keep section 6.3 parallel multi-OUT optional.
- Identical IN/OUT `MT4 Port N` teVirtualMIDI collision — **patched** in lab session (merged bidirectional create); keep an eye on DAW IN/OUT pairing.
- Shared `MidiBackend` across concurrent `DeviceSession` instances not rejected — still open (was 1-5 / Epic 3). — **closed by story 3.4**: one `VirtualMidiBackend` instance per `DeviceSession` / unit in the multi-unit host.
- Device-host counter lines were invisible in PowerShell during successful retest (reader-thread `cout` buffering) — **patched** to `cerr` + flush + Start hint (re-confirm next session).

## Deferred from: code review of 1-6-notes-and-cc-round-trip-on-all-ports.md (2026-08-05)

- Hardware AC1/AC2 round-trip proof (notes+CC on all 2 IN + 4 OUT via MIDI-OX/DAW) — deferred: infra OK for story close; Windows smoke remains a manual checklist.
- CTRL_CLOSE_EVENT may terminate the process before `DeviceSession::Stop()` finishes closing Virtual Ports and WinUSB.
- Device→host path can forward raw Emagic demux spans that are not complete MIDI messages (no message framer in 1.6) — **patched** 2026-08-05 (`MidiMessageFramer`).
- `SendToHost` does not reject payloads above teVirtualMIDI default max Sysex length — **raised in story 2.3** (reject above 65535 with English diagnostic).

## Deferred from: code review of 1-5-virtualmidi-backend-and-stable-mt4-port-names.md (2026-08-05)

- No recovery for stale Virtual Ports left after a crashed Bridge — revisit when multi-unit / reconnect lifecycle is hardened.
- Same AD-5 display names on IN and OUT need Windows collision proof with teVirtualMIDI (exact spelling vs `#2` suffixes).
- `--start-session` prints expected names but does not enumerate live Windows MIDI endpoints — strengthen when hardware Validation Matrix automation exists.
- Hand-rolled teVirtualMIDI flag constants unpinned to an SDK version/checksum (AQ-3).
- Shared `MidiBackend` across concurrent `DeviceSession` instances is not rejected — Epic 3 multi-unit ownership. — **closed by story 3.4** (one backend per session).

## Deferred from: code review of 1-4-devicesession-and-emagic-cable-mapper-usermode.md (2026-08-05)

- Synchronous `WriteBulk` / `ReadBulk` use blocking WinUSB calls with no `PIPE_TRANSFER_TIMEOUT` — address when a continuous MIDI I/O pump lands (Story 1.5+).
- No synchronization between transport `Close` and in-flight bulk I/O — needed once a reader thread shares the session handle.
- Emagic wire protocol: raw MIDI bytes `0xF5` / `0xFF` are indistinguishable from port-switch / end-of-valid-data framing (same limitation as Linux quirk reference); no V1 escaping layer.

## Deferred from: code review of 1-1-scaffold-bridge-project-and-windows-build-gate.md (2026-08-04)

- Fill the Observed versions table in `docs/dev/windows-ci-toolchain.md` with CMake and Visual Studio values from the first green `windows-2022` Actions log (workflow already prints them).

## Deferred from: spec-ci-cd-windows-pipeline.md (2026-08-05)

- source_spec: `_bmad-output/implementation-artifacts/spec-ci-cd-windows-pipeline.md`
  summary: Add lean C++ unit tests without hardware (EmagicCableMapper, DeviceProfile, MapperSmoke) plus a CMake Tests target and CI job.
  evidence: CI/CD ticket deliberately deferred the test factory — no Tests target today; keep the merge gate lean until the pure-logic surface is wired.

## Deferred from: spec-unit-tests-without-hardware.md (2026-08-05)

- source_spec: `_bmad-output/implementation-artifacts/spec-unit-tests-without-hardware.md`
  summary: Cache Catch2 / `_deps` across Windows CI runs to cut configure+compile time on clean runners.
  evidence: Every clean `windows-2022` job re-fetches and rebuilds Catch2; not required for the lean merge gate but adds flaky network time.

- source_spec: `_bmad-output/implementation-artifacts/spec-unit-tests-without-hardware.md`
  summary: Add a unit test for encode when buffer fits MIDI but has no room for trailing 0xFF pad.
  evidence: `appendTrailingPad` silently omits the end marker when capacity is exhausted; current smoke always uses a large buffer that always expects 0xFF.

## Deferred from: code review of 2-1-midi-clock-and-transport-realtime.md (2026-08-05)

- Dense-clock / transport path may stall when `processBulkRead` holds `usbIoMutex_` across decode while host→device Encode+WriteBulk needs the same lock — revisit only if short DAW clock smoke shows Bridge-induced dropouts (story 2.1 AC); already noted in Epic 1 CR deferred-work.

## Deferred from: code review of 2-2-mtc-quarter-frame-and-full-frame.md (2026-08-05)

- Duplicated MTC quarter-frame / full-frame vectors in `FramerMtcSmoke.cpp` vs `tests/MidiMessageFramerTests.cpp` can drift; same dual-harness pattern as story 2.1 realtime — consolidate shared helpers when touching the framer test factory again.

## Deferred from: spec-mt4-separate-virtual-ports.md (2026-08-06)

- Identical IN/OUT `MT4 Port N` teVirtualMIDI collision — previously **patched** with merged bidirectional create — **superseded** by directional `MT4 Input N` / `MT4 Output Y` separate faces (no shared handle). Re-verify in lab: Device Inquiry on Output must not appear on Input; update any remaining historical notes that still cite the merged workaround as current.
- Partial `CreatePortSet` errors still omit which directional face failed — unchanged; improve diagnostics when touching VirtualMIDI create again.
- Lab soak (Matrix-Control presence 2–5 min + MIDI-OX no-echo check) remains manual on Windows hardware.
- source_spec: `_bmad-output/implementation-artifacts/spec-mt4-separate-virtual-ports.md`
  summary: Amend planning SSOT (AD-5 / product SPEC) from undirected `MT4 Port N` to directional `MT4 Input` / `MT4 Output` naming.
  evidence: Code and operator smokes already ship directional names; architecture/epics still document the old shared-label contract.
- source_spec: `_bmad-output/implementation-artifacts/spec-mt4-separate-virtual-ports.md`
  summary: Harden uniqueness checks against wide/case-folded teVirtualMIDI alias collisions beyond exact UTF-8 string equality.
  evidence: Current validation compares UTF-8 strings only; driver alias rules may be wider.
## Deferred from: spec-sysex-matrix-mid-roundtrip.md (2026-08-07)

- source_spec: `_bmad-output/implementation-artifacts/spec-sysex-matrix-mid-roundtrip.md`
  summary: Mid-stream USB/reorder drops still exist; size-reject + one dump-request retry masks them for palier-1 gate — root cause at harvest/reorder remains open.
  evidence: Lab stamps `185403Z` (retry on len=319) and earlier `discard` wrong lengths before guards; Apple driver 100 % on same MT4.
- source_spec: `_bmad-output/implementation-artifacts/spec-sysex-matrix-mid-roundtrip.md`
  summary: Leading-F0 repair keys on first product byte `0x10` during dump expect — Matrix-shaped for this lab; false-positive risk if stray `0x10` arrives in the same window.
  evidence: Review; intentional narrow guard for Emagic Matrix dump body after lost `F0`.
- source_spec: `_bmad-output/implementation-artifacts/spec-sysex-matrix-mid-roundtrip.md`
  summary: Dump-request retry `WriteBulk` runs while reader may hold `usbIoMutex_` — sync OUT under contention.
  evidence: Edge review; revisit if retry latency or OUT stall shows in longer librarian runs.
- source_spec: `_bmad-output/implementation-artifacts/spec-sysex-matrix-mid-roundtrip.md`
  summary: Temporary first-burst diagnostic stderr (`first-burst IN`, leading-F0 repair lines) still enabled — gate or remove after palier credibility is locked.
  evidence: Intentional lab instrumentation during Quick Dev close.
- source_spec: `_bmad-output/implementation-artifacts/spec-sysex-matrix-mid-roundtrip.md`
  summary: Post-start calm is best-effort (opens librarian gate on 500 ms timeout even if pending/queued not ideal).
  evidence: Edge review; calm alone did not close the hole; keep for hygiene only.

## Deferred from: spec-mt4-device-inquiry-alternate-drop.md (2026-08-07)

- source_spec: `_bmad-output/implementation-artifacts/spec-mt4-device-inquiry-alternate-drop.md`
  summary: `usbIoMutex_` held across demux and SendToHost on the async IN path may delay host OUT WriteBulk under try_lock.
  evidence: Review BH-07; pre-existing async IN design; lab Identity path stays ~46 ms — revisit only with jitter evidence.
- source_spec: `_bmad-output/implementation-artifacts/spec-mt4-device-inquiry-alternate-drop.md`
  summary: Ungated stderr/stdout Inquiry and Identity lab lines on every message.
  evidence: Review BH-09; intentional while closing the first-shot gate — gate or throttle after V1 credibility lab is done.
- source_spec: `_bmad-output/implementation-artifacts/spec-mt4-device-inquiry-alternate-drop.md`
  summary: `BulkOutMaxPacketSize` is discovered but unused by encode/WriteBulk.
  evidence: Review BH-11; leftover from discarded full-packet pad experiments.
- source_spec: `_bmad-output/implementation-artifacts/spec-mt4-device-inquiry-alternate-drop.md`
  summary: Post-kick IN drain hard-caps at 8 packets (same as init drains).
  evidence: Edge review; unlikely after kick with idle bus; change only if lab shows residual >8.

## Deferred from: quick-dev spec-sysex-matrix-bank-burst-2.md (2026-08-07)

- source_spec: _bmad-output/implementation-artifacts/spec-sysex-matrix-bank-burst-2.md
  summary: Bank harness can discard a wrong-size SysEx then Pass on a later 275 B match within the same trial timeout.
  evidence: Edge review; pre-existing _wait_matched_sysex behavior shared with macOS gate - tighten only if a lab shows silent wrong-size frames.
- source_spec: _bmad-output/implementation-artifacts/spec-sysex-matrix-bank-burst-2.md
  summary: Patch-dump Pass does not bind reply slot/program identity to the requested slot.
  evidence: Blind/Edge review; _is_patch_dump checks 275 B + prefix/suffix only - same as accepted macOS control.
- source_spec: _bmad-output/implementation-artifacts/spec-sysex-matrix-bank-burst-2.md
  summary: Update macOS paliers lab report Windows narrative now that Bridge bank gate is closed.
  evidence: Blind review; report still frames early Windows Bridge first-dump losses - do not reopen hardware control, but refresh Windows status later.

## Deferred from: code review Epic 2 Group A (2026-08-08)

- Idle-finalize synthesizes trailing F7 for 274/350-byte holds — deferred (A3): Reporté pour ne pas bloquer les correctifs overnight 99/100 (expect/flush, retry, abandon).
- Post-Start IN calm timeout still opens librarian OUT — deferred (B3): Reporté pour ne pas bloquer les correctifs overnight 99/100 (expect/flush, retry, abandon).
- SysEx holds >400 bytes never abandoned while `anyInFramerHoldingSysex` gates all host OUT — long SysEx / palier-3 concern, not the short dump overnight hole.
- Removed sync bulk-IN capacity guard and soft `LastReadTimedOut` continue — async IN ring model; revisit only if Start/pump evidence shows mis-sized reads.
- `AddHostOutOk` on dump-request retry masks short-dump reject rate in counters — hygiene only.

## Deferred from: quick-dev spec-post-epic2-cr-bank-mid-day-gate.md (2026-08-08)

- source_spec: `_bmad-output/implementation-artifacts/spec-post-epic2-cr-bank-mid-day-gate.md`
  summary: Cold-start first dump_patch after fresh Bridge start still TIMEOUT last=none (Lab B 5/5; many Lab A index 0001).
  evidence: Day-gate labs post-835c992; warm path mostly healthy; Bridge up, send_fail~0.
  status: resolved-by-615882d
- source_spec: `_bmad-output/implementation-artifacts/spec-post-epic2-cr-bank-mid-day-gate.md`
  summary: Absolute 100%-per-start bank/mid day gate remains red (~50% bank starts fail at 98-99%).
  evidence: Lab A 10/20 starts fail; 13 TIMEOUT last=none total; residual intermittency under gate.
  status: resolved-by-cd64193
- source_spec: `_bmad-output/implementation-artifacts/spec-post-epic2-cr-bank-mid-day-gate.md`
  summary: Day-gate harness scores cold first dump with no warmup discard — optional settle vs Bridge fix decision.
  evidence: Blind Hunter; do not auto-mask first dump without product decision.
  status: resolved-keep-scoring

## Deferred from: quick-dev spec-cold-start-premier-dump-matrix.md (2026-08-08)

- source_spec: `_bmad-output/implementation-artifacts/spec-cold-start-premier-dump-matrix.md`
  summary: Rare mid-burst bank TIMEOUT last=none after cold-start F0 repair (1/20 Starts at 99%; slot 08 index 0009).
  evidence: Mid 5×10 all green; bank 19/20 at 100%; sole miss was mid-burst not first dump; dump 0001 RECV on failing Start.
  status: resolved-by-cd64193
- source_spec: `_bmad-output/implementation-artifacts/spec-cold-start-premier-dump-matrix.md`
  summary: Absolute 100%-per-start bank day gate still red until mid-burst residual is closed or gate policy relaxed.
  evidence: post-epic2-cr-coldfix bank run 20260808T162947Z overall_pass=false.
  status: resolved-by-cd64193

## Deferred from: quick-dev spec-mid-burst-bank-timeout-matrix.md (2026-08-08)

- source_spec: `_bmad-output/implementation-artifacts/spec-mid-burst-bank-timeout-matrix.md`
  summary: Mid-SysEx bulk-IN URB loss (275−N×32 short F0…F7) still occurs under bank load; ring 32 + two size-reject rewrites mask it for the day gate.
  evidence: Lab post-epic2-cr-midburst still had dual-short exhaust at 1/20; retry2 gate green with many retries_left logs; root assembler drop not eliminated.
- source_spec: `_bmad-output/implementation-artifacts/spec-mid-burst-bank-timeout-matrix.md`
  summary: abandonIdlePartialSysexHoldUnlocked can feed non-reply hold sizes into rejectShortMatrixDumpAndRetry under expect.
  evidence: Pre-existing path; larger rewrite budget could burn dump re-requests on stuck non-dump holds.

## Deferred from: story 2-2 MTC lab closeout (2026-08-09)

- source_spec: `_bmad-output/implementation-artifacts/2-2-mtc-quarter-frame-and-full-frame.md`
  summary: Author a dedicated real-DAW / Scarlett UAT guide covering MTC (and broader Bridge features) when home lab gear is available; Python DIN-loopback harness is the 2.2 closeout proof only.
  evidence: 2026-08-09 decision — Scarlett unavailable on the road; `mtc-loopback-lab.py` Pass on Out2→In2 after demux fix.
- source_spec: `_bmad-output/implementation-artifacts/2-2-mtc-quarter-frame-and-full-frame.md`
  summary: OUT-hinted IN sticky (`hintInCableFromOut`) can mis-attribute unlabeled IN traffic if the host is sending on Out N while independent DIN traffic arrives on another In without an Emagic `F5` tag.
  evidence: Stress 2026-08-09 `midi-concurrent-in-stress.py` ~2.2 min / 220 rounds — Matrix dumps 220/220 on In1 and 0 on In2 (good); Out2 notes mostly on In2 (1759) but **1× note72 on In1** (`cross_note72_on_in1=1`) → overall_pass=false. Log `tests/lab-logs/midi-concurrent-in/concurrent-in-20260809T214726Z.log`. V1 must harden before public release.

## Deferred from: quick-dev spec-epic-2-pc-only-closure.md (2026-08-10)

- source_spec: `_bmad-output/implementation-artifacts/spec-epic-2-pc-only-closure.md`
  summary: Author / run a dedicated real-DAW / Scarlett UAT for MIDI clock + transport (story 2.1) when home lab gear is available; Python DIN-loopback harness is the 2.1 closeout proof only.
  evidence: 2026-08-10 decision — Scarlett unavailable; `midi-clock-loopback-lab.py` Pass on Out2→In2 (`20260809T221926Z`); mirrors 2.2 harness barème.
- source_spec: `_bmad-output/implementation-artifacts/spec-epic-2-pc-only-closure.md`
  summary: Fill Matrix-Control GUI hard-gate rows (smoke §6 / English Matrix-Control checklist) on Win10 when GUI time is available; script mid/bank day-gates close story 2.4 librarian shapes only.
  evidence: 2026-08-10 mid `20260809T220849Z` + bank `20260809T221054Z` `overall_pass=true`; GUI rows left blank on purpose.

## Deferred from: quick-dev spec-windows-bridge-post-midburst-soak.md (2026-08-09)

- source_spec: `_bmad-output/implementation-artifacts/spec-windows-bridge-post-midburst-soak.md`
  summary: Mid-SysEx bulk-IN URB loss root is still not eliminated; ring 48 + retries 4 recover soak triple-short storms without proving which lever alone closed the hole.
  evidence: Palier A overnight-20260809T165115Z cycles 208 bank / 209 mid — TIMEOUT last=none after size-reject exhaust at budget 2; day-gate re-green after combo deepen.
- source_spec: `_bmad-output/implementation-artifacts/spec-windows-bridge-post-midburst-soak.md`
  summary: Larger size-reject rewrite budget can stretch the 3500 ms expect window across several rewrite OUTs during a short storm (mutex / completion pressure).
  evidence: Code-review of post-soak constant bump; nested rewrite guard still drops shorts without consuming budget.

## Deferred from: code review of 3-1-auto-start-without-daily-administrator.md (2026-08-10)

- Offline unit tests only assert Auto-Start constants (`--auto-session`, task name, wait bounds), not register/unregister / fallback / dual-backend cleanup — Windows COM lab remains the practical gate.
- Daily Auto-Start still launches a console Bridge; CTRL_CLOSE orphan Virtual Ports risk remains (already documented; smoke prefers Ctrl+C). Auto-Start increases daily exposure but does not invent a new lifecycle owner.

## Deferred from: code review of 3-2-hot-plug-recovery-without-windows-reboot.md (2026-08-10)

- Offline hot-plug coverage only asserts wait-constant aliases — thin offline pattern carried from 3.1; hardware SM-4 remains the gate (`tests/unit/HotPlugContractTests.cpp`).
- Surprise-removal `DeviceSession::Stop` still runs best-effort finish-magic `WriteBulk` that can block on a dead device — pre-existing; story 3.2 documented order only (harden with hang evidence).
- After SM-4 lab evidence: try to shorten mid-session re-attach latency after replug — V1 reuses Auto-Start cadence (2 s poll / 900 s fail-closed / Start retries); story 3.2 already allowed a shorter hot-plug-specific bound or faster detection (`CM_Register_Notification`) once real rack-move timings are measured; do not change fail-closed honesty.

## Deferred from: code review of 3-4-two-mt4-units-with-stable-distinguishable-names.md (2026-08-10)

- Offline hot-plug coverage still only asserts wait-constant aliases while the multi-unit product loop no longer calls that wait helper — thin offline pattern; hardware SM-4 / dual-MT4 smoke remain the gate (`tests/unit/HotPlugContractTests.cpp`).
- Topology identity keys use the full USB instance ID and change when a serial-less MT4 moves hub/port — known fallback limit; AQ-1 stays lab notes (`src/Usb/WinUsbEnumerate.cpp`).
- Two Bridge processes can race the same `%LOCALAPPDATA%` identity registry file — V1 assumes a single Bridge host (`src/Device/UnitIdentityRegistry.cpp`).

## Deferred from: code review of revue-code-transverse-epic-3-daily-studio.md (2026-08-10)

- CTRL_CLOSE / window-close may kill before `Stop`/`DestroyPortSet`; Auto-Start daily path widens orphan VirtualMIDI exposure — already deferred from 3-1; reconfirmed as Epic 3 joint (prefer Ctrl+C; no new lifecycle owner this pass).
- Surprise-removal `DeviceSession::Stop` still best-effort finish-magic `WriteBulk` on a possibly dead bus after ports are destroyed — already deferred from 3-2; reconfirmed under multi-unit/multi-client teardown.
- Topology-only identity (no serial) can allocate a new `K` when the unit moves USB hub/port — already deferred from 3-4 / AQ-1; not closed by this transversal review.
- Dual physical MT4 hardware matrix remains lab-unproven (`docs/tests/smoke-epic3-dual-mt4-mt4.md`); offline registry/naming and one-session-per-unit code paths are not a substitute Pass.

## Deferred from: code review of 5-2-publish-measurement-method-and-baseline-tables.md (2026-08-11)

- Define refresh/archive protocol for `docs/dev/measurements/baseline-latest.md` when a newer run supersedes “latest” (overwrite vs dated history vs archive) — ops process not specified in Story 5.2.
- Symmetric soft-echo-ON confirm flag for software-loop to mirror hardware `--confirm-soft-echo-off` — harness design from Story 5.1; out of 5.2 docs-only scope unless a later harness story reopens it.
