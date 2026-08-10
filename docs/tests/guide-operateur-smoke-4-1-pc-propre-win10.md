---
organization: Ten Square Software
project: unitor-win64-driver
title: Guide opérateur — Smoke 4.1 sur PC Win10 propre
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
---

# Guide opérateur — valider l’installateur public (Story 4.1) sur un PC Win10 propre

Ce guide te dit **quoi faire aujourd’hui**, dans l’ordre, sur une machine Windows 10 64 bits qui n’a pas encore le Bridge / l’installateur Unitor.  
Le tableau officiel à remplir reste : [`smoke-epic4-public-installer-mt4.md`](smoke-epic4-public-installer-mt4.md) (colonne **Win10 x64**).

**Règle d’honnêteté :** case vide ≠ Pass. Note **Pass**, **Fail** ou **N/A** + une courte raison.

**Astuce lab :** crée un **point de restauration Windows** dès que le PC est encore propre (avant loopMIDI / Setup Unitor). Tu pourras ensuite revenir à cet état et **rejouer** le smoke autant de fois que besoin.

---

## Ce que tu valides (en une phrase)

Un musicien sur PC propre peut installer le Bridge via notre installateur, associer le MT4 à WinUSB, avoir le démarrage auto, et voir un succès **seulement** quand tout est vraiment OK — sans Zadig comme chemin principal.

---

## Matériel et logiciels à avoir sous la main

| Élément | Pourquoi |
|---|---|
| PC **Windows 10 x64** « propre » | Claim lab obligatoire pour la 4.1 |
| **MT4** USB + alimentation | Lignes WinUSB / ports |
| Clé USB (ou autre transfert) | Apporter le fichier `UnitorMt4Bridge-Setup.exe` |
| Installateur **loopMIDI** ou **rtpMIDI** (Tobias) | Installe le pilote virtualMIDI (`teVirtualMIDI.dll`) |
| MIDI-OX ou une DAW (Live, etc.) | Voir les ports après install (optionnel mais utile pour la ligne 6) |
| Droits **Administrateur** une fois | Créer / restaurer un point de restauration ; UAC de l’installateur Unitor |
| Ce guide + le fichier smoke ouvert pour noter | Remplir la matrice au fil de l’eau |

**Ce PC ne doit pas avoir déjà :**

- Unitor MT4 Bridge / Ten Square Software installé
- Association WinUSB MT4 faite à la main / Zadig « pour gagner du temps »
- Auto-Start Bridge déjà enregistré

Si tu n’es pas sûr : Paramètres → Applications → chercher « Unitor » / « Ten Square », et Gestionnaire de périphériques → le MT4 ne doit pas déjà être sous « WinUSB » avant le test.

---

## Avant d’arriver sur le PC propre (machine de build)

Sur ta machine de lab / de build Windows (celle qui a déjà le dépôt et Inno Setup) :

1. Compile un `Bridge.exe` (de préférence **Release**).
2. À la racine du dépôt :

```powershell
python scripts\packaging\verify-installer-contract.py
.\scripts\packaging\build-public-installer.ps1
```

3. Récupère le fichier :

```text
builds\installer\UnitorMt4Bridge-Setup.exe
```

4. Copie-le sur la clé USB (avec ce guide si tu veux).

Tu n’as **pas** besoin de Visual Studio ni d’Inno sur le PC propre pour jouer le smoke : l’EXE Setup suffit.

---

## Ordre des tests (important)

Crée d’abord le **point de restauration** (PC encore propre), puis le **refus** quand virtualMIDI manque, **puis** loopMIDI/rtpMIDI, **puis** le parcours succès, **puis** désinstall. Pour rejouer : restaure le point, recommence depuis la ligne 4.

Sans point de restauration, une fois loopMIDI / WinUSB / Auto-Start installés, repartir « propre » pour la ligne 4 devient long et approximatif.

```text
0) Contrôle PC propre + POINT DE RESTAURATION « avant smoke 4.1 »
1) virtualMIDI ABSENT  → lance Setup → doit BLOQUER  (ligne 4)
2) Installe loopMIDI ou rtpMIDI
3) MT4 branché         → Setup jusqu’au succès     (lignes 1–3, 5, 7)
4) Vérifie démarrage auto / ports                  (ligne 6)
5) Commandes lab depuis le dossier installé        (ligne 9)
6) Désinstalle                                     (ligne 8)
7) Remplis la matrice smoke
8) (Optionnel) Restaure le point → rejoue depuis (1)
```

