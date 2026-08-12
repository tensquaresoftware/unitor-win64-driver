# Scripts quality cleanup — checklist (un fichier par conversation)

Snapshot findings : `python scripts/quality/lint-touched.py --all --report-worst 200`  
Référence seuils : `conventions.md` §3.4  
Outil : `scripts/quality/lint-touched.py`  
Date snapshot : **2026-08-12**

## Comment utiliser

1. Prendre la **prochaine** case non cochée (ordre = priorité / score lizard).
2. Ouvrir un **chat Cursor frais**.
3. Copier le **prompt réutilisable** ci-dessous ; remplacer `CHEMIN_CIBLE` par le chemin du fichier choisi.
4. À la fin de la conversation : cocher la case ici + noter le commit (si tu commits) / findings restants éventuels.
5. Une conversation = **un** fichier primaire de la liste. Extraire un module partagé lab (`scripts/lab/…`) uniquement si c’est nécessaire pour faire passer **ce** fichier (WET→DRY), sans refactorer toute la liste d’un coup.

Hors liste (déjà verts sous `--all` au snapshot) :  
`scripts/dev/resolve-bmad-chat-title.py`, `scripts/quality/lint-touched.py`.

---

## Prompt réutilisable (copier-coller)

```text
/bmad-quick-dev

## Objectif

Assainir **un seul** script Python pour qu’il passe la porte qualité scripts (conventions.md §3.4), sans big-bang sur tout le dossier lab.

## Fichier cible (remplacer)

CHEMIN_CIBLE = XXX

## Contexte (ne pas rediscuter)

- Projet : unitor-win64-driver.
- Porte unique C++ + scripts : `python scripts/quality/lint-touched.py`.
- Seuils scripts (§3.4) : fonction ~70 nloc (core) / ~90 (lab|packaging) ; ≤4 params ; CCN ≤12 (glue ≤14) ; nesting ≤5 ; fichier utile ~700 lignes.
- Mode touched : exit 1 sur dérive du diff. Mode `--all` : diagnostic dette, exit 0.
- Checklist d’avancement : `docs/dev/scripts-quality-cleanup-checklist.md`.
- Comportement lab (CLI, topo MIDI, Pass/Fail, logs) : **ne pas casser** — refactor structurel seulement.
- Langue code / logs script / messages : anglais. Chat : français clair.
- Pas de commit sans demande explicite de Guillaume.

## Livrable de cette conversation

0. Renommer la conversation `Qualité — XXX`(ex: scripts/lab/sysex-long-loopback.py donne "Qualité — SysEx Long Loopback")
1. Refactorer **uniquement** `CHEMIN_CIBLE` (et, si indispensable pour ce fichier, un petit module partagé neuf ou déjà extrait sous `scripts/lab/` / `scripts/packaging/` — Boy Scout limité).
2. Faire disparaître **tous** les findings lizard `[scripts:…]` qui concernent ce fichier sous :
   `python scripts/quality/lint-touched.py --all`
   (filtre mental / grep sur le chemin cible ; les autres fichiers sales peuvent rester rouges).
3. Sur le diff de ce ticket : `python scripts/quality/lint-touched.py` → **exit 0**.
4. Spec Quick Dev EN courte sous `_bmad-output/implementation-artifacts/` (intent, ACs, Design Notes : avant/après findings pour ce fichier).
5. Mettre à jour la checklist : cocher la case du fichier + une ligne de note (commit hash si fourni plus tard, ou « pending commit »).

## Méthode

1. Lire `conventions.md` §3.4 + le fichier cible.
2. Lancer `--all`, lister les findings **de ce chemin seulement**.
3. Extraire helpers / options struct / réduire nesting / découper `run_lab` / `build_parser` / `main` — KISS, noms intentionnels.
4. Si duplication Bridge/MIDI déjà présente ≥3× et bloquante pour ce fichier : extraire un module partagé minimal (une fois), sans migrer tous les callers hors besoin.
5. Revérifier `--all` (plus de findings sur la cible) + mode touched vert.
6. Ne pas lancer de lab hardware sauf si le refactor change la CLI publique de façon risquée — alors smoke doc / `--help` minimum.

## Hors scope

- Traiter un deuxième fichier de la checklist dans la même conversation.
- Changer les seuils §3.4 ou la logique de `lint-touched.py` (sauf bug évident découvert en chemin — alors HALT et demander).
- Nouveau scénario SysEx (Request All `04H` type 0), overnight, épic 6, C++ Bridge.
- Reformater tout `scripts/` « pour faire joli ».

## Critères de fin

- [ ] Findings `[scripts:…]` pour `CHEMIN_CIBLE` = 0 sous `--all`
- [ ] `python scripts/quality/lint-touched.py` vert sur le diff
- [ ] Comportement / CLI du script préservés (flags documentés inchangés sauf renommage justifié + note)
- [ ] Spec EN + case cochée dans `docs/dev/scripts-quality-cleanup-checklist.md`
```

