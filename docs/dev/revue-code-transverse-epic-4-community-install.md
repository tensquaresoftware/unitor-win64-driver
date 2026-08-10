# Prompt — Revue de code transverse Epic 4 (Community Install and Trust)

Copier-coller tel quel dans un **nouveau chat Cursor** (contexte frais), **après** (ou en s’appuyant sur) les revues Epic 1–3.  
Skill conseillé : **bmad-code-review** (+ edge-case / adversariale si utile).  
Projet : `C:\Users\Guillaume\Dev\Projects\unitor-win64-driver`

---

## Contexte de la Story

On cherche les failles **entre** l’installateur public, les docs utilisateur, l’honnêteté licences / backends, et la politique Authenticode / SmartScreen — ce qui pourrait encore bloquer ou tromper un musicien Windows, alors que chaque story 4.x prise seule semblait OK.

---

## Qui tu es / comment parler

- Agent BMad développeur / reviewer sur **unitor-win64-driver**.
- Français clair, intention produit d’abord (ce que ça change pour l’utilisateur MIDI / communauté).
- Noms gardés : **MT4**, **WinUSB**, **virtualMIDI**, **MIDI**, **Bridge**, **SmartScreen**, **Authenticode**, **loopMIDI**, **rtpMIDI**.
- Pas de jargon BMad non glosé ; chemins/symboles en complément.
- **Pas de commit** sans demande explicite.
- Règles : clarity-bar, `conventions.md` §3, lint sur diff C++ touché (souvent peu de C++ dans cet épic).

---

## Pourquoi cette revue (contexte produit)

Epic 4 est **clôturé** côté sprint (`epic-4: done`, stories 4.1–4.4 `done`). Les revues **story par story** sont passées. Comme pour Epic 1–3, on veut une **revue transverse** : joints aux interfaces packaging / docs / confiance, pas une relecture isolée du dernier commit.

Preuves / honnêteté lab à respecter :

- Smoke Public Installer Win10 (2026-08-10) : virtualMIDI-absent **Pass** ; wizard / progression / pas de faux succès / UAC **Pass** ; **Fail** association WinUSB sur PC propre (`0xE000022F` INF non signé) ; Auto-Start / uninstall / régression install dir **N/A** (rollback). Voir `docs/tests/smoke-epic4-public-installer-mt4.md`.
- Ne **pas** inventer un Pass « install communauté complète sur PC clean » — le frein catalogue / Authenticode est connu (4.4 / OQ-3).
- Docs 4.2 / licences 4.3 / SmartScreen 4.4 : smokes docs-first souvent déjà Pass ; croiser la cohérence avec le Fail install réel.
- Version unique CMake → Bridge → Setup : câblée en fin de 4.1 ; vérifier qu’aucun second numéro fantôme ne reste.

Priorité Guillaume : **parcours communauté crédible** (install, docs, confiance) **sans** rouvrir la chasse SysEx Epic 2 ni le runtime hot-plug Epic 3 sauf si l’installateur les casse.

Prompts frères (ne pas dupliquer leurs chasses) :

- `docs/dev/revue-code-transverse-epic-1-mt4-midi.md`
- `docs/dev/revue-code-transverse-epic-2-transport-sysex.md`
- `docs/dev/revue-code-transverse-epic-3-daily-studio.md`

---

## Objectif Epic 4

Epic 4 = « Community Install and Trust » (sprint : **done**).

But produit : un nouvel utilisateur Windows peut suivre un installateur court, des docs claires, des messages de licence honnêtes, et une politique de signature / SmartScreen explicite — pour viser le premier MIDI le soir même **sans** boîte à outils développeur.

Stories (toutes **done**) :

1. Public Installer (barre UX AD-12) — done (lab WinUSB clean = Fail catalog)  
2. Docs utilisateur first MIDI / SysEx — done  
3. Docs techniques + honnêteté MIT / virtualMIDI / Windows MIDI Services — done  
4. Politique Authenticode + SmartScreen — done (cert OQ-3 différé)

SSOT : `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md`  
Sprint : `_bmad-output/implementation-artifacts/sprint-status.yaml`  
Stories : `_bmad-output/implementation-artifacts/4-1-*.md` … `4-4-*.md`  
Différé : `_bmad-output/implementation-artifacts/deferred-work.md`  
Smokes :

