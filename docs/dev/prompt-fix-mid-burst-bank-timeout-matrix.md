# Prompt — Fix mid-burst bank TIMEOUT Matrix (post cold-start F0 repair)

Copier-coller tel quel dans un **nouveau chat Cursor** (contexte frais).  
Skill conseillé : **bmad-quick-dev**.  
Projet : `C:\Users\Guillaume\Dev\Projects\unitor-win64-driver`

---

## Contexte de la Story

Le premier dump Matrix après Start Bridge est revenu : mid 5×10 passe à 100 %. La gate bank 20×100 reste rouge à cause de **trous rares en milieu de rafale** (`TIMEOUT last=none`), pas du cold-start.

---

## Qui tu es / comment parler

- Agent BMad sur **unitor-win64-driver** (Windows 10/11, Bridge WinUSB + virtualMIDI).
- Français clair, intention produit d’abord.
- Noms gardés : **MT4**, **Matrix**, **Matrix-Control**, **WinUSB**, **virtualMIDI**, **SysEx**, **Bridge**.
- Pas de commit sans demande explicite.
- Objectif de cette passe : **éliminer (ou fortement réduire) les `TIMEOUT last=none` mid-burst** pour que la gate bank jour passe à **100 % par Start** — pas de redesign large, pas de palier 3 longs, pas d’overnight 8 h comme objectif.

Appliquer aussi : barème de clarté BMad, `conventions.md` §3, `scripts/quality/lint-touched.py` si tu modifies du C++.

---

## État du dépôt (IMPORTANT — lire avant de coder)

### Correctif cold-start déjà en working tree (à conserver)

Le trou **premier dump post-Start** a été corrigé dans cette session (pas encore commité au moment où ce prompt a été rédigé) :

| Fichier | Changement |
|---|---|
| `src/Device/DeviceSessionSupport.cpp` | `maybePrependLostLeadingF0` accepte un span **1 octet `0x10`** sous expect (plus seulement `10 06` same-span) |
| `src/Device/DeviceSessionSupport.h` | commentaire aligné |
| `tests/unit/DeviceInquiryHelpersTests.cpp` | tests `[f0-repair]` |
| `_bmad-output/implementation-artifacts/spec-cold-start-premier-dump-matrix.md` | spec cold-start (in-progress) |
| `_bmad-output/implementation-artifacts/deferred-work.md` | résidu mid-burst noté |

**Avant toute autre chose :** `git status` — si ces fichiers sont encore modifiés/non suivis, **les garder**. Ne pas les revert. Si Guillaume a déjà commité entre-temps, partir de ce commit.

### Correctifs déjà sur `main` (ne pas les défaire)

| Commit | Rôle |
|---|---|
| `835c992` | expect avant flush, retry différé, abandon→retry, 2ᵉ short sans tuer pompe, pas de drain Write/SendToHost depuis callback virtualMIDI, expiry expect, plafond différé, repair F0 (ensuite trop strict → réparé en WIP ci-dessus) |
| `5d77070` | findings revue + deferred |
| `12003de` | evidence day-gate + prompt cold-start |

**Interdit :** masquer le symptôme en assouplissant le lab (ignorer un dump, baisser `pass-percent`, discard mid-burst). Le fix vise le Bridge.

---

## Preuve lab (2026-08-08, câblage In correct — pas Thru)

### Baseline pré-fix cold-start (pour contraste)

- Bank `post-epic2-cr` : 10/20 Starts 100 % ; beaucoup de TIMEOUT index **0001** + mid-burst.
- Mid `post-epic2-cr` : **5/5** Starts perdent uniquement le 1er `dump_patch`.

### Après repair F0 1-byte (cette session)

#### Lab B — mid (`5×10`, gate 100 %) — **VERT**

