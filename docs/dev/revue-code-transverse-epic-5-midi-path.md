# Prompt — Revue de code transverse Epic 5 (MIDI Path Proof / Studio-Done Gate)

Copier-coller tel quel dans un **nouveau chat Cursor** (contexte frais), **après** (ou en s’appuyant sur) les revues Epic 1–4.  
Skill conseillé : **bmad-code-review** (+ edge-case / adversariale si utile).  
Projet : `C:\Users\Guillaume\Dev\Projects\unitor-win64-driver`

---

## Contexte de la Story

On cherche les failles **entre** le banc de mesure MIDI, l’écho logiciel du Bridge, les chiffres publiés et la décision « studio prêt » — ce qui pourrait encore fausser ou sur-vendre la latence, alors que chaque story 5.x prise seule semblait OK.

---

## Qui tu es / comment parler

- Agent BMad développeur / reviewer sur **unitor-win64-driver**.
- Français clair, intention produit d’abord (ce que ça change pour la mesure / la décision studio).
- Noms gardés : **MT4**, **WinUSB**, **virtualMIDI**, **MIDI**, **Bridge**, **QPC**.
- Pas de jargon BMad non glosé ; chemins/symboles en complément.
- **Pas de commit** sans demande explicite.
- Règles : clarity-bar, `conventions.md` §3, lint sur diff C++ touché.

---

## Pourquoi cette revue (contexte produit)

Epic 5 est **clôturé** côté sprint (`epic-5: done`, stories 5.1–5.3 `done`). Les revues **story par story** sont passées. Comme pour Epic 1–4, on veut une **revue transverse** : joints harness × soft-echo × méthode × décision, pas une relecture isolée du dernier commit.

Découpage retenu (2026-08-11) :

1. **Vague 1 — code** (harness + SoftEchoGate + Bridge + CI/tests)
2. **Vague 2 — docs / preuves / décision** (après vague 1)

Prompts frères :

- `docs/dev/revue-code-transverse-epic-1-mt4-midi.md`
- `docs/dev/revue-code-transverse-epic-2-transport-sysex.md`
- `docs/dev/revue-code-transverse-epic-3-daily-studio.md`
- `docs/dev/revue-code-transverse-epic-4-community-install.md`

---

## Objectif Epic 5

Epic 5 = « MIDI Path Proof (Studio-Done Gate) » (sprint : **done**).

Stories (toutes **done**) :

1. Harness in-repo + soft-echo Bridge — done  
2. Méthode + tableaux provisoires — done  
3. Décision Studio-Done / ancres tempo — done  

---

### Review Findings — Wave 1 (code) — 2026-08-11

*(Couches : Blind Hunter + Edge Case Hunter + Acceptance Auditor. Triage parent.)*

#### Decision-needed

- [x] [Review][Decision] How to enforce honest `path_type` labeling — **Décidé 2026-08-11 : option 1** honor-system both sides (`--confirm-soft-echo-on` for software-loop + existing `--confirm-soft-echo-off` for hardware-loop); no Bridge probe.
- [x] [Review][Decision] Soft-echo ON + `sendData_` failure — **Décidé 2026-08-11 : option 1** drop / fail-closed, never fall through to USB.
- [x] [Review][Decision] Observe timestamp plane — **Décidé 2026-08-11 : option 1** keep `CALLBACK_WINDOW`; document message-pump delay in method (Wave 2 docs).

#### Patch

- [x] [Review][Patch] Require `--confirm-soft-echo-on` for software-loop (mirror hardware `--confirm-soft-echo-off`) [`tools/midi-path-harness/Main.cpp`]
- [x] [Review][Patch] Soft-echo ON + `sendData_` failure: drop (no USB fallthrough); treat as handled-failed [`src/Midi/VirtualMidiBackend.cpp`]
- [x] [Review][Patch] Soft-echo env must not arm Auto-Start / studio sessions (`UNITOR_MIDI_SOFT_ECHO` + `--auto-session` / session dispatch) [`src/App/Main.cpp` / `src/Midi/SoftEchoGate.cpp`]
- [x] [Review][Patch] Abort sample/run when inject QPC stamp is `0` (counter failure) [`tools/midi-path-harness/QpcClock.cpp` / `WinMmMidiIo.cpp` / `MidiPathRunner.cpp`]
- [x] [Review][Patch] Fail open when multiple WinMM devices tie at best port-match rank [`tools/midi-path-harness/WinMmMidiIo.cpp`]
- [x] [Review][Patch] Match Note On channel (status byte), not only note+velocity [`tools/midi-path-harness/WinMmMidiIo.cpp`]
- [x] [Review][Patch] Send matching Note Off after each successful observe (hardware-loop stuck notes) [`tools/midi-path-harness/WinMmMidiIo.cpp` / `MidiPathRunner.cpp`]
- [x] [Review][Patch] `waitForObserve` PeekMessage only the MIDI IN hwnd (not entire thread queue) [`tools/midi-path-harness/WinMmMidiIo.cpp`]
- [x] [Review][Patch] Unit-test SoftEchoGate truth table (`--soft-echo` / `--no-soft-echo` / env) [`tests/unit/SoftEchoGateTests.cpp`]
- [x] [Review][Patch] CI merge echo: say harness artifact present, not “passed” as if run [`.github/workflows/windows-build.yml`]
- [x] [Review][Patch] Document `--soft-echo` / `--no-soft-echo` in Bridge help/usage [`src/App/Main.cpp`]

