---
organization: Ten Square Software
project: unitor-win64-driver
title: Smoke Epic 1 — MT4 (notes / CC)
author: Guillaume DUPONT
created: 2026-08-05
updated: 2026-08-05
---

# Guide de smoke — Epic 1 (MT4 sous Windows)

Ce guide te sert pour **les premiers essais réels** ce soir : brancher le MT4, lancer le Bridge, voir les ports `MT4 Port N`, faire passer des notes et des CC dans les deux sens.

**Comment l’utiliser**

- Suis les étapes **dans l’ordre**.
- À chaque ligne de validation, remplace ou complète avec **✅** (OK) ou **❌** (problème).
- Si une étape échoue, **arrête-toi** sur cette section, note ce que tu vois dans **📌 Remarques GD**, et passe éventuellement à la section « En cas de blocage » en bas.
- Les cases à cocher markdown ne sont **pas** utilisées ici — uniquement ✅ / ❌.

**Ce que ce smoke prouve (Epic 1)**

- Le MT4 parle au Bridge via **WinUSB** (pas un pilote kernel custom du projet).
- Le Bridge crée **2 entrées + 4 sorties** nommées `MT4 Port 1` … `MT4 Port 4`.
- Notes et CC circulent **boîtier → PC** et **PC → boîtier**.

**Hors scope ce soir** (ne pas les exiger)

- Horloge MIDI, MTC, SysEx, hot-plug, multi-client, installateur public.

**Retour labo (2026-08-05, Boot Camp MacBook Pro 2014)**

| Étape | Résultat |
|---|---|
| Bind | **Cas B** (Zadig / INF lab sur nœud parent sans `MI_02`) + Bridge `--dev-zadig` |
| Sections 1–3 | ✅ open / probe USB |
| Section 4 | ✅ session + **2 IN / 4 OUT** visibles dans Ableton Live 12 |
| Section 5 (boîtier → PC) | ❌ LEDs In physiques OK ; **aucun** note/CC dans Live sur les ports IN virtuels |
| Section 6 (PC → boîtier) | ✅ globalement (notes + CC7) ; 1er envoi notes sur Out 1 a un instant allumé **les 4** LEDs Out (voir §6) |
| Section 8 | ✅ Ctrl+C → Patch rouge, USB orange éteint, ports Live en orange (déconnectés) |

**Ne pas enchaîner** `--probe-usb` puis `--start-session` sans rebuild récent : le probe réveille le boîtier ; la session doit pouvoir le reréveiller (finish + retry). Pour le smoke MIDI, lance **uniquement** `--start-session --dev-zadig`.

---

## 0. Préparer la machine et le matériel

### 0.1 Matériel

1. PC sous **Windows 10 ou 11 64 bits**.
2. Interface **MT4** Emagic, câble USB.
3. Au moins une source MIDI physique branchée sur un **IN** du MT4 (clavier, contrôleur), **ou** un moyen d’envoyer du MIDI vers un **OUT** physique (module, autre boîtier, LED/activité sur le MT4).
4. Écouteurs / enceintes / module si tu veux **entendre** le retour — sinon ShowMIDI (ou équivalent) suffit pour **voir** les messages.

### 0.2 Logiciels à avoir installés

1. **Outils de build** déjà utilisés pour ce projet (CMake + Visual Studio / Build Tools x64) — pour compiler le Bridge avec les correctifs d’aujourd’hui.
2. **VirtualMIDI** (teVirtualMIDI) : en pratique, installer **loopMIDI** (ou rtpMIDI) depuis le site Tobias Erichsen, pour que `teVirtualMIDI.dll` soit disponible sur la machine.
3. Un utilitaire pour **voir** le MIDI entrant, par ex. **ShowMIDI** (recommandé dans la Validation Matrix du projet). Une DAW (Ableton, Reason, etc.) convient aussi.
4. (Optionnel) Un moyen d’**envoyer** du MIDI depuis le PC vers un port OUT virtuel (DAW, clavier virtuel, utilitaire MIDI out).

### 0.3 Code à tester

