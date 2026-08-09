---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 3.3 — Multi-client DAW + ShowMIDI (MT4)
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
---

# Guide de smoke — Epic 3.3 Multi-client DAW + ShowMIDI (MT4 sous Windows)

Ce guide te sert pour **Story 3.3** : une DAW de la matrice (Ableton Live 12 ou Reason Studios 12) **et** ShowMIDI ouvrent les **mêmes** ports virtuels en même temps, et les deux voient le MIDI — sans que le Bridge impose un verrou « un seul client ».

Il est calqué sur les guides Epic 1–2 : français, résultat **au fil de l’eau**.

**Barème d’honnêteté :** case vide ≠ Pass. Remplacer ShowMIDI par un autre utilitaire = **changement de PRD**, pas une réécriture silencieuse de la story. MidiView a déjà provoqué un BSOD sur cette machine — **SM-7 exige toujours ShowMIDI**. Si ShowMIDI n’est pas dispo, arrête-toi et escalade (n’invente pas de substitut).

## Contrat multi-client V1 (ce que tu valides)

| Sujet | Contrat |
|---|---|
| Hôte produit | `Bridge.exe --auto-session` (ou `--start-session` pour un labo one-shot). Processus de session utilisateur seulement (AD-20) |
| Qui gère le multi-client | **VirtualMIDI** (teVirtualMIDI) autorise plusieurs apps sur le même port virtuel en parallèle |
| Plafond | Jusqu’à **8** applications concurrentes par port VirtualMIDI (docs auteur Tobias Erichsen / AD-8). Le Bridge **n’invente pas** une limite plus basse de son côté |
| Politique Bridge | Le Bridge **ne** doit **pas** ajouter d’exclusive-open / « le premier qui ouvre gagne » / refus au compteur de clients par-dessus VirtualMIDI (AD-8) |
| Flags de création | Faces IN : `PARSE_TX \| INSTANTIATE_TX` ; faces OUT : `PARSE_RX \| INSTANTIATE_RX` seulement — pas de bit exclusive-open |
| Cycle de vie des ports | Seule une `DeviceSession` live crée / détruit le jeu de ports (AD-9). Le multi-client = ouverture côté apps des ports déjà créés — l’App n’appelle jamais Create/DestroyPortSet |
| Noms affichés (K=1) | Utilise les **vrais** noms : `MT4 In N` / `MT4 Out N` (pas seulement le raccourci AD-5 `MT4 Port N`) |
| Hôtes concurrents | Ableton Live 12 **ou** Reason Studios 12 **et** ShowMIDI sur les mêmes ports ; les deux restent ouverts ; les deux voient le MIDI |
| Négatif | Verrou exclusif / « port in use » causé par une **politique Bridge** = **Fail** |
| Bizarreries hôte | À documenter si tu les vois (rescan, pairing IN/OUT bizarre, Win11 / Windows MIDI Services). Elles **ne** changent **pas** AD-8 |
| Ouverts à côté | **AQ-3** (pin SDK) et **AQ-4** (coexistence Win11 / Windows MIDI Services) — note les bizarreries ; **ne** bloque **pas** un Pass SM-7 sur le pin AQ-3 |

### Hors scope (stories suivantes)

| Sujet | Story |
|---|---|
| Noms / persistance pour deux MT4 | **3.4** |
| Chapitre multi-client poli dans `docs/user/` | **4.2** |
| Installateur public | **4.1** |
| Mesures de latence MIDI | Epic **5** |
| Boucle de recréation hot-plug | **3.2** (ne pas rouvrir) |

### Références (fichier source de vérité)

- Epics Story 3.3
- PRD FR-9 / SM-7 / UJ-2
- Architecture AD-8 (OQ-7 clos), AD-7, AD-9
- SPEC CAP-9
- Docs auteur VirtualMIDI : multi-client ≤8 apps/port (tobias-erichsen.de)

