# Pilote Windows 64 bits pour l'interface MIDI USB Emagic MT4

> **État projet (Correct Course 2026-08-10) — lire avant d’utiliser ce brief historique**
>
> Posture **hobby / open source** : pas de certificat Authenticode dans cette ligne de projet ; pas de Releases community liées au SDK virtualMIDI (hors scope — binaires publics prévus via **Windows MIDI Services / Win11**, épic 6) ; **épic 5** (latence) d’abord sur stack lab virtualMIDI + Win10. Install hobby = SmartScreen documenté + **WinUSB guidé** (Zadig), pas « Setup seul = succès magique sur PC neuf ». SSOT : root `README.md` + `_bmad-output/planning-artifacts/sprint-change-proposal-2026-08-10.md`.
>
> Le texte ci-dessous reste le brief de **démarrage** d’origine (contexte matériel / Linux / Prodikeys). Ne pas le traiter comme la promesse produit actuelle.

## Contexte et objectif

Je démarre un projet **greenfield** sous Cursor avec la méthode **BMad**. L'objectif est de développer un pilote/pont logiciel permettant à l'interface MIDI USB **Emagic MT4** (sortie en 2001, rachetée par Apple en 2002 avec Emagic) de fonctionner sur **Windows 10/11 64 bits**, alors qu'aucun pilote officiel 64 bits n'a jamais été publié par le fabricant (le dernier pilote officiel date de Windows XP 32 bits).

Ce projet a une vraie utilité communautaire : de nombreux utilisateurs de la famille Emagic Unitor8 / AMT8 / MT4 cherchent une solution depuis des années sans succès (voir forums cités plus bas).

## Identification du matériel

- **Fabricant (VID)** : Emagic — `086A` (hexadécimal)
- **Produit (PID) MT4** : `0003`
- Combinaison USB : `VID_086A&PID_0003`
- Produits apparentés (même famille de protocole, PID différent) : Unitor8 (`0001`), AMT8 (`0002`)
- Le MT4 propose 2 entrées / 4 sorties MIDI physiques (32 canaux d'entrée / 64 canaux de sortie via multiplexage MIDI).
- Le périphérique est alimenté directement par l'USB (pas d'alimentation externe).

## Ce qu'on sait déjà (acquis avant de démarrer le code)

1. **Le protocole n'est pas 100% class-compliant USB-MIDI.** Le noyau Linux le gère via une "quirk" dédiée dans le pilote générique `snd-usb-audio` : `QUIRK_MIDI_EMAGIC`. Autrement dit, Windows voit le périphérique mais son pilote générique de classe audio/MIDI ne sait pas interpréter le mapping propriétaire des câbles MIDI Emagic — d'où l'absence de ports MIDI fonctionnels malgré une énumération USB réussie.
2. **Le protocole a déjà été rétro-ingénieré et documenté en open source.** Emagic avait transmis la documentation du protocole série/USB aux développeurs Linux à l'époque. Le code résultant est public (licence GPL) dans le noyau Linux :
   - `sound/usb/quirks-table.h` — déclare le VID/PID et la structure `snd_usb_midi_endpoint_info` (mapping des câbles IN/OUT) pour Unitor8, AMT8 et MT4.
   - `sound/usb/midi.c` — implémentation du traitement des messages Emagic (recherchable via le terme `emagic` dans le fichier).
   - Dépôt à consulter : https://github.com/torvalds/linux/blob/master/sound/usb/midi.c et https://github.com/torvalds/linux/blob/master/sound/usb/quirks-table.h
3. **Un projet de référence quasi identique existe déjà** pour un autre périphérique legacy non class-compliant (clavier Creative Prodikeys, protocole HID propriétaire) : il combine **WinUSB (installé via Zadig)** + **virtualMIDI SDK de Tobias Erichsen** pour réimplémenter un pilote MIDI 64 bits sans écrire de driver kernel. C'est une excellente base d'inspiration architecturale :
   - https://github.com/CrazyRedMachine/Prodikeys64
