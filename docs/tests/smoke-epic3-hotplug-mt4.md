---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 3.2 — Hot-plug sans reboot Windows (MT4)
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
---

# Guide de smoke — Epic 3.2 Hot-plug (MT4 sous Windows)

Ce guide te sert pour **Story 3.2** : session live avec ports virtuels, tu débranches puis rebranches le MT4, et tu récupères des ports utilisables **sans redémarrer Windows**.

Il est calqué sur les guides Epic 1–2 : français, résultat **au fil de l’eau**.

**Barème d’honnêteté :** case vide ≠ Pass. Un débranche au milieu d’un SysEx peut demander un rescan MIDI dans l’hôte ou un redémarrage Bridge supervisé (bord UJ-2) — note ce qui s’est passé ; **ne** dis **pas** que le UAT GUI Matrix-Control est clos ici. Exiger un reboot Windows pour retrouver les ports = **Fail**.

## Contrat hot-plug V1 (ce que tu valides)

| Sujet | Contrat |
|---|---|
| Hôte produit | `Bridge.exe --auto-session` (processus de session utilisateur seulement ; **pas** un service Windows — AD-20) |
| Détection de la perte | Échec du pump / `!IsRunning()` inattendu alors que l’annulation **n’est pas** demandée |
| Teardown | `DeviceSession::Stop()` détruit les ports via `MidiBackend` (AD-9). Console anglaise multi-unité : `MT4 unit K=… disconnected; tearing down that unit only` ; quand **plus aucune** unité live : `MT4 disconnected; waiting for replug...` |
| Attente / rescan | Après perte de **toutes** les unités live : sondage GUID WinUSB **Absent** (efface un Present périmé) puis **Present** — toutes les **2 s**, progression toutes les **30 s**, échec fermé après **900 s** (`Hot-plug recovery` / fail closed). Une unité encore présente garde ses ports ; la unité manquante attend un Absent d’identité avant retry Start |
| Recréation | **Nouvelle** `DeviceSession::Start` sous l’identité AD-6 ; console : `Hot-plug recovery: new DeviceSession started for K=…`. V1 une unité : noms `MT4 In N` / `MT4 Out N`. L’App **ne** doit **pas** appeler `CreatePortSet` / `DestroyPortSet` directement |
| AQ-2 (préférence UX) | Défaut V1 = **recréation silencieuse dans le même process** + diagnostics console anglais ; pas de boîte tray/GUI à valider |
| Visibilité côté hôte | Ableton / Reason / MIDI-OX peuvent demander un **rescan** MIDI ; un redémarrage Bridge supervisé est une **échappatoire autorisée** (AD-10), pas le seul chemin |
| Labo one-shot | `Bridge.exe --start-session` / `--run-midi` **quittent** encore sur perte USB en milieu de session (scripts labo qui attendent la fin du process) |
| Échappatoire | Redémarrage Bridge supervisé OK si la recréation in-process ne suffit pas ; reboot Windows requis = **échec V1** |
| Arrêt propre | Préfère **Ctrl+C** (pas la croix). Risque reporté : `CTRL_CLOSE` peut laisser des ports orphelins (`deferred-work.md`) |

### Hors scope (stories suivantes)