- `docs/tests/smoke-epic4-public-installer-mt4.md`
- `docs/tests/guide-operateur-smoke-4-1-pc-propre-win10.md`
- `docs/tests/smoke-epic4-user-docs-mt4.md`
- `docs/tests/smoke-epic4-license-honesty-mt4.md`
- `docs/tests/smoke-epic4-authenticode-smartscreen-mt4.md`

---

## Mission (revue transverse Epic 4)

Revue **transversale** centrée sur les **joints** entre Installateur × Auto-Start (Epic 3) × WinUSB INF × virtualMIDI × docs utilisateur × licences × SmartScreen / catalog. Cherche ce qui peut laisser un musicien bloqué, trompé (faux succès, docs qui survendent), ou avec un Bridge / Auto-Start incohérent après install / désinstall — alors que chaque story seule était verte.

### Focus (dans l’ordre)

1. **Installateur × portes AD-12**  
   - virtualMIDI manquant → blocage avant succès ; succès seulement si WinUSB + Auto-Start + virtualMIDI.  
   - Échec porte → rollback (pas d’install « à moitié » présentée comme OK).  
   - Messages utilisateur courts (pas de prose contributeur tronquée).  
   - Elevation : admin une fois ; Auto-Start en utilisateur interactif (pas profil admin élevé).

2. **Installateur × runtime Epic 3**  
   - Enregistre / désenregistre uniquement via CLI Bridge existante (`--register-auto-start` / `--unregister-auto-start` / `--auto-session`).  
   - Pas de second mécanisme Auto-Start, pas de service Session-0, pas de dual Scheduler+Run.  
   - Upgrade / CloseApplications : ne pas tuer une session MIDI sans prévenir.  
   - Chemin install absolu stable ; WorkingDirectory / Auto-Start cohérents.

3. **WinUSB INF × confiance (4.1 × 4.4)**  
   - GUID / HWID alignés transport.  
   - Payload sans `.cat` production → Fail clean machine **attendu** jusqu’à certificat ; docs et messages ne doivent pas prétendre le contraire.  
   - Lab `sign-lab-package.ps1` ≠ Authenticode public ; ne pas les confondre dans UI ou README.

4. **Docs utilisateur × réalité install (4.2 × 4.1)**  
   - Le guide ne doit pas promettre un succès WinUSB clean si le Setup unsigned échoue encore.  
   - SmartScreen / unsigned : étapes claires (EN + FR).  
   - virtualMIDI : loopMIDI / rtpMIDI ; OQ-1 embed MSI non survenu.

5. **Licences / backends × façade (4.3 × README)**  
   - MIT ≠ virtualMIDI ≠ Windows MIDI Services (futur).  
   - Ten Square Software cohérent installateur / docs / ARP.

6. **Version unique**  
   - CMake `project(VERSION)` = Bridge `--version` = Setup AppVersion / File version ; pas de second SSOT fantôme.

### Hors focus (ne pas élargir)

- Chasse overnight SysEx / Matrix → Epic 2.  
- Hot-plug / dual-MT4 runtime sauf si l’installateur les casse.  
- Acheter le certificat / trancher OQ-3 — décision Guillaume, pas patch code.  
- Epic 5 latence / harness.  
- Contourner avec Zadig pour « sauver » un Pass install.  
- Embed MSI Tobias sans OQ-1.

### Livrable attendu

1. Bilan français clair, findings classés **Bloquant / Sérieux / Léger**, max ~10, **priorisés pour le parcours communauté**.  
2. Pour chacun : symptôme utilisateur, lieu, pourquoi la revue story a pu le rater, fix **minimal**.  
3. Marquer explicitement **non prouvé en lab** (parcours succès clean WinUSB) vs faille déjà visible offline.  
4. **Top 3** prochaines actions (patch + smoke / certificat) avant Epic 5 ou rétro.  
5. Code seulement si finding net ; **pas de commit** sans demande.

### Méthode suggérée