Les correctifs de la revue d’intégration Epic 1 (verrou multi-OUT, timeout USB écriture, etc.) doivent être **sur le PC Windows** avant le build.

1. Copie / sync / pull du dépôt `unitor-win64-driver` sur le PC Windows (la branche / l’état qui contient les patches de ce soir).
2. Ouvre un terminal (Invite de commandes ou PowerShell) à la **racine du dépôt**.

Validation :

- Dépôt présent sur le PC Windows : ✅
- Je suis bien à la racine du dépôt (je vois `CMakeLists.txt`, `src/`, `installer/`) : ✅
- VirtualMIDI / loopMIDI installé : ✅ (installé via loopMIDI)
  - ShowMIDI (ou DAW) prêt : ✅ (Ableton Live 12 Suite & ShowMIDI installés)

📌 Remarques GD :



---

## 1. Compiler le Bridge

Objectif : obtenir un `Bridge.exe` frais sous `builds/`.

1. À la racine du dépôt, configure (exemple Debug) :

```text
cmake -S . -B builds/debug -A x64
```

2. Compile :

```text
cmake --build builds/debug --config Debug
```

(Tu peux utiliser `builds/release` + `--config Release` si tu préfères.)

3. Repère l’exécutable. Sous Visual Studio / générateur multi-config, il est en général ici :

```text
builds\debug\Debug\Bridge.exe
```

