---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 3.4 — Two MT4 units with stable distinguishable names
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
lab_result: physical dual not proven (single MT4 lab; offline registry + naming contracts Pass)
---

# Guide de smoke — Epic 3.4 Deux MT4, noms stables et distincts

Ce guide te sert pour **Story 3.4** : deux interfaces MT4 sur le même PC, chacune avec sa propre session et un jeu de ports clairement nommé, pour que les DAW se souviennent des bons ports après relaunch / replug.

Il est calqué sur les guides Epic 3 : français, résultat **au fil de l’eau**. Les noms affichés dans la matrice sont les **vrais** noms Windows (`MT4 In/Out` / `MT4 #K In/Out`), pas le raccourci planning `MT4 Port N`.

**Barème d’honnêteté :** case vide ≠ Pass. Si tu n’as **qu’un** MT4 physique, tu peux Passer les preuves **offline / simulées** (registre + noms) — mais tu **dois** écrire clairement que le dual physique n’est **pas** prouvé. Ne coche pas Pass physique avec une seule boîte.

## Contrat dual-MT4 V1 (ce que tu valides)

| Sujet | Contrat |
|---|---|
| Hôte produit | `Bridge.exe --auto-session` (ou `--start-session` pour un labo one-shot). Processus de session utilisateur seulement |
| Une boîte = une session | Chaque MT4 a son propre WinUSB, son mapper, son jeu de ports VirtualMIDI — **pas** de handle USB partagé, **pas** d’un seul backend pour deux boîtes |
| Noms unit 1 (`K==1`) | `MT4 In N` / `MT4 Out N` (silkscreen In/Out) |
| Noms unit K≥2 | `MT4 #K In N` / `MT4 #K Out N` exactement |
| Qui choisit `K` | Uniquement le registre d’identité du Bridge (serial USB préféré ; chemin d’instance / topologie en secours). Les backends MIDI reçoivent des noms déjà prêts |
| Stabilité | Débrancher l’unité A **ne** renumérote **pas** l’unité B. Replug / relaunch Bridge → mêmes `K` pour les identités déjà connues |
| Cascade Emagic | **Hors scope** — ne pas claim stacked / cascade multi-chassis |
| AQ-1 | Observation lab seulement (serial utilisable ou topologie primaire) — **ne** bloque **pas** le design |

### Hors scope (stories suivantes / autres)