- Lire intent Epic 4 dans `epics.md` + stories `4-1`…`4-4` + `deferred-work.md` (sections Epic 4).  
- Code / packaging à prioriser :
  - `installer/public-installer.iss` + helpers `installer/*.ps1`
  - `scripts/packaging/build-public-installer.ps1`, `verify-installer-contract.py`, `sign-public-artifacts.ps1`
  - `installer/sign-lab-package.ps1` (lab only)
  - `CMakeLists.txt` / `BridgeVersion*.in` (version SSOT)
  - `docs/user/*`, `docs/dev/license-and-backends.md`, `docs/dev/authenticode-and-smartscreen.md`
  - Smokes Epic 4  
- Revue transverse, **pas** « diff de la story 4.4 seule ».

### Commande d’amorce (exemple)

```text
/bmad-code-review

Revue TRANSPARENTALE Epic 4 (Community Install and Trust).
Focus : joints Installateur × Auto-Start × WinUSB/catalog × docs × SmartScreen.
Clean WinUSB Fail connu — rester honnête. Pas de commit.
Lis le prompt : docs/dev/revue-code-transverse-epic-4-community-install.md
Français clair.
```

---

## Checklist opérateur (si tu veux croiser lab après findings / certificat)

- [ ] Win10 x64 propre + Setup courant  
- [ ] virtualMIDI absent → blocage  
- [ ] virtualMIDI présent + catalogue de confiance → WinUSB + succès + Auto-Start + ports  
- [ ] Désinstall → plus d’Auto-Start  
- [ ] SmartScreen / signature : comportement documenté vs réel  

Bonne revue.

---

### Review Findings

*(Code review transverse Epic 4 — 2026-08-10. Couches : Blind Hunter + Edge Case Hunter + Acceptance Auditor.)*

#### Decision-needed

- [x] [Review][Decision] Formuler l’épic 4 « done » comme **docs + packaging + fail-close**, pas comme « premier MIDI le soir même sur PC propre » — jusqu’à OQ-3 / `.cat` production (AD-12-5 Fail lab `0xE000022F`). **Décidé 2026-08-10 :** README Status + guides alignés ; succès clean reste OQ-3.

#### Patch

- [x] [Review][Patch] Guides EN+FR : ne plus survenir l’install WinUSB clean ; expliquer frein catalogue / INF ; dépannage ≠ « juste relancer » ; nuancer « What works » / accroche soir même [`docs/user/unitor-mt4-bridge-user-guide.md` / `unitor-mt4-bridge-guide-utilisateur.md`]
- [x] [Review][Patch] Guides : corriger l’étape inventée « branchez quand l’assistant le demande » (Setup n’invite pas) [`docs/user/*`]
- [x] [Review][Patch] Avant `Abort` si Auto-Start déjà enregistré : `--unregister-auto-start` best-effort [`installer/public-installer.iss`]
- [x] [Review][Patch] Messages rollback honnêtes (fichiers/ARP vs résidu Driver Store possible) [`installer/public-installer.iss`]
- [x] [Review][Patch] Upgrade : re-register best-effort si Auto-Start existait avant et register échoue [`installer/public-installer.iss`]
- [x] [Review][Patch] Packaging : refuser Setup si `Bridge.exe --version` ≠ `AppVersion` résolu [`scripts/packaging/build-public-installer.ps1`]
- [x] [Review][Patch] README Status : état réel (fail-close OK ; succès clean bloqué catalogue / OQ-3) [`README.md`]
- [x] [Review][Patch] Désinstall / docs : Auto-Start = utilisateur courant ; WinUSB peut rester ; autres comptes Windows [`installer/public-installer.iss` / `docs/user/*`]

#### Defer

- [ ] [Review][Defer] `.cat` / Authenticode production + re-smoke AD-12-5…9 sur PC propre — OQ-3 Guillaume
- [ ] [Review][Defer] Smoke 4.2 rows UJ-1/UJ-2 hardware encore vides — lab après install clean possible
- [ ] [Review][Defer] Timeout si `pnputil` / Bridge hang sous Inno `Exec` — limitation Inno déjà notée
- [ ] [Review][Defer] Unregister Auto-Start multi-profil Windows exhaustif — hors V1 (doc suffit pour l’instant)
- [ ] [Review][Defer] Notes Completion 4.4 encore « blank 4.1 » — cosmétique story file