---

## Étape 0 — Contrôle « PC propre »

Sur le PC Win10 :

1. Connecte-toi avec un compte **utilisateur normal** (celui qui utilisera le Bridge au quotidien).
2. Vérifie qu’il n’y a pas déjà Unitor / Ten Square dans Applications.
3. Branche le MT4 une seconde : Gestionnaire de périphériques — note l’état (souvent « périphérique inconnu » ou autre **avant** notre install). Débranche si tu veux, ou laisse branché pour plus tard. (GD : état = "Périphérique inconnu")
4. **Ne lance pas Zadig.**
5. **Ne lance pas encore** Setup Unitor ni loopMIDI / rtpMIDI — le point de restauration doit capturer cet état.

Ouvre [`smoke-epic4-public-installer-mt4.md`](smoke-epic4-public-installer-mt4.md) et prépare la colonne Win10.

---

## Étape 0b — Point de restauration Windows (à faire maintenant)

But : figer un Windows **sans** Unitor, **sans** association WinUSB MT4 faite par nous, et de préférence **sans** virtualMIDI encore installé pour ce smoke — pour pouvoir tout remettre d’un coup après un essai.

### Limites (honnêteté lab)

- Un point de restauration annule surtout **fichiers système, pilotes, réglages Windows** liés à la protection système. Ce n’est pas une image disque complète (pas un clone BitLocker / pas un « reset usine »).
- Les **fichiers personnels** dans Documents / Bureau ne sont en général **pas** effacés par une restauration — pratique pour garder notes et `UnitorMt4Bridge-Setup.exe` sur le Bureau ou une clé USB.
- Après restauration, vérifie quand même Applications + `teVirtualMIDI.dll` + absence d’Unitor (voir contrôle ci-dessous). Si quelque chose reste, note-le ; au pire, désinstalle à la main ou refais un point plus tôt la prochaine fois.
- La **protection système** doit être **activée** sur le disque système (souvent `C:`). Sinon Windows refuse de créer un point.

### Activer la protection système (si besoin)

1. Barre de recherche Windows → tape `Créer un point de restauration` → ouvre le résultat (panneau **Propriétés système** / Protection du système).
2. Onglet **Protection du système**.
3. Sélectionne le disque système (`C:`) → **Configurer…**
4. Choisis **Activer la protection** → alloue un peu d’espace (ex. 5–10 Go si le disque le permet) → OK.

### Créer le point (avant tout install du smoke)

1. Toujours dans **Protection du système** → **Créer…**
2. Description claire, par exemple :

```text
Avant installation MT4
```

3. Attends la confirmation de succès. Garde le nom / l’heure en tête (ou une photo d’écran).
4. Option rapide en PowerShell **Administrateur** (équivalent) :

```powershell
Checkpoint-Computer -Description "Avant installation MT4" -RestorePointType MODIFY_SETTINGS
```

Si la commande échoue avec un message sur la fréquence des points : Windows limite parfois la création ; attends quelques minutes, ou crée-le via l’interface graphique ci-dessus.

**Ne continue le smoke qu’après** ce point créé avec succès.

### Restaurer ce point (après un essai, pour rejouer)

