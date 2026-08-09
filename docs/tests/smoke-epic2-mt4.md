---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 2 — MT4 (horloge, MTC, SysEx, Matrix-Control, longévité)
author: Guillaume DUPONT
created: 2026-08-05
updated: 2026-08-06
---


# Guide de smoke — Epic 2 (MT4 sous Windows)

Ce guide te sert pour **le labo matériel Epic 2** sur ton PC Boot Camp (Windows 10 Pro) avec le MT4 : horloge MIDI et transport, MTC, gros SysEx, Matrix-Control, et (si tu as le temps) une session longue d’environ 4 h.

Il est calqué sur [`smoke-epic1-mt4.md`](smoke-epic1-mt4.md) : français, étapes dans l’ordre, résultat **au fil de l’eau**.

Les checklists techniques anglaises par story (agents / matrice de validation) sont sous [`docs/tests/checklists/`](checklists/). **Celui-ci** est le guide opérateur pour toi.

**Comment l’utiliser**

- Suis les parties **dans l’ordre** (0 → 1 → 2 → …). Tu peux faire une soirée « courte » (parties 0–5) et reporter Matrix-Control / 4 h.
- **À chaque micro-étape**, juste après la flèche `→`, note le résultat **au moment où tu le constates** :
  - **✅** = OK
  - **❌** = problème
  - **N/A** = non applicable (+ une courte raison)
- Tu peux ajouter une remarque sur la **même ligne** après l’emoji (`→ ✅ — …`), ou sur la ligne suivante indentée avec **📌**.
- Si une étape échoue : **arrête-toi**, note ce que tu vois, puis va à « En cas de blocage ».
- Les cases à cocher markdown ne sont **pas** utilisées — uniquement ✅ / ❌ / N/A.
- Une seule session Bridge à la fois. Pour un essai valide : arrête avec **Ctrl+C** (pas la croix de la fenêtre).
- En bas de chaque grande partie, un sous-titre **Remarques libres 📌** (ex. `### 0.5`) reste dispo pour ce qui ne rentre pas dans une étape.

**Exemple de notation**

```text
1. Faire ceci. → ✅
2. Faire cela. → ❌ — MIDI-OX silencieux, câble OUT ?
3. Optionnel. → N/A — pas de Matrix-1000 ce soir
   📌 Live 12 n’envoie pas Continue dans cette config.
```

**Ce que ce smoke prouve (Epic 2)**

| Partie | Story | Ce que tu valides |
|---|---|---|
| 3 | 2.1 | Horloge MIDI (`F8`) + Start / Continue / Stop (`FA` / `FB` / `FC`) dans les deux sens |
| 4 | 2.2 | MTC quarter-frame (`F1`) + au moins un full-frame SysEx de sync |
| 5 | 2.3 | Gros SysEx (Inquiry, ~275 B, ~351 B) + petite rafale sans redémarrer le Bridge |
| 6 | 2.4 | Vecteurs minimum Matrix-Control (avec Matrix-1000 si possible) |
| 7 | 2.5 | Session continue ~4 h avec un peu de SysEx, sans redémarrage obligatoire |

**Hors scope** (ne pas les exiger ce labo)

- Latence / jitter « Studio-Done » → Epic 5.
- Hot-plug, multi-client DAW → Epic 3 (stories 3.2+).
- Auto-Start sans admin → [`smoke-epic3-autostart-mt4.md`](smoke-epic3-autostart-mt4.md) (story 3.1).
- Installateur public → Epic 4.
- Relier Matrix-Control **dans** le Bridge (interdit) — Matrix-Control reste une appli externe.
- Refaire tout le smoke notes/CC Epic 1 (un contrôle rapide suffit si Epic 1 est déjà vert).

**Prérequis fort**

- Smoke Epic 1 notes/CC **vert** sur au moins **1 IN + 1 OUT** (voir [`smoke-epic1-mt4.md`](smoke-epic1-mt4.md)).
- Même machine Boot Camp / même bind WinUSB que le labo Epic 1 (cas B + `--dev-zadig` si c’est encore ton cas).

**Retour labo** _(à remplir après la session — synthèse)_

| Étape | Résultat |
|---|---|
| OS / machine Bridge | MacBook Pro Intel i7 / Windows 10 Pro x64 (Boot Camp) + MT4 + Live 12 + MIDI-OX |
| Machine DIN / observateur | MacBook Pro M5 / macOS Tahoe + Scarlett 6i6 + Live 12 + MIDI Monitor |
| Bind | Cas A / Cas B + `--dev-zadig` : |
| Partie 1 `--test-mapper` | |
| Partie 2 session + ports | |
| Partie 3 horloge / transport | |
| Partie 4 MTC | ✅ harness 2026-08-09 (`mtc-loopback-lab.py` Out2→In2 + demux fix) |
| Partie 5 SysEx pipe | ✅ (MIDI-OX ; BSOD MidiView archivé) |
| Partie 6 Matrix-Control | |
| Partie 7 longévité ~4 h | fait / reporté / N/A |

---

## Setup labo GD (référence rapide)

Tu n’as **pas** de clavier / contrôleur avec prises MIDI DIN. L’**Oxygen 61** est USB uniquement : utile pour jouer des notes **dans** Ableton sur la machine où il est branché, **pas** pour alimenter directement un IN DIN du MT4.

Le montage qui marche pour toi (comme en Epic 1) :

| Rôle | Machine | Matériel / logiciel |
|---|---|---|
| **Bridge + MT4** | MacBook Pro **Intel** sous **Windows 10** (Boot Camp) | MT4 en USB, `Bridge.exe`, Ableton Live 12, **MIDI-OX**, loopMIDI / teVirtualMIDI |
| **Source / observateur DIN** | MacBook Pro **M5** (macOS) | Focusrite **Scarlett 6i6** (MIDI DIN IN + OUT), Ableton Live 12, **MIDI Monitor** |

