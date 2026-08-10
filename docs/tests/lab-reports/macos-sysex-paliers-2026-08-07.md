# Rapport lab macOS — paliers SysEx MT4 (2026-08-07)

**Machine :** MacBook Pro M5, macOS Tahoe  
**Interface :** Emagic MT4, **pilote Apple** (pas de Bridge, pas de WinUSB, pas de virtualMIDI)  
**But :** établir une référence hardware « clean » avant de continuer l’investigation du Bridge Windows (trou récurrent sur le premier gros dump après Start).

Ce rapport est la **mémoire opérationnelle** pour la reprise sur PC. Les journaux bruts restent sous `tests/lab-logs/`.

---

## Synthèse

| Palier | Quoi | Verdict macOS | Preuve SSOT |
|---|---|---|---|
| **1** | Patch 275 o / master 351 o (push + dump) | **100 % clean** | `tests/lab-logs/sysex-matrix-mid-macos/sysex-matrix-mid-20260807T165511Z.log` |
| **2** | Rafale 100× dump patch (banque) | **100 % clean** | `tests/lab-logs/sysex-matrix-bank-macos/sysex-matrix-bank-20260807T171139Z.log` |
| **3** | Gros SysEx en boucle DIN (1 Ko / 4 Ko / 14,7 Ko) | **100 % clean** | `tests/lab-logs/sysex-long-loopback-macos/sysex-long-loopback-20260807T174438Z.log` |

**Conclusion pour le PC :** la MT4 + le Matrix (paliers 1–2) + la tuyauterie USB/DIN (palier 3) sont fiables à 100 % sous le pilote Apple. Les pertes / TIMEOUT côté Windows Bridge ne s’expliquent **pas** par un défaut matériel Matrix/câble/MT4 observé sur ce Mac.

---

## Setup commun (Mac)

- Ports Apple utilisés : **OUT = `MT4 Port 1`**, **IN = `MT4 Port 1`**
- Paliers 1–2 : Matrix-1000 alimenté, DIN **Out 1 ↔ In 1**
- Palier 3 : Matrix **débranché**, câble DIN **Out 1 → In 1** (boucle pure)
- Fermer DAW / éditeurs qui monopolisent les ports pendant le lab
- Dépendances : `python3 -m pip install -r scripts/lab/requirements-device-inquiry.txt`
- Jamais `--with-bridge` sur macOS

Prompts d’origine (archivés) :

- `docs/tests/lab-prompts/lab-palier-1-sysex-matrix-mid.md`
- `docs/tests/lab-prompts/lab-palier-2-sysex-matrix-bank.md`
- `docs/tests/lab-prompts/lab-palier-3-sysex-long-loopback.md`

---

## Palier 1 — SysEx « taille éditeur » (Matrix)

**Intention :** même stress que le lab Windows mid-size (patch / master, les deux sens).

**Harness :** `scripts/lab/sysex-matrix-mid-loop.py`  
**Checklist :** `docs/tests/checklists/smoke-mt4-sysex-matrix-mid.md` (+ README macOS sous `tests/lab-logs/sysex-matrix-mid-macos/`)  
**Spec liée :** `_bmad-output/implementation-artifacts/spec-sysex-matrix-mid-macos.md`

| Scénario | Session 1 | Session 2 (process frais) | 1er essai après froid |
|---|---|---|---|
| dump_patch (275 o) | 10/10 | 10/10 | Pass ~128–130 ms |
| dump_master (351 o) | 10/10 | 10/10 | Pass ~147–151 ms |
| push_patch | 10/10 | — | — |
| push_master | 10/10 | — | — |

`overall_pass=true`, exit 0.  
**Point de comparaison Windows :** le trou Bridge « premier dump après Start » **ne se reproduit pas** sur Apple.

---

## Palier 2 — Rafale banque (100× dump patch)

**Intention :** stress device→host à l’échelle ~banque (~100 frames × 275 o), gate = 100 dumps patch séquentiels (pas de Dump All Oberheim dans ce lot).

**Harness :** `scripts/lab/sysex-matrix-bank-loop.py`  
**Checklist :** `docs/tests/checklists/smoke-mt4-sysex-matrix-bank.md`  
**Spec :** `_bmad-output/implementation-artifacts/spec-sysex-matrix-bank-burst.md`