| Sujet | Story |
|---|---|
| Chapitre multi-MT4 poli dans `docs/user/` | **4.2** — [`docs/user/unitor-mt4-bridge-user-guide.md`](../user/unitor-mt4-bridge-user-guide.md#two-mt4-interfaces) ; smoke [`smoke-epic4-user-docs-mt4.md`](smoke-epic4-user-docs-mt4.md) |
| Installateur public | **4.1** |
| Preuve multi-client (DAW + MIDI-OX) | **3.3** — [`smoke-epic3-multiclient-mt4.md`](smoke-epic3-multiclient-mt4.md) |
| Boucle hot-plug (étendue ici à N unités, pas une 2ᵉ lifecycle) | **3.2** — [`smoke-epic3-hotplug-mt4.md`](smoke-epic3-hotplug-mt4.md) |
| Auto-Start sans admin quotidien | **3.1** — [`smoke-epic3-autostart-mt4.md`](smoke-epic3-autostart-mt4.md) |
| Mesures de latence MIDI | Epic **5** |
| Backend Windows MIDI Services | hors V1 |
| Claim AMT8 / Unitor8 produit | hors V1 |

### Références (fichier source de vérité)

- Epics Story 3.4
- PRD FR-5 / FR-10 / UJ-3 / SM-8
- Architecture AD-4 / AD-5 / AD-6 / AD-9
- SPEC CAP-5 / CAP-10
- AQ-1 reporté (notes lab seulement)

## Prérequis

- Stories **3.1** / **3.2** / **3.3** OK sur cette machine si tu valides le chemin produit complet
- Epic 1–2 déjà OK (notes/CC + bind WinUSB)
- VirtualMIDI installé
- `Bridge.exe` sous `builds/` (ex. `builds/debug`)
- Idéalement **deux** MT4 WinUSB bindés avec le GUID projet ; sinon un seul + preuves offline / simulation d’identités
- Le mode lab `--dev-zadig` reste **une seule** boîte (énumération GUID projet seulement). Le dual produit se valide avec le bind WinUSB du projet, pas Zadig multi-instance

## Comment noter

- **✅** / **❌** / **N/A** (+ courte raison)
- Case vide = non joué (**≠** Pass)
- Win10 x64 **obligatoire** pour clore un claim **physique** dual ; Win11 x64 en plus quand dispo
- Ligne « physical dual » : **Pass seulement** avec deux boîtes réellement branchées

## Matrice Pass / Fail (SM-8 / FR-10)

| # | Vérification | Win10 x64 | Win11 x64 | Notes |
|---|---|---|---|---|
| 1 | Deux MT4 connectés (ou chemin dual simulé documenté) → Bridge `--auto-session` | | | |
| 2 | Les hôtes listent deux jeux distincts : unit1 `MT4 In/Out N` ; unit2 `MT4 #2 In/Out N` (noms **live**) | | | |
| 3 | Activité MIDI légère sur un câble par unité → sessions indépendantes (pas de cross-wire) | | | |
| 4 | Débrancher unit 2 seulement → unit 1 noms/`K` inchangés ; rebrancher unit 2 → mêmes noms `#2` | | | |
| 5 | Relancer Bridge avec les deux branchés → mêmes `K` pour identités connues | | | |
| 6 | **Négatif :** claim cascade / stacked Emagic = Fail / hors scope | | | |
| 7 | **Honnêteté :** un seul MT4 physique → documenter « physical dual not proven » ; ne pas Pass physique | ✅ | | Voir front matter `lab_result` |
| 8 | AQ-1 : noter quelle clé d’identité était primaire (serial vs topology) | | | Offline registry contracts Pass ; hardware key TBD |

## Preuves offline (sans deux boîtes)

Ces lignes **ne** remplacent **pas** un Pass physique dual :

```text
builds\debug\Bridge.exe --test-port-names
ctest --test-dir builds\debug -R BridgeTests
```

Attendu : noms K=1 / K=2 exacts ; registre « known unit keeps K when peer leaves ».

## Commandes (référence)

```text
builds\debug\Bridge.exe --auto-session
builds\debug\Bridge.exe --start-session
builds\debug\Bridge.exe --test-port-names
builds\debug\Bridge.exe --test-mapper
```

**Drill produit (deux MT4) :** démarre `--auto-session`, confirme les diagnostics `Unit K=` + noms live, ouvre un hôte MIDI, vérifie les deux jeux, débranche unit 2, confirme unit 1 intact, rebranche unit 2, confirme `#2` inchangé, relance Bridge.

**Registre persistant :** `%LOCALAPPDATA%\unitor-win64-driver\unit-identity-registry.txt` (identité → `K`).

## Notes AQ-1 (lab)

| Date | OS | Clé primaire observée | Serial utilisable ? | Notes |
|---|---|---|---|---|
| 2026-08-10 | Win10 | *(à remplir au premier dual physique)* | | Design : serial préféré ; topology fallback toujours disponible |

## Docs liées

- Manuel utilisateur multi-MT4 : [`docs/user/unitor-mt4-bridge-user-guide.md`](../user/unitor-mt4-bridge-user-guide.md#two-mt4-interfaces)
- Auto-Start (3.1) : [`smoke-epic3-autostart-mt4.md`](smoke-epic3-autostart-mt4.md)
- Hot-plug (3.2) : [`smoke-epic3-hotplug-mt4.md`](smoke-epic3-hotplug-mt4.md)
- Multi-client (3.3) : [`smoke-epic3-multiclient-mt4.md`](smoke-epic3-multiclient-mt4.md)
- Bind WinUSB (admin une fois) : [`../dev/winusb-bind.md`](../dev/winusb-bind.md)