**Câblage DIN (à laisser en place pour les parties 3–5)**

Choisis un numéro de port MT4 (ex. **Port 1**) et reste cohérent :

1. **Scarlett MIDI OUT** → câble DIN → **MT4 IN physique N**  
   (le Mac **envoie** vers le boîtier → Bridge → ports virtuels IN sur le PC)
2. **MT4 OUT physique N** → câble DIN → **Scarlett MIDI IN**  
   (le Bridge **envoie** depuis le PC → boîtier → le Mac **reçoit** / observe)

Tu peux utiliser **deux câbles DIN** en même temps (un dans chaque sens) pour éviter de rebrancher entre les essais.

**Deux sens, en français clair**

| Nom dans ce guide | Chaîne concrète | Ce que tu prouves |
|---|---|---|
| **Mac → PC** (boîtier → Bridge → apps PC) | Live Mac → Scarlett OUT → MT4 IN N → Bridge → `MT4 Input N` → Live PC / MIDI-OX | Ce qui **entre** dans le MT4 arrive bien dans Windows |
| **PC → Mac** (apps PC → Bridge → boîtier) | Live PC → `MT4 Output N` → MT4 OUT N → Scarlett IN → MIDI Monitor / Live Mac | Ce que Windows **envoie** sort bien du MT4 |

**Où regarder les octets**

- Sur le **Mac** : **MIDI Monitor** (excellent) sur le port MIDI de la Scarlett.
- Sur le **PC** : **MIDI-OX** (Display / log fichier) et/ou Ableton Live 12 sur `MT4 Input N` / `MT4 Output N`.
  - À partir de la **partie 5** (SysEx) : **MIDI-OX** uniquement pour observer et sauver le flux.
  - **MidiView** : **interdit** sur ce labo — BSOD `htmididriver64.sys` au SysEx ~275 B (2026-08-06). ShowMIDI : abandonné (pas de log fichier pratique).

**Oxygen 61**

- Branché en USB sur le **PC** : tu joues dans Live PC (notes locales) — ne traverse **pas** le MT4.
- Branché en USB sur le **Mac** : idem côté Mac.
- Pour des notes **à travers** le MT4 : envoie des clips / notes depuis **Live Mac** vers la Scarlett OUT (sens Mac → PC), ou depuis **Live PC** vers `MT4 Output N` (sens PC → Mac).

---

## 0. Préparer les deux machines et le matériel

### 0.1 Matériel

1. **PC Boot Camp** : Windows 10 64 bits (ligne **obligatoire** de la matrice). → ✅
2. **MT4** Emagic + USB sur le PC (port direct de préférence, pas hub externe). → ✅
3. **Mac M5** : Scarlett 6i6 branchée (drivers Focusrite OK), alimentation OK. → ✅
4. **Deux câbles MIDI DIN** (ou au minimum un, à rebrancher selon le sens). → ✅
5. (Optionnel) **Oxygen 61** en USB sur l’une des deux machines pour jouer des notes **dans** Live — pas obligatoire pour horloge / MTC. → 
6. Pour la partie **6 (Matrix-Control)** : **Matrix-1000** sur un câble MIDI du MT4. Sans Matrix-1000, tu fais quand même les parties 1–5 (et une partie de 7) avec le duo Mac/PC + Scarlett. → ✅
7. Écouteurs si tu veux entendre un retour — MIDI Monitor / MIDI-OX / Live suffisent pour **voir**. → 

### 0.2 Logiciels

**Sur le PC (Boot Camp)**

1. CMake + Visual Studio / Build Tools x64. → ✅
2. **VirtualMIDI** via **loopMIDI** (ou rtpMIDI). → ✅
3. **Ableton Live 12**. → ✅
4. **MIDI-OX** (observateur / log MIDI sur le PC). → ✅
   📌 MidiView testé puis **abandonné** (BSOD `htmididriver64.sys` en §5.2).
5. (Partie 6) **Matrix-Control** (appli externe). → ✅

**Sur le Mac M5**

1. **Ableton Live 12**. → ✅
2. **MIDI Monitor**. → ✅
3. Focusrite Control / pile audio Scarlett fonctionnelle (MIDI DIN visible dans Live et MIDI Monitor). → ✅

### 0.3 Code à tester (PC seulement)

1. Sync / pull du dépôt `unitor-win64-driver` sur le PC Windows — état qui contient **toutes** les stories Epic 2. → ✅
2. Terminal à la **racine** du dépôt (`CMakeLists.txt`, `src/`, `installer/`). → ✅
3. Note commit (`git rev-parse --short HEAD`) et/ou chemin de `Bridge.exe`. → ✅ — `49b294a` — `builds\debug\Debug\Bridge.exe`

### 0.4 Câbler Scarlett ↔ MT4

1. Scarlett MIDI **OUT** → MT4 **IN** physique N (ex. IN 1). → ✅
2. MT4 **OUT** physique N (ex. OUT 1) → Scarlett MIDI **IN**. → ✅
3. Sur le Mac : Live + MIDI Monitor voient bien le périphérique MIDI de la Scarlett (souvent nommé autour de « Scarlett 6i6 » / MIDI Port). → ✅
4. Note le **N** choisi : tu l’aligneras sur `MT4 Input N` / `MT4 Output N` côté PC. → ✅ — IN/OUT 1 → `MT4 Input 1` / `MT4 Output 1`

### 0.5 Remarques libres 📌



---

## 1. Compiler le Bridge et passer le contrôle sans matériel

Objectif : un `Bridge.exe` frais, et la preuve logicielle que le framer / mapper Epic 2 est bien dans ce build. **Uniquement sur le PC.**

### 1.1 Configure + build

À la racine du dépôt :