4. **Des utilisateurs ont déjà exprimé le besoin et partagé des pistes** sur les forums suivants (à consulter pour glaner d'éventuelles copies de la doc protocole traduite, ou du code partiel déjà tenté) :
   - https://www.modwiggler.com/forum/viewtopic.php?t=142226 (fil le plus riche : mention explicite que la doc protocole Emagic a circulé)
   - https://gearspace.com/board/music-computers/141084-emagic-amt-8-drivers-2.html
   - https://forum.cockos.com/archive/index.php/t-191736.html
5. **Le manuel utilisateur officiel Unitor8/AMT8** (utile pour la partie fonctionnelle : modes Patch, LTC/VITC, Fast Mode/AMT) est disponible ici : https://www.deepsonic.ch/deep/docs_manuals/emagic_unitor8_mkII_amt8_manual.pdf

## Architecture technique visée

**Option retenue : approche usermode via WinUSB, pas de pilote kernel-mode custom.**

Raisons : un vrai pilote kernel-mode (KMDF, modèle Port/Miniport audio) nécessite le WDK complet, une signature Microsoft (attestation signing via le Partner Center) pour se charger sur un Windows 64 bits en Secure Boot activé, et une expertise driver kernel pointue. C'est disproportionné pour ce projet et un frein majeur à la distribution vers d'autres utilisateurs.

Pipeline proposé :

1. **Couche transport USB** : le périphérique est lié au pilote générique **WinUSB** (`winusb.sys`, fourni et déjà signé par Microsoft) soit via un fichier `.inf` custom minimal ciblant `VID_086A&PID_0003`, soit manuellement via l'outil **Zadig** (https://zadig.akeo.ie/) pendant le développement.
   - Doc Microsoft de référence : https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/introduction-to-winusb-for-developers
   - Choix du bon modèle de driver USB : https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-considerations
   - Installation WinUSB pour développeurs : https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-installation
2. **Couche protocole** : une application/service en mode utilisateur (C++, idéalement compatible avec mon stack JUCE existant) dialogue avec le périphérique via l'API `winusb.dll` (transferts bulk/interrupt), en réimplémentant la logique de mapping des câbles trouvée dans `midi.c`/`quirks-table.h` du noyau Linux.
3. **Couche exposition MIDI vers les DAW** : les ports MIDI virtuels sont créés soit via le **virtualMIDI SDK** de Tobias Erichsen (https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html — attention, licence propriétaire, à vérifier pour un usage/redistribution en open source), soit via la nouvelle **API Windows MIDI Services** (MIDI 2.0, native depuis Windows 11 24H2, gratuite et maintenue par Microsoft) si la cible Windows le permet — à trancher lors du sprint d'architecture.

## Fichiers attendus dans le livrable final

- `mt4driver.inf` — associe le VID/PID du MT4 à WinUSB (facultatif si Zadig suffit pour un usage perso, nécessaire pour une distribution grand public simplifiée).
- Binaire de service/application usermode (le "vrai" cœur du projet, en C++).
- Éventuellement un installeur regroupant WinUSB + le service + la dépendance MIDI virtuelle.

## Contraintes et risques à surveiller dès le sprint de planification

- **Timing/latence MIDI** : le MIDI est très sensible à la gigue. Une implémentation usermode (vs kernel) introduit un peu plus de latence — à mesurer, probablement négligeable pour un usage studio normal mais à valider.
- **Documentation du protocole incomplète** : si aucune copie de la doc originale Emagic n'est retrouvée sur les forums, il faudra s'appuyer uniquement sur la lecture du code Linux (`midi.c`) et potentiellement sur une capture USB (Wireshark + USBPcap) en dernier recours pour valider empiriquement le comportement.
- **Cas multi-interfaces empilées** (plusieurs Unitor8/AMT8/MT4 en cascade) : mentionné comme fragile même sous Linux/ALSA — à traiter comme fonctionnalité avancée, pas comme MVP.
- **Licence du virtualMIDI SDK** : à clarifier avant toute redistribution publique du projet (propriétaire, conditions de Tobias Erichsen) — alternative gratuite : Windows MIDI Services si la contrainte de version Windows est acceptable.
- **Signature de code** (même en usermode, pas de signature kernel requise, mais un Authenticode classique reste recommandé pour la confiance utilisateur/SmartScreen).

## Ce que j'attends de BMad pour ce projet

Merci de démarrer un **projet greenfield** avec la méthode BMad en tenant compte de tout ce contexte. Étapes souhaitées :

1. Phase Analyst/Brainstorming : consolider ce brief en *Project Brief* formel, identifier les inconnues restantes (notamment la disponibilité réelle de la documentation protocole).
2. Phase PM : définir un MVP scope réaliste (ex. : MVP = MT4 seul, 2 IN / 4 OUT, sans mode Patch ni LTC/VITC, sans cascade multi-interfaces).
3. Phase Architecte : trancher précisément l'architecture (WinUSB + virtualMIDI SDK vs WinUSB + Windows MIDI Services), définir la stack (C++ pur / JUCE / autre), et le plan de rétro-ingénierie du protocole à partir de `midi.c`.
4. Découpage en stories avec revues de code (`/bmad-bmm-code-review`) comme d'habitude dans mon workflow.

Mon environnement : MacBook Pro M5 (macOS Tahoe) comme machine principale de développement avec Cursor, mais le build/test final devra se faire sur PC Windows 10 64 bits (cible réelle du pilote) — à anticiper dans la stratégie CI/CD et de test.

## Nom du projet / repo

**Nom retenu : `unitor-win64-driver`** (convention kebab-case).

Justification :

- Le terme **"Unitor"** fait référence à la famille de produits (Unitor8 / AMT8 / MT4 partagent la même architecture de protocole), sans reprendre le nom de la marque **Emagic**, qui reste une marque déposée (aujourd'hui propriété d'Apple suite au rachat de 2002). Le nom "Emagic" est volontairement absent du nom du projet pour limiter tout risque lié au droit des marques — il continue en revanche d'apparaître légitimement dans le README/la documentation technique à titre descriptif et d'interopérabilité (à l'image du projet de référence Prodikeys64, qui mentionne "Creative" dans sa description sans l'utiliser dans son nom de repo).
- **"win64"** indique clairement la plateforme et l'architecture ciblées (Windows 64 bits), ce qui était le principal manque identifié dans les premières pistes de noms.
- **"driver"** reste le terme le plus immédiatement compréhensible pour un utilisateur final cherchant une solution, même si l'architecture technique retenue (cf. fichier principal) est en réalité une combinaison WinUSB + composant usermode + pont MIDI virtuel, plutôt qu'un driver kernel-mode au sens strict.

Autres noms envisagés et écartés : `unitor-winusb-bridge`, `unitor-win64-midi`, `unitor-x64-bridge`, `win64-unitor-driver` — conservés ici à titre de mémoire de décision, au cas où le nom final serait rediscuté en phase de branding avec BMad.

## Compatibilité AMT8 — analyse à intégrer à l'architecture

Le MVP scope initial cible uniquement le MT4 (que je possède). L'AMT8 fait partie de la même famille de protocole et mérite d'être anticipée dès la conception, sans pour autant faire partie du périmètre de développement actif de la V1.

**Élément clé** : dans `sound/usb/quirks-table.h` (noyau Linux), Unitor8, AMT8 et MT4 sont tous les trois traités par la même quirk `QUIRK_MIDI_EMAGIC`, avec la même structure `snd_usb_midi_endpoint_info`, seule la valeur des bitmasks change :

| Modèle  | PID    | `in_cables` | `out_cables` | Interface USB (`ifnum`) |
| ------- | ------ | ----------- | ------------ | ----------------------- |
| MT4     | `0003` | `0x8003`    | `0x800f`     | `2`                     |
| AMT8    | `0002` | `0x80ff`    | `0x80ff`     | `2`                     |
| Unitor8 | `0001` | `0x80ff`    | `0x80ff`     | `2`                     |

La cohérence du numéro d'interface (`ifnum = 2`) entre les trois modèles est un bon indicateur structurel : cela suggère que le protocole de transport USB sous-jacent est identique, et que seule la topologie logique des câbles MIDI (nombre de ports IN/OUT exposés) diffère selon le modèle.

**Recommandation d'architecture pour l'Architecte BMad** : modéliser la couche protocole comme un mapping déclaratif plutôt que comme du code spécifique par modèle. Concrètement :

- Une interface/abstraction unique (type `IEmagicCableMapper`) qui implémente une seule fois la logique de dé-multiplexage des câbles MIDI Emagic (portée de `midi.c`).
- Une table de configuration par périphérique (`DeviceProfile`), indexée par PID, contenant `in_cables`, `out_cables`, `ifnum`, et les éventuelles fonctionnalités additionnelles (mode Patch, LTC/VITC, Fast Mode/AMT — présentes selon le manuel Unitor8/AMT8, à vérifier lesquelles sont réellement exposées sur l'AMT8 par rapport au seul Unitor8 Mk II qui est le modèle haut de gamme).

Avec ce découpage, l'ajout du support AMT8 après la V1 ne serait pas une story de développement (le code de mapping des câbles serait déjà générique), mais **une story de validation matérielle** : ajout de l'entrée `DeviceProfile` pour le PID `0002`, campagne de tests sur du matériel AMT8 réel (que je ne possède pas actuellement — nécessite soit une acquisition, soit un partenariat avec un testeur communautaire possédant cet appareil), et ajustements empiriques si des écarts de comportement apparaissent en pratique.

Quid de la Unitor8 mk2 ? Possibilité de rendre mon driver compatible avec ce modèle plus récent ?

## Impact sur le scope BMad

À intégrer dans la phase PM / définition du MVP :

- **V1 (MVP)** : MT4 uniquement, architecture pensée dès le départ pour être multi-device (`DeviceProfile` par PID), mais un seul profil réellement implémenté et testé.
- **V1.x (post-MVP, backlog)** : ajout du profil AMT8 (et éventuellement Unitor8), conditionné à l'accès à du matériel de test réel — à formuler comme story avec dépendance externe explicite ("nécessite accès matériel AMT8"), plutôt que comme simple tâche de développement classique.
