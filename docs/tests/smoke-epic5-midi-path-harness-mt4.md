---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 5.1 — MIDI Path harness scaffold (MT4)
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-11
lab_result: Win10 Pass (software-loop plumbing; harness 50 samples ~2 ms mean) — not Studio-Done; hardware-loop not run
---

# Guide de smoke — Epic 5.1 MIDI Path harness (échafaudage)

Ce guide sert pour **Story 5.1** : prouver que l’outil de mesure du **chemin MIDI** (pas la taille de buffer ASIO) **compile**, tourne en boucle logicielle Bridge, et reste honnête sur ce qu’il mesure.

**Barème d’honnêteté :** case vide ≠ Pass. Les chiffres de cette story sont une **preuve de plomberie**, pas Studio-Done (méthode/tables → Story **5.2** ; ancres → Story **5.3**).

## Ce que tu valides

| Sujet | Contrat |
|---|---|
| Artefact | `MidiPathHarness.exe` sous `builds/` (ex. `builds/debug` ou `builds/ci`) |
| Horloge | Inject / observe avec `QueryPerformanceCounter` (pas `Sleep` comme timer) |
| Plan de mesure | Client WinMM hôte contre les ports virtuels Bridge — pas les stamps internes MidiBackend / WinUSB |
| Boucle logicielle | Bridge avec `--soft-echo` (ou `UNITOR_MIDI_SOFT_ECHO=1`), harness `--path software-loop` |
| Boucle DIN | Optionnelle : soft-echo **OFF**, câble DIN Out→In, `--path hardware-loop --confirm-soft-echo-off` |
| ASIO | **Hors scope** — jamais une preuve du chemin MIDI |
| Soft-echo studio | Gate **OFF** par défaut — `--no-soft-echo` force l’arrêt même si `UNITOR_MIDI_SOFT_ECHO` est collée |

### Hors scope (stories suivantes)

| Sujet | Story |
|---|---|
| Méthode publiée + tables de baseline | **5.2** (`docs/dev/measurements/`) |
| Décision Studio-Done / ancres NFR-P1/P2 | **5.3** / OQ-2 |
| Backend Windows MIDI Services | Epic **6** |

## Prérequis

- Win10 x64 lab, virtualMIDI installé, MT4 bindé WinUSB
- `Bridge.exe` et `MidiPathHarness.exe` sous `builds/`
- Pour software-loop : démarrer Bridge avec soft-echo lab, ex.  
  `Bridge.exe --start-session --soft-echo`  
  (ou `UNITOR_MIDI_SOFT_ECHO=1` + session habituelle)
- Teardown : préférer **Ctrl+C** sur Bridge (`CTRL_CLOSE` peut orpheliner des ports virtuels)

## Comment noter

- **✅** / **❌** / **N/A** (+ courte raison)
- Case vide = non joué (**≠** Pass)
- Win10 x64 pour clore le claim software-loop

## Matrice Pass / Fail

| # | Check | Win10 | Notes |
|---|---|---|---|
| 1 | Configure/build → `MidiPathHarness.exe` présent sous `builds/` | ✅ | Release `builds/ci` 2026-08-11 |
| 2 | `MidiPathHarness --help` mentionne software-loop / hardware-loop, QPC, et dit clairement que ASIO n’est pas une preuve | ✅ | |
| 3 | software-loop : Bridge soft-echo ON + harness `--path software-loop --out "MT4 Out 1" --in "MT4 In 1"` imprime `path_type=software-loop` et des latences µs | ✅ | 50 samples; mean ≈ 2.0 ms; capsule [`lab-evidence/midi-path-harness-software-loop-2026-08-11/`](lab-evidence/midi-path-harness-software-loop-2026-08-11/) |
| 4 | hardware-loop (optionnel) : DIN présent, soft-echo OFF, `--path hardware-loop --confirm-soft-echo-off` complète ; sans DIN → échec honnête (pas Pass inventé) ; sans `--confirm-soft-echo-off` → refus CLI | | Non joué (≠ Pass) |
| 5 | Aucune sortie / doc de ce smoke ne cite la taille de buffer ASIO comme preuve MIDI Path | ✅ | |
| 6 | Soft-echo resté OFF hors lab (chemin studio / Auto-Start inchangé) | ✅ | Gate default OFF ; lab only `--soft-echo` ; `--no-soft-echo` coupe l’env collée |

## Bizarreries connues (ne pas « corriger » dans 5.1)

- Soft-echo saute le chemin USB — les chiffres software-loop ne prouvent **pas** le DIN.
- Sous charge, le bulk IN peut influencer un hardware-loop ; premières baselines : lab calme.
- CTRL_CLOSE peut laisser des ports virtuels orphelins — Ctrl+C.

## Références

- Story 5.1 / Epic 5 — CAP-16, NFR-P3, AD-11, AD-13, SM-9 (harness exists)
- Architecture AD-11 / AD-13
- Correct Course 2026-08-10 (Epic 5 débloqué sur virtualMIDI + Win10)