```text
cmake -S . -B builds/debug -A x64
cmake --build builds/debug --config Debug
```

(Tu peux utiliser `builds/release` + `--config Release` si tu préfères.)

Chemin typique :

```text
builds\debug\Debug\Bridge.exe
```

Sinon : `Get-ChildItem -Path builds -Recurse -Filter Bridge.exe`

1. Configure CMake OK. → ✅
2. Build OK. → ✅
3. Chemin `Bridge.exe` trouvé. → ✅ — `builds\debug\Debug\Bridge.exe`

### 1.2 Contrôle profil (sans USB)

```text
builds\debug\Debug\Bridge.exe
```

Attendu : sortie DeviceProfile, **exit 0**, pas d’erreur de bind WinUSB.

1. Sans argument : exit 0. → ✅

### 1.3 Contrôle synthétique Epic 2 (sans matériel)

```text
builds\debug\Debug\Bridge.exe --test-mapper
```

Attendu : **exit 0** (horloge / transport, MTC, vecteurs SysEx).

1. `--test-mapper` : exit 0. → ✅
   📌 Message : `Mapper synthetic tests passed`

### 1.4 (Recommandé) Tests automatisés C++

```text
ctest --test-dir builds/debug -C Debug
```

(ou `BridgeTests`). Attendu : tout passe.

1. `ctest` / BridgeTests. → ✅
   📌 Message : `100% tests passed out of 1`

### 1.5 Remarques libres 📌



---

## 2. Brancher le MT4, ouvrir l’USB, démarrer la session

Objectif : retrouver le chemin labo Epic 1 (WinUSB + ports `MT4 Input` / `MT4 Output`) avant horloge / SysEx.

**Rappel Boot Camp** : souvent **cas B** — nœud `USB\VID_086A&PID_0003` sans `MI_02` → Zadig + **`--dev-zadig`**. Détail : [`smoke-epic1-mt4.md`](smoke-epic1-mt4.md) sections 2–4.

### 2.1 Bind WinUSB (si pas déjà fait aujourd’hui)

1. Branche le MT4 sur le **PC**, attends la détection. → ✅
2. Gestionnaire de périphériques : WinUSB sur le nœud MT4. → ✅
3. Si rebinding : Epic 1 §2 (INF ou Zadig). Méthode utilisée : → ✅ — Zadig (cas B)

### 2.2 Ouverture USB seule (optionnel)

```text
builds\debug\Debug\Bridge.exe --open-device --dev-zadig
```

(Sans `--dev-zadig` en cas A / GUID.) Attendu : exit 0, `WinUSB open succeeded`.

1. Ouverture USB seule. → ✅
   📌 Message : `WinUSB open succeeded`

### 2.3 Lancer la session MIDI

**Important :** lance **uniquement** `--start-session` (ne pas enchaîner `--probe-usb` juste avant).

**Cas Zadig (Boot Camp typique) :**

```text
builds\debug\Debug\Bridge.exe --start-session --dev-zadig
```

**Cas INF / GUID :**

```text
builds\debug\Debug\Bridge.exe --start-session
```

(`--run-midi` = alias.) Laisse la fenêtre ouverte ; **Ctrl+C** pour arrêter.

Console attendue (libellés en anglais, normal) :

- Noms `MT4 Input 1` / `MT4 Output 1` → ✅ (je vois les deux entrées et les quatre sorties)
- `DeviceSession started for MT4 with Virtual Ports` → ✅
- MIDI I/O qui tourne → ✅ ("MIDI I/O Running")

**loopMIDI** ne liste en général **pas** ces ports — regarde **Ableton PC / MIDI-OX**. (GD : ports E/S MT4 OK dans Ableton Live)

1. Session démarrée, messages console OK. → ✅
2. Flag `--dev-zadig` utilisé ? → 

### 2.4 Vérifier les ports + contrôle notes/CC via Scarlett

Dans **Ableton PC** → Réglages → MIDI :

1. **2 entrées** visibles : `MT4 Input 1`, `MT4 Input 2`. → ✅
2. **4 sorties** visibles : `MT4 Output 1` … `MT4 Output 4`. → ✅
3. **Track** coché sur les ports que tu testes (Sync plus tard pour l’horloge). → ✅

**Contrôle rapide Mac → PC** (notes à travers le MT4) :

1. Sur le **Mac** : Live envoie des notes vers la **Scarlett MIDI OUT** (pas vers un port virtuel inventé). → ✅
2. Câble : Scarlett OUT → MT4 IN 1. → ✅
3. Sur le **PC** : MIDI-OX / Live sur `MT4 Input 1` → notes visibles. → ✅

**Contrôle rapide PC → Mac** :

1. Sur le **PC** : Live envoie des notes vers `MT4 Output 1` (Track coché). → ✅
2. Câble : MT4 OUT 1 → Scarlett IN. → ✅
3. Sur le **Mac** : MIDI Monitor (et/ou Live) sur l’entrée MIDI Scarlett → notes visibles. → ✅

**Mode Computer :** le Bridge envoie déjà le coup de pouce (CC) au démarrage de session. **Le SysEx seul ne réveille pas** le mode Computer — si le boîtier a été manipulé hors Bridge, relance une session propre.

### 2.5 Remarques libres 📌



---

## 3. Horloge MIDI et transport (story 2.1)

Objectif : Timing Clock + Start / Stop / Continue passent **sans trous** imputables au Bridge, dans les deux sens, sur une courte séance.

**Définition d’échec :** Live perd le sync, ou Start/Stop/Continue manque, **alors que** les notes circulent encore → ❌ Bridge.

**Astuce Ableton (les deux machines)** : dans Préférences → Link/Tempo/MIDI (libellés selon version), tu actives l’**envoi** ou la **réception** Sync / MIDI Clock **par port**. Sur le PC, le port concerné est `MT4 Input N` ou `MT4 Output N` selon le sens ; sur le Mac, le port MIDI de la **Scarlett**.

