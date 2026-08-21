---
organization: Ten Square Software
project: unitor-win64-driver
title: Unitor MT4 Bridge — Guide Windows 11 / Windows MIDI Services
author: Guillaume DUPONT
created: 2026-08-21
updated: 2026-08-21
version: "1.0"
product_version: "0.1.0"
---

Ce guide est pour les musiciens sous **Windows 11** avec le Setup communautaire **Windows MIDI Services** (`UnitorMt4Bridge-Setup-win11-wms-…`). Vous n’avez **pas** besoin de virtualMIDI.

Sous **Windows 10**, utilisez le [guide Win10 / virtualMIDI](unitor-mt4-bridge-win10-virtualmidi-guide-utilisateur.md). English: [`unitor-mt4-bridge-win11-wms-user-guide.md`](unitor-mt4-bridge-win11-wms-user-guide.md). Point d’entrée : [`README.md`](README.md).

Projet **hobby / open source** (Ten Square Software) : sources gratuites, **pas** de certificat de signature, **pas** de promesse que Setup seul réussit toujours sur un PC neuf.

## Sommaire

1. [Prérequis](#prérequis)
2. [Installation](#installation)
3. [Démarrage automatique](#démarrage-automatique)
4. [Premier test MIDI](#premier-test-midi)
5. [Premier test SysEx](#premier-test-sysex)
6. [Dépannage](#dépannage)
7. [MIDI coincé après arrêt / redémarrage](#midi-coincé-après-arrêt--redémarrage)
8. [Débrancher et rebrancher la MT4](#débrancher-et-rebrancher-la-mt4)
9. [Ce qui marche / ce qui ne marche pas](#ce-qui-marche--ce-qui-ne-marche-pas)
10. [Deux interfaces MT4](#deux-interfaces-mt4)

---

# Prérequis

| Besoin | Détail |
|---|---|
| Ordinateur | **Windows 11**, **64 bits**, avec **Windows MIDI Services** disponible |
| Matériel | Une interface MIDI **Emagic MT4** |
| virtualMIDI | **Non requis** sur ce chemin |

Si Windows MIDI Services manque, le Bridge **refuse** de présenter des ports vides comme un succès. Installez / activez WMS sous Windows 11, ou suivez le chemin Win10 + virtualMIDI si c’est votre machine.

## Logiciels utiles plus tard

| Usage | Exemple |
|---|---|
| Premier test MIDI | Ableton Live 12 ou MIDI-OX |
| Premier test SysEx | Matrix-Control, ou MIDI-OX |

# Installation

1. Branchez la **MT4** (alimentation + USB) **avant** ou pendant Setup — l’assistant **ne s’arrête pas** pour vous le demander.
2. Téléchargez `UnitorMt4Bridge-Setup-win11-wms-{version}.exe` depuis les **Releases** du projet (pas un miroir au hasard).
3. Acceptez le dossier proposé, en général :

   `C:\Program Files\Ten Square Software\Unitor MT4 Bridge\`

4. Acceptez l’élévation Administrateur **une fois**.
5. Attendez **Installation successful** — ou **Installation incomplete** si une porte a échoué.

Setup réussit quand Windows 11 + Windows MIDI Services (`midisrv`) sont disponibles, l’association USB (WinUSB) et le démarrage automatique sont OK. virtualMIDI n’est **pas** contrôlé sur cette saveur. Une liste de ports MIDI vide n’est **pas** un succès.

**Même AppId :** ce Setup partage le même identifiant produit Windows que le Setup Win10. En installer un **remplace** l’autre sous Program Files et réécrit le démarrage automatique vers ce backend MIDI. Pas d’installation côte à côte des deux saveurs.

**WinUSB** est déjà dans Windows. Sur un PC propre, l’association de **cette** MT4 échoue souvent sans **catalogue** de confiance (*trusted catalog*) — ce projet **ne fournit pas** de certificat.

Si l’étape WinUSB de Setup **échoue**, suivez [Association USB (WinUSB) en échec](#association-usb-winusb-en-échec) — **Zadig** est le correctif supporté.

## Windows SmartScreen (Setup non signé ou non reconnu)

Windows peut afficher **Microsoft Defender SmartScreen** (« Windows a protégé votre PC »). Cela peut arriver pour un build **non signé**, ou signé mais **pas encore** crédité par la réputation. Un avertissement **ne** signifie **pas** automatiquement un malware.

**Ne continuez que si vous avez téléchargé Setup depuis les Releases de ce projet.**

Pour vérifier la signature : clic droit → **Propriétés** → **Signatures numériques**. Si l’onglet manque, le fichier est en général **non signé**. Ce projet **n’achète pas** de certificat Authenticode (OQ-3).

Quand la politique du PC le permet :

1. Choisissez **Informations complémentaires**.
2. Choisissez **Exécuter quand même**.

Autre piste : clic droit → **Propriétés** → cochez **Débloquer** si affiché, puis Appliquer / OK. Vérifiez aussi **Signatures numériques** sur le fichier Setup.

Ne désactivez pas SmartScreen globalement. Sur un PC géré, **Exécuter quand même** peut être bloqué.

Politique contributeur : [`docs/dev/authenticode-and-smartscreen.md`](../dev/authenticode-and-smartscreen.md).

# Démarrage automatique

Après l’installation, Unitor MT4 Bridge démarre avec votre session Windows. Les ports MIDI apparaissent sans ouvrir le programme chaque jour, et sans mot de passe Administrateur au quotidien.

Le Bridge est un **programme de session utilisateur**, **pas** un service Windows.

## Comment savoir que tout est prêt

1. Connectez-vous (ou branchez la MT4).
2. Ouvrez votre DAW ou MIDI-OX.
3. Cherchez **`MT4 In 1`**, **`MT4 In 2`**, **`MT4 Out 1`** … **`MT4 Out 4`**.

Vous pouvez aussi lancer **Unitor MT4 Bridge** depuis le menu Démarrer.

## Couper le démarrage automatique

Dans le menu Démarrer, choisissez **Unregister Auto-Start**. La désinstallation retire le démarrage automatique **pour l’utilisateur Windows qui désinstalle**. Les autres comptes peuvent garder leur propre entrée.

# Premier test MIDI

| Sens | Noms sous Windows |
|---|---|
| Entrées | `MT4 In 1`, `MT4 In 2` |
| Sorties | `MT4 Out 1` … `MT4 Out 4` |

1. Confirmez l’installation et le démarrage automatique (ou lancez le Bridge une fois).
2. Branchez la MT4.
3. Ouvrez Ableton Live 12 ou MIDI-OX.
4. Activez au moins **`MT4 In 1`** et **`MT4 Out 1`**.
5. Envoyez notes ou CC et vérifiez l’autre sens.

### Computer Mode

Envoyez d’abord un peu de MIDI ordinaire (CC court ou quelques notes). **Le SysEx seul ne réveille pas le Computer Mode.**

# Premier test SysEx

1. Bridge en cours ; ports visibles.
2. Envoyez un CC court pour le Computer Mode.
3. Dans Matrix-Control (ou MIDI-OX), sélectionnez les ports et terminez un dump/restore (ou un court échange SysEx).

# Dépannage

## SmartScreen bloque Setup

Voir [Windows SmartScreen](#windows-smartscreen-setup-non-signé-ou-non-reconnu).

## Windows MIDI Services manquant

Si Setup refuse de démarrer, ou si les ports restent vides :

1. Fermez le DAW et quittez **Unitor MT4 Bridge**.
2. **Redémarrez** le PC.
3. Après connexion, ouvrez **Paramètres → Windows Update** et installez les mises à jour en attente.
4. Si Microsoft propose une activation **Windows MIDI Services** / MIDI sur votre build, suivez cette interface — **pas** de PowerShell Admin sur ce chemin musicien.
5. Relancez le Setup Win11, ou le Bridge une fois, puis rescanez le MIDI dans le DAW.

PC sous Windows 10 ? Utilisez le [guide Win10 / virtualMIDI](unitor-mt4-bridge-win10-virtualmidi-guide-utilisateur.md).

## Association USB (WinUSB) en échec

### Correctif supporté — WinUSB guidé (Zadig)

1. MT4 branchée et alimentée.
2. Setup issu des Releases du projet.
3. **[Zadig](https://zadig.akeo.ie/)** : associez **WinUSB** à l’interface MIDI composite **MI_02** (`USB\VID_086A&PID_0003&MI_02`).
4. Relancez Setup si besoin, puis **Unitor MT4 Bridge**.
5. Contributeurs : [`docs/dev/winusb-bind.md`](../dev/winusb-bind.md).

## Aucun port visible

Bridge lancé, MT4 branchée, WMS disponible, rescan MIDI dans le DAW.

## Pas de réponse SysEx

CC ou notes d’abord (Computer Mode), puis réessayez le SysEx.

# MIDI coincé après arrêt / redémarrage

Si le MIDI semble bloqué après un arrêt du Bridge ou une fermeture malpropre (cas « sticky » Windows MIDI Services / midisrv), recovery **sans PowerShell** et **sans** terminal Admin :

1. Fermez le DAW (et les autres apps MIDI).
2. Quittez **Unitor MT4 Bridge** s’il tourne encore.
3. **Redémarrez** le PC.
4. Après connexion, lancez **Unitor MT4 Bridge** **une fois** (ou laissez l’Auto-Start), puis rouvrez le DAW.

# Débrancher et rebrancher la MT4

1. Débranchez : les ports de cette interface disparaissent.
2. Rebranchez : le Bridge recrée les ports.
3. Au besoin, rescan MIDI, ou quittez / relancez le Bridge.

**OK :** rescan hôte et/ou redémarrage supervisé du Bridge.  
**Pas** le correctif hot-plug normal : un reboot Windows (réservé au cas MIDI coincé ci-dessus).

# Ce qui marche / ce qui ne marche pas

## Ce qui marche (chemin communautaire Win11 WMS)

- Installation **sans** virtualMIDI
- WinUSB via Setup quand Windows accepte le paquet ; sinon Zadig guidé
- 2 entrées / 4 sorties `MT4 In` / `MT4 Out`
- Notes, CC, horloge / transport, MTC, SysEx
- Démarrage automatique sans Admin quotidien
- Hot-plug avec rescan / relance du Bridge

## Ce que ce chemin ne fait pas

- Succès WinUSB Setup-seul sur tout PC propre sans étape guidée
- Intégration ou redistribution de virtualMIDI / teVirtualMIDI.dll
- Certificat Authenticode payant / SmartScreen silencieux
- Promettre Windows 10 sur ce Setup WMS
- Patch mode, LTC/VITC, Fast Mode / AMT, topologies Emagic en cascade
- AMT8 / Unitor8 garantis sans matériel validé
- Pilote MIDI noyau custom
- Réparation Admin midisrv comme recovery musicien par défaut

Licence : [license-and-backends.md](../dev/license-and-backends.md). Signature : [authenticode-and-smartscreen.md](../dev/authenticode-and-smartscreen.md).

# Deux interfaces MT4

| Interface | Noms de ports |
|---|---|
| Première | `MT4 In N` / `MT4 Out N` |
| Deuxième et suivantes | `MT4 #2 In N` / `MT4 #2 Out N`, etc. |

**Honnêteté validation :** l’usage quotidien d’**une** MT4 physique est le chemin prouvé ici. Le nommage dual est implémenté ; ne supposez pas une preuve lab dual fermée depuis ce seul guide.
