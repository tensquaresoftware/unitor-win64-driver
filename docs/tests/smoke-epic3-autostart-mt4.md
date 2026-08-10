---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 3.1 — Auto-Start sans admin au quotidien (MT4)
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
---

# Guide de smoke — Epic 3.1 Auto-Start (MT4 sous Windows)

Ce guide te sert pour **Story 3.1** : le Bridge démarre tout seul sur ta **session Windows utilisateur**, pour que les ports virtuels apparaissent après la connexion (ou au premier branchement USB), **sans** lancer le Bridge à la main et **sans** UAC / admin au quotidien.

Il est calqué sur [`smoke-epic1-mt4.md`](smoke-epic1-mt4.md) / [`smoke-epic2-mt4.md`](smoke-epic2-mt4.md) : français, résultat **au fil de l’eau**.

**Barème d’honnêteté :** une case vide ≠ Pass. Ce guide clôt la partie **runtime Auto-Start** de SM-5 / FR-3 / CAP-3. L’installeur + la doc utilisateur polie restent Epic **4**. Le débranche / rebranche en cours de session, c’est Story **3.2** — voir [`smoke-epic3-hotplug-mt4.md`](smoke-epic3-hotplug-mt4.md).

## Contrat Auto-Start V1 (ce que tu valides)

| Sujet | Contrat |
|---|---|
| Processus | Le Bridge reste un process **interactif de ta session** (pas un service Windows Session-0 — AD-20) |
| Interdit | Service Session-0 / projet SCM (AD-20) |
| Enregistrement | D’abord une tâche **Planificateur de tâches** au logon : `TASK_LOGON_INTERACTIVE_TOKEN` (« Exécuter seulement si l’utilisateur est connecté »), **RunLevel Limited** (pas les privilèges les plus élevés), action = chemin absolu du `Bridge.exe` qui s’enregistre + `--auto-session` |
| Si le Planificateur refuse | **HKCU Run est un primaire V1 acceptable** (pas un plan B dégradé) : si le Planificateur renvoie accès refusé (ou autre échec), le Bridge enregistre `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` (même exe absolu + `--auto-session`). La console dit quel mécanisme a été utilisé. Les deux Passent en non-admin sans UAC |
| Nom de tâche | `UnitorMt4BridgeAutoStart` |
| Ligne de commande | `Bridge.exe --register-auto-start` / `--unregister-auto-start` / `--auto-session` |
| Privilèges | Le bind WinUSB / signature lab reste **admin une fois** (install). Enregistrer / lancer / désenregistrer au quotidien = utilisateur interactif **sans élévation** (NFR-D2) |
| Comportement `--auto-session` | MT4 déjà là → démarre la session (ports virtuels). Absent → attend / rescane le GUID WinUSB `{aa209017-cf8a-49ad-a0e7-701187ff7e05}` |
| Attente | Sondage toutes les **2 s**, progression toutes les **30 s**, échec fermé après **900 s** (15 min) avec diagnostics **en anglais** — pas de hang silencieux |
| VirtualMIDI manquant | Échec fermé + chemin de correction en anglais (liste de ports vide ≠ succès) |
| Arrêt propre | Préfère **Ctrl+C** (pas la croix de la console). Risque connu reporté : `CTRL_CLOSE` peut laisser des ports orphelins (`deferred-work.md`) |
| Chemin labo conservé | `Bridge.exe --start-session` / `--run-midi` (+ optionnel `--dev-zadig`) inchangés pour les labs |

### Hors scope (stories suivantes)

| Sujet | Story |
|---|---|
| Débranche / rebranche en session | **3.2** |
| Multi-client DAW + MIDI-OX (SM-7) | **3.3** — [`smoke-epic3-multiclient-mt4.md`](smoke-epic3-multiclient-mt4.md) |
| Noms stables pour deux MT4 | **3.4** — [`smoke-epic3-dual-mt4-mt4.md`](smoke-epic3-dual-mt4-mt4.md) |
| Packaging Auto-Start dans l’installeur public | **4.1** |
| Doc utilisateur polie | **4.2** |
| Mesures de latence MIDI | Epic **5** |

### Références (fichier source de vérité)

- Epics Story 3.1
- PRD FR-3 / NFR-D2 / SM-5 (partie Auto-Start)
- Architecture AD-10, AD-12 items (6)(7), AD-20
- SPEC CAP-3

## Prérequis

- Epic 1–2 déjà OK sur cette session Windows (notes/CC + bind WinUSB)
- VirtualMIDI installé pour les lignes qui exigent des ports ; la ligne 6 le retire / désactive volontairement
- `Bridge.exe` compilé sous `builds/` (ex. `builds/debug`)
- Enregistrement / désenregistrement / auto-session en utilisateur **standard** (non-admin) pour la claim « quotidien »

## Comment noter

- **✅** / **❌** / **N/A** (+ courte raison) — ou Pass / Fail / N/A dans le tableau
- Case vide = non joué (**ne compte pas** comme Pass)
- Win10 x64 **obligatoire** pour clore le claim labo ; Win11 x64 en plus quand dispo

## Matrice Pass / Fail

| # | Vérification | Win10 x64 | Win11 x64 | Notes |
|---|---|---|---|---|
| 1 | Enregistrement Auto-Start en non-admin : `Bridge.exe --register-auto-start` exit 0 ; stdout anglais montre **soit** le nom de tâche Planificateur **soit** la valeur HKCU Run, plus le chemin absolu + `--auto-session` ; **pas d’UAC**. Run seul après refus du Planificateur = Pass | | | |
| 2 | Déconnexion / reconnexion **ou** reboot avec MT4 déjà branché → ports virtuels sans lancer le Bridge à la main | | | |
| 3 | Après login sans MT4, brancher le MT4 → ports sans lancement manuel (attente / rescan ≤ 900 s) | | | |
| 4 | Processus en session utilisateur (Gestionnaire des tâches → Détails : `Bridge.exe` sous l’utilisateur connecté ; **pas** de service Session-0) | | | |
| 5 | Pas de prompt UAC au lancement Auto-Start quotidien | | | |
| 6 | VirtualMIDI retiré / désactivé : échec fermé + message de correction en anglais (pas de succès vide silencieux) | | | |
| 7 | `Bridge.exe --unregister-auto-start` puis logon → le Bridge **ne** démarre **pas** | | | |

## Commandes (référence)

```text
builds\debug\Bridge.exe --register-auto-start
builds\debug\Bridge.exe --unregister-auto-start
builds\debug\Bridge.exe --auto-session
builds\debug\Bridge.exe --start-session
builds\debug\Bridge.exe --test-mapper
builds\debug\Bridge.exe --test-port-names
```

## Hors scope pour ce smoke

- Dire que SM-5 est entièrement clos (installeur + doc utilisateur polie)
- Récupération hot-plug après ports déjà live → **3.2** ([`smoke-epic3-hotplug-mt4.md`](smoke-epic3-hotplug-mt4.md))
- Câblage MSI de l’installeur public → **4.1**

## Docs liées

- Hot-plug (3.2) : [`smoke-epic3-hotplug-mt4.md`](smoke-epic3-hotplug-mt4.md)
- Multi-client DAW + MIDI-OX (3.3) : [`smoke-epic3-multiclient-mt4.md`](smoke-epic3-multiclient-mt4.md)
- Bind WinUSB (admin une fois) : [`../dev/winusb-bind.md`](../dev/winusb-bind.md)
- Smoke transport Epic 2 : [`smoke-epic2-mt4.md`](smoke-epic2-mt4.md)
