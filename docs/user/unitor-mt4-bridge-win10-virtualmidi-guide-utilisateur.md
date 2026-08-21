---
organization: Ten Square Software
project: unitor-win64-driver
title: Unitor MT4 Bridge — Guide Windows 10 / virtualMIDI
author: Guillaume DUPONT
created: 2026-08-21
updated: 2026-08-21
version: "1.0"
product_version: "0.1.0"
---

Ce guide s’adresse aux utilisateurs **motivés sous Windows 10** (et Win11 qui choisissent le Setup virtualMIDI) avec `UnitorMt4Bridge-Setup-win10-virtualmidi-…`.

Vous **installez vous-même** le pilote **virtualMIDI** de Tobias Erichsen. Ce projet **n’embarque jamais** ni ne redistribue `teVirtualMIDI.dll`, le MSI virtualMIDI, ni le SDK (OQ-1).

Chemin Win11 plus simple sans virtualMIDI : [guide Win11 / Windows MIDI Services](unitor-mt4-bridge-win11-wms-guide-utilisateur.md). English: [`unitor-mt4-bridge-win10-virtualmidi-user-guide.md`](unitor-mt4-bridge-win10-virtualmidi-user-guide.md). Entrée : [`README.md`](README.md).

Hobby / open source (Ten Square Software) : **pas** de certificat de signature ; WinUSB sur PC propre demande souvent une étape **guidée** sans **catalogue** de confiance (*trusted catalog*).

## Sommaire

