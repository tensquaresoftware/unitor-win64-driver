# Prompt — Revue de code transverse Epic 1 (MT4 MIDI de base)

Copier-coller tel quel dans un **nouveau chat Cursor** (contexte frais), avec le skill **bmad-code-review** (ou revue adversariale / edge-case si tu préfères).  
Projet : `C:\Users\Guillaume\Dev\Projects\unitor-win64-driver`  
Branche typique : `main` (dernier commit utile côté Bridge : `7d34b63` ou plus récent).

---

## Contexte de la Story

On cherche les failles **entre** les stories d’Epic 1 — le tuyau WinUSB → session → virtualMIDI — qui pourraient encore faire perdre une réponse MIDI/SysEx rare, alors que chaque story prise seule semblait OK.

---

## Qui tu es / comment parler

- Agent BMad développeur / reviewer sur **unitor-win64-driver**.
- Réponds en **français clair**, intention produit d’abord (ce que ça change pour l’utilisateur MIDI).
- Garde les noms matériel/protocole en anglais : **MT4**, **WinUSB**, **virtualMIDI**, **MIDI**, **SysEx**, **Matrix**.
- Pas de jargon BMad brut non glosé (`AC#…`, `oneshot`, etc.).
- Chemins / symboles C++ : en bas ou en liste, pas dans la phrase porteuse de sens.
- **Pas de commit** sauf demande explicite de Guillaume.

Applique aussi : `.cursor/rules/bmad-clarity-bar.mdc`, `conventions.md` §3, `scripts/quality/lint-touched.py` si tu modifies du C++.

---

## Pourquoi cette revue (preuve lab)

Overnight Windows Matrix (~8 h, 2026-08-07/08) :

- Mid (patch/master calmes) ≈ **95,6 %** de cycles OK.
- Banque (100 dumps d’affilée) ≈ **84 %** de cycles OK.
- Échec typique : **une seule** réponse dump manquante (`TIMEOUT` « aucune trame »), souvent **99/100** en banque — **pas** un crash Bridge, **pas** (surtout) une troncature au milieu.
- Journal : `tests/lab-logs/overnight-matrix/overnight-20260807T222620Z.log`

Hypothèse de travail : les revues **story par story** ont manqué des joints transverses (verrous, files, threads WinUSB, livraison virtualMIDI).

Priorité produit (accord Guillaume) : **stabiliser les SysEx Matrix « courts » avant le palier 3 SysEx longs**.

Cette nuit (prévu) : overnight **8 h sur Mac M5** (pilote Apple, mêmes labs mid + bank) comme contrôle matériel — ne bloque pas cette revue.

---

## Objectif Epic 1

Epic 1 = « First Working MT4 MIDI » (statut sprint : **done**).

But produit : binder la MT4 en WinUSB, lancer le Bridge C++17, voir des ports stables **MT4 In/Out N** (2 IN / 4 OUT), notes/CC aller-retour — sans pilote kernel custom.

Stories (fichier sprint / epics) :

1. Scaffold Bridge + build gate  
2. DeviceProfile déclaratif MT4  
3. Ouverture transport WinUSB  
4. DeviceSession + demux câbles Emagic  
5. Backend virtualMIDI + noms de ports  
6. Notes/CC sur tous les ports  

SSOT epics : `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md`  
Sprint : `_bmad-output/implementation-artifacts/sprint-status.yaml`

---

## Mission (revue transverse — pas une relecture story isolée)

Fais une **revue de code transversale** d’Epic 1 : cherche les faiblesses **aux interfaces** entre modules, les courses (threads), les files qui débordent en silence, les états qui restent collés, les chemins d’erreur qui avalent une trame sans la rendre visible.

### Focus (dans l’ordre)

1. **WinUSB bulk IN asynchrone** : anneau, thread de completion, file de livraison vers le reader — une URB / un paquet peut-il disparaître ou être livré hors ordre sous charge ?
2. **DeviceSession** : qui tient le verrou USB pendant Write vs Read ; le demux Emagic (câble courant, marqueur de port) ; ce qui se passe si l’entrée USB n’est pas vidée assez vite pendant une sortie longue ou répétée.
3. **virtualMIDI / SendToHost** : peut-on appeler depuis un mauvais thread ? file / drop silencieux côté host ?
4. **Noms / mapping ports** : In1 ↔ câble 0, etc. — confusion possible (rappel : écho DIN Out2 sans marqueur de port tombe encore sur In1 — hors Matrix overnight, mais lié au demux Epic 1).
5. **Arrêt / Start de session** : résidus qui font rater le *premier* échange après un Start frais.

### Hors focus (ne pas élargir)

- Installateur, multi-MT4, hot-plug Epic 3+, docs marketing.
- Refonte « pour plus tard » AMT8/Unitor8 hors DeviceProfile.
- Réécrire toute la dette historique hors trou prouvé.

### Livrable attendu

1. **Bilan court** (français clair) : 5–10 findings max, classés :
   - **Bloquant** (peut expliquer un timeout « aucune trame » rare)  
   - **Sérieux** (risque sous stress / overnight)  
   - **Léger** (qualité / lisibilité, pas la chasse actuelle)  
2. Pour chaque finding : symptôme utilisateur possible, où dans le code (chemin + symbole), pourquoi les revues story ont pu le rater, **piste de fix minimale** (pas un redesign).  
3. **Top 3 actions** pour la suite (repro lab + patch), alignées « SysEx courts d’abord ».  
4. Si tu proposes du code : patch **minimal**, `lint-touched.py` clean, **pas de commit** sans demande.

### Méthode suggérée

- Lire l’intent Epic 1 dans `epics.md` + stories `1-*` sous `_bmad-output/implementation-artifacts/` si présentes.  
- Parcourir surtout : `src/Usb/`, `src/Device/`, `src/Protocol/EmagicCableMapper*`, `src/Midi/`.  
- Croiser avec le symptôme overnight (timeout dump, `bridge_fail` souvent à 0).  
- Utiliser les skills review du repo (`bmad-code-review`, edge-case, adversariale) **en mode transverse Epic 1**, pas « diff de la dernière story seule ».

### Commande d’amorce (exemple)

```text
/bmad-code-review

Revue de code TRANSPARENTALE Epic 1 (MT4 MIDI de base), pas une story isolée.
Focus : joints WinUSB async IN ↔ DeviceSession ↔ virtualMIDI qui peuvent perdre une trame rare (timeout « aucune réponse » overnight Matrix).
Lis le prompt complet collé depuis le Bureau : revue-code-transverse-epic-1-mt4-midi.md
Ne commit pas. Français clair.
```

---

## Rappel matériel / lab (si tu as besoin de repro)

- Windows BootCamp + Zadig + `builds/debug/Debug/Bridge.exe`  
- Matrix sur **MT4 In 1 / Out 1**  
- Labs : `scripts/lab/sysex-matrix-mid-loop.py`, `sysex-matrix-bank-loop.py`, lanceur `overnight-matrix-stress.py`  
- Ne pas confondre avec palier 3 longs (`sysex-long-loopback.py`) — **plus tard**.

Bonne revue.
