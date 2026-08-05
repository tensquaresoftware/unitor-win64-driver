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

- Dépôt présent sur le PC Windows : ✅ / ❌
- Je suis bien à la racine du dépôt (je vois `CMakeLists.txt`, `src/`, `installer/`) : ✅ / ❌
- VirtualMIDI / loopMIDI installé : ✅ / ❌
- ShowMIDI (ou DAW) prêt : ✅ / ❌

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

- Configure CMake OK : ✅ / ❌
- Build OK : ✅ / ❌
- Chemin exact de `Bridge.exe` noté : ✅ / ❌  
  → Chemin : _________________________________
- Lancement sans argument : sortie 0 : ✅ / ❌

📌 Remarques GD :



---

## 2. Brancher le MT4 et vérifier WinUSB

Objectif : Windows voit le MT4 sur l’interface MIDI Emagic, associée à **WinUSB**, avec le GUID du projet.

### 2.1 Brancher

1. Branche le MT4 en USB.
2. Attends quelques secondes que Windows le détecte.

### 2.2 Chemin principal — INF du dépôt (préféré)

1. Ouvre **PowerShell en administrateur**.
2. Place-toi dans le dépôt, puis :

```powershell
.\installer\bind-mt4-winusb.ps1
```

3. Si Windows refuse l’INF non signé : passe par le Gestionnaire de périphériques (section suivante), ou active le mode test uniquement sur machine de labo.

### 2.3 Variante — Gestionnaire de périphériques

1. Ouvre le **Gestionnaire de périphériques**.
2. Trouve le nœud du MT4 / USB composite lié à l’**interface 2** (souvent sous « Périphériques USB » ou « Autres périphériques »).
3. Clic droit → **Mettre à jour le pilote** → **Parcourir mon ordinateur**.
4. Pointe vers le dossier `installer\` du dépôt (fichier `mt4-winusb.inf`).
5. Termine l’assistant (accepte les avertissements de signature seulement si tu es OK sur cette machine de labo).

### 2.4 Variante contributeur — Zadig (secours seulement)

À n’utiliser **que** si l’INF ne passe pas. Bind WinUSB sur **MI_02** (pas une autre interface du composite).  
Ensuite tu lancera le Bridge avec `--dev-zadig` en plus des autres flags.

### 2.5 Vérifier dans le Gestionnaire de périphériques

1. Propriétés du périphérique → onglet **Pilote** : pile WinUSB / pas un pilote kernel custom de ce projet.
2. Onglet **Détails** → **ID de matériel** : tu dois voir quelque chose comme :

```text
USB\VID_086A&PID_0003&MI_02
```

3. (Si tu sais ouvrir la base de registre du périphérique) paramètres : présence du GUID  

```text
{aa209017-cf8a-49ad-a0e7-701187ff7e05}
```

Validation :

- MT4 branché et détecté : ✅ / ❌
- Bind WinUSB effectué (INF / Gestionnaire / Zadig — préciser lequel) : ✅ / ❌  
  → Méthode : _________________________________
- Hardware ID contient `VID_086A&PID_0003&MI_02` : ✅ / ❌
- GUID projet présent (si vérifié) : ✅ / ❌ / non vérifié

📌 Remarques GD :



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

- `--open-device` réussi (exit 0) : ✅ / ❌
- Message d’erreur éventuel (copier-coller) :



📌 Remarques GD :



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
- `MIDI I/O running — notes/CC smoke ready (Ctrl+C to stop)`

Le processus **reste ouvert** : ne ferme pas la fenêtre tant que tu testes.

### 4.4 Vérifier les ports côté Windows

Dans ShowMIDI / DAW, tu dois voir **par unité MT4** :

- **2 ports d’entrée** (MIDI IN côté apps) : `MT4 Port 1`, `MT4 Port 2`
- **4 ports de sortie** (MIDI OUT côté apps) : `MT4 Port 1` … `MT4 Port 4`

Les noms IN et OUT portent le **même libellé** pour le même numéro de port (comportement voulu).

Validation :

- Session démarrée, messages console OK : ✅ / ❌
- 2 IN visibles : ✅ / ❌
- 4 OUT visibles : ✅ / ❌
- Noms exacts `MT4 Port N` (pas un suffixe bizarre du type `#2`, ou le noter) : ✅ / ❌
- Échec éventuel (VirtualMIDI manquant, open USB, etc.) — coller le message :



📌 Remarques GD :



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

- Notes visibles : ✅ / ❌
- CC visibles : ✅ / ❌