### 3.1 Sens PC → Mac (Live PC → MT4 OUT → Scarlett → MIDI Monitor / Live Mac)

1. Câble : **MT4 OUT N** → **Scarlett MIDI IN** (déjà en place si §0.4). → ✅
   📌 Port / câble : `MT4 Output …` / MT4 OUT … → Scarlett IN
2. Sur le **PC** : dans Live, coche **Sync** (sortie) sur `MT4 Output N` uniquement (évite d’arroser les 4 OUT au début). → ✅
3. Sur le **Mac** : ouvre **MIDI Monitor** sur l’entrée Scarlett ; optionnellement Live Mac en réception Sync sur la Scarlett si tu veux voir l’asservissement. → ✅
4. Lance le transport **Live PC** (**Play**), laisse tourner ~1–2 min, puis **Stop**. → ✅
5. **Continue** : si Live te le permet dans ta config, teste ; sinon **N/A** + raison. → ✅

Ce que tu dois voir côté Mac (MIDI Monitor) — note au fur et à mesure :

1. Rafale d’octets d’horloge (`F8`) pendant le Play. → ✅ (voir `tests/lab-logs/smoke-epic2/mtc-3.1.txt`)
2. `FA` au démarrage. → ✅
3. `FC` à l’arrêt. → ✅
4. `FB` si Continue disponible. → ✅
5. Pas de « trous » évidents sur cette courte séquence. → ✅
6. Contrôle notes encore OK sur le même OUT (clip notes Live PC → MIDI Monitor). → ✅
7. Live Mac asservi (si testé). → ✅

### 3.2 Sens Mac → PC (Live Mac → Scarlett → MT4 IN → Live PC / MIDI-OX)

1. Câble : **Scarlett MIDI OUT** → **MT4 IN N**. → ✅
   📌 Port / câble : Scarlett OUT → MT4 IN 1 → `MT4 Input 1`
2. Sur le **Mac** : Live envoie Sync / MIDI Clock **uniquement** vers la Scarlett MIDI OUT. → ✅
3. Sur le **PC** : Live / MIDI-OX sur `MT4 Input N` ; coche Sync **entrée** si tu veux que Live PC **suive** l’horloge du Mac. → ✅
   📌 Observateur PC : MIDI-OX + Live 12
4. Lance **Play** sur Live Mac, puis **Stop**. → ✅
5. Observe `F8` / `FA` / `FC` (et Continue si possible) dans MIDI-OX et/ou l’asservissement Live PC. → ✅
6. Contrôle notes Mac → PC encore OK sur le même IN. → ✅

Détail observation Mac → PC : (voir `tests/lab-logs/smoke-epic2/mtc-3.2.txt`)

1. Horloge observée ou Live PC asservi. → ✅
2. Start (`FA`). → ✅
3. Stop (`FC`). → ✅
4. Continue (`FB`). → ✅
5. Pas de trous Bridge. → ✅
6. Notes encore OK sur ce IN. → ✅

### 3.3 Stabilité courte (même session Bridge)

Sans redémarrer le Bridge : enchaîne 3.1 et 3.2 (ou au moins un sens complet). Console PC sans tempête WriteBulk / SendToHost.

1. Courte séance sans redémarrer le Bridge. → ✅
2. Console sans tempête d’erreurs. → ✅
3. Message d’échec éventuel (copier) : → ✅

### 3.4 Remarques libres 📌

Voir les logs MIDI (`tests/lab-logs/smoke-epic2/mtc-3.1.txt` et `mtc-3.2.txt`) pour la sync dans les deux sens.

---

## 4. MTC — timecode MIDI (story 2.2)

Objectif : le Bridge transporte le **MTC quarter-frame** et au moins un **full-frame** de sync, dans les deux sens, sans trous imputables au Bridge.

**En clair**

- **Quarter-frame** : `F1` + 1 octet (8 types pour une image SMPTE). MIDI Monitor les montre très clairement.
- **Full-frame** : SysEx `F0 7F … 01 01 hr mn sc fr F7` (souvent id `7F`) pour un **repère / locate** — ce n’est **pas** encore la preuve librarian (parties 5–6).

**Hors scope :** bits utilisateur SMPTE, MMC, générateur MTC dans le Bridge.

**Ableton :** active l’envoi / la réception de **MIDI Timecode (MTC)** sur le bon port (PC : `MT4 Input N` / `MT4 Output N` ; Mac : Scarlett). Ne confonds pas avec MIDI Clock (`F8`) de la partie 3.

### 4.1 Préparer

1. Même câblage Scarlett ↔ MT4 que §0.4. → ✅
2. Si possible, dédie `MT4 Output N` (et l’`MT4 Input` associé si besoin) au MTC pour ce test (le MTC est bavard). → ✅
3. **MIDI Monitor** sur le Mac = meilleur « microscope » pour `F1` et le SysEx full-frame. → ✅
4. MIDI-OX / Live PC prêts pour le sens Mac → PC. → ✅

### 4.2 Sens PC → Mac (Live PC envoie le MTC)

1. Live PC : envoi MTC activé sur `MT4 Output N`. → ✅
   📌 Port OUT / câble : 1
2. Mac : MIDI Monitor sur Scarlett IN (câble MT4 OUT N → Scarlett IN). → ✅
3. Lance le transport / le timecode Live PC assez longtemps pour voir des **quarter-frames** en continu. → je ne sais pas
4. Cherche aussi **au moins un full-frame** (souvent à un locate / re-sync — si Live n’en envoie pas facilement, note ce que tu obtiens ; l’absence de full-frame côté générateur Live ≠ automatiquement ❌ Bridge si les quarter-frames passent et qu’un full-frame d’une autre source passe). → je ne sais pas
5. Contrôle notes encore OK sur le même OUT. → ✅

