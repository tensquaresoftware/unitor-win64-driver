# Prompt — Revue de code transverse Epic 3 (Daily Studio Resilience)

Copier-coller tel quel dans un **nouveau chat Cursor** (contexte frais), **après** (ou en s’appuyant sur) les revues Epic 1 et Epic 2.  
Skill conseillé : **bmad-code-review** (+ edge-case / adversariale si utile).  
Projet : `C:\Users\Guillaume\Dev\Projects\unitor-win64-driver`

---

## Contexte de la Story

On cherche les failles **entre** les quatre stories d’Epic 3 — démarrage auto, hot-plug, multi-clients, deux MT4 — qui pourraient encore casser une session studio du quotidien, alors que chaque story prise seule semblait OK.

---

## Qui tu es / comment parler

- Agent BMad développeur / reviewer sur **unitor-win64-driver**.
- Français clair, intention produit d’abord (ce que ça change pour l’utilisateur MIDI / studio).
- Noms gardés : **MT4**, **WinUSB**, **VirtualMIDI**, **MIDI**, **MIDI-OX**, **DAW**, **SysEx**, **Bridge**.
- Pas de jargon BMad non glosé ; chemins/symboles en complément.
- **Pas de commit** sans demande explicite.
- Règles : clarity-bar, `conventions.md` §3, lint sur diff C++ touché.

---

## Pourquoi cette revue (contexte produit)

Epic 3 est **clôturé** côté sprint (`epic-3: done`, stories 3.1–3.4 `done`). Les revues **story par story** et leurs patches sont passées. Comme pour Epic 1 et 2, on veut maintenant une **revue transverse** : courses et joints aux interfaces, pas une relecture isolée du dernier commit.

Preuves / honnêteté lab à respecter :

- Auto-Start, hot-plug, multi-client (Live 12 + MIDI-OX) : guides smoke Epic 3 — barème case vide ≠ Pass.
- Dual-MT4 : preuve **offline / registre + noms** OK ; **dual physique non prouvé** (un seul MT4 en lab) — le front matter du smoke dual le dit. Ne pas inventer un Pass matériel.
- Risques déjà notés en différé (ne pas les redécouvrir en silence, les re-prioriser si le joint Epic 3 les aggrave) :
  - `CTRL_CLOSE` / kill brutal → ports VirtualMIDI orphelins (Auto-Start augmente l’exposition quotidienne).
  - Surprise-removal : `Stop` peut encore tenter un WriteBulk « finish magic » sur un bus mort.
  - Couverture offline hot-plug encore mince ; le multi-unit host a changé la boucle produit.
  - `--dev-zadig` reste single-unit lab (pas le chemin dual produit).

Priorité Guillaume : **fiabiliser le studio du quotidien** (ports prêts, débranche/rebranche, DAW + MIDI-OX, noms stables multi-boîtes) **sans** rouvrir la chasse SysEx Matrix d’Epic 2 ni démarrer l’installeur Epic 4.

Prompts frères (ne pas dupliquer leurs chasses) :

- `docs/dev/revue-code-transverse-epic-1-mt4-midi.md`
- `docs/dev/revue-code-transverse-epic-2-transport-sysex.md`

---

## Objectif Epic 3

Epic 3 = « Daily Studio Resilience » (sprint : **done**).

But produit : plus besoin de lancer le Bridge à la main chaque jour ; un débranche/rebranche de MT4 ramène des ports utilisables **sans reboot Windows** ; une DAW et MIDI-OX peuvent partager les mêmes ports ; deux MT4 ont chacune sa session et des noms clairement distincts et stables.

Stories (toutes **done**) :

1. Auto-Start sans Administrateur quotidien — done  
2. Hot-plug recovery sans reboot Windows — done  
3. Multi-client DAW + MIDI-OX — done  
4. Deux MT4, noms stables et distincts — done  

SSOT : `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md`  
Sprint : `_bmad-output/implementation-artifacts/sprint-status.yaml`  
Stories : `_bmad-output/implementation-artifacts/3-1-*.md` … `3-4-*.md`  
Différé : `_bmad-output/implementation-artifacts/deferred-work.md`  
Smokes :

- `docs/tests/smoke-epic3-autostart-mt4.md`
- `docs/tests/smoke-epic3-hotplug-mt4.md`
- `docs/tests/smoke-epic3-multiclient-mt4.md`
- `docs/tests/smoke-epic3-dual-mt4-mt4.md`

---

## Mission (revue transverse Epic 3)

