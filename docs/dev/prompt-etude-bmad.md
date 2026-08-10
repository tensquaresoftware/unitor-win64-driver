# Prompt BMad — démarrage de l’étude unitor-win64-driver

---

## Consigne à BMad

Démarre un **projet greenfield** avec la méthode BMad pour `unitor-win64-driver`.

Parcours souhaité (adapte si `bmad-help` propose mieux, mais ne saute pas l’essentiel) :

1. **Analyst / Brainstorming** — consolider ce brief en Project Brief formel ; lister les inconnues restantes (doc protocole Emagic, captures USB, licence virtualMIDI pour redistribution).
2. **PM** — MVP réaliste et backlog post-MVP (AMT8 / Unitor8 / Unitor8 mk2, éventuelle v2 Win11 / second backend).
3. **Architecte** — trancher la stack C++ usermode, le plan de réimplémentation protocole (référence Linux, pas copie GPL), le backend MIDI virtuel pour **Windows 10**, le découpage `DeviceProfile`.
4. **Découpage** en epics/stories avec revues `bmad-code-review` dans le workflow habituel.

Contraintes non négociables déjà tranchées par le porteur de projet (Guillaume / Ten Square Software) — les respecter, ne pas les rouvrir sauf risque bloquant documenté :

| Décision | Choix |
| --- | --- |
| Plateforme cible MVP | **Windows 10 et 11, 64 bits** — Win10 est obligatoire |
| Type de solution | **Usermode** (WinUSB + appli/service C++) — **pas** de pilote kernel custom en V1 |
| Backend MIDI virtuel MVP | **virtualMIDI SDK** (Tobias Erichsen) — Windows MIDI Services = piste **v2 / second backend** (Win11 only, pas dispo sur Win10) |
| Licence du code du projet | **MIT** (réimplémentation originale ; ne pas vendorer de sources GPL Linux) |
| Périmètre matériel V1 | **MT4 uniquement** (`VID 086A` / `PID 0003`), architecture multi-device dès le départ |
| Qualité | Appliquer `conventions.md` + porte `scripts/quality/lint-touched.py` dès qu’il y a du C++ |

Lire en priorité dans le dépôt :

- `docs/dev/prompt-demarrage-projet-bmad.md` (brief détaillé d’origine)
- `README.md`
- `conventions.md`
- `contributing.md`

---

## Contexte produit

**Problème.** Les interfaces USB MIDI Emagic de la famille Unitor (MT4, AMT8, Unitor8, Unitor8 mk2) n’ont jamais reçu de pilote Windows 64 bits officiel (dernier paquet vendeur : Windows XP 32 bits). Sous Windows 10/11 le périphérique s’énumère en USB, mais le pilote de classe générique ne comprend pas le **mapping propriétaire des câbles MIDI Emagic** → aucun port MIDI utilisable dans les DAW.

**Objectif.** Fournir un pont logiciel communautaire pour faire fonctionner au moins le **MT4** sur Windows 10/11 64 bits, avec une architecture extensible aux cousins AMT8 / Unitor8.

**Utilité.** Besoin exprimé depuis des années (forums Mod Wiggler, Gearspace, Cockos, etc.).

**Nom du dépôt.** `unitor-win64-driver` (kebab-case). « Unitor » = famille de protocole ; « Emagic » volontairement absent du nom (marque, aujourd’hui Apple) mais OK dans docs descriptives.

---

## Matériel

| Modèle | VID | PID | I/O physiques (attendu) | Rôle |
| --- | --- | --- | --- | --- |
| **MT4** | `086A` | `0003` | 2 IN / 4 OUT | **MVP V1** — matériel disponible |
| AMT8 | `086A` | `0002` | 8×8 (attendu) | Post-MVP — validation matérielle requise |
| Unitor8 | `086A` | `0001` | 8×8 (attendu) | Post-MVP — validation matérielle requise |

- USB MT4 : `VID_086A&PID_0003`, alimenté par le bus.
- Sous Linux, les trois modèles partagent `QUIRK_MIDI_EMAGIC` et `ifnum = 2` ; seuls les bitmasks `in_cables` / `out_cables` changent (`quirks-table.h`).

| Modèle | `in_cables` | `out_cables` | `ifnum` |
| --- | --- | --- | --- |
| MT4 | `0x8003` | `0x800f` | `2` |
| AMT8 | `0x80ff` | `0x80ff` | `2` |
| Unitor8 | `0x80ff` | `0x80ff` | `2` |

---

## Ce qui est déjà acquis (avant code)

1. **Pas 100 % class-compliant USB-MIDI.** Windows voit l’appareil ; le mapping câbles Emagic manque → pas de ports MIDI utiles.
2. **Protocole déjà rétro-ingénieré côté Linux (GPL).** Références à *lire*, pas à copier dans le dépôt :
   - https://github.com/torvalds/linux/blob/master/sound/usb/quirks-table.h
   - https://github.com/torvalds/linux/blob/master/sound/usb/midi.c (terme `emagic`)