| Session | sent/ok | 1er dump | Durée approx. |
|---|---|---|---|
| 1 | 100/100 | Pass ~129 ms | ~13,6 s |
| 2 (process frais) | 100/100 | Pass ~130 ms | ~13,6 s |

Pacing ≥10 ms, slots 0…99, timeout par frame ~3 s.  
`overall_pass=true`.

---

## Palier 3 — Gros SysEx, boucle DIN (sans Matrix)

**Intention :** prouver que la MT4 transporte des SysEx **beaucoup plus longs** que patch/master, avec comparaison **octet à octet** (option A : pas de répondeur Python).

**Harness :** `scripts/lab/sysex-long-loopback.py`  
**Fixture :** `tests/fixtures/sysex/long-loopback-14708.syx` (~14 708 o)  
**Checklist :** `docs/tests/checklists/smoke-mt4-sysex-long-loopback.md`  
**Spec :** `_bmad-output/implementation-artifacts/spec-sysex-long-loopback.md`

| Payload | Session 1 | Session 2 | Round-trip approx. |
|---|---|---|---|
| synth 1024 o | 20/20 | 20/20 | ~0,35 s |
| synth 4096 o | 20/20 | 20/20 | ~1,3 s |
| fixture 14708 o | 20/20 | 20/20 | ~4,7 s |

Preuves :

- SSOT confirmée (câble boucle + Matrix débranché) : `…174438Z.log` → `overall_pass=true`
- Run propre antérieur : `…173924Z.log` → `overall_pass=true`
- Run prématuré `…173328Z.log` : échec partiel sur les premiers `synth_1024` (`last=none`) tant que la boucle n’était pas live — **ne pas** utiliser comme preuve d’échec hardware

---

## Implications pour le Bridge Windows

1. **Ne pas accuser** Matrix / câble DIN / MT4 hardware tant que le même scénario n’a pas été rejoué sous Bridge avec instrumentation.
2. Rejouer sur PC, dans l’ordre :
   - Palier 1 avec `--with-bridge` (déjà partiellement connu : trou premier dump).
   - Palier 2 avec `--with-bridge` (harness prêt).
   - Palier 3 avec `--with-bridge` + **même boucle DIN** Out1→In1 (Matrix hors jeu) — stress gros SysEx pur Bridge.
3. Sur le Bridge, garder la barre **100 %**, ≥2 Starts frais, pas de warm-up Inquiry opaque.
4. Plafond côté Bridge à garder en tête pour le palier 3 Windows : hold SysEx framer `kMaxSysexHoldBytes = 1024` sur le chemin device→host assemblé — le lab macOS a volontairement dépassé 1 Ko via le pilote Apple ; sous Bridge, distinguer « transport OK sous plafond » vs « rejet propre oversize » si on pousse au-delà.

---

## Commandes de rejeu macOS (référence)

```text
# Palier 1
python3 scripts/lab/sysex-matrix-mid-loop.py \
  --out-port "MT4 Port 1" --in-port "MT4 Port 1" \
  --pass-percent 100 --fresh-sessions 2 \
  --log-dir tests/lab-logs/sysex-matrix-mid-macos

# Palier 2
python3 scripts/lab/sysex-matrix-bank-loop.py \
  --out-port "MT4 Port 1" --in-port "MT4 Port 1" \
  --count 100 --interval 0.01 --pass-percent 100 --fresh-sessions 2 \
  --log-dir tests/lab-logs/sysex-matrix-bank-macos

# Palier 3 (boucle DIN, sans Matrix)
python3 scripts/lab/sysex-long-loopback.py \
  --out-port "MT4 Port 1" --in-port "MT4 Port 1" \
  --count 20 --interval 0.05 --reply-timeout 8 \
  --sizes 1024,4096 --pass-percent 100 --fresh-sessions 2 \
  --log-dir tests/lab-logs/sysex-long-loopback-macos
```

---

## Commits de référence (main au moment du lab)

- `5913443` — lab mid-size macOS (palier 1)
- `d6c8341` — lab bank-burst macOS (palier 2)
- Palier 3 (harness + logs + ce rapport) : à committer avec le reste du working tree si pas encore poussé
