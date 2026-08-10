---
organization: Ten Square Software
project: unitor-win64-driver
title: Unitor MT4 Bridge — Manuel utilisateur
author: Guillaume DUPONT
created: 2026-08-10
updated: 2026-08-10
version: "1.0"
product_version: "0.1.0"
---

Ce manuel explique comment installer et utiliser **Unitor MT4 Bridge** avec une interface MIDI **Emagic MT4** sous Windows 10 ou 11 (64 bits).

Suivez les sections dans l’ordre. Vous pourrez envoyer et recevoir du MIDI le jour même, puis faire un premier échange SysEx avec un éditeur ou un librarian.

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
| virtualMIDI | Le pilote **[virtualMIDI](https://www.tobias-erichsen.de/software/virtualmidi.html)** (Tobias Erichsen), déjà installé |

## Installer virtualMIDI

Unitor MT4 Bridge s’appuie sur **virtualMIDI** pour créer les ports MIDI virtuels visibles dans votre DAW. Installez-le avant le Bridge, par exemple avec :

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

1. Téléchargez et lancez `UnitorMt4Bridge-Setup.exe`.
2. Acceptez le dossier proposé, en général :

   `C:\Program Files\Ten Square Software\Unitor MT4 Bridge\`

3. Branchez la MT4 lorsque l’assistant le demande (association USB).
4. Autorisez l’élévation Administrateur **une seule fois** (Program Files et association USB).
5. Attendez l’écran indiquant que l’installation a réussi.

L’installation réussit lorsque :

- virtualMIDI est bien détecté,
- l’association USB (WinUSB) est OK,
- le démarrage automatique est enregistré pour votre compte Windows.

Si quelque chose manque (par exemple virtualMIDI), l’assistant affiche un message d’aide : corrigez le point indiqué, puis relancez l’installeur.

**WinUSB** est le composant USB standard de Microsoft. L’installeur l’associe à la MT4 pour que le Bridge puisse communiquer avec elle. Vous n’avez rien à configurer de plus pour un usage normal.

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

Dans le menu Démarrer, choisissez **Unregister Auto-Start** si vous ne voulez plus que le Bridge se lance à la connexion. La désinstallation du logiciel retire aussi ce démarrage automatique.

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

## virtualMIDI manquant

Installez **loopMIDI** ou **rtpMIDI**, vérifiez `teVirtualMIDI.dll` dans `C:\Windows\System32\`, puis relancez l’installeur Unitor MT4 Bridge.

## Association USB (WinUSB) en échec

Relancez l’installeur avec la MT4 branchée. Dans le Gestionnaire de périphériques, l’interface MIDI de la MT4 doit apparaître associée à WinUSB.

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

- Installer via l’assistant fourni (association WinUSB — pas de pilote noyau custom)
- Utiliser **2 entrées** et **4 sorties** virtuelles (`MT4 In` / `MT4 Out`)
- Faire circuler notes, CC, horloge / transport, MTC et SysEx (échelle éditeur / librarian)
- Profiter du démarrage automatique sans Administrateur au quotidien (Bridge en session utilisateur, pas un service Windows)
- Débrancher et rebrancher la MT4, puis récupérer les ports par rescan ou relance du Bridge (sans redémarrer Windows)
- Ouvrir plusieurs logiciels sur les mêmes ports (dans la limite de virtualMIDI, environ huit par port)
- Brancher une seconde MT4 lorsque vous en avez une (voir [Deux interfaces MT4](#deux-interfaces-mt4))

## Ce que cette version ne fait pas (V1)

N’attendez **pas** ceci dans cette version :

- Patch mode, LTC/VITC, Fast Mode / fonctions AMT des manuels Unitor
- Topologies Emagic en cascade / empilées
- Support garanti AMT8 / Unitor8 sans matériel validé pour ces modèles
- Windows MIDI Services comme backend MIDI V1 (option possible plus tard, Win11 seulement)
- Un pilote MIDI noyau custom
- Zadig comme chemin d’installation communautaire recommandé (secours contributeur seulement)
- Chiffres de latence / jitter MIDI « studio-done » publiés (mesurés plus tard)

Voir aussi (contributeurs / évaluateurs ; page technique en anglais) : [Licence et backends MIDI](../dev/license-and-backends.md) — MIT (ce dépôt) ≠ virtualMIDI (propriétaire) ≠ Windows MIDI Services (pas la V1).

# Deux interfaces MT4

| Interface | Noms des ports |
|---|---|
| Première | `MT4 In N` / `MT4 Out N` |
| Deuxième et suivantes | `MT4 #2 In N` / `MT4 #2 Out N`, etc. |

Chaque interface a son propre jeu de ports. Débrancher l’une ne change pas les noms de l’autre.

Si vous n’avez qu’une MT4, ignorez les noms avec `#2` jusqu’à en avoir besoin.

**Honnêteté de validation :** l’usage quotidien et le nommage pour **une** MT4 physique sont le chemin prouvé documenté ici. Le nommage dual-MT4 est implémenté ; traitez une seconde unité comme supportée côté logiciel, mais ne supposez pas une preuve lab dual fermée à partir de ce manuel seul.