## Prérequis

- Stories **3.1** Auto-Start et **3.2** hot-plug OK sur cette session si tu utilises le chemin produit
- Epic 1–2 déjà OK (notes/CC + bind WinUSB)
- VirtualMIDI installé (loopMIDI / rtpMIDI pour que `teVirtualMIDI.dll` soit là)
- `Bridge.exe` sous `builds/` (ex. `builds/debug`)
- Ableton Live 12 **ou** Reason Studios 12 dispo
- **ShowMIDI** dispo (obligatoire — pas de substitut sans changement de PRD)

## Comment noter

- **✅** / **❌** / **N/A** (+ courte raison)
- Case vide = non joué (**≠** Pass)
- Win10 x64 **obligatoire** pour clore le claim ; Win11 x64 en plus quand dispo

## Matrice Pass / Fail (SM-7)

| # | Vérification | Win10 x64 | Win11 x64 | Notes |
|---|---|---|---|---|
| 1 | Bridge live (`--auto-session` ou `--start-session`) → ports visibles (`MT4 In N` / `MT4 Out N` pour K=1) | | | |
| 2 | Ouvrir Ableton Live 12 **ou** Reason Studios 12 sur les mêmes ports concernés | | | |
| 3 | Ouvrir ShowMIDI sur les mêmes ports (les deux restent ouverts en même temps) | | | |
| 4 | Générer du MIDI (notes/CC sur au moins un chemin IN et/ou OUT) → **les deux** hôtes voient l’activité | | | |
| 5 | **Négatif :** verrou exclusif / « port in use » dû à une politique Bridge = **Fail** | | | |
| 6 | Noter toute bizarrerie hôte (rescan, pairing IN/OUT, Win11/WMS) — ça **ne** change **pas** AD-8 | | | |

## Commandes (référence)

```text
builds\debug\Bridge.exe --auto-session
builds\debug\Bridge.exe --start-session
builds\debug\Bridge.exe --test-mapper
builds\debug\Bridge.exe --test-port-names
```

**Drill produit :** démarre avec `--auto-session`, confirme `MT4 In N` / `MT4 Out N`, ouvre la DAW sur ces ports, ouvre ShowMIDI sur les mêmes, envoie / observe du MIDI, confirme que les deux voient l’activité et qu’aucun n’a été refusé par le Bridge.

**Labo one-shot :** `--start-session` convient pour un essai unique. La recréation hot-plug avec hôtes déjà ouverts reste Story **3.2**.

## Bizarreries observées (à remplir si tu en vois)

| Date | Hôte / OS | Bizarrerie | Change AD-8 ? |
|---|---|---|---|
| | | | **Non** (les bizarreries ne changent jamais la règle) |

## Hors scope pour ce smoke

- Persistance / noms pour deux MT4 → **3.4**
- Chapitre utilisateur poli multi-client → **4.2**
- Installateur public / redistribuable VirtualMIDI → **4.1** / OQ-1
- Harness latence / jitter → Epic **5**
- Modifier la recréation hot-plug → **3.2**
- Passer le backend V1 sur Windows MIDI Services
- Remplacer ShowMIDI par MidiView / MIDI-OX / UI loopMIDI sans changement de PRD

## Docs liées

- Auto-Start (3.1) : [`smoke-epic3-autostart-mt4.md`](smoke-epic3-autostart-mt4.md)
- Hot-plug (3.2) : [`smoke-epic3-hotplug-mt4.md`](smoke-epic3-hotplug-mt4.md)
- Ownership longévité : [`checklists/smoke-epic2-longevity-mt4.md`](checklists/smoke-epic2-longevity-mt4.md)
- Smoke transport Epic 2 : [`smoke-epic2-mt4.md`](smoke-epic2-mt4.md)
- Bind WinUSB (admin une fois) : [`../dev/winusb-bind.md`](../dev/winusb-bind.md)
