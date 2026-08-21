---
organization: Ten Square Software
project: unitor-win64-driver
title: Unitor MT4 Bridge — Guide utilisateur Windows 11
author: Guillaume DUPONT
created: 2026-08-21
updated: 2026-08-21
version: "1.1"
product_version: "0.1.0"
---

# Unitor MT4 Bridge — Windows 11

**Unitor MT4 Bridge** permet d’utiliser une interface MIDI **Emagic MT4** sous **Windows 64 bits**. Emagic ne propose plus de pilote officiel pour Windows moderne : le dernier pilote Windows Emagic ne prenait en charge que les systèmes **32 bits**.

Ce projet open source (Ten Square Software) est un effort **hobby** communautaire : sources gratuites sur GitHub, **pas** de certificat de signature payant, et **pas** de promesse que l’installation seule réussit toujours sur un PC tout neuf.

## Ce que couvre cette version

- Dans cette **première version (v1)**, seul le fonctionnement avec la **MT4** est garanti : c’est le modèle testé et documenté par le développeur.
- Dans une future **v2**, la prise en charge des **AMT8**, **Unitor8** et **Unitor8 mk2** pourra suivre — selon l’accueil du projet et la demande des utilisateurs. Ces modèles **ne** sont **pas** promis en v1.

## Deux éditions du Bridge

| Édition | Windows | Comment le MIDI arrive dans votre logiciel | Installation |
|---|---|---|---|
| **Ce guide** | **Windows 11** | **Windows MIDI Services** (intégré) | Relativement simple |
| Autre guide | **Windows 10** | **virtualMIDI** (développé par Tobias Erichsen) — à installer soi-même | Plus technique |

Cette édition Windows 11 est en général la plus simple si votre PC le permet. virtualMIDI **n’est jamais** redistribué avec le Bridge (pour des questions de droits & licence). Le guide Windows 10 explique pourquoi et comment.

Vous souhaitez installer et utiliser le Bridge sous Windows 10 ? Ouvrez le [guide Windows 10](unitor-mt4-bridge-win10-virtualmidi-guide-utilisateur.md). English: [`unitor-mt4-bridge-win11-wms-user-guide.md`](unitor-mt4-bridge-win11-wms-user-guide.md). Choix : [`README.md`](README.md).

## Sommaire