Revue **transversale** centrée sur les **joints** entre Auto-Start, hot-plug multi-unité, multi-clients VirtualMIDI, et registre d’identité / noms. Cherche ce qui peut laisser des ports orphelins, croiser deux boîtes, bloquer un second host, ou faire échouer un replug alors que chaque story seule était verte.

### Focus (dans l’ordre)

1. **Hôte produit multi-unité (`--auto-session`)**  
   - Une boîte = une `DeviceSession` + un backend VirtualMIDI + un jeu de noms `K` — pas de handle WinUSB partagé, pas un seul backend pour deux boîtes.  
   - Arrivée / départ d’**une** unité : Stop/Destroy **seulement** cette unité ; les autres gardent ports et `K`.  
   - Courses entre énumération WinUSB, ouverture par chemin d’instance, et boucle hot-plug.

2. **Hot-plug × Auto-Start**  
   - Première dispo au logon / plug-after-login vs recreate mid-session : mêmes helpers ou divergences dangereuses ?  
   - Absent→Present (settle) vs « encore listé Present » après surprise-removal.  
   - `Start` qui réussit puis `IsRunning()` faux immédiatement — Destroy/Stop bien appelé ?  
   - Cancel (Ctrl+C) pendant wait de recovery : exit propre, pas de ports fantômes.

3. **Identité / ordinal `K` (registre)**  
   - Seul le registre Device (`DeviceSessionManager` / `UnitIdentityRegistry`) possède `K` (serial préféré, topologie en secours).  
   - Débrancher A ne renumérote pas B ; relaunch Bridge → mêmes `K` pour identités connues.  
   - Persistance fichier user-scoped : corruption, collision, chemin manquant, double écriture.  
   - MidiBackend reçoit des noms **déjà prêts** — ne doit pas re-dériver `K` depuis USB.

4. **Multi-clients × lifecycle**  
   - Le Bridge n’ajoute **pas** de politique exclusive-open par-dessus VirtualMIDI.  
   - Hosts déjà ouverts (DAW + MIDI-OX) pendant un unplug/replug : recreate silencieux vs besoin de rescan ; pas de seconde autorité de ports.  
   - Plafond documenté ≤8 clients/port — pas de gate inventé côté Bridge.

5. **Joints avec Epic 1–2 (sans rouvrir leur chasse)**  
   - S’appuyer sur findings Epic 1 (verrou USB, anneau IN, demux) et Epic 2 (SysEx / outbound) **seulement** si un joint Epic 3 les aggrave (ex. Stop pendant WriteBulk mort, recreate sous charge).  
   - Dire clairement ce qui est **spécifique Epic 3**.

### Hors focus (ne pas élargir)

- Installateur public / packaging Auto-Start dans MSI → **Epic 4**.  
- Docs utilisateur poli `docs/user/` → **4.2**.  
- Chasse overnight SysEx Matrix / palier longs → restée Epic 2.  
- Cascade / stacked Emagic multi-chassis — non-goal.  
- Refonte AMT8 / Unitor8 hors DeviceProfile.  
- Inventer un Service Session-0 ou une 2ᵉ autorité de ports.  
- Claim « dual physique Pass » sans deux boîtes — honnêteté smoke obligatoire.

### Livrable attendu

1. Bilan français clair, findings classés **Bloquant / Sérieux / Léger**, max ~10, **priorisés pour le studio quotidien** (ports prêts, hot-plug, multi-clients, noms multi-MT4).  
2. Pour chacun : symptôme utilisateur, lieu code, pourquoi la revue story a pu le rater, fix **minimal**.  
3. Marquer explicitement ce qui reste **non prouvé en lab** (surtout dual physique) vs faille code déjà visible offline.  
4. **Top 3** prochaines actions (patch + smoke) avant d’attaquer Epic 4 ou la rétro.  
5. Code seulement si finding net ; lint clean ; **pas de commit** sans demande.

### Méthode suggérée

- Lire intent Epic 3 dans `epics.md` + stories `3-1`…`3-4` + `deferred-work.md` (sections Epic 3).  
- Code à prioriser :
  - `src/App/MidiSessionCli.cpp`, `MidiSessionMultiHost*.cpp`, `Mt4PresenceWait*`, `Mt4WinUsbPresence*`, Auto-Start registration  
  - `src/Device/DeviceSessionManager*`, `UnitIdentityRegistry*`, `DeviceSession*` (Stop / Destroy)  
  - `src/Midi/VirtualMidiBackend*` (flags create, pas de exclusive)  
  - `src/Usb/` open-by-path / enumerate multi-match  
