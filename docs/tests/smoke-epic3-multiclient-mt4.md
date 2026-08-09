---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 3.3 — Multi-client DAW + MIDI-OX (MT4)
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
lab_result: Win10 Pass (Live 12 + MIDI-OX)
---

# Guide de smoke — Epic 3.3 Multi-client DAW + MIDI-OX (MT4 sous Windows)

Ce guide te sert pour **Story 3.3** : une DAW de la matrice (Ableton Live 12 ou Reason Studios 12) **et** **MIDI-OX** ouvrent les **mêmes** ports virtuels en même temps, et les deux voient le MIDI — sans que le Bridge impose un verrou « un seul client ».

Il est calqué sur les guides Epic 1–2 : français, résultat **au fil de l’eau**.

**Barème d’honnêteté :** case vide ≠ Pass. **MIDI-OX** est l’utilitaire multi-client V1 de la Validation Matrix. **ShowMIDI** et **MidiView** sont **retirés** (faiblesses lab / BSOD MidiView) — ne les rouvre pas pour SM-7. Remplacer MIDI-OX par un autre utilitaire = **changement de PRD**. Si MIDI-OX n’est pas dispo, arrête-toi et escalade.

## Contrat multi-client V1 (ce que tu valides)

| Sujet | Contrat |
|---|---|
| Hôte produit | `Bridge.exe --auto-session` (ou `--start-session` pour un labo one-shot). Processus de session utilisateur seulement (AD-20) |
| Qui gère le multi-client | **VirtualMIDI** (teVirtualMIDI) autorise plusieurs apps sur le même port virtuel en parallèle |
| Plafond | Jusqu’à **8** applications concurrentes par port VirtualMIDI (AD-8 + docs auteur Tobias Erichsen). Le Bridge **n’invente pas** une limite plus basse de son côté |
| Politique Bridge | Le Bridge **ne** doit **pas** ajouter d’exclusive-open / « le premier qui ouvre gagne » / refus au compteur de clients par-dessus VirtualMIDI (AD-8) |
| Flags de création | Faces IN : `PARSE_TX \| INSTANTIATE_TX` ; faces OUT : `PARSE_RX \| INSTANTIATE_RX` seulement — pas de bit exclusive-open |
| Cycle de vie des ports | Seule une `DeviceSession` live crée / détruit le jeu de ports (AD-9). Le multi-client = ouverture côté apps des ports déjà créés — l’App n’appelle jamais Create/DestroyPortSet |
| Noms affichés (K=1) | Utilise les **vrais** noms : `MT4 In N` / `MT4 Out N` (pas seulement le raccourci AD-5 `MT4 Port N`) |
| Ports concernés (minimum) | Au moins **`MT4 In 1`** et **`MT4 Out 1`** ouverts dans **les deux** hôtes. Tu peux en ouvrir plus ; le minimum pour un Pass SM-7 est cette paire |
| Hôtes concurrents | Ableton Live 12 **ou** Reason Studios 12 **et** MIDI-OX sur les mêmes ports ; les deux restent ouverts ; les deux voient le MIDI. **Ordre d’ouverture libre** (DAW puis MIDI-OX, ou l’inverse) |
| Négatif | Verrou exclusif / « port in use » causé par une **politique Bridge** = **Fail**. Si le refus vient d’un mode exclusif de la DAW / de l’hôte (ou Win11 / Windows MIDI Services) → **pas** un Fail Bridge : note en ligne 6 / tableau bizarreries, ligne 5 = **N/A** |
| Bizarreries hôte | À documenter si tu les vois (rescan, pairing IN/OUT bizarre, Win11 / Windows MIDI Services). Elles **ne** changent **pas** AD-8 |
| Ouverts à côté | **AQ-3** (pin SDK) et **AQ-4** (coexistence Win11 / Windows MIDI Services) — note les bizarreries ; **ne** bloque **pas** un Pass SM-7 sur le pin AQ-3 |

### Hors scope (stories suivantes)

| Sujet | Story |
|---|---|
| Noms / persistance pour deux MT4 | **3.4** |
| Chapitre multi-client poli dans `docs/user/` | **4.2** |
| Installateur public / redistribuable VirtualMIDI | **4.1** / OQ-1 |
| Mesures de latence MIDI | Epic **5** |
| Boucle de recréation hot-plug | **3.2** (ne pas rouvrir) |
| Backend V1 sur Windows MIDI Services | hors V1 (AQ-4) |
| Remplacer MIDI-OX (ShowMIDI / MidiView / UI loopMIDI) | changement de PRD requis — ShowMIDI / MidiView déjà retirés |

### Références (fichier source de vérité)

- Epics Story 3.3
- PRD FR-9 / SM-7 / UJ-2
- Architecture **AD-8** (OQ-7 clos) — `ARCHITECTURE-SPINE.md` : jusqu’à **8** clients/port ; Bridge sans exclusive-open ; AD-7, AD-9
- SPEC CAP-9
- Docs auteur VirtualMIDI (Tobias Erichsen / tobias-erichsen.de) cités par AD-8

## Prérequis