Observations PC → Mac : Je te laisse inspecté mon log MIDI "MTC 4.2.txt"

1. Quarter-frame (`F1`) dans MIDI Monitor. → ?
2. Au moins un full-frame observé. → ?
3. Pas de trous Bridge. → ?
4. Notes encore OK. → ✅

### 4.3 Sens Mac → PC (Live Mac envoie le MTC)

1. Live Mac : envoi MTC vers Scarlett MIDI OUT. → ✅
2. Câble : Scarlett OUT → MT4 IN N. → ✅
   📌 Port IN / câble : 1
3. PC : MIDI-OX / Live sur `MT4 Input N` (réception MTC / observation). → ✅ — MIDI-OX, flux entrant OK (voir `tests/lab-logs/smoke-epic2/mtc-4.3.txt`)
4. Quarter-frames visibles sur le PC. → je ne sais pas
5. Au moins un full-frame si le Mac en envoie. → je ne sais pas
6. Notes encore OK. → ✅
7. Live PC asservi au MTC (si testé). → ✅

### 4.4 Stabilité courte

Même session Bridge ; pas de redémarrage pour « récupérer » le MTC ; console propre.

1. Courte sync sans redémarrer. → ✅
2. Console propre. → ✅

### 4.5 Remarques libres 📌

**Clôture 2026-08-09 (sans Scarlett) :** script `scripts/lab/mtc-loopback-lab.py --with-bridge`.
Topo réelle : câble rouge **Out 2 → In 2**. Un premier essai « voyait » In 1 à cause d’un bug demux Emagic (écho sans `F5` collé sur câble 0) — corrigé via `hintInCableFromOut` dans `EmagicCableMapper`.
Résultat sur In 2 : `qf=72/72` + `full_frame=1` — log `tests/lab-logs/mtc-loopback/mtc-loopback-20260809T213749Z.log`.
UAT Ableton/Scarlett reporté à un futur guide UAT dédié.

---

## 5. SysEx transparent + petite rafale (story 2.3)

Objectif : de **gros** messages SysEx passent complets dans les deux sens, et une **petite rafale** (plusieurs ~275 B) se termine **sans** redémarrer le Bridge.

Ce n’est **pas** encore Matrix-Control complet (partie 6) — ici on prouve le **tuyau + tampons**.

### Reprise labo (après BSOD MidiView) — lire avant de continuer

Tu t’étais arrêté au **Patch ~275 B** (§5.2). Voici comment reprendre **sans** rouvrir MidiView.

1. **N’ouvre pas MidiView.** Son pilote `htmididriver64.sys` a provoqué le BSOD (confirmé à l’écran + minidump).
2. Relance une session Bridge propre sur le PC :
   ```text
   builds\debug\Debug\Bridge.exe --start-session --dev-zadig
   ```
3. Ouvre **MIDI-OX** ; Options → MIDI Devices : sélectionne `MT4 Input 1` en **Input** (pour Mac → PC). Active le **Display** / log que tu sais exporter en fichier texte.
4. Sur le Mac : SysEx Librarian + Scarlett OUT → MT4 IN 1 (comme avant).
5. Reprends ci-dessous à **§5.2 étape 4** (Patch). Tu peux d’abord renvoyer l’Inquiry pour vérifier que MIDI-OX affiche bien les 6 octets, puis enchaîne Patch / Master.

**Outil PC pour cette partie (et la suite) : MIDI-OX**

- Un peu old-school, mais fiable pour **voir** le flux et **sauver** un log texte.
- Écoute `MT4 Input` / `MT4 Output` (IN pour Mac → PC).
- Peut aussi **envoyer** un `.syx` si besoin (Display / SysEx / Send).
- **MidiView** : interdit sur ce labo (BSOD). **ShowMIDI** : abandonné (pas de log fichier pratique).

### 5.1 Tailles à couvrir

| Scénario | Taille approx. | Commentaire |
|---|---|---|
| Device Inquiry | 6 B | `F0 7E 7F 06 01 F7` — `tests/fixtures/sysex/DeviceEnquiry.syx` |
| Réponse Inquiry | ~15 B | si un appareil répond — `DeviceEnquiryReply.syx` (à re-déposer si besoin) |
| Patch-shaped | ~275 B | `tests/fixtures/sysex/Patch.syx` (= Matrix-Control `PatchInit.syx`) |
| Master-shaped | ~351 B | `tests/fixtures/sysex/Master.syx` (= Matrix-Control `MasterInit.syx`) |
| Rafale courte | plusieurs ~275 B | rythme calme, **pas** un flood |

**Comment générer / envoyer sans Matrix-1000**

- **MIDI Monitor** (Mac) : excellent pour **voir** ; selon version / outils, tu peux aussi renvoyer des messages — pratique pour Inquiry.
- Fichiers `.syx` + **SysEx Librarian** (Mac) ou envoi depuis **MIDI-OX** (PC).
- Live n’est **pas** toujours le meilleur émetteur de gros SysEx : ne te bloque pas dessus — SysEx Librarian / MIDI-OX + observation **MIDI Monitor (Mac)** / **MIDI-OX (PC)** suffisent pour la partie 5.
- Si Matrix-Control + Matrix-1000 sont là : tu peux déjà t’en servir ici, sans exiger tous les vecteurs de la partie 6.

### 5.2 Sens Mac → PC (SysEx entre dans le MT4)

**Tentative 1 (MidiView) — interrompue par BSOD**

1. Session Bridge active (mode Computer OK). → ✅
2. Câble : Scarlett OUT → MT4 IN 1 ; MidiView écoutait `MT4 Input 1`. → ✅ (ne plus reproduire avec MidiView)
   📌 Port / câble / outil d’envoi : Port 1 / SysEx Librarian (Mac)
   📌 Log MidiView : non sauvé (crash avant) — Inquiry 6 B vu OK à l’écran