- Tests : `tests/unit/*` liés hot-plug, multi-client contract, identity registry ; ne pas prendre un Pass offline pour un Pass hardware.  
- Smokes Epic 3 comme contrat opérateur.  
- Revue transverse (skill **bmad-code-review**), **pas** « diff de la story 3.4 seule ».

### Commande d’amorce (exemple)

```text
/bmad-code-review

Revue TRANSPARENTALE Epic 3 (Daily Studio Resilience).
Focus : joints Auto-Start × hot-plug multi-unité × multi-clients × registre K / noms.
Dual physique non prouvé — rester honnête. Pas de commit.
Lis le prompt : docs/dev/revue-code-transverse-epic-3-daily-studio.md
Français clair.
```

---

## Checklist opérateur (si tu veux croiser lab après findings)

- [ ] Win10 x64 + Bridge `--auto-session` (ou Auto-Start enregistré)  
- [ ] Unplug/replug MT4 sans reboot — ports reviennent ; hosts rescannent si besoin  
- [ ] Live 12 (ou Reason 12) + MIDI-OX sur les mêmes ports — activité des deux côtés  
- [ ] (Quand 2× MT4 dispo) deux jeux de noms distincts ; unplug unité 2 ne renumérote pas unité 1  
- [ ] Ctrl+C pour teardown propre ; éviter de valider le happy path avec croix de fenêtre seule  

Bonne revue.

---

### Review Findings

*(Code review transverse Epic 3 — 2026-08-10. Couches : Blind Hunter + Edge Case Hunter + Acceptance Auditor.)*

#### Decision-needed

- [x] [Review][Decision] Cold start multi-unité tout-ou-rien — **Décidé A1 (2026-08-10) :** garder l’unité saine ; voir Patch ci-dessous.
- [x] [Review][Decision] Registre K illisible — **Décidé B2 (2026-08-10) :** refuser de démarrer (fail closed) ; voir Patch ci-dessous.

#### Patch

- [x] [Review][Patch] Rétablir Absent→Present + échec fermé 900 s sur le hot-plug multi-unité [`src/App/MidiSessionMultiHostHotPlug.cpp`]
- [x] [Review][Patch] Auto-Start : annuler la tâche si le clear HKCU Run échoue [`src/App/AutoStartRegistration.cpp`]
- [x] [Review][Patch] Refuser deux identités serial distinctes qui partagent le même `K` [`src/Device/UnitIdentityRegistry.cpp`]
- [x] [Review][Patch] Réconcilier live par `K` / alias (évite teardown serial↔topology) [`src/App/MidiSessionMultiHostHotPlug.cpp`]
- [x] [Review][Patch] Même identité, chemin device changé → Stop/Start cette unité seulement [`src/App/MidiSessionMultiHostHotPlug.cpp`]
- [x] [Review][Patch] Verrou inter-processus sur le fichier registre `K` [`src/Device/UnitIdentityRegistryIo.cpp`]
- [x] [Review][Patch] Échec `persistRegistry` après hot-plug Start : retry ou Stop de la nouvelle unité [`src/App/MidiSessionMultiHostHotPlug.cpp`]
- [x] [Review][Patch] Start soft échoué : ne pas laisser un `K` nouvellement bound orphelin [`src/App/MidiSessionMultiHost.cpp`]
- [x] [Review][Patch] Recreate VirtualMIDI : backoff si alias encore tenu par DAW/MIDI-OX [`src/Midi/VirtualMidiBackend.cpp`]
- [x] [Review][Patch] Aligner le smoke hot-plug sur les messages multi-unité réels [`docs/tests/smoke-epic3-hotplug-mt4.md`]
- [x] [Review][Patch] Cold start : si une unité échoue, garder les unités déjà démarrées (A1) [`src/App/MidiSessionMultiHost.cpp`]
- [x] [Review][Patch] Registre K illisible : fail closed, ne pas démarrer avec carte vide (B2) [`src/App/MidiSessionMultiHost.cpp`]

#### Defer

- [x] [Review][Defer] CTRL_CLOSE / kill → ports VirtualMIDI orphelins ; Auto-Start augmente l’exposition [`src/App/MidiSessionCli.cpp:28`] — deferred, pre-existing
- [x] [Review][Defer] Surprise-removal `Stop` encore WriteBulk finish-magic sur bus mort [`src/Device/DeviceSession.cpp:443`] — deferred, pre-existing
- [x] [Review][Defer] Identité topology-only : nouveau `K` si déplacement hub/port sans serial [`src/Usb/WinUsbEnumerate.cpp`] — deferred, known fallback
- [x] [Review][Defer] Dual physique MT4 non prouvé en lab (registre/noms offline OK) — deferred, lab honesty gate