1. [Vérifier les prérequis](#vérifier-les-prérequis)
2. [Installer le Bridge](#installer-le-bridge)
3. [Passer l’avertissement Windows SmartScreen](#passer-lavertissement-windows-smartscreen)
4. [Utiliser le démarrage automatique](#utiliser-le-démarrage-automatique)
5. [Lire les voyants de la MT4](#lire-les-voyants-de-la-mt4)
6. [Faire un premier essai MIDI](#faire-un-premier-essai-midi)
7. [Faire un premier essai SysEx](#faire-un-premier-essai-sysex)
8. [Résoudre les problèmes courants](#résoudre-les-problèmes-courants)
9. [Rétablir le MIDI quand il est coincé](#rétablir-le-midi-quand-il-est-coincé)
10. [Débrancher et rebrancher la MT4](#débrancher-et-rebrancher-la-mt4)
11. [Savoir ce qui marche et ce qui ne marche pas](#savoir-ce-qui-marche-et-ce-qui-ne-marche-pas)
12. [Utiliser deux interfaces MT4](#utiliser-deux-interfaces-mt4)
13. [Lexique](#lexique)

---

# Vérifier les prérequis

| Besoin | Détail |
|---|---|
| Ordinateur | **Windows 11**, **64 bits**, avec **Windows MIDI Services** disponible |
| Matériel | Une interface MIDI **Emagic MT4** |
| Pilote MIDI tiers | **Non requis** dans cette édition — vous **n’installez pas** virtualMIDI |

Si Windows MIDI Services manque, le Bridge **refuse** de présenter des ports vides comme un succès. Mettez Windows à jour, ou utilisez l’édition Windows 10 si c’est votre machine.

## Logiciels utiles ensuite

Pour les essais du quotidien, ouvrez **votre logiciel de musique habituel** (votre DAW) — par exemple Ableton Live, Cubase, Reaper ou Bitwig. Tout éditeur capable d’envoyer et recevoir des notes MIDI, des CC ou même des messages SysEx convient pour un premier essai avec votre home studio.

# Installer le Bridge

Le fichier téléchargé ressemble à : `unitor-mt4-bridge-{version}-win11-wms-setup.exe` (page **Releases** du projet — pas un miroir au hasard).

1. Branchez la **MT4** (alimentation + USB) **avant** ou pendant l’installation — l’assistant **ne s’arrête pas** pour vous le demander.
2. Lancez le programme d’installation téléchargé.
3. Acceptez le dossier proposé, en général :

   `C:\Program Files\Ten Square Software\Unitor MT4 Bridge\`

4. Acceptez l’élévation Administrateur **une fois**.
5. Attendez un message de succès clair — ou une installation incomplète si un contrôle a échoué (ce n’est **pas** une install réussie).

L’installation réussit quand Windows 11 + Windows MIDI Services sont disponibles, l’association USB (WinUSB) et le démarrage automatique sont OK. virtualMIDI n’est **pas** contrôlé sur cette édition. Une liste de ports MIDI vide n’est **pas** un succès.

**Un seul programme d’installation à la fois :** les éditions Windows 11 et Windows 10 partagent la **même identité produit Windows**. En installer une **remplace** l’autre sous Program Files et réécrit le démarrage automatique. Pas d’installation côte à côte des deux éditions.

**WinUSB** est déjà dans Windows. Sur un PC neuf, Windows refuse souvent d’associer tout seul la MT4 à ce pilote, faute de **catalogue de confiance** (*trusted catalog*) signé — ce projet n’en fournit pas.

Si l’étape WinUSB **échoue**, suivez [Associer la MT4 à WinUSB](#associer-la-mt4-à-winusb) — **Zadig** est le correctif supporté.

# Passer l’avertissement Windows SmartScreen

Windows peut afficher **Microsoft Defender SmartScreen** (« Windows a protégé votre PC »). Cela peut arriver pour un fichier **non signé**, ou signé mais **pas encore** crédité par la réputation. Un avertissement **ne** signifie **pas** automatiquement un malware.

**Ne continuez que si vous avez téléchargé l’installateur depuis les Releases de ce projet.**

Pour vérifier la signature : clic droit → **Propriétés** → **Signatures numériques**. Si l’onglet manque, le fichier est en général **non signé**. Pour ce projet communautaire, le développeur a préféré **ne pas acheter** de certificat Authenticode dont le coût annuel est très élevé.

Quand la politique du PC le permet :

1. Choisissez **Informations complémentaires**.
2. Choisissez **Exécuter quand même**.

Autre piste : clic droit → **Propriétés** → cochez **Débloquer** si affiché, puis Appliquer / OK.

Ne désactivez pas SmartScreen globalement. Sur un PC géré, **Exécuter quand même** peut être bloqué.

# Utiliser le démarrage automatique

Après l’installation, **Unitor MT4 Bridge** se lance tout seul quand vous ouvrez votre session Windows. Ce n’est **pas** un service Windows qui tourne en arrière-plan pour tout le monde : c’est un programme lié **à votre compte utilisateur**.

Une fois connecté (ou après avoir branché la MT4), Windows devrait proposer les ports **`MT4 In 1`**, **`MT4 In 2`**, **`MT4 Out 1`** … **`MT4 Out 4`**. Vous pouvez aussi lancer **Unitor MT4 Bridge** à la main depuis le menu Démarrer.

Pour **désactiver** le démarrage automatique : dans le menu Démarrer, ouvrez le dossier du Bridge et choisissez **Unregister Auto-Start**. La désinstallation retire aussi le démarrage automatique, mais seulement pour l’utilisateur Windows qui désinstalle.

# Lire les voyants de la MT4

Sur la face avant de la MT4, les voyants aident à comprendre ce qui se passe :

| Voyant | Couleur | À quoi ça sert (en pratique) |
|---|---|---|
| **MIDI In** | Rouge | Clignote quand la MT4 **reçoit** du MIDI (depuis l’ordinateur ou un appareil branché) |
| **MIDI Out** | Vert | Clignote quand la MT4 **envoie** du MIDI |
| **Patch** | Rouge | Lié au mode Patch de la MT4 (hors usage courant de cette v1 du Bridge) |
| **USB** | Orange | Indique en général que la MT4 est alimentée côté USB et que le lien avec l’ordinateur / le Bridge est actif |

Si le voyant **USB** orange reste éteint alors que le câble est branché, vérifiez l’alimentation, le câble USB, puis que le Bridge tourne. Les voyants **MIDI In** / **MIDI Out** sont utiles pendant vos essais de notes ou de SysEx.

# Faire un premier essai MIDI

| Sens | Noms sous Windows |
|---|---|
| Entrées | `MT4 In 1`, `MT4 In 2` |
| Sorties | `MT4 Out 1` … `MT4 Out 4` |

1. Confirmez l’[installation du Bridge](#installer-le-bridge) et le [démarrage automatique](#utiliser-le-démarrage-automatique) (ou lancez le Bridge une fois).
2. Branchez la MT4.
3. Ouvrez **votre DAW habituel**.
4. Activez au moins **`MT4 In 1`** et **`MT4 Out 1`**.
5. Envoyez des notes (ou des contrôleurs) et vérifiez l’autre sens.

### Réveiller le Computer Mode

La MT4 a un état appelé **Computer Mode** : elle doit « se réveiller » pour bien parler à l’ordinateur. Pour cela, envoyez d’abord un peu de **MIDI ordinaire** — par exemple quelques notes, ou un mouvement de molette / curseur de contrôleur (CC) — depuis votre DAW vers un port **`MT4 Out`**.

Ensuite seulement, lancez un échange SysEx. **Le SysEx seul ne réveille pas** le Computer Mode : si vous commencez directement par un dump, la MT4 peut rester muette.

# Faire un premier essai SysEx

1. Bridge en cours ; ports visibles.
2. Envoyez un court message MIDI ordinaire pour le Computer Mode.
3. Dans **votre DAW** ou l’éditeur de votre synthé, sélectionnez les ports et terminez un court échange SysEx (par exemple une sauvegarde / restauration de presets si vous utilisez un éditeur MIDI comme [Matrix-Control](https://github.com/tensquaresoftware/matrix-control), développé par Ten Square Software).

# Résoudre les problèmes courants

## Windows MIDI Services manquant

Si l’installateur refuse de démarrer, ou si les ports restent vides :

1. Fermez le DAW et quittez **Unitor MT4 Bridge**.
2. **Redémarrez** le PC.
3. Après connexion, ouvrez **Paramètres → Windows Update** et installez les mises à jour en attente.
4. Si Microsoft propose une activation **Windows MIDI Services** / MIDI sur votre build, suivez cette interface — **pas** de PowerShell Admin sur cette édition destinée aux musiciens.
5. Relancez l’installateur Windows 11, ou le Bridge une fois, puis rescanez le MIDI dans le DAW.

PC sous Windows 10 ? Utilisez le [guide Windows 10](unitor-mt4-bridge-win10-virtualmidi-guide-utilisateur.md).

## Associer la MT4 à WinUSB

L’installateur indique qu’il n’a pas pu associer la MT4 à WinUSB ; l’installation est souvent annulée.

1. Branchez la MT4 et lancez **[Zadig](https://zadig.akeo.ie/)**.
2. Dans la liste, sélectionnez l’interface MIDI composite de la MT4 appelée **MI_02** (identifiant USB `VID_086A&PID_0003&MI_02`) — pas une autre ligne USB au hasard.
3. Choisissez le pilote **WinUSB**, puis validez l’installation.
4. Relancez ensuite l’installateur du Bridge si besoin, puis **Unitor MT4 Bridge**.

Sur un PC propre, Windows refuse souvent le paquet faute de **catalogue de confiance** (*trusted catalog*) — Zadig est le contournement supporté.

## Aucun port visible

Reprenez les contrôles simples :

1. Bridge lancé (démarrage automatique ou lancement manuel).
2. MT4 branchée.
3. Windows MIDI Services disponible.
4. Rescan MIDI dans le DAW.

## Pas de réponse SysEx

Envoyez d’abord des notes ou des contrôleurs pour le **Computer Mode**, puis réessayez le SysEx.

# Rétablir le MIDI quand il est coincé

Si le MIDI semble bloqué après un arrêt du Bridge ou une fermeture malpropre, recovery **simple** — **sans** PowerShell et **sans** terminal Admin :

1. Fermez le DAW (et les autres apps MIDI).
2. Quittez **Unitor MT4 Bridge** s’il tourne encore.
3. **Redémarrez** le PC.
4. Après connexion, lancez **Unitor MT4 Bridge** **une fois** (ou laissez le démarrage automatique), puis rouvrez le DAW.

# Débrancher et rebrancher la MT4

1. Débranchez : les ports de cette interface disparaissent.
2. Rebranchez : le Bridge recrée les ports.
3. Au besoin, rescan MIDI, ou quittez / relancez le Bridge.

**OK :** rescan dans le logiciel et/ou redémarrage supervisé du Bridge.  
**Pas** le correctif normal de débranchement : un redémarrage Windows (réservé au cas MIDI coincé ci-dessus).

# Savoir ce qui marche et ce qui ne marche pas

## Ce qui marche dans cette édition Windows 11

- Installation **sans** virtualMIDI
- WinUSB via l’installateur quand Windows accepte le paquet ; sinon Zadig guidé
- 2 entrées / 4 sorties `MT4 In` / `MT4 Out`
- Notes, contrôleurs, horloge / transport, MTC, SysEx
- Démarrage automatique sans Admin quotidien
- Débranchement / rebranchement avec rescan / relance du Bridge

## Ce que cette édition ne fait pas

- Succès WinUSB « installateur seul » sur tout PC propre sans étape guidée / **trusted catalog**
- Intégration ou redistribution de virtualMIDI
- Certificat Authenticode payant / SmartScreen silencieux
- Promettre Windows 10 sur cette édition Windows MIDI Services
- Patch mode, LTC/VITC, Fast Mode / AMT, topologies Emagic en cascade
- AMT8 / Unitor8 / Unitor8 mk2 garantis en v1
- Pilote MIDI noyau custom

Notes licence (contributeurs) : [license-and-backends.md](../dev/license-and-backends.md).

# Utiliser deux interfaces MT4

| Interface | Noms de ports |
|---|---|
| Première | `MT4 In N` / `MT4 Out N` |
| Deuxième et suivantes | `MT4 #2 In N` / `MT4 #2 Out N`, etc. |

**Honnêteté :** l’usage quotidien d’**une** MT4 physique est le chemin prouvé ici. Le nommage pour une deuxième unité existe ; ne supposez pas une preuve lab dual fermée depuis ce seul guide.

# Lexique

| Terme | Nom complet | Sens simple |
|---|---|---|
| **MT4** | Emagic MT4 | Interface MIDI à quatre ports ciblée par cette v1 |
| **Bridge** | Unitor MT4 Bridge | Programme Windows qui relie la MT4 à votre DAW |
| **DAW** | Station de travail audio | Votre logiciel de musique habituel (Ableton Live, Cubase, Reaper, Bitwig, …) |
| **Windows MIDI Services** | Pile MIDI Microsoft sous Windows 11 | Façon intégrée de créer les ports `MT4 In` / `MT4 Out` — **sans** installer virtualMIDI |
| **WinUSB** | Pilote USB Windows | Comment Windows parle à la MT4 une fois associée |
| **Zadig** | Outil d’association tiers | Utilisé quand l’installateur ne peut pas associer WinUSB sur un PC propre |
| **SmartScreen** | Microsoft Defender SmartScreen | Avertissement Windows sur les téléchargements peu connus — **Exécuter quand même** uniquement pour les Releases de ce projet |
| **Trusted catalog** | Catalogue de confiance Windows | Absent ici (pas de certificat payant) — Zadig guidé est le contournement hobby |
| **SysEx** | System Exclusive | Messages MIDI pour les sauvegardes / éditeurs de sons |
| **Computer Mode** | Comportement Emagic MT4 | État « prêt pour l’ordinateur » — réveillez-le avec du MIDI ordinaire avant le SysEx |
| **Démarrage automatique** | Auto-Start | Le Bridge se lance avec votre session Windows |
| **virtualMIDI** | Pilote Tobias Erichsen | Uniquement pour l’édition **Windows 10** — pas dans cette édition Windows 11 |
| **MI_02** | Interface USB composite | La bonne ligne à choisir dans Zadig pour la partie MIDI de la MT4 |

---

Encore un projet mené avec succès via la méthode [BMad](https://github.com/bmad-code-org/bmad-method) !