#### Defer

- [x] [Review][Defer] Document that observe QPC under `CALLBACK_WINDOW` includes message-pump delay — Wave 2 method docs — **done 2026-08-11** in method Known confounders
- [x] [Review][Defer] Late same note/vel arm-race across samples after slow path — mitigated by midiIn drain before arm; revisit if lab shows false short samples — deferred, pre-existing residual risk
- [x] [Review][Defer] JSON escape only `\`/`"` (control chars in WinMM names) — practical MT4 names are clean; polish later — deferred, low practical risk

#### Dismissed (not listed as open work)

- `volatile` observe fields — same-thread `CALLBACK_WINDOW` pump; not a cross-thread race under current design
- Soft-echo no-matching-IN → USB fallthrough — intentional after Story 5.1 review (code comment)
- SysEx `DWORD` truncate / harness ignores `MIM_LONGDATA` — Note On measurement plane only; >4GB SysEx not realistic
- `GetTickCount` 49-day wrap — harness is short-lived

---

### Review Findings — Wave 2 (docs / evidence / decision) — 2026-08-11

*(Couches : Blind Hunter + Edge Case Hunter + Acceptance Auditor. Triage parent. Wave 1 C++ not re-opened.)*

#### Decision-needed

- [x] [Review][Decision] Gate **(a)** at n=100 p99≡max — **Décidé 2026-08-11 : option 1** keep **(a)**; honesty labels only (stop claiming n=100 strengthens p99; state p99≡max under current index).

#### Patch

- [x] [Review][Patch] Method + baseline + Gate: stop implying n=100 strengthens p99; state p99≡max at n=100 under current index; label Gate/hardware row accordingly [`docs/dev/measurements/method-midi-path.md` / `baseline-latest.md` / `studio-done-gate-decision.md`]
- [x] [Review][Patch] Gate rationale: say “clears / under healthy ceiling ≤4–5 ms / ≤1–2 ms”, not “within healthy ≈4–5 / ≈1–2” [`studio-done-gate-decision.md`]
- [x] [Review][Patch] Baseline + recipes: add `--confirm-soft-echo-on` to software-loop operator recipe; smoke row 3 → N/A (or blank) until a confirm-on re-run capsule exists [`baseline-latest.md` / software capsule / `smoke-epic5-…`]
- [x] [Review][Patch] Baseline Plane cells: include run-level `studio_done=false`; keep Gate claim on decision banner only [`baseline-latest.md`]
- [x] [Review][Patch] Software Date/UTC: do not stamp local wall time as `…Z`; record local + timezone or true UTC [`baseline-latest.md` / software capsule]
- [x] [Review][Patch] Hardware capsule + baseline: one line local 2026-08-11 (UTC+2) = UTC 2026-08-10T22:55:20Z [`hardware capsule` / `baseline-latest.md`]
- [x] [Review][Patch] Software-loop capsule honesty: Gate **(a)** closed on hardware capsule; software alone still cannot clear NFR-P1; drop “5.3/OQ-2 pending” / “hardware not run” [`software capsule README` / lab-evidence index]
- [x] [Review][Patch] Story 5.3 Dev Notes evidence snapshot: align to superseding **(a)** (hardware + classical jitter); mark morning **(c)** historical only [`5-3-…md`]
- [x] [Review][Patch] Method Known confounders: CALLBACK_WINDOW / message-pump delay on observe path (Wave 1 defer) [`method-midi-path.md`]
- [x] [Review][Patch] `sprint-status.yaml` header NEXT → Epic 6 (epic-5 already done)
- [x] [Review][Patch] deferred-work 5.2 bullet: mark soft-echo-ON confirm Done (strike open duplicate)
- [x] [Review][Patch] Glossary Studio-Done anchors: Win10 lab closing caveat [`glossary.md`]
- [x] [Review][Patch] Architecture AD-11: drop `[ASSUMPTION]` on now-published Win10 hardware + classical jitter facts [`ARCHITECTURE-SPINE.md`]
- [x] [Review][Patch] Method: refuse/blank rule for n=0 / empty series publishability [`method-midi-path.md`]
- [x] [Review][Patch] README Epic 5 done line: short caveat pointer to decision / DIN lab path [`README.md`]
- [x] [Review][Patch] PRD `.memlog.md` / validation-matrix TBD stubs: point to confirmed Gate + AD-11 measurements path (low)

#### Defer

- [x] [Review][Defer] Soft-echo OFF machine-readable field on harness JSON for Gate confirm — already deferred after Story 5.3; human bridge excerpt remains attestation — deferred, pre-existing
- [x] [Review][Defer] Rewrite NFR-P1 lead away from “beyond the host USB path” — already decided Story 5.3 (keep wording + measurement-plane footnote) — deferred/dismissed as prior decision

#### Dismissed (Wave 2)

- Re-litigate NFR-P1 lead sentence without footnote (footnote already present on PRD/decision/method)
- Mixing software/hardware into one unlabeled series (already fenced)
- Morning **(c)** vs **(a)** on Gate decision page itself (already superseded clearly)