1. [Prérequis](#prérequis)
2. [Installer virtualMIDI vous-même](#installer-virtualmidi-vous-même)
3. [Installation (Setup Bridge)](#installation-setup-bridge)
4. [Démarrage automatique](#démarrage-automatique)
5. [Premier test MIDI](#premier-test-midi)
6. [Premier test SysEx](#premier-test-sysex)
7. [Dépannage](#dépannage)
8. [Débrancher et rebrancher la MT4](#débrancher-et-rebrancher-la-mt4)
9. [Ce qui marche / ce qui ne marche pas](#ce-qui-marche--ce-qui-ne-marche-pas)
10. [Deux interfaces MT4](#deux-interfaces-mt4)

---

# Prérequis

| Besoin | Détail |
|---|---|
| Ordinateur | **Windows 10** ou **Windows 11**, **64 bits** (ce Setup = chemin virtualMIDI) |
| Matériel | Une **Emagic MT4** |
| virtualMIDI | Déjà installé **par vous** — voir ci-dessous |

## Logiciels utiles plus tard

| Usage | Exemple |
|---|---|
| Premier test MIDI | Ableton Live 12 ou MIDI-OX |
| Premier test SysEx | Matrix-Control, ou MIDI-OX |

# Installer virtualMIDI vous-même

Le Setup communautaire Win10 parle au DAW via **virtualMIDI**. Installez-le chez Tobias Erichsen **avant** le Setup Bridge, par exemple :

- **[loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html)** (le plus courant), ou
- **[rtpMIDI](https://www.tobias-erichsen.de/software/rtpmidi.html)**

Vérifiez ensuite `teVirtualMIDI.dll` dans `C:\Windows\System32\`.

**Pourquoi soi-même ?** Le SDK virtualMIDI est propriétaire. Ce dépôt MIT **n’a pas** d’autorisation de redistribution (OQ-1) et **n’embarquera jamais** la DLL ni le MSI dans Setup.

Sans la DLL, Setup **échoue clairement** — une liste de ports vide n’est pas une installation réussie.

Aide opérateur (optionnelle, avancée) : `installer/check-virtualmidi.ps1` dans le dépôt.

# Installation (Setup Bridge)

1. Branchez la **MT4** avant ou pendant Setup — l’assistant **ne s’arrête pas** pour vous le demander.
2. Téléchargez `UnitorMt4Bridge-Setup-win10-virtualmidi-{version}.exe` depuis les **Releases**.
3. Acceptez le dossier sous Program Files (`Ten Square Software\Unitor MT4 Bridge`).
4. Autorisez Administrateur **une fois**.
5. Attendez le succès — ou l’échec si une porte a manqué.

Succès = DLL présente + WinUSB OK + démarrage automatique enregistré.

**Même AppId :** ce Setup partage le même identifiant produit Windows que le Setup Win11 WMS. En installer un **remplace** l’autre sous Program Files et réécrit le démarrage automatique vers ce backend MIDI.

## Windows SmartScreen (Setup non signé ou non reconnu)

Windows peut afficher **Microsoft Defender SmartScreen** (« Windows a protégé votre PC »). Cela peut arriver pour un build **non signé**, ou signé mais **pas encore** crédité par la réputation. Un avertissement **ne** signifie **pas** automatiquement un malware.

**Ne continuez que si vous avez téléchargé Setup depuis les Releases de ce projet.**

Pour vérifier la signature : clic droit → **Propriétés** → **Signatures numériques**. Si l’onglet manque, le fichier est en général **non signé**. Ce projet **n’achète pas** de certificat Authenticode (OQ-3).

Quand la politique du PC le permet :

1. Choisissez **Informations complémentaires**.
2. Choisissez **Exécuter quand même**.

Autre piste : clic droit → **Propriétés** → cochez **Débloquer** si affiché, puis Appliquer / OK.

Ne désactivez pas SmartScreen globalement. Politique : [`docs/dev/authenticode-and-smartscreen.md`](../dev/authenticode-and-smartscreen.md).

## WinUSB en échec sur PC propre

Cause fréquente : Windows refuse un paquet non signé / sans **catalogue** de confiance (*trusted catalog*). Association guidée **[Zadig](https://zadig.akeo.ie/)** pour **MI_02** (`USB\VID_086A&PID_0003&MI_02`). Détail contributeur : [`docs/dev/winusb-bind.md`](../dev/winusb-bind.md).

# Démarrage automatique

Le Bridge démarre avec votre session (pas un service Windows). Ports `MT4 In` / `MT4 Out` après connexion ou branchement. **Unregister Auto-Start** pour désactiver.

# Premier test MIDI

| Sens | Noms |
|---|---|
| Entrées | `MT4 In 1`, `MT4 In 2` |
| Sorties | `MT4 Out 1` … `MT4 Out 4` |

1. Confirmez l’[installation](#installation-setup-bridge) et le [démarrage automatique](#démarrage-automatique) (ou lancez le Bridge une fois).
2. Branchez la MT4.
3. Ouvrez Ableton Live 12 ou MIDI-OX.
4. Activez au moins **`MT4 In 1`** et **`MT4 Out 1`**.
5. Envoyez notes ou CC et vérifiez l’autre sens.

MIDI ordinaire d’abord pour le **Computer Mode** — le SysEx seul ne le réveille pas.

## Plusieurs applications à la fois

virtualMIDI permet en général environ huit clients par port. Désactivez le mode MIDI exclusif si un hôte se plaint.

# Premier test SysEx

1. Bridge en cours ; ports visibles.
2. Envoyez un CC court pour le Computer Mode.
3. Dans Matrix-Control (ou MIDI-OX), sélectionnez les ports et terminez un dump/restore (ou un court échange SysEx).

# Dépannage

## virtualMIDI manquant

Installez loopMIDI ou rtpMIDI, confirmez `teVirtualMIDI.dll`, relancez Setup. Ce projet **n’embarquera jamais** la DLL.

## SmartScreen / WinUSB / pas de ports / pas de SysEx

Même colonne vertébrale que Win11 : honnêteté SmartScreen, Zadig pour WinUSB sans **trusted catalog**, rescan MIDI, Computer Mode avant SysEx. Sur ce chemin motivé, les contrôles PowerShell techniques sont acceptables — ce n’est **pas** la recovery principale du chemin confort Win11.

## Ports absents après débranchement / rebranchement

Attendez, rescanez le MIDI, ou quittez/relancez le Bridge. Un reboot Windows n’est **pas** le correctif hot-plug normal.

# Débrancher et rebrancher la MT4

Débrancher → ports de cette unité disparaissent. Rebrancher → le Bridge les recrée. Rescan ou relance supervisée = OK. Exiger un reboot pour un hot-plug ordinaire = échec.

# Ce qui marche / ce qui ne marche pas

## Ce qui marche

- Chemin communautaire Win10 **parallèle** avec virtualMIDI **auto-installé**
- Même colonne WinUSB / Auto-Start / MIDI / SysEx / hot-plug une fois les prérequis OK
- Usage lab virtualMIDI sur Win10/Win11

## Ce que ce chemin ne fait pas

- Livrer ou embarquer MSI/SDK/DLL virtualMIDI
- Présenter Win10 comme la promesse communautaire **confort** (c’est Win11 + WMS)
- WinUSB Setup-seul sur tout PC propre sans étape guidée / **trusted catalog**
- Certificat Authenticode / SmartScreen silencieux
- Patch mode, LTC/VITC, Fast Mode / AMT, topologies en cascade, AMT8/Unitor8 garantis, pilote MIDI noyau

Voir [license-and-backends.md](../dev/license-and-backends.md).

# Deux interfaces MT4

Première : `MT4 In N` / `MT4 Out N`. Suivantes : `MT4 #2 …`. L’usage quotidien d’une seule MT4 est le chemin prouvé ici.
