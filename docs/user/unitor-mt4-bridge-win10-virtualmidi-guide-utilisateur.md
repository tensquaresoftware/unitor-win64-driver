---
organization: Ten Square Software
project: unitor-win64-driver
title: Unitor MT4 Bridge — Guide utilisateur Windows 10
author: Guillaume DUPONT
created: 2026-08-21
updated: 2026-08-21
version: "1.1"
product_version: "0.1.0"
---

# Unitor MT4 Bridge — Windows 10

**Unitor MT4 Bridge** permet d’utiliser une interface MIDI **Emagic MT4** sous **Windows 64 bits**. Emagic ne propose plus de pilote officiel pour Windows moderne : le dernier pilote Windows Emagic ne prenait en charge que les systèmes **32 bits**.

Ce projet open source (Ten Square Software) est un effort **hobby** communautaire : sources gratuites sur GitHub, **pas** de certificat de signature payant, et **pas** de promesse que l’installation seule réussit toujours sur un PC tout neuf.

## Ce que couvre cette version

- Dans cette **première version (v1)**, seul le fonctionnement avec la **MT4** est garanti : c’est le modèle testé et documenté par le développeur.
- Dans une future **v2**, la prise en charge des **AMT8**, **Unitor8** et **Unitor8 mk2** pourra suivre — selon l’accueil du projet et la demande des utilisateurs. Ces modèles **ne** sont **pas** promis en v1.

## Deux éditions du Bridge

| Édition | Windows | Comment le MIDI arrive dans votre logiciel | Install |
|---|---|---|---|
| Autre guide | **Windows 11** | **Windows MIDI Services** (intégré) | Relativement simple |
| **Ce guide** | **Windows 10** | **virtualMIDI** (Tobias Erichsen) — à installer soi-même | Plus technique |

Ce chemin Windows 10 est une offre **parallèle** pour ceux qui restent sous Windows 10. Ce n’est **pas** la promesse communautaire « confort » (c’est l’édition Windows 11).

virtualMIDI **n’est jamais** redistribué avec le Bridge (droits / licence). Vous le téléchargez et l’installez vous-même chez Tobias Erichsen.

Préférez le chemin Windows 11 plus simple ? Ouvrez le [guide Windows 11](unitor-mt4-bridge-win11-wms-guide-utilisateur.md). English: [`unitor-mt4-bridge-win10-virtualmidi-user-guide.md`](unitor-mt4-bridge-win10-virtualmidi-user-guide.md). Choix : [`README.md`](README.md).

## Sommaire

