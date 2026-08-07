# Lab prompt — palier 3 long SysEx DIN loopback (Windows Bridge, Mac parity)

## Contexte

Après paliers 1 et 2 Windows verts, prouver que le Bridge transporte aussi de **gros SysEx** (1024 / 4096 / fixture ~14 708 o) sans corruption, en boucle DIN, **sans** Matrix.

## Objectif

Stress SysEx **palier 3** sur **Bridge Windows** : intégrité octet à octet via **boucle DIN Out1→In1** (option A), à **100 %**, avec les plafonds Bridge relevés à **16384** (hold + encode).

**Spec:** `_bmad-output/implementation-artifacts/spec-sysex-long-loopback-2.md`  
**Contrôle macOS (déjà done):** `_bmad-output/implementation-artifacts/spec-sysex-long-loopback.md`  
**Rapport hardware SSOT:** `docs/tests/lab-reports/macos-sysex-paliers-2026-08-07.md`

## Prérequis (bloquant)

- Palier 1 **et** palier 2 Windows à **100 %** sur cette machine.
- Harness : `scripts/lab/sysex-long-loopback.py`
- Fixture : `tests/fixtures/sysex/long-loopback-14708.syx` (~14 708 o) — même fichier que macOS.
- Checklist : `docs/tests/checklists/smoke-mt4-sysex-long-loopback.md`
- Câble DIN Out1 → In1 en place ; Matrix hors de ces jacks.
- Fermer DAW / MIDI-OX / Matrix-Control sur ports MT4.
- Zadig + Bridge Debug (via `--with-bridge`).

## Plafonds Bridge (décision déjà prise)

Guillaume a choisi le stress type macOS (y compris la fixture 14 ko). Les deux plafonds sont à **16384** :

- `kMaxSysexHoldBytes` — `src/Protocol/MidiMessageFramer.h` (device→host)
- `kEncodeBufferCapacity` — `src/Device/DeviceSessionHostOutbound.cpp` (host→device)

Oversize **au-dessus** de 16384 : rejet anglais observable (pas Pass partiel silencieux).

Ne pas accuser le câble DIN ni la MT4 si Windows échoue sous le même topo que le contrôle Apple.

## Commande Windows (gate Pass)

```text
python scripts/lab/sysex-long-loopback.py --with-bridge --pass-percent 100
```

Defaults : `MT4 Out 1` / `MT4 In 1`, `--sizes 1024,4096`, fixture on, `--fresh-starts 2`,
`--count 20`, `--interval 0.05`, `--reply-timeout 8`.  
Logs : `tests/lab-logs/sysex-long-loopback/` (+ `bridge-<UTC>-startN.log`).

## Setup

- Confirmer la boucle DIN **avant** Start (un `TIMEOUT last=none` en tête = souvent boucle pas encore live).
- Matrix hors de ces jacks.
- Rebuild Debug Bridge après changement des plafonds.

## Contraintes Bridge

- Pas de `SendToHost` depuis le thread completion WinUSB.
- Anneau USB always-pending + resubmit immédiat.
- Pass = trame reçue **identique** à l’envoyée (réassemblage jusqu’à `F7`).

## Méthode

1. Rebuild + framer unit / FramerSysex smoke.
2. Lab toi-même ; ≥2 Starts frais ; pas de warm-up opaque.
3. Sur KO ≤14708 : instrumenter (len, mismatch offset, compteurs Bridge) — défaut Bridge.
4. Fix C++ minimal si trou prouvé ; `lint-touched.py` clean.
5. Une piste → rebuild → lab → conclusion.

## Hors scope

- Option B répondeur Python sans Ask First
- Matrix-Control / formes Matrix comme gate
- Windows MIDI Services / MidiView / AMT8 / Unitor8
- Baisser la barre 100 %

## Livrable

- `overall_pass=true` exit 0 documenté (stamp sous `tests/lab-logs/sysex-long-loopback/`)
- Spec / checklist à jour
- Commit seulement sur demande explicite de Guillaume