---

## Liste à traiter (ordre recommandé)

Coche `[x]` quand le fichier est **vert** sous `--all` pour ce chemin.  
Colonnes *findings* / *score* = snapshot 2026-08-12 (indicatif ; re-mesurer après chaque passe).

### Vague A — plus lourds (file-size et/ou complexité haute)

- [x] `scripts/lab/sysex-long-loopback.py`
  Snapshot : ~8 findings · score ~1253 · useful ~1087 · `run_lab` / `build_parser` / nesting / params
  Notes : `7502f89` — cleared under `--all` via extract `lab_midi_common.py` + `sysex_long_loopback_lib.py` (structural only; CLI preserved)

- [x] `scripts/packaging/verify-installer-contract.py`  
  Snapshot : ~2 findings · score ~1183 · `main` nloc+ccn extrêmes  
  Notes : `5fb8464` — cleared under `--all` via domain `check_*` helpers; Authenticode policy needles synced to hobby docs (`no certificate purchase` / `Not a hard packaging gate`); bare CLI / OK|FAIL exits preserved

- [x] `scripts/lab/sysex-matrix-mid-loop.py`  
  Snapshot : ~8 findings · score ~1035 · useful ~944 · `run_lab` / parser / params scénarios  
  Notes : `9e2a874` — cleared under `--all` via `lab_midi_common` reuse + `sysex_matrix_mid_loop_lib.py` (local `BRIDGE_FAIL_NEEDLES` subset kept; CLI preserved)

- [x] `scripts/lab/sysex-matrix-bank-loop.py`  
  Snapshot : ~6 findings · score ~1031 · useful ~767 · `run_lab` / parser / nesting / params  
  Notes : `1ef9e49` — cleared under `--all` via `lab_midi_common` reuse + `sysex_matrix_bank_loop_lib.py` (local `BRIDGE_FAIL_NEEDLES` subset kept; CLI preserved)

- [x] `scripts/lab/overnight-combined-stress.py`  
  Snapshot : ~3 findings · score ~1022 · `run_overnight` nloc+ccn · `_run_child` params  
  Notes : `4dbf817` — cleared under `--all` via ChildRun/OvernightContext split (CLI preserved; no companion)

### Vague B — overnight / realtime labs

- [x] `scripts/lab/overnight-macos-sysex-stress.py`  
  Snapshot : ~3 findings · score ~249 · `run_overnight` · `_run_child` params  
  Notes : `d621d85` — cleared under `--all` via ChildRun/OvernightContext split (CLI preserved; no companion)

- [x] `scripts/lab/midi-clock-loopback-lab.py`  
  Snapshot : ~4 findings · score ~119 · `main` ccn · `_run_lab` nloc/params · `_send_status` params  
  Notes : pending commit — cleared under `--all` via LabRun/StatusSend/ClockBatch + phase split (CLI preserved; no companion; local fail needles kept)

- [ ] `scripts/lab/mtc-loopback-lab.py`  
  Snapshot : ~2 findings · score ~119 · `main` ccn · `_run_lab` params  
  Notes :

- [ ] `scripts/lab/overnight-matrix-stress.py`  
  Snapshot : ~3 findings · score ~118 · `run_overnight` · `_run_child` params  
  Notes :

- [ ] `scripts/lab/midi-concurrent-in-stress.py`  
  Snapshot : ~2 findings · score ~108 · nesting `_run_stress`  
  Notes :

- [ ] `scripts/lab/overnight-long-loopback-stress.py`  
  Snapshot : ~1 finding · score ~107 · `_run_child` params  
  Notes :

- [ ] `scripts/lab/device-inquiry-loop.py`  
  Snapshot : ~2 findings · score ~16 · nesting / params `_run_inquiry_loop`  
  Notes :

### Déjà OK au snapshot (ne pas ouvrir de conversation « cleanup » sauf régression)

- [x] `scripts/dev/resolve-bmad-chat-title.py`
- [x] `scripts/quality/lint-touched.py`

---

## Vérification globale (quand la liste A+B est toute cochée)

```bat
python scripts/quality/lint-touched.py --all --report-worst 50
```

Attendu : **plus aucun** finding `[scripts:…]` (ou seulement des fichiers hors checklist ajoutés plus tard — les traiter alors en fin de liste).

Critère de fin de chantier dette : `--all` ne signale plus de scripts sales sous `scripts/`.