3. **Patron architectural proche :** [Prodikeys64](https://github.com/CrazyRedMachine/Prodikeys64) — WinUSB (Zadig) + virtualMIDI, sans driver kernel.
4. **Demandes utilisateurs / pistes :**  
   - https://www.modwiggler.com/forum/viewtopic.php?t=142226  
   - https://gearspace.com/board/music-computers/141084-emagic-amt-8-drivers-2.html  
   - https://forum.cockos.com/archive/index.php/t-191736.html
5. **Manuel Unitor8/AMT8** (modes Patch, LTC/VITC, Fast Mode/AMT — hors MVP) :  
   https://www.deepsonic.ch/deep/docs_manuals/emagic_unitor8_mkII_amt8_manual.pdf

---

## Architecture cible (déjà orientée)

### Pipeline V1

1. **Transport USB** — lier le device à **WinUSB** (`winusb.sys`, signé Microsoft) via INF minimal et/ou [Zadig](https://zadig.akeo.ie/) en dev.  
   Docs : [Intro WinUSB](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/introduction-to-winusb-for-developers), [considerations](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-considerations), [installation](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-installation).
2. **Protocole** — service/appli **C++17** usermode via `winusb.dll` ; **réimplémentation originale** du multiplexage câbles Emagic ; table **`DeviceProfile`** par PID.
3. **Exposition DAW** — ports MIDI virtuels via **virtualMIDI SDK** (Tobias Erichsen) pour couvrir **Windows 10**.  
   - Clarifier tôt les conditions de **redistribution** (runtime / SDK propriétaire ≠ MIT du repo).  
   - **Windows MIDI Services** : non dispo sur Win10 (confirmé Microsoft : Win11 only) → backlog v2 ou second backend derrière la même abstraction.

### Pourquoi pas de driver kernel en V1

WDK + attestation signing Secure Boot + expertise noyau = disproportionné et frein à la distrib communautaire. WinUSB garde la partie noyau signée chez Microsoft.

### Abstraction demandée à l’architecte

- Une seule logique de mapping câbles (pas un fork par modèle).
- `DeviceProfile` : `in_cables`, `out_cables`, `ifnum`, capacités optionnelles.
- Interface du type « backend MIDI virtuel » pour pouvoir ajouter Windows MIDI Services plus tard **sans réécrire le protocole**.
- Stack C++ : trancher C++ pur vs outils partagés (ex. JUCE) avec justification.

### Livrables envisagés

- INF WinUSB pour le/les VID/PID
- Binaire usermode (cœur du produit)
- Installeur éventuel (WinUSB + service + dépendance virtualMIDI, en respectant sa licence)
- Authenticode recommandé pour SmartScreen (pas une signature kernel)

---

## MVP / hors scope

**V1 (MVP) — inclus**

- MT4 seul, 2 IN / 4 OUT
- MIDI de base studio (latence/gigue à mesurer, chemin usermode)
- Archi multi-`DeviceProfile` dès le départ, un seul profil testé

**V1 — exclus**

- Patch / LTC / VITC / Fast Mode / AMT
- Cascades multi-interfaces (fragile même sous ALSA)
- Support garanti AMT8 / Unitor8 / Unitor8 mk2 sans matériel de test
- Cible « Windows MIDI Services only »

**Post-MVP**

- Profils AMT8 / Unitor8 / Unitor8 mk2 = stories de **validation matérielle** (dépendance externe explicite)
- Éventuelle **v2 Win11** ou **second backend** Windows MIDI Services

---

## Risques à traiter dès la planification

- Timing / gigue MIDI en usermode
- Doc protocole Emagic introuvable → s’appuyer sur `midi.c` + captures USB (Wireshark / USBPcap) si besoin
- Licence / redistribution **virtualMIDI** vs MIT du projet
- Signature Authenticode du binaire usermode
- Compétence : le porteur n’est **pas** expert drivers ; rester sur le chemin usermode et expliquer les choix en français clair
- Dev principal sur **macOS** (Cursor) ; build/tests USB/DAW sur **PC Windows 10 64 bits** — anticiper CI/CD et stratégie de test

---

## Conventions projet (déjà en place)

- Chat agent : français ; docs BMad générés : anglais ; code : anglais
- Dossiers / fichiers hors C++ : **kebab-case** ; sources C++ sous `src/` : **PascalCase**
- Porte qualité : `python scripts/quality/lint-touched.py` (seuils dans `conventions.md` §3)
- Titres Agents BMad : règle `.cursor/rules/bmad-agent-chat-titles.mdc` + helper `scripts/dev/resolve-bmad-chat-title.py`

---

## Ce que j’attends en sortie de cette étude

Artifacts BMad en **anglais**, décisions et questions en **français clair** dans le chat (barème de clarté du dépôt) :

1. Project Brief consolidé  
2. MVP / non-goals / risques / dépendances externes (matériel AMT8, licence virtualMIDI)  
3. Architecture tranchée (WinUSB + virtualMIDI V1, abstraction backend, `DeviceProfile`, plan protocole sans GPL vendored)  
4. Epics / stories prêtes pour implémentation itérative  

Commence par confirmer la compréhension du problème en une phrase produit, puis propose la prochaine commande BMad adaptée (brief / brainstorming / PRD / etc.) avec **Recommandation BMad**.