1. [Vérifier les prérequis](#vérifier-les-prérequis)
2. [Installer virtualMIDI soi-même](#installer-virtualmidi-soi-même)
3. [Installer le Bridge](#installer-le-bridge)
4. [Passer l’avertissement Windows SmartScreen](#passer-lavertissement-windows-smartscreen)
5. [Utiliser le démarrage automatique](#utiliser-le-démarrage-automatique)
6. [Faire un premier essai MIDI](#faire-un-premier-essai-midi)
7. [Faire un premier essai SysEx](#faire-un-premier-essai-sysex)
8. [Résoudre les problèmes courants](#résoudre-les-problèmes-courants)
9. [Débrancher et rebrancher la MT4](#débrancher-et-rebrancher-la-mt4)
10. [Savoir ce qui marche et ce qui ne marche pas](#savoir-ce-qui-marche-et-ce-qui-ne-marche-pas)
11. [Utiliser deux interfaces MT4](#utiliser-deux-interfaces-mt4)
12. [Lexique](#lexique)

---

# Vérifier les prérequis

| Besoin | Détail |
|---|---|
| Ordinateur | **Windows 10**, **64 bits** (cette édition tourne aussi sous Windows 11 si vous choisissez l’installateur virtualMIDI) |
| Matériel | Une **Emagic MT4** |
| Pilote MIDI tiers | **virtualMIDI**, installé **par vous** — voir ci-dessous |

## Logiciels utiles ensuite

Pour les essais du quotidien, ouvrez **votre logiciel de musique habituel** (votre DAW) — par exemple Ableton Live, Cubase, Reaper ou Bitwig. Tout éditeur capable d’envoyer et recevoir du SysEx avec votre synthé convient pour un premier essai SysEx.

# Installer virtualMIDI soi-même

Sur ce chemin, le Bridge parle au DAW via **virtualMIDI**. Installez-le chez Tobias Erichsen **avant** l’installateur du Bridge, par exemple :

- **[loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html)** (le plus courant), ou
- **[rtpMIDI](https://www.tobias-erichsen.de/software/rtpmidi.html)**

Vérifiez ensuite `teVirtualMIDI.dll` dans `C:\Windows\System32\`.

**Pourquoi soi-même ?** virtualMIDI est propriétaire. Ce dépôt MIT **n’a pas** d’autorisation de redistribution et **n’embarquera jamais** la DLL ni le paquet d’installation dans le téléchargement du Bridge.

Sans la DLL, l’installateur du Bridge **échoue clairement** — une liste de ports vide n’est pas une installation réussie.

# Installer le Bridge

Le fichier téléchargé ressemble à : `UnitorMt4Bridge-Setup-win10-virtualmidi-{version}.exe` (page **Releases**).

1. Branchez la **MT4** avant ou pendant l’installation — l’assistant **ne s’arrête pas** pour vous le demander.
2. Lancez le programme d’installation téléchargé.
3. Acceptez le dossier sous Program Files (`Ten Square Software\Unitor MT4 Bridge`).
4. Autorisez Administrateur **une fois**.
5. Attendez le succès — ou l’échec si un contrôle a manqué.

Succès = DLL présente + WinUSB OK + démarrage automatique enregistré.

**Un seul programme d’installation à la fois :** les éditions Windows 11 et Windows 10 partagent la **même identité produit Windows**. En installer une **remplace** l’autre sous Program Files et réécrit le démarrage automatique.

**WinUSB** est déjà dans Windows. Sur un PC propre, l’association échoue souvent sans **trusted catalog** (catalogue de confiance). Association guidée **[Zadig](https://zadig.akeo.ie/)** pour **MI_02** (`USB\VID_086A&PID_0003&MI_02`).

# Passer l’avertissement Windows SmartScreen

Windows peut afficher **Microsoft Defender SmartScreen** (« Windows a protégé votre PC »). Cela peut arriver pour un fichier **non signé**, ou signé mais **pas encore** crédité par la réputation. Un avertissement **ne** signifie **pas** automatiquement un malware.

**Ne continuez que si vous avez téléchargé l’installateur depuis les Releases de ce projet.**

Pour vérifier la signature : clic droit → **Propriétés** → **Signatures numériques**. Si l’onglet manque, le fichier est en général **non signé**. Ce projet **n’achète pas** de certificat Authenticode.

Quand la politique du PC le permet :

1. Choisissez **Informations complémentaires**.
2. Choisissez **Exécuter quand même**.

Autre piste : clic droit → **Propriétés** → cochez **Débloquer** si affiché, puis Appliquer / OK.

Ne désactivez pas SmartScreen globalement.

# Utiliser le démarrage automatique

Le Bridge démarre avec votre session (pas un service Windows). Ports `MT4 In` / `MT4 Out` après connexion ou branchement. **Unregister Auto-Start** pour désactiver.

# Faire un premier essai MIDI

| Sens | Noms |
|---|---|
| Entrées | `MT4 In 1`, `MT4 In 2` |
| Sorties | `MT4 Out 1` … `MT4 Out 4` |

1. Confirmez l’[installation du Bridge](#installer-le-bridge) et le [démarrage automatique](#utiliser-le-démarrage-automatique) (ou lancez le Bridge une fois).
2. Branchez la MT4.
3. Ouvrez **votre DAW habituel**.
4. Activez au moins **`MT4 In 1`** et **`MT4 Out 1`**.
5. Envoyez des notes (ou des contrôleurs) et vérifiez l’autre sens.

MIDI ordinaire d’abord pour le **Computer Mode** — le SysEx seul ne le réveille pas.

## Plusieurs applications à la fois

virtualMIDI permet en général environ huit clients par port. Désactivez le mode MIDI exclusif si un hôte se plaint.

# Faire un premier essai SysEx

1. Bridge en cours ; ports visibles.
2. Envoyez un court message MIDI ordinaire pour le Computer Mode.
3. Dans **votre DAW** ou l’éditeur de votre synthé, sélectionnez les ports et terminez un court échange SysEx (par exemple une sauvegarde / restauration de sons).

# Résoudre les problèmes courants

## virtualMIDI manquant

Installez loopMIDI ou rtpMIDI, confirmez `teVirtualMIDI.dll`, relancez l’installateur du Bridge. Ce projet **n’embarquera jamais** la DLL.

## SmartScreen, WinUSB, pas de ports, pas de SysEx

Mêmes idées que le guide Windows 11 : honnêteté SmartScreen, Zadig sans **trusted catalog**, rescan MIDI, Computer Mode avant SysEx. Sur ce chemin plus technique, des contrôles PowerShell sont acceptables si besoin — ce n’est **pas** la recovery principale du chemin confort Windows 11.

## Ports absents après débranchement / rebranchement

Attendez, rescanez le MIDI, ou quittez/relancez le Bridge. Un redémarrage Windows n’est **pas** le correctif normal.

# Débrancher et rebrancher la MT4

Débrancher → ports de cette unité disparaissent. Rebrancher → le Bridge les recrée. Rescan ou relance supervisée = OK. Exiger un redémarrage pour un simple débranchement = échec.

# Savoir ce qui marche et ce qui ne marche pas

## Ce qui marche

- Chemin communautaire Windows 10 **parallèle** avec virtualMIDI **installé par vous**
- Même colonne WinUSB / démarrage automatique / MIDI / SysEx / débranchement une fois les prérequis OK

## Ce que ce chemin ne fait pas

- Livrer ou embarquer virtualMIDI
- Présenter Windows 10 comme la promesse communautaire **confort** (c’est Windows 11 + Windows MIDI Services)
- WinUSB « installateur seul » sur tout PC propre sans étape guidée / **trusted catalog**
- Certificat Authenticode / SmartScreen silencieux
- Patch mode, LTC/VITC, Fast Mode / AMT, topologies en cascade, AMT8 / Unitor8 / Unitor8 mk2 garantis en v1, pilote MIDI noyau

Voir [license-and-backends.md](../dev/license-and-backends.md).

# Utiliser deux interfaces MT4

Première : `MT4 In N` / `MT4 Out N`. Suivantes : `MT4 #2 …`. L’usage quotidien d’une seule MT4 est le chemin prouvé ici.

# Lexique

| Terme | Nom complet | Sens simple |
|---|---|---|
| **MT4** | Emagic MT4 | Interface MIDI à quatre ports ciblée par cette v1 |
| **Bridge** | Unitor MT4 Bridge | Programme Windows qui relie la MT4 à votre DAW |
| **DAW** | Station de travail audio | Votre logiciel de musique habituel (Ableton Live, Cubase, Reaper, Bitwig, …) |
| **virtualMIDI** | Pilote MIDI Tobias Erichsen | Crée des ports virtuels pour le DAW — **vous** l’installez ; ce projet ne le livre jamais |
| **teVirtualMIDI.dll** | Bibliothèque système virtualMIDI | Fichier sous `System32` qui prouve que le pilote est installé |
| **loopMIDI** / **rtpMIDI** | Applications Tobias Erichsen | Façons courantes d’obtenir virtualMIDI |
| **WinUSB** | Pilote USB Windows | Comment Windows parle à la MT4 une fois associée |
| **Zadig** | Outil d’association tiers | Utilisé quand l’installateur ne peut pas associer WinUSB sur un PC propre |
| **SmartScreen** | Microsoft Defender SmartScreen | Avertissement Windows — **Exécuter quand même** uniquement pour les Releases de ce projet |
| **Trusted catalog** | Catalogue de confiance Windows | Absent ici (pas de certificat payant) — Zadig guidé est le contournement hobby |
| **SysEx** | System Exclusive | Messages MIDI pour les sauvegardes / éditeurs de sons |
| **Computer Mode** | Comportement Emagic MT4 | État « prêt pour l’ordinateur » — réveillez-le avec du MIDI ordinaire avant le SysEx |
| **Windows MIDI Services** | Pile MIDI Microsoft sous Windows 11 | Utilisée par l’**autre** édition — pas requise sur ce chemin Windows 10 |
