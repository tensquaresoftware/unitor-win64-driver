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

| Édition | Windows | Comment le MIDI arrive dans votre logiciel | Installation |
|---|---|---|---|
| **Ce guide** | **Windows 10** | **virtualMIDI** (développé par Tobias Erichsen) — à installer soi-même | Plus technique |
| Autre guide | **Windows 11** | **Windows MIDI Services** (intégré) | Relativement simple |

Ce guide Windows 10 s’adresse à ceux qui restent sous Windows 10 (ou qui choisissent volontairement virtualMIDI). Ce n’est **pas** l’édition la plus simple : sous Windows 11, préférez l’édition **Windows MIDI Services** si vous le pouvez.

virtualMIDI **n’est jamais** redistribué avec le Bridge (pour des questions de droits & licence). Vous le téléchargez et l’installez vous-même depuis le site internet de Tobias Erichsen.

Vous souhaitez installer et utiliser le bridge sous Windows 11 ? Ouvrez le [guide Windows 11](unitor-mt4-bridge-win11-wms-guide-utilisateur.md). English: [`unitor-mt4-bridge-win10-virtualmidi-user-guide.md`](unitor-mt4-bridge-win10-virtualmidi-user-guide.md). Choix : [`README.md`](README.md).

## Sommaire

1. [Vérifier les prérequis](#vérifier-les-prérequis)
2. [Installer virtualMIDI](#installer-virtualmidi)
3. [Installer le Bridge](#installer-le-bridge)
4. [Passer l’avertissement Windows SmartScreen](#passer-lavertissement-windows-smartscreen)
5. [Utiliser le démarrage automatique](#utiliser-le-démarrage-automatique)
6. [Lire les voyants de la MT4](#lire-les-voyants-de-la-mt4)
7. [Faire un premier essai MIDI](#faire-un-premier-essai-midi)
8. [Faire un premier essai SysEx](#faire-un-premier-essai-sysex)
9. [Résoudre les problèmes courants](#résoudre-les-problèmes-courants)
10. [Débrancher et rebrancher la MT4](#débrancher-et-rebrancher-la-mt4)
11. [Savoir ce qui marche et ce qui ne marche pas](#savoir-ce-qui-marche-et-ce-qui-ne-marche-pas)
12. [Utiliser deux interfaces MT4](#utiliser-deux-interfaces-mt4)
13. [Lexique](#lexique)

---

# Vérifier les prérequis

| Besoin | Détail |
|---|---|
| Ordinateur | **Windows 10**, **64 bits** (cette édition du Bridge tourne aussi sous Windows 11 si vous choisissez l’installateur virtualMIDI) |
| Matériel | Une interface MIDI **Emagic MT4** |
| Pilote MIDI tiers | **virtualMIDI**, installé **par vous** — voir ci-dessous |

## Logiciels utiles ensuite

Pour les essais du quotidien, ouvrez **votre logiciel de musique habituel** (votre DAW) — par exemple Ableton Live, Cubase, Reaper ou Bitwig. Tout éditeur capable d’envoyer et recevoir des notes MIDI, des CC ou même des messages SysEx convient pour un premier essai avec votre home studio.

# Installer virtualMIDI

Dans cette version compatible Windows 10, le Bridge parle au DAW via **virtualMIDI**. Installez-le depuis le site internet de Tobias Erichsen **avant** d’installer le Bridge, en téléchargeant par exemple :

- **[loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html)** (le plus courant), ou
- **[rtpMIDI](https://www.tobias-erichsen.de/software/rtpmidi.html)**

Vérifiez ensuite la présence du fichier `teVirtualMIDI.dll` dans `C:\Windows\System32\`.

**Pourquoi installer ce logiciel soi-même ?** virtualMIDI est propriétaire. Ce dépôt MIT **n’a pas** d’autorisation de redistribution et **n’embarquera jamais** la DLL ni le paquet d’installation dans le téléchargement du Bridge.

Sans la DLL, l’installateur du Bridge **échoue clairement** — une liste de ports vide n’est pas une installation réussie.

# Installer le Bridge

Le fichier téléchargé ressemble à : `unitor-mt4-bridge-{version}-win10-virtualmidi-setup.exe` (page **Releases**).

1. Branchez la **MT4** avant ou pendant l’installation — l’assistant **ne s’arrête pas** pour vous le demander.
2. Lancez le programme d’installation téléchargé.
3. Acceptez le dossier sous Program Files (`Ten Square Software\Unitor MT4 Bridge`).
4. Autorisez Administrateur **une fois**.
5. Attendez le succès — ou l’échec si un contrôle a manqué.

Succès = DLL présente + WinUSB OK + démarrage automatique enregistré.

**Un seul programme d’installation à la fois :** les éditions Windows 11 et Windows 10 partagent la **même identité produit Windows**. En installer une **remplace** l’autre sous `Program Files` et réécrit le démarrage automatique.

**WinUSB** est déjà installé dans Windows. Sur un PC neuf, Windows refuse souvent d’associer tout seul la MT4 à ce pilote, faute de **catalogue de confiance** (*trusted catalog*) signé — ce projet n’en fournit pas.

Dans ce cas, utilisez l’outil gratuit **[Zadig](https://zadig.akeo.ie/)** :

1. Branchez la MT4 et lancez Zadig.
2. Dans la liste, sélectionnez l’interface MIDI composite de la MT4 appelée **MI_02** (identifiant USB `VID_086A&PID_0003&MI_02`) — pas une autre ligne USB au hasard.
3. Choisissez le pilote **WinUSB**, puis validez l’installation.
4. Relancez ensuite l’installateur du Bridge si besoin.

# Passer l’avertissement Windows SmartScreen

Windows peut afficher **Microsoft Defender SmartScreen** (« Windows a protégé votre PC »). Cela peut arriver pour un fichier **non signé**, ou signé mais **pas encore** crédité par la réputation. Un avertissement **ne** signifie **pas** automatiquement un malware.

**Ne continuez que si vous avez téléchargé l’installateur depuis les Releases de ce projet.**

Pour vérifier la signature : clic droit → **Propriétés** → **Signatures numériques**. Si l’onglet manque, le fichier est en général **non signé**. Pour ce projet communautaire, le développeur a préféré **ne pas acheter** de certificat Authenticode dont le coût annuel est très élevé.

Quand la politique du PC le permet :

1. Choisissez **Informations complémentaires**.
2. Choisissez **Exécuter quand même**.

Autre piste : clic droit → **Propriétés** → cochez **Débloquer** si affiché, puis Appliquer / OK.

Ne désactivez pas SmartScreen globalement.

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

| Sens | Noms |
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

Ensuite seulement, lancez un échange de messages SysEx si vous souhaitez travailler avec ce type de messages MIDI. **Le SysEx seul ne réveille pas** le Computer Mode : si vous commencez directement par un dump, la MT4 peut rester muette.

## Plusieurs applications à la fois

Avec **virtualMIDI**, un même port peut en général être ouvert par **environ huit applications** en même temps (DAW + éditeur + moniteur MIDI, etc.).

Si un logiciel refuse d’ouvrir un port déjà utilisé, ou affiche une erreur du type « port exclusif » / « device in use » :

1. Ouvrez les **préférences MIDI** (ou **périphériques MIDI**) de **chaque** application concernée.
2. Cherchez une option nommée souvent **Exclusive**, **Exclusive Mode**, **Mode exclusif** ou **Accès exclusif** sur l’entrée / la sortie `MT4 In` / `MT4 Out`.
3. **Décochez** cette option, validez, puis rouvrez les ports ou redémarrez l’application.

L’emplacement exact dépend du logiciel (Ableton Live, Cubase, Reaper, Bitwig, …), mais l’idée est toujours la même : ne pas réserver le port à une seule application.

# Faire un premier essai SysEx

1. Bridge en cours ; ports visibles.
2. Envoyez un court message MIDI ordinaire pour le Computer Mode.
3. Dans **votre DAW** ou l’éditeur de votre synthé, sélectionnez les ports et terminez un court échange SysEx (par exemple une sauvegarde / restauration de presets si vous utilisez un éditeur MIDI comme [Matrix-Control](https://github.com/tensquaresoftware/matrix-control), développé par Ten Square Software).

# Résoudre les problèmes courants

## virtualMIDI manquant

Installez loopMIDI ou rtpMIDI, confirmez la présence du fichier `teVirtualMIDI.dll`, relancez l’installateur du Bridge. Ce projet **n’embarquera jamais** la DLL.

## SmartScreen, WinUSB, pas de ports, pas de SysEx

Reprenez les mêmes contrôles que dans le guide Windows 11, formulés simplement :

1. Avertissement SmartScreen → **Exécuter quand même** uniquement si le fichier vient des Releases de ce projet.
2. Association USB → Zadig sur **MI_02** si Windows refuse le paquet faute de catalogue de confiance (*trusted catalog*).
3. Ports invisibles → actualisez la liste MIDI dans le DAW, vérifiez que le Bridge tourne.
4. SysEx sans réponse → réveillez d’abord le **Computer Mode** avec du MIDI ordinaire.

Sous Windows 10, si vous êtes à l’aise avec l’ordinateur, vous pouvez aussi vérifier des détails techniques (PowerShell, services Windows). Ce n’est **pas** obligatoire pour la plupart des musiciens, et ce n’est **pas** la méthode principale recommandée sous Windows 11 (où un simple redémarrage du PC suffit souvent).

## Ports absents après débranchement / rebranchement

Attendez, rescanez le MIDI, ou quittez/relancez le Bridge. Un redémarrage Windows n’est **pas** le correctif normal.

# Débrancher et rebrancher la MT4

Débrancher → ports de cette unité disparaissent. Rebrancher → le Bridge les recrée. Rescan ou relance supervisée = OK. Exiger un redémarrage pour un simple débranchement = échec.

# Savoir ce qui marche et ce qui ne marche pas

## Ce qui marche

- Édition Windows 10 avec virtualMIDI **installé par vous**
- Même enchaînement WinUSB / démarrage automatique / MIDI / SysEx / débranchement une fois les prérequis OK

## Ce que cette édition ne fait pas

- Livrer ou embarquer virtualMIDI
- Présenter Windows 10 comme l’édition la plus simple (préférez Windows 11 + Windows MIDI Services si possible)
- WinUSB « installateur seul » sur tout PC neuf sans étape guidée / **trusted catalog**
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
| **Windows MIDI Services** | Pile MIDI Microsoft sous Windows 11 | Utilisée par l’**autre** édition — pas requise sur cette édition Windows 10 |
| **MI_02** | Interface USB composite | La bonne ligne à choisir dans Zadig pour la partie MIDI de la MT4 |

---

Encore un projet mené avec succès via la méthode [BMad](https://github.com/bmad-code-org/bmad-method) !