3. Inquiry (6 B) — trame complète dans MidiView. → ✅
4. Patch ~275 B (`Patch.syx`) — trame complète dans MidiView. → ❌ BSOD — `htmididriver64.sys` / MidiView.exe (minidump `080626-13078-01.dmp`)

**Tentative 2 (reprise) — MIDI-OX, sans MidiView**

1. Session Bridge relancée (`--start-session --dev-zadig`). → ✅
2. MIDI-OX écoute `MT4 Input 1` (Display / log fichier prêt). → ✅
   📌 Fichier log MIDI-OX : `tests/lab-logs/smoke-epic2/sysex-5.2.txt`
3. (Contrôle rapide) Renvoyer Inquiry (6 B) — trame complète dans MIDI-OX. → ✅
4. Envoyer `tests/fixtures/sysex/Patch.syx` (~275 B) depuis le Mac — trame **complète** dans MIDI-OX (pas de BSOD). → ✅
5. Envoyer `tests/fixtures/sysex/Master.syx` (~351 B) — trame complète dans MIDI-OX. → ✅

### 5.3 Sens PC → Mac (SysEx sort du MT4)

1. Envoie depuis le PC (**MIDI-OX** / outil SysEx / Live / Matrix-Control) vers `MT4 Output N`. → ✅
   📌 Port / câble / outil d’envoi : Port 1 / MIDI-OX
2. Câble : MT4 OUT N → Scarlett IN. → ✅
3. Inquiry : trame complète dans **MIDI Monitor** (Mac). → ✅
4. ~275 B : trame complète dans MIDI Monitor. → ✅
5. ~351 B : trame complète dans MIDI Monitor. → ✅
6. (Optionnel) MIDI-OX peut aussi logger ce que tu envoies / un chemin de retour sur le PC. → ✅
   📌 Voir `tests/lab-logs/smoke-epic2/midi-ox-5.3-send.txt` (et `sysex-5.3.txt` côté Mac)

### 5.4 Petite rafale (sans redémarrer)

1. Envoie **plusieurs** trames ~275 B à la suite (rythme calme, idéalement **≥ 10 ms** entre trames). → ✅
2. Pas de redémarrage Bridge pour finir la série. → ✅
3. Sens testé (Mac→PC / PC→Mac / les deux) : → ✅ — les deux
4. Si ❌ : symptôme (troncature / fusion / drop / restart) + message console : → N/A
5. Sauve le log MIDI-OX (et/ou MIDI Monitor) de la rafale. → ✅
   📌 Fichier(s) : `tests/lab-logs/smoke-epic2/midi-monitor-5.4.txt` (PC→Mac) & `midi-ox-5.4.txt` (Mac→PC)

### 5.5 Remarques libres 📌

- **2026-08-06** — BSOD pendant §5.2 Patch avec **MidiView** ouvert. Inquiry OK juste avant. Code `0x3B` SYSTEM_SERVICE_EXCEPTION ; module `htmididriver64.sys` ; process `MidiView.exe`. Confirmé à l’écran BSOD + minidump Desktop `080626-13078-01.dmp`.
- Reprise avec **MIDI-OX** uniquement : Inquiry + Patch + Master OK dans les deux sens ; rafale OK sans redémarrer le Bridge ; **aucun** nouveau BSOD. MidiView désinstallé.
- Fixtures : `tests/fixtures/sysex/` (`DeviceEnquiry.syx`, `Patch.syx`, `Master.syx`).
- Preuves session : `tests/lab-logs/smoke-epic2/` (noms kebab-case).
- MIDI-OX gère affichage E/S + envoi/réception SysEx — outil PC retenu pour la suite (partie 6+).


---

## 6. Matrix-Control — vecteurs minimum (story 2.4)

Objectif : avec **Matrix-Control** (appli externe sur le **PC**) + de préférence un **Matrix-1000** sur un câble MIDI du MT4, valider les vecteurs « portes dures ».

**Sans Matrix-Control / sans Matrix-1000 :** marque **N/A** / reportée — ce n’est pas un échec des parties 1–5. Windows **10** = colonne obligatoire.

**Lien avec le setup Scarlett :** pour cette partie, le chemin nominal est **Matrix-Control ↔ ports virtuels `MT4 Input` / `MT4 Output` ↔ MT4 ↔ Matrix-1000**. La Scarlett / le Mac ne sont **pas** nécessaires, sauf pour le vecteur **#7** (injection d’un SysEx non-patch) où un envoi SysEx sur le PC + **MIDI-OX** en observation/log suffit souvent ; tu peux aussi injecter depuis le Mac via Scarlett → MT4 IN **si** c’est le même port virtuel que Matrix-Control écoute — plus délicat, préfère injecter sur le PC et logger avec MIDI-OX pour #7.

**Règle d’espacement :** rythme stock Matrix-Control. Essai volontairement trop serré (< ~10 ms) → **invalide**, ne pas ❌ le Bridge.

**Timeout hôte (~2 s) :** si Matrix-Control expire mais **MIDI-OX** / MIDI Monitor montre la trame intacte (idéalement avec log fichier) → note « timeout hôte », pas automatiquement ❌ Bridge.

### 6.1 Préparer

1. Bridge en `--start-session` (`--dev-zadig` si besoin). Note commit / chemin. → 
2. Mode Computer OK. → 
3. Branche le **Matrix-1000** sur le câble MIDI MT4 choisi (IN/OUT selon le câblage synth). → 
4. Ouvre Matrix-Control sur le **PC** ; sélectionne MIDI From = `MT4 Input X`, MIDI To = `MT4 Output Y`. → 
   📌 Port N / câble : …