- Stories **3.1** Auto-Start et **3.2** hot-plug OK sur cette session si tu utilises le chemin produit
- Epic 1–2 déjà OK (notes/CC + bind WinUSB)
- VirtualMIDI installé (loopMIDI / rtpMIDI pour que `teVirtualMIDI.dll` soit là)
- `Bridge.exe` sous `builds/` (ex. `builds/debug`)
- Ableton Live 12 **ou** Reason Studios 12 dispo
- **MIDI-OX** dispo (obligatoire — pas de substitut sans changement de PRD ; pas ShowMIDI / MidiView)
- Dans la DAW : désactive les modes MIDI **exclusive / takeover / « un seul client »** sur les ports sous test si l’option existe — sinon le second hôte peut voir « port in use » **sans** faute Bridge

## Comment noter

- **✅** / **❌** / **N/A** (+ courte raison)
- Case vide = non joué (**≠** Pass)
- Win10 x64 **obligatoire** pour clore le claim ; Win11 x64 en plus quand dispo

### Attribuer un « port in use » (ligne 5)

1. Les deux hôtes sont ouverts sur `MT4 In 1` / `MT4 Out 1` (au minimum) ?
2. Si refus : ferme **MIDI-OX** (ou la DAW), réessaie l’autre seul — si un hôte seul ouvre OK, ce n’est **pas** un verrou Bridge à la création des ports.
3. Vérifie les prefs exclusive/takeover de la DAW (voir Prérequis).
4. Si le Bridge tourne et les ports existent, mais le second hôte est refusé **uniquement** à cause d’un mode hôte / WMS → ligne 5 **N/A**, documente en ligne 6.
5. Fail ligne 5 **seulement** si tu as une preuve que le **Bridge** impose l’exclusivité (politique / refus côté Bridge), pas VirtualMIDI multi-client.

## Matrice Pass / Fail (SM-7)

| # | Vérification | Win10 x64 | Win11 x64 | Notes |
|---|---|---|---|---|
| 1 | Bridge live (`--auto-session` ou `--start-session`) → ports visibles (`MT4 In N` / `MT4 Out N` pour K=1) | ✅ | | 2026-08-10 |
| 2 | Ouvrir Ableton Live 12 **ou** Reason Studios 12 sur au moins `MT4 In 1` + `MT4 Out 1` | ✅ | | Live 12 |
| 3 | Ouvrir MIDI-OX sur les **mêmes** ports (ordre libre ; les deux restent ouverts) | ✅ | | Utilitaire V1 |
| 4 | Générer du MIDI (notes/CC sur au moins un chemin IN et/ou OUT) → **les deux** hôtes voient l’activité | ✅ | | C4 Out 2 + observation concurrente Live + MIDI-OX |
| 5 | **Négatif :** « port in use » / verrou **dû à une politique Bridge** = **Fail** ; exclusivité hôte / WMS = **N/A** + ligne 6 | ✅ | | Pas de verrou Bridge observé |
| 6 | Noter toute bizarrerie hôte (rescan, pairing IN/OUT, Win11/WMS) — ça **ne** change **pas** AD-8 | ✅ | | Voir tableau bizarreries |

## Commandes (référence)

```text
builds\debug\Bridge.exe --auto-session
builds\debug\Bridge.exe --start-session
builds\debug\Bridge.exe --test-mapper
builds\debug\Bridge.exe --test-port-names
```

**Drill produit :** démarre avec `--auto-session`, confirme `MT4 In N` / `MT4 Out N`, ouvre la DAW **ou** MIDI-OX en premier (ordre libre) sur au moins `MT4 In 1` + `MT4 Out 1`, ouvre le second hôte sur les mêmes, envoie / observe du MIDI, confirme que les deux voient l’activité et qu’aucun n’a été refusé par le Bridge.

**Si le MT4 est débranché pendant la matrice :** arrête SM-7 (ne note pas Pass/Fail ambigu). Termine d’abord la recovery **3.2**, puis rejoue cette matrice à froid.

**Labo one-shot :** `--start-session` convient pour un essai unique. La recréation hot-plug avec hôtes déjà ouverts reste Story **3.2**.

## Bizarreries observées (à remplir si tu en vois)

| Date | Hôte / OS | Bizarrerie | Change AD-8 ? |
|---|---|---|---|
| 2026-08-10 | Live 12 + MIDI-OX / Win10 | ShowMIDI et MidiView retirés de la matrice (faiblesses / BSOD). Lab multi-client Pass avec **MIDI-OX**. | **Non** |

## Docs liées

- Auto-Start (3.1) : [`smoke-epic3-autostart-mt4.md`](smoke-epic3-autostart-mt4.md)
- Hot-plug (3.2) : [`smoke-epic3-hotplug-mt4.md`](smoke-epic3-hotplug-mt4.md)
- Ownership longévité : [`checklists/smoke-epic2-longevity-mt4.md`](checklists/smoke-epic2-longevity-mt4.md)
- Smoke transport Epic 2 : [`smoke-epic2-mt4.md`](smoke-epic2-mt4.md)
- Bind WinUSB (admin une fois) : [`../dev/winusb-bind.md`](../dev/winusb-bind.md)