| Sujet | Story |
|---|---|
| Multi-client DAW + MIDI-OX (SM-7) | **3.3** — [`smoke-epic3-multiclient-mt4.md`](smoke-epic3-multiclient-mt4.md) |
| Noms / persistance pour deux MT4 | **3.4** — [`smoke-epic3-dual-mt4-mt4.md`](smoke-epic3-dual-mt4-mt4.md) |
| Chapitre hot-plug poli dans `docs/user/` | **4.2** — [`docs/user/unitor-mt4-bridge-user-guide.md`](../user/unitor-mt4-bridge-user-guide.md#unplug-and-replug-the-mt4) ; smoke [`smoke-epic4-user-docs-mt4.md`](smoke-epic4-user-docs-mt4.md) |
| Installateur public | **4.1** |
| Mesures de latence MIDI | Epic **5** |

### Références (fichier source de vérité)

- Epics Story 3.2
- PRD FR-11 / NFR-R2 / SM-4 / UJ-4
- Architecture AD-9, AD-10, AD-20
- SPEC CAP-11
- AQ-2 reporté (préférence UX seulement ; le cycle de vie reste AD-9)

## Prérequis

- Story **3.1** Auto-Start OK sur cette session ([`smoke-epic3-autostart-mt4.md`](smoke-epic3-autostart-mt4.md))
- Epic 1–2 déjà OK (notes/CC + bind WinUSB)
- virtualMIDI installé
- `Bridge.exe` sous `builds/` (ex. `builds/debug`)
- Un hôte de la matrice ouvert pendant le drill : MIDI-OX et/ou une DAW qui liste les ports virtuels

## Comment noter

- **✅** / **❌** / **N/A** (+ courte raison)
- Case vide = non joué (**≠** Pass)
- Win10 x64 **obligatoire** pour clore le claim ; Win11 x64 en plus quand dispo

## Matrice Pass / Fail

| # | Vérification | Win10 x64 | Win11 x64 | Notes |
|---|---|---|---|---|
| 1 | `--auto-session` live + MIDI-OX et/ou DAW de la matrice → ports utilisables (`MT4 In N` / `MT4 Out N`) | | | |
| 2 | Débrancher le MT4 → ports détruits (pas de noms orphelins sur le happy path / arrêt Ctrl+C) ; le **process** Bridge **reste vivant** en `--auto-session` (console d’attente replug). Si tu as utilisé un redémarrage Bridge supervisé, note-le | | | |
| 3 | Rebrancher le MT4 → ports utilisables **sans reboot Windows** | | | |
| 4 | Rescan MIDI hôte (ou redémarrage Bridge supervisé documenté) restaure la visibilité si besoin | | | |
| 5 | Récupération = **nouvelle** session sous les noms AD-5/AD-6 — la console montre à nouveau la bannière de démarrage après replug | | | |
| 6 | **Négatif :** exiger un reboot Windows pour retrouver les ports = **Fail** | | | |

## Commandes (référence)

```text
builds\debug\Bridge.exe --auto-session
builds\debug\Bridge.exe --start-session
builds\debug\Bridge.exe --run-midi
builds\debug\Bridge.exe --test-mapper
builds\debug\Bridge.exe --test-port-names
```

**Drill produit :** démarre avec `--auto-session`, ouvre MIDI-OX/DAW, débranche, attends la bannière d’attente (anglais), rebranche, confirme le retour des ports, rescane l’hôte si besoin.

**Labo one-shot :** `--start-session` / `--run-midi` quittent au débranche (scripts qui attendent la fin du process). Ne les utilise **pas** seuls pour claimer la récupération FR-11.

**Échappatoire redémarrage supervisé (AD-10) :** si la recréation in-process ne rend pas les ports visibles dans l’hôte, arrête avec Ctrl+C, relance `Bridge.exe --auto-session`, puis rescane. Toujours **sans reboot Windows**.

## Hors scope pour ce smoke

- Politique multi-client / exclusive-open → **3.3** ([`smoke-epic3-multiclient-mt4.md`](smoke-epic3-multiclient-mt4.md))
- Stabilité des noms hot-plug pour deux MT4 → **3.4** — [`smoke-epic3-dual-mt4-mt4.md`](smoke-epic3-dual-mt4-mt4.md)
- Installateur public / doc utilisateur polie → **4.1** / **4.2** ([`docs/user/unitor-mt4-bridge-user-guide.md`](../user/unitor-mt4-bridge-user-guide.md#unplug-and-replug-the-mt4), [`smoke-epic4-user-docs-mt4.md`](smoke-epic4-user-docs-mt4.md))
- UAT GUI Matrix-Control
- Longévité Epic 2 ~4 h (un redémarrage supervisé OK pour hot-plug **n’excuse pas** un Fail longévité)

## Docs liées

- Manuel utilisateur hot-plug : [`docs/user/unitor-mt4-bridge-user-guide.md`](../user/unitor-mt4-bridge-user-guide.md#unplug-and-replug-the-mt4)
- Auto-Start (3.1) : [`smoke-epic3-autostart-mt4.md`](smoke-epic3-autostart-mt4.md)
- Multi-client DAW + MIDI-OX (3.3) : [`smoke-epic3-multiclient-mt4.md`](smoke-epic3-multiclient-mt4.md)
- Ownership longévité : [`checklists/smoke-epic2-longevity-mt4.md`](checklists/smoke-epic2-longevity-mt4.md)
- Smoke transport Epic 2 : [`smoke-epic2-mt4.md`](smoke-epic2-mt4.md)
- Bind WinUSB (admin une fois) : [`../dev/winusb-bind.md`](../dev/winusb-bind.md)