Sinon cherche `Bridge.exe` sous `builds\` (Explorateur ou PowerShell : `Get-ChildItem -Path builds -Recurse -Filter Bridge.exe`).

4. Teste que l’exe démarre sans USB (smoke profil uniquement) :

```text
builds\debug\Debug\Bridge.exe
```

Tu dois obtenir une sortie de contrôle DeviceProfile et un **code de sortie 0**, **sans** message d’erreur sur le bind WinUSB (cette commande n’ouvre pas encore l’USB).

Validation :

- Configure CMake OK : ✅
- Build OK : ✅
- Chemin exact de `Bridge.exe` noté : ✅
  → Chemin : builds\debug\Debug\Bridge.exe
- Lancement sans argument : sortie 0 : ✅ (je ne vois pas de code "0" de sortie, mais pas d'erreur non plus)

📌 Remarques GD : Un dossier `build\` (sans « s ») à la racine n’est **pas** le chemin attendu du projet — les builds vont sous `builds\`. Tu peux supprimer `build\` s’il vient d’un essai CMake hors convention.



---

## 2. Brancher le MT4 et vérifier WinUSB

Objectif : Windows voit le MT4 associé à **WinUSB**, et le Bridge peut l’ouvrir.

**Deux cas possibles sur le terrain**

| Cas | Ce que tu vois dans le Gestionnaire | Suite |
|---|---|---|
| **A — nœuds séparés** | Un enfant avec ID `…&MI_02` | INF du dépôt (ou Gestionnaire → INF) ; Bridge **sans** `--dev-zadig` |
| **B — un seul nœud** (vu sur Boot Camp / MacBook) | Un seul « MT4 » / périphérique inconnu : `USB\VID_086A&PID_0003` **sans** `MI_02` | **Zadig** sur ce nœud ; Bridge **avec** `--dev-zadig` |

Le numéro « 2 » est un **canal USB interne** du boîtier (celui du MIDI Emagic), **pas** un canal MIDI 1–16. Windows peut le montrer à part (`MI_02`) ou le laisser caché dans le nœud parent : dans les deux cas le Bridge doit pouvoir parler au MIDI.

### 2.1 Brancher

1. Branche le MT4 en USB (de préférence un port du PC, pas un hub externe).
2. Attends quelques secondes que Windows le détecte.
3. Ouvre le **Gestionnaire de périphériques** et regarde si tu as un ID avec `MI_02` (cas A) ou seulement `VID_086A&PID_0003` (cas B).

### 2.2 Chemin principal — INF du dépôt (cas A, préféré si `MI_02` existe)

1. Ouvre **PowerShell en administrateur**.
2. Place-toi à la **racine du dépôt** (là où se trouvent `CMakeLists.txt` et `installer\`) — **pas** dans `builds\debug\Debug` :

```powershell
cd C:\Users\Guillaume\Dev\Projects\unitor-win64-driver
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\installer\bind-mt4-winusb.ps1
```

3. Si PowerShell refuse les scripts : la ligne `Set-ExecutionPolicy … Bypass` ci-dessus ne change la règle **que pour cette fenêtre**.
4. Si `pnputil` refuse l’INF **non signé** (« ne contient pas d’informations de signature numérique ») : passe au Gestionnaire (2.3) ou à Zadig (2.4).

### 2.3 Variante — Gestionnaire de périphériques (cas A)

1. Cible le nœud dont l’ID de matériel contient **`MI_02`** (pas le parent sans `MI_…`).
2. Clic droit → **Mettre à jour le pilote** → **Parcourir mon ordinateur** → de préférence **Choisir dans une liste** → **Disque fourni…**
3. Pointe vers `installer\mt4-winusb.inf` du dépôt.
4. Accepte l’avertissement non signé seulement sur machine de labo.

**Piège vu en labo :** forcer « Périphérique USB composite » sur le parent **sans** `MI_02` peut laisser le périphérique en **erreur (code 10)** et **ne crée pas** d’enfants `MI_02`. Dans ce cas : désinstalle ce pilote, rebranche, et passe à Zadig (2.4).

### 2.4 Variante contributeur — Zadig (cas B, ou si l’INF ne passe pas)

1. Désinstalle tout pilote cassé / composite en erreur sur le MT4, puis rebranche.
2. Lance **Zadig** → **Options** → coche **List All Devices**.
3. Sélectionne le **MT4** (`086A` / `0003`).
4. Si tu vois un menu **Interface** : choisis l’interface **2**.
   Si tu ne vois **qu’une** entrée « MT4 » sans menu Interface : c’est le cas B — installe **WinUSB** sur cette entrée (c’est normal).
5. **Install Driver** / Replace Driver.
6. Ensuite, **toutes** les commandes Bridge de ce guide doivent inclure `--dev-zadig`.

**Si `--probe-usb` échoue avec erreur 121** (écriture USB qui expire alors que le bon canal 2 est trouvé) : remplace le pilote Zadig/libwdi par l’INF du dépôt sur le **même** nœud parent — Gestionnaire → Mettre à jour → Disque fourni → `installer\mt4-winusb.inf` (l’INF accepte aussi `USB\VID_086A&PID_0003` sans `MI_02`). Puis reteste `--probe-usb`.

### 2.5 Vérifier dans le Gestionnaire de périphériques

1. Propriétés → onglet **Pilote** : service / pile **WinUSB** (pas un pilote kernel custom de ce projet).
2. Onglet **Détails** → **ID de matériel** :
   - Cas A : quelque chose comme `USB\VID_086A&PID_0003&MI_02`
   - Cas B : `USB\VID_086A&PID_0003` (sans `MI_02`) suffit si WinUSB est bien chargé
3. GUID projet `{aa209017-cf8a-49ad-a0e7-701187ff7e05}` : attendu surtout après INF ; **souvent absent** après Zadig (d’où `--dev-zadig`).

Validation :

- MT4 branché et détecté : ✅
- Bind WinUSB effectué (INF / Gestionnaire / Zadig — préciser lequel) : ✅
  → Méthode : Zadig (cas B, nœud parent sans MI_02 ; Boot Camp MacBook Pro 2014)
- Hardware ID : `VID_086A&PID_0003` (avec ou sans `MI_02`) : ✅ (sans MI_02)
- GUID projet présent (si vérifié) : non vérifié / absent (Zadig)

📌 Remarques GD : Sur cette machine le composite forcé échoue (code 10) ; Zadig sur le nœud unique + Bridge `--dev-zadig` ouvre correctement (après correctifs open parent + canal USB 2 associé).

---

## 3. Ouvrir le MT4 sans session MIDI (contrôle USB)

Objectif : confirmer que le Bridge **ouvre** le boîtier avant de créer des ports virtuels.

1. Ferme tout ancien `Bridge.exe` encore ouvert.
2. Dans un terminal, lance (adapte le chemin) :

**Chemin INF / GUID (cas normal) :**

```text
builds\debug\Debug\Bridge.exe --open-device
```

**Chemin Zadig seulement :**

```text
builds\debug\Debug\Bridge.exe --open-device --dev-zadig
```

3. Observe :
   - **Succès** : code de sortie **0**, pas de diagnostic d’échec sur stderr.
   - **Échec** : code non nul + message en **anglais** expliquant le problème (bind manquant, GUID absent, etc.). C’est voulu : le Bridge ne doit pas « réussir » en silence.

Validation :

- `--open-device` réussi (exit 0) : ✅ (`--open-device --dev-zadig` → `WinUSB open succeeded`)
- Message d’erreur éventuel (copier-coller) : _(résolu)_ avant correctifs : matching HWID sans MI_02 + ifnum 0 vs 2

📌 Remarques GD : Rebuild requis après les correctifs Usb (parent Zadig + interface associée).

---

## 4. Lancer la session MIDI (ports virtuels)

Objectif : créer les ports `MT4 Port N` et laisser la pompe notes/CC tourner.

### 4.1 Avant de lancer

1. ShowMIDI (ou la DAW) peut être déjà ouvert, mais **rafraîchis / rouvre** la liste des ports MIDI **après** le démarrage de la session.
2. Un seul Bridge de session à la fois.

### 4.2 Lancer

**Cas normal (INF / GUID) :**

```text
builds\debug\Debug\Bridge.exe --start-session
```

(`--run-midi` est un alias équivalent.)

**Cas Zadig :**

```text
builds\debug\Debug\Bridge.exe --start-session --dev-zadig
```

### 4.3 Ce que tu dois voir dans la console

À peu près ceci (libellés en anglais, c’est normal) :

- Une liste des noms de ports attendus (`MT4 Port 1` …).
- `DeviceSession started for MT4 with Virtual Ports`
- `MIDI I/O running - notes/CC smoke ready (Ctrl+C to stop)`  
  (tiret ASCII `-` uniquement — un tiret typographique s’affiche en `ÔÇö` sous PowerShell OEM)

Le processus **reste ouvert** (curseur qui clignote) : **c’est normal**. Ne ferme pas la fenêtre tant que tu testes ; **Ctrl+C** pour arrêter.

**loopMIDI** n’affiche en général **pas** les ports créés par Bridge (il ne liste que ses propres ports). Regarde dans **Ableton / ShowMIDI**.

### 4.4 Vérifier les ports côté Windows

Dans ShowMIDI / DAW, tu dois voir **par unité MT4** :

- **2 ports d’entrée** (MIDI IN côté apps) : `MT4 Port 1`, `MT4 Port 2`
- **4 ports de sortie** (MIDI OUT côté apps) : `MT4 Port 1` … `MT4 Port 4`

Les noms IN et OUT portent le **même libellé** pour le même numéro de port (comportement voulu).

Validation :

- Session démarrée, messages console OK : ✅
- 2 IN visibles : ✅ (Ableton Live 12 → Réglages → MIDI → Input Ports)
- 4 OUT visibles : ✅ (idem Output Ports ; Track coché pour notes/CC)
- Noms exacts `MT4 Port N` (pas un suffixe bizarre du type `#2`, ou le noter) : ✅
- Échec éventuel (VirtualMIDI manquant, open USB, etc.) — coller le message : _(aucun)_