5. Ouvre **MIDI-OX** sur le même `MT4 Input X` utile (observation) (Display / log prêt pour les dumps douteux ou le #7). **Pas MidiView.** → 

### 6.2 Vecteurs portes dures

Pour chaque ligne : remplis **Résultat** dès que tu as fini le vecteur (✅ / ❌ / Skip / N/A) + notes au besoin.

| # | Vecteur | Quoi faire dans Matrix-Control | Résultat | Notes |
|---|---|---|---|---|
| **1** | Device Inquiry | Ouvre / reconnecte ; l’UI détecte l’appareil. Matrix-1000 : identité fabricant `10`, famille `06 00`, membre `02 00`. Autre Oberheim : documente les octets + Skip #1 (pas ❌ Bridge). | →  |  |
| **2** | Dump patch (~275 B) | Demande un dump ; trame `F0 10 06 01 … F7` de **275 octets** complète (pas coupée / fusionnée), souvent dans les ~2 s. | →  |  |
| **3** | Dump Master (~351 B) | Dump Master **351 octets** sans redémarrer le Bridge. | →  |  |
| **4** | Push / edit-buffer (~275 B) | Écrit un patch vers un slot **et/ou** edit-buffer (`0D`). Pass = le synth accepte (son / UI). | →  |  |
| **5** | Flux éditeur live | Knobs / Matrix Mod au rythme stock ; pas de restart Bridge ni drop évident. Pass = au moins une classe 7 B / 9 B. | →  |  |
| **7** | Fil mélangé | Démarre un dump patch (#2). Pendant l’attente, injecte un Inquiry `F0 7E 7F 06 01 F7` sur le **même** port virtuel (**MIDI-OX** pour envoyer et/ou logger). Puis un dump patch **ultérieur** doit encore arriver intact. | →  |  |

### 6.3 Optionnel — stress banque (#6)

≈ 100 × 275 B à ≥ 10 ms. Pas une porte dure.

1. Stress banque (#6). → 
   📌 Notes : …

### 6.4 Bilan Matrix-Control (Win10) — synthèse rapide

| # | Résultat | Notes (Port N / câble / sens / build) |
|---|---|---|
| 1 Inquiry | | |
| 2 Patch 275 B | | |
| 3 Master 351 B | | |
| 4 Push / edit-buffer | | |
| 5 Live editor | | |
| 7 Mixed-wire | | |
| 6 Banque (opt.) | | |

### 6.5 Remarques libres 📌



---

## 7. Longévité ~4 h (story 2.5)

Objectif : session d’**environ 4 heures** avec un peu de SysEx, **sans** redémarrage **obligatoire** du Bridge.

Tu peux **reporter** cette partie. Un essai 30–60 min aide, mais **ne clôture pas** seul l’exigence ~4 h.

### 7.1 Avant le chrono

1. Sur le **PC** : désactive veille / hibernation / suspension sélective USB ; session déverrouillée. → 
2. Rappel : PC endormi ou USB suspendu pendant la fenêtre → essai **annulé** ou ❌ (lu / OK). → 
3. Rappel : arrêt valide = **Ctrl+C** seulement, pas la croix (lu / OK). → 
4. Évite de rediriger la console 4 h. Logs qui meurent en cours → essai annulé (lu / OK). → 
5. Note commit + heure de début **avant** Pass. → 
   📌 Commit / chemin Bridge : …
   📌 Début (heure murale) : …
6. Choix des contrôles horaires : Mac + Scarlett, et/ou entièrement PC (Matrix-Control / MIDI-OX). → 
   📌 Mac / Scarlett utilisés pendant le soak ? oui / non — détail : …
   📌 Outils : Live PC / Live Mac / MIDI-OX / MIDI Monitor / Matrix-Control : …
   📌 Ports / câbles : …

### 7.2 Pendant ~4 h

```text
builds\debug\Debug\Bridge.exe --start-session --dev-zadig
```

Au moins **une fois par heure** — coche au fil de l’eau :

| Heure ~ | Notes/CC ≥1 IN + ≥1 OUT | SysEx (requis) | Horloge / MTC (bonus) | Remarque |
|---|---|---|---|---|
| H+1 | →  | →  | →  |  |
| H+2 | →  | →  | →  |  |
| H+3 | →  | →  | →  |  |
| H+4 | →  | →  | →  |  |

Vers ~2 h et à la fin : Gestionnaire des tâches (mémoire / handles de `Bridge.exe`).

1. Ressources ~2 h notées. → 
2. Ressources fin notées. → 
   📌 Ressources (début / milieu / fin) : …

### 7.3 Critères (au moment où tu clôtures)

**Pass** si : durée **≥ 3 h 45**, pas de restart obligatoire, ports utilisables à la fin, SysEx au rythme prévu, tableau rempli.

**Fail** si : crash, pompe morte, restart forcé, dumps qui exigent restart, sommeil PC / USB, etc.

1. Fin (heure murale) : → 
2. Durée (≥ 3 h 45 ?) : → 
3. SysEx : rythme OK ? → 
4. Anomalies : → 
5. Essai ~4 h. → 
6. Essai 30–60 min (optionnel). → 

### 7.4 Remarques libres 📌



---

## 8. Arrêt propre (Ctrl+C)

1. Focus console Bridge (**PC**). → 
2. **Ctrl+C** une fois ; attends jusqu’à ~3 s. → 
3. Processus terminé. → 
4. Ableton PC / MIDI-OX : `MT4 Input` / `MT4 Output` disparus ou non sélectionnables (souvent orange dans Live). → 
5. LEDs MT4 : Patch rouge, USB orange éteinte (comme Epic 1). → 

### 8.1 Remarques libres 📌



---

## 9. Bilan de la session Epic 2

**Verdict personnel**

1. Horloge / transport (2.1) : oui / non / partiel → 
2. MTC (2.2) : oui / non / partiel → 
3. SysEx pipe (2.3) : oui / non / partiel → 
4. Matrix-Control (2.4) : oui / non / reporté / N/A matériel → 
5. Longévité ~4 h (2.5) : oui / non / reporté → 
6. Setup Scarlett + deux Live utile / à ajuster : → 
7. Prêt à enchaîner Epic 3 : oui / non / avec réserves → 

**Ce qui a le mieux marché**

- …

**Ce qui a bloqué ou surpris**

- …

**À retester / à corriger ensuite**

1. …
2. …

**Fichiers / logs utiles** (captures MIDI Monitor, **MIDI-OX** Display/log, Live, console Bridge, commit, minidump si BSOD)

- …

### 9.1 Remarques libres 📌



---

## En cas de blocage — pistes rapides

| Symptôme | Piste |
|---|---|
| `--open-device` / session échoue | Epic 1 §2 : cas B → Zadig + `--dev-zadig`. |
| `--test-mapper` ≠ 0 | Rebuild à jour ; coller la sortie. |
| Ports absents dans loopMIDI | **Normal** — Ableton PC / MIDI-OX. |
| Ports absents dans Ableton PC | Session pas démarrée, ou Live ouverte trop tôt sans refresh. |
| MIDI Monitor ne voit rien (sens PC → Mac) | Câble MT4 **OUT** → Scarlett **IN** ; mauvais port Live PC ; Track non coché sur `MT4 Output N`. |
| MIDI-OX / Live PC silencieux (sens Mac → PC) | Câble Scarlett **OUT** → MT4 **IN** ; Live Mac / SysEx Librarian envoie vers la Scarlett ; mauvais `MT4 Input N` / `MT4 Output N` ; MIDI-OX Input bien sélectionné. |
| BSOD avec MidiView / `htmididriver64.sys` | **Connu** (2026-08-06) — ne plus utiliser MidiView sur ce labo ; reprendre avec MIDI-OX. |
| Oxygen 61 « ne traverse pas » le MT4 | Normal : USB-only — il ne remplace pas un câble DIN. Passe par Live + Scarlett ou Live + `MT4 Input` / `MT4 Output`. |
| Notes OK, horloge absente | Sync non coché sur le **bon** port (PC : `MT4 Input N` / `MT4 Output N` ; Mac : Scarlett). |
| Live ne suit pas l’horloge externe | Réception Sync activée ; l’autre Live envoie bien Clock ; sens de câble correct. |
| Continue introuvable | Souvent limite Live → **N/A** + raison. |
| MTC invisible / confondu avec Clock | MTC ≠ MIDI Clock. Active Timecode (MTC) ; MIDI Monitor doit montrer `F1`, pas seulement `F8`. |
| Full-frame absent mais `F1` OK | Souvent le générateur Live n’envoie pas de full-frame — note N/A générateur ; teste une autre source si tu en as une. |
| SysEx coupé / fusionné | Taille, Port N, sens Mac↔PC ; ❌ Bridge si reproductible au rythme calme. |
| SysEx seul, boîtier « mort » | Relancer `--start-session` (mode Computer), pas seulement un SysEx. |
| Matrix-Control timeout ~2 s | Capture MIDI-OX (log fichier) / MIDI Monitor ; possible timeout hôte si trame intacte. |
| Session qui meurt toute seule | Copier le message anglais console. |
| Ctrl+C lent (~3 s) | Attendu (timeout bulk IN). |
| Croix fenêtre / ports orphelins | Soak **annulé** ; nettoyer avant le prochain essai. |
| PC endormi pendant le soak | Essai annulé ou ❌. |

---

## Rappels commandes (aide-mémoire)

```text
REM Profil seul (pas d’USB)
Bridge.exe

REM Contrôle synthétique Epic 2 (pas de matériel)
Bridge.exe --test-mapper

REM Ouverture USB seule
Bridge.exe --open-device
Bridge.exe --open-device --dev-zadig

REM Session labo (laisser tourner, Ctrl+C pour arrêter)
Bridge.exe --start-session
Bridge.exe --start-session --dev-zadig

REM Alias équivalent
Bridge.exe --run-midi --dev-zadig
```

Adapte toujours le chemin complet vers ton `Bridge.exe` sous `builds\`.

**Ordre conseillé pour une soirée labo (ton setup)**

1. **0** — allumer PC + Mac, câbler Scarlett ↔ MT4, vérifier MIDI Monitor + MIDI-OX (~15–25 min).  
2. **1–2** — build + session Bridge + spot notes via Scarlett (~20–40 min si bind OK).  
3. **3** — horloge dans les deux sens (Live PC ↔ Live Mac) (~20–40 min).  
4. **4** — MTC, MIDI Monitor comme microscope (~20–40 min).  
5. **5** — SysEx + rafale ; **MIDI-OX** log fichier côté PC (**pas MidiView**) (~30–60 min).  
6. **6** — Matrix-Control quand Matrix-1000 prêt (Scarlett optionnelle ; MIDI-OX pour #7 / preuves).  
7. **7** — ~4 h sur une plage dédiée.

Checklists techniques anglaises (référence agents / IDs de matrice) :  
[`smoke-epic2-clock-mt4.md`](checklists/smoke-epic2-clock-mt4.md) · [`smoke-epic2-mtc-mt4.md`](checklists/smoke-epic2-mtc-mt4.md) · [`smoke-epic2-sysex-mt4.md`](checklists/smoke-epic2-sysex-mt4.md) · [`smoke-epic2-matrix-control-mt4.md`](checklists/smoke-epic2-matrix-control-mt4.md) · [`smoke-epic2-longevity-mt4.md`](checklists/smoke-epic2-longevity-mt4.md)