1. Ferme Bridge / Setup / DAW.
2. Recherche → `Créer un point de restauration` → **Protection du système** → **Restauration du système…**
3. **Suivant** → choisis le point `Avant installation MT4` (ou le tien) → suis l’assistant (redémarrage).
4. Après reboot, contrôle rapide « état propre » :
   - Pas d’entrée **Unitor MT4 Bridge** / Ten Square dans Applications
   - Pas de `C:\Program Files\Ten Square Software\Unitor MT4 Bridge\`
   - `C:\Windows\System32\teVirtualMIDI.dll` **absent** si tu veux rejouer la **ligne 4** (sinon réinstalle / désinstalle virtualMIDI selon le scénario)
   - MT4 non déjà forcé sous WinUSB « à la main »
5. Reprends à **Étape 1** (ligne 4). Tu peux laisser le **même** point de restauration tant que tu restaures **avant** de le recréer ; recrée un point neuf seulement si tu as installé d’autres logiciels hors smoke entre deux campagnes.

### Variante plus lourde (si tu rejoues souvent)

Sur une machine dédiée lab : **machine virtuelle** + instantané (snapshot) Hyper-V / VMware / VirtualBox, ou image disque. Plus fiable qu’un point de restauration pour un cycle quotidien — hors scope minimal de ce guide, mais meilleur si ce PC reste un banc de test.

---

## Étape 1 — Ligne 4 : virtualMIDI manquant (doit bloquer)

**Prérequis :** `teVirtualMIDI.dll` **absent** de `C:\Windows\System32\` (PC vraiment propre = souvent déjà le cas).

1. Lance `UnitorMt4Bridge-Setup.exe` (double-clic).
2. Windows / SmartScreen peut avertir (binaire non signé public) : note-le, continue seulement si tu acceptes le risque lab — ce n’est **pas** le smoke Authenticode (story 4.4). (GD : modale "Voulez-vous autoriser cette application provenant d'un éduteur inconnu à apporter des modifications à votre ordinateur" - en-tête fond jaune)
3. **Attendu :** une boîte d’erreur **en anglais** qui explique d’installer loopMIDI ou rtpMIDI, puis l’installateur **s’arrête**.  
   Pas d’écran de succès. Pas de « Unitor MT4 Bridge » dans Applications. (GD : OK)
4. Note **Pass** ou **Fail** sur la ligne **4**.

Si virtualMIDI est déjà là par surprise : désinstalle loopMIDI/rtpMIDI (ou le produit Tobias), redémarre si besoin, vérifie l’absence de `teVirtualMIDI.dll`, recommence cette étape.

---

## Étape 2 — Installer virtualMIDI (prérequis produit)

1. Installe **loopMIDI** ou **rtpMIDI** (chemin communauté documenté).
2. Vérifie que `C:\Windows\System32\teVirtualMIDI.dll` existe.
3. Redémarre Windows si l’installateur Tobias le demande.

GD : OK

---

## Étape 3 — Lignes 1, 2, 3, 5, 7 : parcours succès avec MT4

1. Branche le **MT4** (alimentation + USB).
2. Relance `UnitorMt4Bridge-Setup.exe`.
3. Accepte l’**UAC** une fois (admin pour Program Files + association pilote) — c’est normal.
4. Parcours le wizard en observant :
   - peu d’étapes, peu de jargon → ligne **1**
   - barre / statut pendant copie et association → ligne **2**
5. **Attendu en fin :**
   - écran de succès **seulement** si virtualMIDI OK **et** WinUSB OK **et** Auto-Start enregistré → ligne **3**
   - un seul UAC à l’install ; pas de demande admin pour le Bridge au quotidien → ligne **7**
6. Gestionnaire de périphériques : le MT4 (interface MIDI / HWID attendu) doit apparaître sous **WinUSB** (ou équivalent Microsoft WinUSB) → ligne **5**.

Chemin d’install attendu :

```text
C:\Program Files\Ten Square Software\Unitor MT4 Bridge\Bridge.exe
```

### Si l’association WinUSB échoue (INF non signé)

Sur un PC vraiment propre, Windows peut **refuser** un INF sans catalogue / signature publique. C’est un frein lab connu (mitigation : script lab, pas Authenticode public).

- Si Setup affiche un échec d’association et **pas** de succès : note **Fail** honnête sur la ligne **5** (et probablement **3**), avec la raison « INF unsigned / catalog ».
- Mitigation lab (seulement si tu as le dépôt + Windows SDK sur une machine admin, souvent **pas** le PC communauté) : `installer\sign-lab-package.ps1` — ce n’est **pas** la signature publique. Documente ce que tu as fait.

Ne contourne **pas** avec Zadig pour « sauver » un Pass de la ligne 5 : ce ne serait plus le parcours installateur.

---

## Étape 4 — Ligne 6 : démarrage auto et ports

Sans lancer le Bridge à la main :

1. Vérifie qu’une entrée Auto-Start existe :
   - Planificateur de tâches : tâche du type `UnitorMt4BridgeAutoStart`, **ou**
   - clé Exécution utilisateur (HKCU Run) pointant vers `Bridge.exe --auto-session`
2. Fais **au moins une** de ces preuves :
   - **A :** déconnexion / reconnexion Windows avec MT4 déjà branché → `Bridge.exe` tourne sous ton utilisateur, ports visibles dans MIDI-OX / DAW  
   - **B :** redémarrage PC avec MT4 branché → idem  
   - **C :** login **sans** MT4, puis brancher le MT4 → attente / apparition des ports (peut prendre jusqu’à ~15 min max contrat ; en pratique souvent bien moins)
3. Pas de nouvel UAC au lancement quotidien.
4. Arrêt propre préféré : **Ctrl+C** dans la console Bridge si elle est visible (évite la croix seule).

Note **Pass** / **Fail** ligne **6**.

---

## Étape 5 — Ligne 9 : commandes lab depuis l’install

Ouvre une invite **non admin** :

```powershell
cd "C:\Program Files\Ten Square Software\Unitor MT4 Bridge"
.\Bridge.exe --version
.\Bridge.exe --test-port-names
.\Bridge.exe --test-mapper
```

(Adapte si une commande demande une session active / MT4 branché — note ce que tu obtiens.)

Option session manuelle lab :

```text
.\Bridge.exe --start-session
```

Note **Pass** / **Fail** ligne **9**.

---

## Étape 6 — Ligne 8 : désinstallation propre

1. Paramètres → Applications → **Unitor MT4 Bridge** (éditeur Ten Square Software) → Désinstaller.
2. Accepte UAC si demandé.
3. Vérifie :
   - le dossier Program Files a disparu (ou est vide / retiré)
   - Auto-Start retiré (plus de tâche / plus de Run)
4. Déconnecte-toi / reconnecte-toi (ou reboot) : le Bridge **ne** doit **pas** se relancer.

**Honnêteté :** des entrées résiduelles dans le magasin de pilotes Windows peuvent rester — ce n’est pas un Fail si Auto-Start et les binaires sont partis. Note-le si tu le vois.

Note **Pass** / **Fail** ligne **8**.

---

## Étape 7 — Remplir la matrice et conclure

Dans [`smoke-epic4-public-installer-mt4.md`](smoke-epic4-public-installer-mt4.md), colonne **Win10 x64** :

| Ligne | Sujet |
|---|---|
| 1 | Peu d’étapes / jargon minimal |
| 2 | Progression visible |
| 3 | Succès seulement si les trois portes OK |
| 4 | virtualMIDI manquant → blocage |
| 5 | WinUSB sur MT4 après install |
| 6 | Auto-Start + ports sans lancement manuel |
| 7 | Un seul admin à l’install |
| 8 | Désinstall retire Auto-Start |
| 9 | Commandes lab OK depuis l’install |

Ajoute en Notes : date, build (`UnitorMt4Bridge-Setup.exe` / version si connue), et tout écart (SmartScreen, INF refusé, reboot demandé par `pnputil`, etc.).

**Win11 :** laisse vide ou N/A si tu n’as pas testé aujourd’hui — ce n’est pas bloquant pour clore le claim Win10.

---

## Ce que tu ne fais pas dans ce smoke

- Prétendre que la doc utilisateur (4.2) ou Authenticode (4.4) sont validées
- Utiliser Zadig comme chemin « officiel » pour forcer un Pass
- Embed / redistribuer le MSI Tobias sans feu vert (hors sujet 4.1)
- Remplir Pass sur une case non jouée

---

## Après la session (retour projet)

1. Si tu veux **rejouer** demain sur la même machine : **Restauration du système** vers le point « Avant smoke 4.1… » (Étape 0b), plutôt que de tout désinstaller à la main.
2. Commit / mise à jour de la matrice smoke **sur demande** (ne commit pas sans qu’on te le demande).
3. Si tout Pass Win10 : la story 4.1 peut passer à `done` côté lab (il peut rester le raccord « une seule version Bridge / installateur » côté code — autre sujet).
4. Si Fail INF non signé : note-le clairement ; ce n’est pas un Pass déguisé.

Bonne session lab.

---

## Résultat session lab — 2026-08-10

Matrice officielle remplie dans [`smoke-epic4-public-installer-mt4.md`](smoke-epic4-public-installer-mt4.md).

| Ligne | Win10 | En bref |
|---|---|---|
| 1 | Pass | Wizard court OK ; textes d’échec tronqués (corrigés après coup dans le Setup, non rejoués) |
| 2 | Pass | Progression visible |
| 3 | Pass | Pas de faux succès → « Installation incomplete » + annulation |
| 4 | Pass | virtualMIDI absent → blocage + message anglais |
| 5 | **Fail** | INF non signé refusé (`0xE000022F`) |
| 6 | N/A | Pas atteint (rollback) |
| 7 | Pass | Un UAC à l’install |
| 8 | N/A | Rien laissé d’installé |
| 9 | N/A | Dossier install non conservé |

**Verdict :** le Setup communautaire **refuse correctement** de mentir quand WinUSB n’est pas associé ; le parcours succès complet attend un catalogue / signature de confiance (hors mitigation lab).