📌 Remarques GD : Lab 2026-08-05 Boot Camp — session avec `--dev-zadig`. Ports créés par Bridge (teVirtualMIDI), pas via la UI loopMIDI.



---

## 5. Sens boîtier → PC (câbles IN physiques → ports virtuels IN)

Objectif : ce qui entre dans le MT4 apparaît sur `MT4 Port 1` / `MT4 Port 2` dans ShowMIDI / DAW.

### 5.1 Préparer l’écoute

1. Dans ShowMIDI (ou DAW), sélectionne / surveille **`MT4 Port 1`** (entrée).
2. Branche un clavier (ou contrôleur) sur l’**entrée physique 1** du MT4.

### 5.2 Tester Port 1

1. Joue quelques **notes**.
2. Bouge un **CC** (mod wheel, fader, etc.).
3. Confirme que ShowMIDI / DAW affiche bien notes + CC sur **`MT4 Port 1`**.

Validation Port 1 IN :

- Notes visibles : ❌
- CC visibles : ❌

### 5.3 Tester Port 2

1. Surveille **`MT4 Port 2`** (entrée) dans ShowMIDI / DAW.
2. Branche la source sur l’**entrée physique 2** du MT4 (ou change de câble).
3. Notes + CC à nouveau.

Validation Port 2 IN :

