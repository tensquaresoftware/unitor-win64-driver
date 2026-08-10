# Passation BMad — unitor-win64-driver (2026-08-04)

Note opérationnelle pour reprendre sur **MacBook Pro** après la session Windows.  
Chat Cursor de référence : **BMad — PRD : Pilote MT4**.

## État Git (poussé sur `origin/main`)

Branche : `main` — à jour avec le remote après 3 commits :

| Commit | Contenu |
| --- | --- |
| `88fefb8` | Brief produit finalisé |
| `495c538` | PRD draft |
| `0818357` | Mentions Unitor8 mk2 dans les prompts d’étude |

Remote : https://github.com/tensquaresoftware/unitor-win64-driver.git

Sur le Mac : `git pull` avant de continuer.

## Ce qui est fait

### Project Brief — **terminé** (`status: ready`)

- `_bmad-output/planning-artifacts/briefs/brief-unitor-win64-driver-2026-08-04/brief.md`
- `addendum.md` + `.memlog.md` associés

Décisions produit **verrouillées** (ne pas rouvrir) : Win10/11 x64, usermode WinUSB + C++, virtualMIDI V1, MIT, MT4 validé (`086A:0003`), SysEx + clock obligatoires, multi-DeviceProfile / multi-MT4, façade Ten Square Software, Authenticode fortement recommandé mais pas hard gate, etc.

### PRD — **draft rédigé, pas finalisé** (`status: draft`)

- `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md`
- `addendum.md` + `.memlog.md` associés

Contenu déjà dans le draft :

- Vision, utilisateurs, parcours (UJ), glossaire
- 16 FR testables + NFR (latence provisoire, ~4 h, hot-plug, docs, licences…)
- Matrice de validation V1 verrouillée en session :
  - **Ableton Live 12**
  - **Reason Studios 12** (à confirmer le libellé SKU exact si besoin)
  - **Matrix-Control** (SysEx first-party, pas une dépendance runtime)
  - **MIDI-OX** (utilitaire multi-client V1 ; ShowMIDI / MidiView retirés)
  - **Win10 x64 obligatoire** + **Win11 x64**
- Timing : ancres provisoires + **Studio-Done Gate** (mesurer le chemin MIDI avant de dire « done studio »)
- Risques, open questions, traçabilité vers le brief
- Addendum : tables câbles, licences virtualMIDI, alternatives rejetées

Mode de travail utilisé : **Fast path**, enjeux **lancement OSS public**.

### Workflow machine (rappel)

- **Mac** : édition Cursor, docs, code C++, process BMad
- **PC Windows** : builds, WinUSB, virtualMIDI, MT4, DAW, SysEx, mesures timing

## Ce qu’il faut terminer avant l’étape BMad suivante

L’étape suivante naturelle est **Architecture** (`bmad-architecture`), **après** finalisation du PRD.

### 1. Finaliser le PRD (obligatoire avant Architecture)

Reprendre le skill **`bmad-prd`** en intent **Update** / enchaîner **Finalize** sur le workspace existant :

`_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/`

Séquence Finalize BMad attendue :

1. Audit memlog avec l’utilisateur
2. Réconciliation inputs (brief / addendum)
3. Reviewer gate (rubrique qualité PRD — all / subset / skip selon envie)
4. Triage open questions (bloqueurs vs reportés)
5. Polish structure + prose (`doc_standards`)
6. Passer `status: final` + `updated` + event memlog « PRD finalized »

Prompt de reprise possible sur le Mac :

> Reprends `bmad-prd` en Update/Finalize sur `prd-unitor-win64-driver-2026-08-04`. Ne rouvre pas les décisions verrouillées du brief. Objectif : passer le PRD en `status: final` prêt pour Architecture.

### 2. Trancher ou reporter explicitement (open questions du PRD)

| # | Sujet | Action avant Architecture |
| --- | --- | --- |
| 1 | Conditions virtualMIDI (éval vs redistribution / MSI) + contact Tobias | **Gate release installer public** — peut rester open dans le PRD final, mais noter le owner / prochaine action |
| 2 | Seuils latence/gigue définitifs + harness MIDI Path | **OK de rester provisoire** avec Studio-Done Gate (déjà dans le draft) — Architecture doit préciser le harness |
| 3 | Authenticode (certificat perso vs org, timing, coût) | Reporter avec owner ; pas hard gate V1 |
| 4 | Doc protocole Emagic vs fallback Linux + captures USB | Reporter ; Orientation Architecture |
| 5 | CI/CD Mac edit / Win10 validate | Au minimum : build Windows en CI ; détail Architecture |
| 6 | Orthographe exacte des noms si 2× MT4 | **Architecture / UX** — règle produit déjà posée |
| 7 | Multi-client virtualMIDI + MIDI-OX / DAW | **Architecture** à confirmer |
| 8 | Vecteurs SysEx Matrix-Control « pass » | À préciser (Guillaume) quand possible ; sinon open avec owner |

Rien de tout ça ne doit **rouvrir** : usermode, virtualMIDI V1, MIT, MT4-only validé, SysEx V1, Win10 obligatoire.

### 3. Petites vérifications produit optionnelles au Finalize

- Libellé exact « Reason Studios 12 » vs « Reason 12 » dans les docs utilisateur futurs
- Relire le draft PRD une fois sur le Mac (clarté FR du chat ≠ artifact EN — l’artifact reste en anglais)

## Après PRD `final` — prochaine étape BMad

**Recommandé :** `bmad-architecture` (spine d’invariants : WinUSB, DeviceProfile, mapper Emagic, backend MIDI abstrait, multi-instance, installer, harness de mesure).

Puis typiquement : epics/stories (`bmad-create-epics-and-stories`) → sprint → `bmad-dev-story`.

`bmad-ux` seulement si tu veux une spec dédiée aux libellés de ports / parcours install ; sinon les règles produit suffisent souvent dans Architecture.

En cas de doute de routage : `bmad-help`.

## Fichiers clés à ouvrir en premier sur le Mac

1. Ce fichier (passation)
2. Brief : `_bmad-output/planning-artifacts/briefs/brief-unitor-win64-driver-2026-08-04/brief.md`
3. PRD draft : `_bmad-output/planning-artifacts/prds/prd-unitor-win64-driver-2026-08-04/prd.md`
4. Conventions : `conventions.md`
5. Kickoff historique : `docs/dev/prompt-demarrage-projet-bmad.md` / `docs/dev/prompt-etude-bmad.md`

## Hors scope de cette passation

- Pas de code C++ encore
- Pas d’Architecture formalisée
- Pas de contact Tobias encore documenté comme fait
- PRD encore `draft` — **ne pas lancer Architecture comme si le PRD était clos**