- **5/5** Starts à 100 % (dump #1 inclus).
- Log dir : `tests/lab-logs/sysex-matrix-mid/post-epic2-cr-coldfix/`

#### Lab A — bank (`20×100`, gate 100 %) — **ROUGE (mid-burst only)**

**Run 1** — `tests/lab-logs/sysex-matrix-bank/post-epic2-cr-coldfix/`

- **19/20** Starts à 100 % ; **1/20** à 99 %.
- Unique miss : index **0009** slot **`08`** — `TIMEOUT last=none` (pas le premier dump ; dump **0001 RECV** OK sur ce Start).
- Log : `sysex-matrix-bank-20260808T162947Z.log`

**Run 2 (relance)** — `tests/lab-logs/sysex-matrix-bank/post-epic2-cr-coldfix-rerun/`

- **18/20** Starts à 100 % ; **2/20** à 99 %.
- Misses mid-burst : index **0080** slot **`4F`** ; index **0019** slot **`12`** — tous `TIMEOUT last=none`.
- Cold-start toujours OK (pas de pattern index 0001).

Fingerprint commun : Bridge vivant, `send_fail` ≈ 0, trou rare en milieu de rafale, même famille que l’overnight / day-gate résiduel.

### Spec / deferred

- Cold-start : `_bmad-output/implementation-artifacts/spec-cold-start-premier-dump-matrix.md`
- Day-gate evidence : `_bmad-output/implementation-artifacts/spec-post-epic2-cr-bank-mid-day-gate.md` (`status: done`)
- Deferred mid-burst : `_bmad-output/implementation-artifacts/deferred-work.md` (section `spec-cold-start-premier-dump-matrix.md` 2026-08-08)

---

## Hypothèses à tester (ordre)

1. **Expect / livraison mid-burst** — réponse perdue ou non armée pour un dump au milieu de la rafale (`TIMEOUT last=none` alors que dump précédent/suivant OK).
2. **Size-reject / short dump** — rejet silencieux ou fenêtre expect expirée sans retry utile (moins probable si `last=none` total).
3. **Race outbound / IN** — WriteBulk / flush / deferred queue sous charge bank (100 dumps serrés) ; conserver invariants `835c992`.
4. **F0 perdu mid-burst** — repair armé seulement si expect actif et framer pas en hold ; vérifier si un cas mid échappe encore.

Fichiers suspects (point d’entrée) :

- `src/Device/DeviceSessionHostOutbound.cpp` — expect avant flush, write outbound
- `src/Device/DeviceSessionMatrixDump.cpp` — expect / size-reject retry
- `src/Device/DeviceSessionDeviceHost.cpp` / `DeviceSessionBulkInDeliver.cpp` — livraison dump
- `src/Device/DeviceSessionSupport.cpp` — repair F0 (déjà élargi ; ne pas casser)
- Bridge logs des Starts en échec sous `post-epic2-cr-coldfix*` (chercher autour du TIMEOUT : `first-burst`, `leading-F0 repair`, `SysEx size reject`, compteurs `send_ok` / `host_out_ok` / `bulk_in`)

---

## Mission

1. Investiguer le fingerprint **mid-burst `TIMEOUT last=none`** avec les logs coldfix + coldfix-rerun (+ rebuild debug si tu changes du C++).
2. Proposer / implémenter un correctif Bridge **minimal** (pas de discard lab).
3. Rejouer au minimum :
   - **Smoke** : mid reste vert (`5×10`, 100 %) — non-régression cold-start.
   - **Gate bank** : `20×100`, `--pass-percent 100`, `--fresh-starts 20` — **20/20 Starts à 100 %**.
4. Si bank verte : option stress jour (`--fresh-starts 50`) — pas overnight 8 h sauf demande.
5. Pas de commit sauf demande explicite.
6. Livrer en français clair : cause retenue, diff touché, tableau Pass/Fail, chemins de logs.

### Commandes lab (référence)

```bat
cmake --build --preset debug

python scripts/lab/sysex-matrix-mid-loop.py --with-bridge --out-port "MT4 Out 1" --in-port "MT4 In 1" --pass-percent 100 --count 10 --fresh-starts 5 --bridge-exe builds\debug\Debug\Bridge.exe --log-dir tests\lab-logs\sysex-matrix-mid\post-epic2-cr-midburst

python scripts/lab/sysex-matrix-bank-loop.py --with-bridge --out-port "MT4 Out 1" --in-port "MT4 In 1" --pass-percent 100 --count 100 --fresh-starts 20 --bridge-exe builds\debug\Debug\Bridge.exe --log-dir tests\lab-logs\sysex-matrix-bank\post-epic2-cr-midburst
```

### Checklist matériel

- MT4 USB + Matrix sur DIN Out1↔**In** (pas Thru)
- Pas de boucle rouge ; ports libres (pas MIDI-OX / Matrix-Control)

---

## Hors scope

- Overnight 8 h Windows / Mac
- Palier 3 SysEx longs comme objectif
- Lots revue B/C (framer/clock, anneau USB) sauf preuve lab claire
- « Fix » = ignorer des dumps dans le harness ou baisser la gate sous 100 %
- Casser / revert le repair F0 1-byte cold-start

---

Bonne chasse — le warm path et le cold-start tiennent ; c’est le trou rare mid-burst qu’il faut soigner dans le Bridge.