- Notes visibles : ❌
- CC visibles : ❌

### 5.4 Contrôle anti-mélange (rapide)

Sans rien brancher sur l’IN 2, jouer sur l’IN 1 : le trafic ne doit **pas** apparaître sur `MT4 Port 2`.

Validation :

- Pas de « fantômes » sur l’autre port IN : non testé (aucun trafic virtuel reçu)

📌 Remarques GD (lab 2026-08-05) :

- Source = clip MIDI Ableton sur **Mac** → câble DIN → **In 1** puis **In 2** du MT4 (PC Boot Camp).
- LEDs rouges **In 1 / In 2** du MT4 clignotent (notes + CC7) : le boîtier **reçoit** bien le DIN.
- Ableton Live 12 sur le **PC** écoute `MT4 Port 1` / `2` (Track coché) : **aucune** note/CC n’apparaît.
- Conclusion smoke : chemin **physique OK**, chemin **Bridge USB bulk IN → port virtuel IN** non prouvé (bug ou config Live à isoler ensuite). Priorité correctif / debug après ce guide.



---

## 6. Sens PC → boîtier (ports virtuels OUT → câbles OUT physiques)

Objectif : ce que tu envoies sur `MT4 Port N` (OUT virtuel) sort sur le **câble physique N** du MT4.

### 6.1 Préparer l’émission

1. Choisis un outil qui envoie du MIDI **vers** un port OUT Windows (DAW, clavier virtuel, etc.).
2. Branche un module / clavier / indicateur sur la **sortie physique 1** du MT4 (ou écoute l’activité MIDI).

### 6.2 Tester chaque OUT (un par un)

Pour **chaque** port `MT4 Port 1` … `MT4 Port 4` :

1. Dans l’outil d’envoi, choisis **uniquement** ce port OUT.
2. Envoie quelques **notes**, puis un **CC**.
3. Vérifie sur le **câble physique correspondant** (ou un moniteur MIDI branché dessus) que c’est bien ce numéro de port qui réagit — pas un autre.

Validation OUT 1 notes/CC : ✅ notes (après 2ᵉ essai) / ✅ CC7 — *voir remarque 1er envoi notes*  
Validation OUT 2 notes/CC : ✅ / ✅  
Validation OUT 3 notes/CC : ✅ / ✅  
Validation OUT 4 notes/CC : ✅ / ✅

### 6.3 Test important — plusieurs OUT en même temps

C’est le point ciblé par le correctif d’intégration de ce jour.

1. Configure l’outil pour envoyer **en parallèle** (ou en rafale très serrée) vers **au moins deux** OUT virtuels différents, par ex. `MT4 Port 1` et `MT4 Port 3`.
2. Envoie des notes **distinctes** (ex. note grave sur le port 1, note aiguë sur le port 3) pour reconnaître qui est qui.
3. Vérifie que chaque câble physique reçoit **sa** note, sans inversion durable.

Validation multi-OUT :

- Deux OUT en parallèle, routage correct : non testé
- Quatre OUT sollicités dans la même session sans redémarrer le Bridge : ✅ (enchaînement successif Out 1→4 dans la même session Live)

📌 Remarques GD (lab 2026-08-05) :

