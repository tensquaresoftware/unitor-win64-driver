/bmad-quick-dev

## Contexte de la Story

Après le palier 1 Windows à 100 %, on veut que le Bridge tienne aussi une **rafale** de dumps patch Matrix (~100× 275 o) sans perte, comme le pilote Apple sur la même MT4.

## Objectif (unique)

Stress SysEx **palier 2** sur **Bridge Windows** MT4 ↔ Matrix-1000 : valider à **100 %** une rafale **device→host** de taille banque (100× dump patch), après succès du palier 1.

## Prérequis (bloquant)

- Palier 1 Windows **clos** : mid-size patch/master push+dump à 100 % après Start frais (commit `c535069` ou plus récent sur `main`).
- Harness déjà livré : `scripts/lab/sysex-matrix-bank-loop.py` (ne pas réécrire from scratch).
- Checklist : `docs/tests/checklists/smoke-mt4-sysex-matrix-bank.md`
- Spec Windows Bridge (gate actif) : `_bmad-output/implementation-artifacts/spec-sysex-matrix-bank-burst-2.md`
  (sibling macOS done : `spec-sysex-matrix-bank-burst.md`).
- Ne pas commencer si le palier 1 n’est pas vert sur cette machine.

## Preuve hardware déjà tranchée (NE PAS rouvrir)

Rapport SSOT : `docs/tests/lab-reports/macos-sysex-paliers-2026-08-07.md`

Sous macOS + **pilote Apple** (pas Bridge) : palier 2 = **100 %** (2×100 dumps patch, stamp `20260807T171139Z`).  
Donc : ne pas accuser Matrix, câble DIN, ni MT4 si Windows échoue. Le défaut, s’il y en a un, est dans la **pile Bridge (WinUSB + session + virtualMIDI)**.

## Forme du gate (déjà tranchée)

Gate principal = **mode 2** (rafale de dumps patch), pas un Dump All Oberheim ambigu :

- Request : `F0 10 06 04 01 <n> F7` avec balayage de slots (défaut 0…99)
- Pass par frame : exactement **275 o**, préfixe `F0 10 06 01`, fin `F7`
- Pacing ≥10 ms (`--interval 0.01` par défaut)
- Timeout **par frame** ~3 s (pas un seul timeout pour toute la rafale)
- ≥2 Starts frais Bridge ; `--pass-percent 100` ; zéro warm-up Inquiry opaque

Dump All réel = Ask First / follow-up seulement (hors gate).

## Commande cible Windows

```text
python scripts/lab/sysex-matrix-bank-loop.py --with-bridge --pass-percent 100
```

Defaults attendus : `MT4 Output 1` / `MT4 Input 1`, `--fresh-starts 2`, `--count 100`.  
Logs : `tests/lab-logs/sysex-matrix-bank/` (+ `bridge-<stamp>-startN.log`).

## Setup

- Matrix-1000 **alimenté avant** le lancement (confirmer à l’opérateur)
- DIN Out1↔In1 ; fermer MIDI-OX / Matrix-Control / DAW sur ports MT4
- Zadig + `builds/debug/Debug/Bridge.exe --start-session --dev-zadig` (via `--with-bridge`)
- Rebuild Bridge Debug si tu touches le C++

## Contraintes Bridge (hérité palier 1 — respecter)

- **Ne jamais** `SendToHost` depuis le thread de completion WinUSB : completion = harvest → reorder → resubmit → enqueue ; reader = demux / framer → `SendToHost` → drain OUT.
- Les gardes palier 1 (calme post-Start, silence OUT pendant dump expect, réparation `F0` manquant, reject+1 retry taille Matrix) sont déjà sur `main` — **ne pas les retirer** pour « simplifier » ; étendre seulement si le lab banque prouve un trou nouveau.
- Pas de ralentissement artificiel au-delà du pacing Matrix stock (≥10 ms).

## Méthode

1. Clarifier en une phrase le but Windows (harness déjà là) ; mettre à jour spec / checklist si besoin (section Windows Bridge = now, plus « later »).
2. Rebuild Bridge si nécessaire ; lancer **toi-même** le lab dès que Matrix est chaud.
3. Sur KO : index / slot / len / head / tail / `dt_ms` + compteurs Bridge (`demux_spans`, `send_ok`, hold SysEx) — LEDs DIN actives ⇒ perte Bridge.
4. Une piste → rebuild → lab ≥2 Starts → conclusion. Pas cinq rustines empilées.
5. Si C++ : s’appuyer sur le driver Linux Emagic quand c’est pertinent ; `python scripts/quality/lint-touched.py` clean.
6. Documenter stamps dans Design Notes / checklist ; barre **100 %** ou Fail honnête.

## Hors scope

- Palier 3 (SysEx ultra-long / boucle DIN sans Matrix)
- Matrix-Control UI comme gate
- Windows MIDI Services; MidiView/ShowMIDI (retired; use MIDI-OX); AMT8 / Unitor8
- Heartbeat Matrix-Control comme fix
- Baisser la barre 100 % ou « Pass avec warm-up »

## Livrable de fin

- Lab Windows `overall_pass=true` exit 0 sur ≥2 Starts (ou Fail instrumenté + patch minimal + re-lab)
- Spec / checklist à jour pour le gate Bridge
- Commit seulement sur demande explicite de Guillaume