### 5.3 Tester Port 2

1. Surveille **`MT4 Port 2`** (entrée) dans ShowMIDI / DAW.
2. Branche la source sur l’**entrée physique 2** du MT4 (ou change de câble).
3. Notes + CC à nouveau.

Validation Port 2 IN :

- Notes visibles : ✅ / ❌
- CC visibles : ✅ / ❌

### 5.4 Contrôle anti-mélange (rapide)

Sans rien brancher sur l’IN 2, jouer sur l’IN 1 : le trafic ne doit **pas** apparaître sur `MT4 Port 2`.

Validation :

- Pas de « fantômes » sur l’autre port IN : ✅ / ❌

📌 Remarques GD :



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

Validation OUT 1 notes/CC : ✅ / ❌  
Validation OUT 2 notes/CC : ✅ / ❌  
Validation OUT 3 notes/CC : ✅ / ❌  
Validation OUT 4 notes/CC : ✅ / ❌

### 6.3 Test important — plusieurs OUT en même temps

C’est le point ciblé par le correctif d’intégration de ce jour.

1. Configure l’outil pour envoyer **en parallèle** (ou en rafale très serrée) vers **au moins deux** OUT virtuels différents, par ex. `MT4 Port 1` et `MT4 Port 3`.
2. Envoie des notes **distinctes** (ex. note grave sur le port 1, note aiguë sur le port 3) pour reconnaître qui est qui.
3. Vérifie que chaque câble physique reçoit **sa** note, sans inversion durable.

Validation multi-OUT :

- Deux OUT en parallèle, routage correct : ✅ / ❌
- Quatre OUT sollicités dans la même session sans redémarrer le Bridge : ✅ / ❌ / non testé

📌 Remarques GD :



---

## 7. Tenue de session courte (sans redémarrage)

Objectif : quelques minutes de jeu sans devoir relancer le Bridge.

1. Laisse `--start-session` tourner.
2. Enchaîne 2–3 minutes de notes/CC (IN et/ou OUT).
3. Le Bridge ne doit pas s’arrêter tout seul ; la console ne doit pas afficher un échec de pompe suivi d’une session morte.

Validation :

- Session stable ~2–3 min : ✅ / ❌
- Message d’échec éventuel (copier) :



📌 Remarques GD :



---

## 8. Arrêt propre (Ctrl+C)

Objectif : ranger les ports virtuels et l’USB proprement.

1. Remets le focus sur la fenêtre console du Bridge.
2. Appuie sur **Ctrl+C** (une fois, puis attends).
3. Le processus doit se terminer.
4. Dans ShowMIDI / DAW, **rafraîchis** la liste des ports : les `MT4 Port N` de cette session doivent **disparaître** (ou ne plus être sélectionnables comme ports live).

Validation :

- Ctrl+C termine le Bridge : ✅ / ❌
- Ports `MT4 Port N` disparus après arrêt : ✅ / ❌

📌 Remarques GD :



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

- Prêt à enchaîner Epic 2 côté usage notes/CC : oui / non / avec réserves  
- Réserves en une phrase :



**Ce qui a le mieux marché**



**Ce qui a bloqué ou surpris**



**À retester / à corriger ensuite**



**Fichiers / logs utiles** (chemins, captures, messages console)



📌 Remarques GD (bilan) :



---

## En cas de blocage — pistes rapides

| Symptôme | Piste |
|---|---|
| `--open-device` échoue, parle du GUID | Bind INF pas fait, ou mauvais nœud USB ; vérifier `MI_02`. En secours : Zadig + `--dev-zadig`. |
| Session échoue, parle de VirtualMIDI / DLL | Installer loopMIDI / teVirtualMIDI ; rouvrir le terminal après install. |
| Aucun port `MT4 Port N` | La session n’a pas démarré ; ou ShowMIDI ouvert **avant** sans refresh. |
| Ports présents mais silence total | Mauvais câble physique ; mauvais port choisi dans ShowMIDI/DAW ; MT4 pas alimenté / autre interface USB. |
| Notes OK sur un port, mélangées sur plusieurs OUT | Noter précisément ports + séquence — c’était le bug d’intégration corrigé ; si ça revient, c’est un **❌** prioritaire. |
| Session qui meurt toute seule | Copier le message anglais de la console (échec pompe / SendToHost / WriteBulk). |
| Ports qui restent après Ctrl+C | Noter ; éventuellement relancer Windows avant le prochain essai. |

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