- Émission = clips Ableton Live 12 **sur le PC** → ports OUT `MT4 Port N` (Track coché).
- **CC7** : LED verte Out **N** seule pour chaque port 1…4 — OK.
- **Notes** : Out 2 / 3 / 4 → LED Out correspondante seule — OK.
- **Notes Out 1 (1er essai de la session)** : les **4** LEDs vertes Out ont clignoté ensemble ; **retest** notes Out 1 plus tard → seule Out 1. Cohérent avec l’omission connue du sélecteur de câble Emagic (`F5`) quand le mapper croit déjà être sur le câble 0 après init.
- Critère Epic 1 PC→boîtier : **largement OK** ; edge Out 1 au premier envoi = dette connue à corriger.



---

## 7. Tenue de session courte (sans redémarrage)

Objectif : quelques minutes de jeu sans devoir relancer le Bridge.

1. Laisse `--start-session` tourner.
2. Enchaîne 2–3 minutes de notes/CC (IN et/ou OUT).
3. Le Bridge ne doit pas s’arrêter tout seul ; la console ne doit pas afficher un échec de pompe suivi d’une session morte.

Validation :

- Session stable ~2–3 min : ✅ (session tenue pendant les essais IN/OUT Ableton)
- Message d’échec éventuel (copier) : _(aucun)_

📌 Remarques GD :



---

## 8. Arrêt propre (Ctrl+C)

Objectif : ranger les ports virtuels et l’USB proprement.

1. Remets le focus sur la fenêtre console du Bridge.
2. Appuie sur **Ctrl+C** (une fois, puis attends).
3. Le processus doit se terminer.
4. Dans ShowMIDI / DAW, **rafraîchis** la liste des ports : les `MT4 Port N` de cette session doivent **disparaître** (ou ne plus être sélectionnables comme ports live).

Validation :

- Ctrl+C termine le Bridge : ✅
- Ports `MT4 Port N` disparus après arrêt : ✅ (Live : intitulés passent en **orange** = interface déconnectée)
- LED MT4 : Patch **rouge** allumée, USB **orange** éteinte : ✅

📌 Remarques GD : Teardown aligné avec le comportement macOS (retour mode Patch).



---

## 9. (Optionnel) Fermeture brutale de la fenêtre console

**Connu / dette** : fermer la fenêtre (croix) peut tuer le process avant le rangement complet → ports virtuels parfois **orphelins**.

1. Relance `--start-session`.
2. Ferme la fenêtre par la **croix** (ne fais pas Ctrl+C).
3. Regarde si des `MT4 Port N` restent visibles alors que le Bridge est mort.

Validation (informatif, pas un critère de réussite Epic 1) :

- Ports orphelins après fermeture croix : oui / non / non testé

Si oui : redémarrage PC ou nettoyage VirtualMIDI / nouvel essai après Ctrl+C propre — note ta méthode.

📌 Remarques GD :



---

## 10. Bilan de la soirée

Remplis après les tests (même partiels).

**Verdict personnel**

- Prêt à enchaîner Epic 2 côté usage notes/CC : **avec réserves**
- Réserves en une phrase : boîtier→PC virtuel muet ; premier envoi notes Out 1 douteux (F5).

**Ce qui a le mieux marché**

- WinUSB lab (cas B) + init magic + session VirtualMIDI.
- 2 IN / 4 OUT nommés `MT4 Port N` dans Ableton Live 12.
- PC→boîtier notes/CC (surtout Out 2–4 et CC sur tous les Out).
- Arrêt Ctrl+C propre (LEDs + ports Live).

**Ce qui a bloqué ou surpris**

- DIN In 1/2 : LEDs MT4 OK, **silence** dans Live sur les IN virtuels.
- Premier envoi **notes** sur Out 1 : 4 LEDs Out allumées ; retest OK.
- Messages console avec tiret typographique (`ÔÇö`) sous PowerShell — corrigé en ASCII.
- loopMIDI vide alors que Live voit les ports (attendu : Bridge ≠ UI loopMIDI).

**À retester / à corriger ensuite**

1. **P0** — chemin device→host (bulk IN → demux → `SendToHost` → ports IN virtuels) ; confirmer aussi armement / monitoring Live.
2. **P1** — forcer un `F5` (ou reset `currentOutCable_`) après init pour Out 1 au premier encode.
3. Multi-OUT strictement parallèle (section 6.3) non fait.
4. Section 9 (fermeture croix / ports orphelins) non testée.

