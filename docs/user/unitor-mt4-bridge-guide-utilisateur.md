---
organization: Ten Square Software
project: unitor-win64-driver
title: Unitor MT4 Bridge — Guide utilisateur
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
version: "1.1"
product_version: "0.1.0"
---

Ce guide explique comment installer et utiliser **Unitor MT4 Bridge** avec une interface MIDI **Emagic MT4** sous Windows 10 ou 11 (64 bits).

C’est un projet **hobby / open source** (façade Ten Square Software) : sources gratuites sur GitHub, **pas** de certificat de signature dans cette ligne de release, et **pas** de promesse que Setup seul réussit toujours sur un PC neuf.

Suivez les sections dans l’ordre. Après Bridge + virtualMIDI + **association WinUSB** réussis, vous pourrez envoyer et recevoir du MIDI le jour même, puis faire un premier échange SysEx. Sur un PC **propre**, Setup **échoue** souvent à l’association WinUSB sans catalogue de confiance — utilisez alors les étapes **WinUSB guidé** (par exemple **Zadig**) sous [Association USB (WinUSB) en échec](#association-usb-winusb-en-échec).

Version anglaise : [`unitor-mt4-bridge-user-guide.md`](unitor-mt4-bridge-user-guide.md).

## Sommaire

1. [Prérequis](#prérequis)
2. [Installation](#installation)
3. [Démarrage automatique](#démarrage-automatique)
4. [Premier test MIDI](#premier-test-midi)
5. [Premier test SysEx](#premier-test-sysex)
6. [Dépannage](#dépannage)
7. [Débrancher et rebrancher la MT4](#débrancher-et-rebrancher-la-mt4)
8. [Ce qui marche / ce qui ne marche pas](#ce-qui-marche--ce-qui-ne-marche-pas)
9. [Deux interfaces MT4](#deux-interfaces-mt4)

---

# Prérequis

Avant l’installation, préparez :

| Besoin | Détail |
|---|---|
| Ordinateur | **Windows 10** ou **Windows 11**, **64 bits** |
| Matériel | Une interface MIDI **Emagic MT4** |
| virtualMIDI (intérimaire) | Le pilote **[virtualMIDI](https://www.tobias-erichsen.de/software/virtualmidi.html)** (Tobias Erichsen), déjà installé — chemin **lab / Bridge actuel** |

## Installer virtualMIDI (chemin lab intérimaire)

Aujourd’hui, le Bridge s’appuie sur **virtualMIDI** pour créer les ports MIDI virtuels visibles dans votre DAW. Un backend **Windows MIDI Services** sous **Windows 11** est prévu pour les binaires communautaires prêts à l’emploi (sans redistribuer le SDK propriétaire virtualMIDI). En attendant, installez virtualMIDI avant le Bridge, par exemple avec :

- **[loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html)** (choix le plus courant), ou
- **[rtpMIDI](https://www.tobias-erichsen.de/software/rtpmidi.html)**

Ensuite, vérifiez que le fichier `teVirtualMIDI.dll` se trouve dans `C:\Windows\System32\`. Sans ce pilote, l’installation du Bridge ne peut pas aboutir.

## Logiciels utiles ensuite

Ces programmes ne sont pas fournis avec le Bridge ; installez-les si vous en avez besoin :

| Usage | Exemple |
|---|---|
| Premier test MIDI | Ableton Live 12 ou MIDI-OX |
| Premier test SysEx | Matrix-Control, ou MIDI-OX |

# Installation

1. Branchez la **MT4** (alimentation + USB) **avant** ou pendant Setup — l’assistant **ne s’arrête pas** pour vous demander de la brancher.
2. Téléchargez et lancez `UnitorMt4Bridge-Setup.exe`.
3. Acceptez le dossier proposé, en général :

   `C:\Program Files\Ten Square Software\Unitor MT4 Bridge\`

4. Autorisez l’élévation Administrateur **une seule fois** (Program Files et association USB).
5. Attendez l’écran indiquant que l’installation a réussi — ou un écran **incomplet** si une porte a échoué (ce n’est pas une install communautaire réussie).

L’installation réussit lorsque :

- virtualMIDI est bien détecté,
- l’association USB (WinUSB) est OK,
- le démarrage automatique est enregistré pour votre compte Windows.

Si quelque chose manque (par exemple virtualMIDI), l’assistant affiche un message d’aide : corrigez le point indiqué, puis relancez l’installeur.

**WinUSB** est le composant USB standard de Microsoft (déjà dans Windows). Ce qui échoue souvent sur un PC propre, c’est *associer cette MT4 Emagic* à WinUSB lorsque l’INF du projet n’a pas de catalogue de confiance — ce projet hobby **ne fournit pas** de certificat de signature.

**Honnêteté install hobby :**

- Si l’étape WinUSB de Setup **réussit**, c’est bon pour l’USB — pas d’autre outillage au quotidien.
- Si l’étape WinUSB de Setup **échoue** (fréquent sur PC propre), c’est attendu sans catalogue payant. Suivez [Association USB (WinUSB) en échec](#association-usb-winusb-en-échec) — l’association **guidée avec Zadig** (ou le chemin INF contributeur) est le correctif supporté, pas « attendre un certificat ».
- Les scripts de signature lab **ne sont pas** la confiance communautaire.

## Windows SmartScreen (Setup non signe ou non reconnu)

Windows peut afficher **Microsoft Defender SmartScreen** — souvent « Windows a protégé votre PC » ou un avertissement d’application non reconnue — lorsque vous ouvrez `UnitorMt4Bridge-Setup.exe`. Cela peut arriver si une build communautaire est **non signée**, ou si elle est signée mais **pas encore assez réputée**. Un avertissement ne signifie **pas** automatiquement que le fichier est un malware.

**Ne continuez que si vous avez téléchargé le Setup depuis la page de téléchargement / Releases de ce projet** (pas un miroir tiers au hasard). Tant qu’une URL figée n’est pas publiée avec la première release publique taguée, utilisez la page Releases ou de téléchargement de ce dépôt.

Pour vérifier si **ce** fichier Setup est signé : clic droit → **Propriétés** → **Signatures numériques**. Si cet onglet est absent, le fichier est en général **non signé**. Ce projet **ne fournit pas** de certificat Authenticode ; attendez-vous à des builds communautaires non signées lorsque des binaires seront publiés.

Lorsque la stratégie de votre PC le permet :

1. Choisissez **Plus d’infos** (**More info**).
2. Choisissez **Exécuter quand même** (**Run anyway**).

Autre solution si le fichier vient du web : clic droit sur le Setup → **Propriétés** → cochez **Débloquer** (**Unblock**) s’il est proposé (marque Zone.Identifier), puis Appliquer / OK, et relancez le Setup.

**Honnêteté :**

- Sur un PC **entreprise / géré**, la stratégie peut bloquer entièrement **Exécuter quand même** — contactez l’administrateur du PC, ou utilisez une machine personnelle. Ne comptez pas sur le contournement dans tous les cas.
- Ne **désactivez pas** SmartScreen globalement, ne mettez pas en liste blanche des dossiers entiers, et n’exécutez pas de copies depuis des miroirs non fiables.
- SmartScreen est en général plus simple à passer que le frein catalogue WinUSB — voir [Association USB (WinUSB) en échec](#association-usb-winusb-en-échec).

Politique de signature pour contributeurs / release (page technique en anglais) : [`docs/dev/authenticode-and-smartscreen.md`](../dev/authenticode-and-smartscreen.md).

# Démarrage automatique

Après l’installation, Unitor MT4 Bridge se lance avec votre session Windows. Les ports MIDI apparaissent sans que vous ayez à ouvrir le programme à chaque fois, et sans mot de passe Administrateur au quotidien.

Le Bridge est un **programme de session utilisateur** normal. Ce n’est **pas** un service Windows, et vous ne devez pas l’installer comme tel.

## Comment savoir que tout est prêt

1. Connectez-vous à Windows (ou branchez la MT4 si vous êtes déjà connecté).
2. Ouvrez votre DAW ou MIDI-OX.
3. Vérifiez la présence des ports **`MT4 In 1`**, **`MT4 In 2`**, **`MT4 Out 1`** … **`MT4 Out 4`**.

Si ces noms sont là, le Bridge tourne correctement.

Vous pouvez aussi lancer **Unitor MT4 Bridge** depuis le menu Démarrer : cela démarre la même session.

## Arrêter le démarrage automatique

Dans le menu Démarrer, choisissez **Unregister Auto-Start** si vous ne voulez plus que le Bridge se lance à la connexion. La désinstallation retire aussi ce démarrage automatique **pour l’utilisateur Windows qui lance la désinstallation**. Les autres comptes Windows sur le même PC peuvent garder leur propre entrée — ouvrez **Unregister Auto-Start** (ou lancez `Bridge.exe --unregister-auto-start`) en étant connecté avec ce compte.

La désinstallation retire les fichiers du Bridge. L’association WinUSB de la MT4 **peut rester** dans le magasin de pilotes Windows jusqu’à ce qu’un administrateur retire ce paquet — c’est normal et ne maintient pas à lui seul le démarrage automatique.

# Premier test MIDI

Objectif : envoyer et recevoir des notes (et éventuellement des CC) entre la MT4 et votre logiciel.

## Ports à utiliser

| Direction | Noms dans Windows |
|---|---|
| Entrées | `MT4 In 1`, `MT4 In 2` |
| Sorties | `MT4 Out 1`, `MT4 Out 2`, `MT4 Out 3`, `MT4 Out 4` |

## Étapes

1. Vérifiez que l’[installation](#installation) et le [démarrage automatique](#démarrage-automatique) sont en place (ou lancez une fois **Unitor MT4 Bridge**).
2. Branchez la MT4.
3. Ouvrez Ableton Live 12 ou MIDI-OX.
4. Activez au moins **`MT4 In 1`** et **`MT4 Out 1`**.
5. Envoyez des notes ou des CC depuis un clavier vers une entrée de la MT4, ou depuis le logiciel vers une sortie, et vérifiez l’autre sens.

Quand vous voyez de l’activité sur au moins une entrée et une sortie, le premier test MIDI est réussi.

### Computer Mode

Pour que la MT4 réponde correctement, envoyez d’abord un peu de MIDI « classique » — par exemple une courte commande CC ou quelques notes. Cela active le **Computer Mode**.

**Le SysEx seul ne réveille pas le Computer Mode.** Envoyez toujours du MIDI ordinaire (notes ou CC) avant un échange SysEx. Voir aussi [Premier test SysEx](#premier-test-sysex).

## Plusieurs logiciels en même temps

Vous pouvez ouvrir plusieurs applications sur les mêmes ports (par exemple Ableton et MIDI-OX). virtualMIDI accepte jusqu’à environ huit applications par port. Si un logiciel indique que le port est déjà utilisé, regardez dans ses options MIDI s’il y a un mode « exclusif » à désactiver.

# Premier test SysEx

Objectif : envoyer et recevoir des messages SysEx (System Exclusive) via la MT4, par exemple pour un dump / restore avec un éditeur.

## Avec Matrix-Control

Matrix-Control est un éditeur externe utilisé ici comme **cible de validation** (à installer séparément ; ce n’est pas une dépendance d’exécution du Bridge). Procédure recommandée :

1. Le Bridge tourne et les ports `MT4 In` / `MT4 Out` sont visibles ([Premier test MIDI](#premier-test-midi)).
2. Envoyez un court CC pour activer le **Computer Mode** (le SysEx seul ne le réveille pas).
3. Dans Matrix-Control, choisissez les ports virtuels qui correspondent au câble vers votre synthé Matrix.
4. Lancez un dump et un restore (ou une écriture de patch) jusqu’à la fin.

L’échange doit pouvoir se terminer sans relancer le Bridge.

## Avec MIDI-OX

Si vous n’avez pas Matrix-Control :

1. Ouvrez MIDI-OX.
2. Sélectionnez les ports `MT4 In` / `MT4 Out`.
3. Envoyez d’abord du MIDI ordinaire (CC ou notes) si le Computer Mode peut être endormi.
4. Envoyez un fichier SysEx et vérifiez la réponse (ou un aller-retour sur le câble testé).

## Si vous débranchez pendant un dump

Rebranchez la MT4, demandez à votre logiciel de rescanner les ports MIDI, et si besoin relancez **Unitor MT4 Bridge**. Voir [Débrancher et rebrancher la MT4](#débrancher-et-rebrancher-la-mt4). Un redémarrage de Windows **n’est pas** le chemin de récupération.

# Dépannage

## SmartScreen bloque le Setup (« Windows a protégé votre PC »)

Voir [Windows SmartScreen (Setup non signe ou non reconnu)](#windows-smartscreen-setup-non-signe-ou-non-reconnu). Vérifiez que le téléchargement vient de la page de téléchargement / Releases de ce projet, puis utilisez **Plus d’infos → Exécuter quand même** lorsque la stratégie le permet, ou Propriétés → **Débloquer**. Ne désactivez pas SmartScreen globalement.

## virtualMIDI manquant

Installez **loopMIDI** ou **rtpMIDI**, vérifiez `teVirtualMIDI.dll` dans `C:\Windows\System32\`, puis relancez l’installeur Unitor MT4 Bridge.

## Association USB (WinUSB) en échec

Setup indique qu’il n’a pas pu associer la MT4 à WinUSB, et l’installation est souvent annulée (fichiers programme / entrée Ajout-Suppression non laissés comme une install réussie).

Cause fréquente sur un PC **propre** : Windows refuse un paquet pilote **non signé** / sans catalogue de confiance (erreur lab souvent `0xE000022F`). Rebrancher la MT4 et relancer Setup aboutit en général au **même** échec — ce projet **ne fournit pas** de certificat de production pour « corriger » cela.

### Correctif supporté sans certificat payant — WinUSB guidé (Zadig)

1. Vérifiez que la MT4 est branchée et alimentée.
2. Vérifiez que vous avez lancé le Setup du projet (voir l’honnêteté SmartScreen ci-dessus) — pas un miroir au hasard.
3. Lisez le message Setup : s’il parle d’un paquet pilote non signé, c’est le frein catalogue (pas « j’ai oublié de brancher la MT4 »).
4. Utilisez **[Zadig](https://zadig.akeo.ie/)** (ou une autre association guidée documentée) pour associer **WinUSB** à l’interface MIDI composite MT4 **MI_02** (`USB\VID_086A&PID_0003&MI_02`) — pas une interface USB voisine au hasard.
5. Installez / replacez le Bridge (relancez Setup s’il a annulé, ou utilisez un build contributeur), assurez-vous que virtualMIDI est présent, puis lancez **Unitor MT4 Bridge**.
6. Contributeurs qui préfèrent l’INF du dépôt : voir [`docs/dev/winusb-bind.md`](../dev/winusb-bind.md).

Quand l’association **réussit**, le Gestionnaire de périphériques doit montrer l’interface MIDI de la MT4 sous WinUSB.

## Aucun port visible

Vérifiez dans l’ordre :

1. virtualMIDI installé
2. Bridge démarré (démarrage automatique ou lancement manuel)
3. MT4 branchée
4. Rescan des périphériques MIDI dans votre DAW ou MIDI-OX

Ports attendus : `MT4 In 1`…`MT4 In 2`, `MT4 Out 1`…`MT4 Out 4`.

## Pas de réponse SysEx

Envoyez d’abord un CC ou des notes pour activer le Computer Mode (**le SysEx seul ne le réveille pas**), puis recommencez l’échange SysEx.

## Ports absents après un débranchement

1. Attendez quelques instants (le Bridge recrée la session).
2. Rescannez le MIDI dans votre logiciel.
3. Si besoin, quittez puis relancez **Unitor MT4 Bridge**, puis rescanner.

Ne traitez **pas** un redémarrage de Windows comme le correctif normal. Voir [Débrancher et rebrancher la MT4](#débrancher-et-rebrancher-la-mt4).

## Message « port already in use »

Fermez l’autre application, ou désactivez le mode MIDI exclusif dans la DAW. Vous pouvez en général laisser jusqu’à huit logiciels sur le même port.

# Débrancher et rebrancher la MT4

Vous pouvez débrancher puis rebrancher la MT4 pendant une session.

1. Débranchez la MT4 : les ports de cette interface disparaissent.
2. Rebranchez-la : le Bridge recrée les ports.
3. Si votre DAW ne les voit pas tout de suite, lancez un rescan MIDI.
4. Si besoin, relancez **Unitor MT4 Bridge**, puis rescanner.

**Récupération qui compte comme OK :** rescan MIDI dans le logiciel hôte, et/ou relance supervisée du Bridge (quitter puis relancer).

**Récupération qui compte comme échec :** devoir faire un **redémarrage Windows** complet pour retrouver les ports. Vous ne devez pas avoir besoin de redémarrer Windows pour un débranchement / rebranchement.

Pour quitter le Bridge proprement lorsqu’il est ouvert dans une fenêtre console, utilisez **Ctrl+C**.

# Ce qui marche / ce qui ne marche pas

## Ce qui marche (V1)

Avec Unitor MT4 Bridge et une MT4 sous Windows 10 / 11 64 bits :

- Lancer l’assistant fourni (association Microsoft WinUSB — pas de pilote noyau custom), **lorsque Windows accepte le paquet pilote**
- Utiliser **2 entrées** et **4 sorties** virtuelles (`MT4 In` / `MT4 Out`) après une install réussie
- Faire circuler notes, CC, horloge / transport, MTC et SysEx (échelle éditeur / librarian)
- Profiter du démarrage automatique sans Administrateur au quotidien (Bridge en session utilisateur, pas un service Windows)
- Débrancher et rebrancher la MT4, puis récupérer les ports par rescan ou relance du Bridge (sans redémarrer Windows)
- Ouvrir plusieurs logiciels sur les mêmes ports (dans la limite de virtualMIDI, environ huit par port)
- Brancher une seconde MT4 lorsque vous en avez une (voir [Deux interfaces MT4](#deux-interfaces-mt4))
- Consulter les ancres de timing studio publiées pour le chemin MIDI (latence saine ≤4–5 ms p99 et jitter classique ≤1–2 ms p99, Gate **(a)** du 2026-08-11 sur une boucle DIN Win10 Out2→In2 en lab calme — **cibles de lab, pas une garantie de session DAW** ; méthode sous [`docs/dev/measurements/`](../dev/measurements/))

## Ce que cette version ne fait pas

N’attendez **pas** ceci :

- Une association WinUSB réussie via Setup seul sur tout PC propre **sans** étape guidée (ce projet hobby **ne fournit pas** de certificat — voir [Association USB (WinUSB) en échec](#association-usb-winusb-en-échec))
- Une expérience d’installeur commercial « un clic sur PC neuf » comme objectif du projet
- Des Releases GitHub publiques de binaires Bridge/Setup liés au SDK propriétaire virtualMIDI (hors scope community — binaires communautaires prévus après **Windows MIDI Services** sous Win11)
- Patch mode, LTC/VITC, Fast Mode / fonctions AMT des manuels Unitor
- Topologies Emagic en cascade / empilées
- Support garanti AMT8 / Unitor8 sans matériel validé pour ces modèles
- Windows MIDI Services comme backend **déjà** livré (prochain backend community, Win11 seulement)
- Un pilote MIDI noyau custom
- Un SmartScreen silencieux sur un téléchargement non signé (voir [Windows SmartScreen](#windows-smartscreen-setup-non-signe-ou-non-reconnu))

Voir aussi : [Licence et backends MIDI](../dev/license-and-backends.md) — MIT ≠ virtualMIDI (lab intérimaire) ≠ Windows MIDI Services (prochaine community). Politique de signature : [Authenticode and SmartScreen](../dev/authenticode-and-smartscreen.md) (pas d’achat de certificat dans ce projet hobby).

# Deux interfaces MT4

| Interface | Noms des ports |
|---|---|
| Première | `MT4 In N` / `MT4 Out N` |
| Deuxième et suivantes | `MT4 #2 In N` / `MT4 #2 Out N`, etc. |

Chaque interface a son propre jeu de ports. Débrancher l’une ne change pas les noms de l’autre.

Si vous n’avez qu’une MT4, ignorez les noms avec `#2` jusqu’à en avoir besoin.

**Honnêteté de validation :** l’usage quotidien et le nommage pour **une** MT4 physique sont le chemin prouvé documenté ici. Le nommage dual-MT4 est implémenté ; traitez une seconde unité comme supportée côté logiciel, mais ne supposez pas une preuve lab dual fermée à partir de ce guide seul.