**Fichiers / logs utiles** (chemins, captures, messages console)

- Capture Ableton MIDI I/O : 2× `MT4 Port 1/2` IN + 4× OUT Track ON.
- Console : `DeviceSession started…` / `MIDI I/O running…` / Ctrl+C → stopped.

📌 Remarques GD (bilan) : Premier smoke matériel Epic 1 **sympathique** : socle session OK ; round-trip complet **pas** encore vert (IN virtuel).



---

## En cas de blocage — pistes rapides

| Symptôme | Piste |
|---|---|
| `--open-device` échoue, parle du GUID | Bind INF pas fait, ou mauvais nœud USB. Si tu n’as **pas** de `MI_02` : Zadig sur le nœud parent + `--dev-zadig` (rebuild à jour). |
| Script `bind-mt4-winusb.ps1` introuvable | Tu es dans `builds\…` au lieu de la **racine** du dépôt. |
| Exécution de scripts désactivée | `Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass` puis relancer le script. |
| INF : pas de signature numérique | Normal tant que l’INF n’est pas signé ; Gestionnaire (Disque fourni) ou Zadig. |
| « Aucun pilote » / pas de `MI_02` | Cas B : ne force pas « USB composite » ; Zadig sur le MT4 unique. |
| Zadig sans menu Interface 2 | Normal en cas B : Install WinUSB sur l’entrée « MT4 » seule. |
| `--probe-usb` / init : erreur 121 sur OUT 0x2 | Bon canal trouvé mais le boîtier ne répond pas. Débranche/rebranche ; remplace Zadig par `installer\mt4-winusb.inf` (Disque fourni) ; autre port USB ; redémarrage PC. |
| Session échoue, parle de VirtualMIDI / DLL / Win32=1379 | Installer loopMIDI / teVirtualMIDI ; si « existe déjà » : fermer ports loopMIDI homonymes ou rebuild avec merge IN/OUT homonymes. |
| Aucun port `MT4 Port N` dans loopMIDI | **Normal** — Bridge crée les ports via teVirtualMIDI ; regarde Ableton / ShowMIDI. |
| Aucun port `MT4 Port N` dans la DAW | La session n’a pas démarré ; ou DAW ouverte **avant** sans refresh. |
| Ports présents mais silence total (PC→boîtier) | Mauvais câble ; mauvais port OUT ; Track non coché dans Live. |
| LEDs In MT4 OK, silence dans la DAW (boîtier→PC) | Chemin Bridge device→host suspect (lab 2026-08-05) ; vérifier aussi piste Live armée / monitoring sur l’entrée `MT4 Port N`. |
| 1er envoi notes Out 1 allume plusieurs LEDs Out | Dette F5 / `currentOutCable_` après init ; retester après avoir utiliséé un autre Out. |
| Notes OK sur un port, mélangées sur plusieurs OUT | Noter ports + séquence — bug multi-OUT d’intégration ; si ça revient, **❌** prioritaire. |
| Session qui meurt toute seule | Copier le message anglais de la console (échec pompe / SendToHost / WriteBulk). |
| Ports qui restent après Ctrl+C | Noter ; éventuellement relancer Windows avant le prochain essai. |
| `--probe-usb` OK puis `--start-session` init 121 | Probe a réveillé le boîtier ; rebuild avec finish-après-probe / retry session, ou lancer **seulement** `--start-session`. |
| Périphérique « Emagic MT4 » grisé + « Emagic MT4 (WinUSB) » | Normal avec périphériques cachés : fantôme vs nœud actif WinUSB. |

---

## Rappels commandes (aide-mémoire)

```text
REM Profil seul (pas d’USB)
Bridge.exe

REM Ouverture USB seule
Bridge.exe --open-device
Bridge.exe --open-device --dev-zadig

REM Session notes/CC (laisser tourner, Ctrl+C pour arrêter)
Bridge.exe --start-session
Bridge.exe --start-session --dev-zadig
```

Adapte toujours le chemin complet vers ton `Bridge.exe` sous `builds\`.
