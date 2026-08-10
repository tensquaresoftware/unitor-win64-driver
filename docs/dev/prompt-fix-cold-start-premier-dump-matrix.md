# Prompt — Fix cold-start premier dump Matrix (post gate jour bank+mid)

Copier-coller tel quel dans un **nouveau chat Cursor** (contexte frais).  
Skill conseillé : **bmad-quick-dev**.  
Projet : `C:\Users\Guillaume\Dev\Projects\unitor-win64-driver`

---

## Contexte de la Story

Après les correctifs short-dump (`835c992`), la gate jour bank+mid reste rouge : le Matrix répond bien une fois « chaud », mais le **premier dump patch juste après un Start Bridge** tombe encore souvent en `TIMEOUT` sans aucune trame, et il reste quelques trous rares en milieu de rafale banque.

---

## Qui tu es / comment parler

- Agent BMad sur **unitor-win64-driver** (Windows 10/11, Bridge WinUSB + virtualMIDI).
- Français clair, intention produit d’abord.
- Noms gardés : **MT4**, **Matrix**, **Matrix-Control**, **WinUSB**, **virtualMIDI**, **SysEx**, **Bridge**.
- Pas de commit sans demande explicite.
- Objectif de cette passe : **éliminer (ou fortement réduire) le trou du premier dump post-Start**, puis rejouer la gate jour — pas de redesign large, pas de palier 3 longs, pas d’overnight 8 h comme objectif.

Appliquer aussi : barème de clarté BMad, `conventions.md` §3, `scripts/quality/lint-touched.py` si tu modifies du C++.

---

## Preuve lab (2026-08-08, câblage In correct — pas Thru)

### Lab A — bank (`20×100`, gate 100 %/Start)

- Résultat : **10/20** Starts à 100 % ; **10/20** à 98–99 % ; **13** `TIMEOUT last=none` ; Bridge vivant, `send_fail` ≈ 0.
- Beaucoup de TIMEOUT sur **index 0001 / slot 00** (premier dump du Start).
- Aussi quelques misses mid-burst (ex. slots `29`, `63`, `58`, `32`, `2D`).
- Log : `tests/lab-logs/sysex-matrix-bank/post-epic2-cr/sysex-matrix-bank-20260808T160736Z.log`
- Bridges : `tests/lab-logs/sysex-matrix-bank/post-epic2-cr/bridge-20260808T160736Z-start*.log`

### Lab B — mid (`5×10`, gate 100 %)

- **Chaque** Start : 1er `dump_patch` → `TIMEOUT last=none` ; dumps 2–10 OK ; `dump_master` / push **100 %**.
- Log : `tests/lab-logs/sysex-matrix-mid/post-epic2-cr/sysex-matrix-mid-20260808T161402Z.log`

### Spec / deferred

- `_bmad-output/implementation-artifacts/spec-post-epic2-cr-bank-mid-day-gate.md`
- Entrées cold-start dans `_bmad-output/implementation-artifacts/deferred-work.md` (section 2026-08-08 day-gate)

### Correctifs déjà sur `main` (ne pas les défaire)

| Commit | Rôle |
|---|---|
| `835c992` | expect avant flush, retry différé, abandon→retry, 2ᵉ short sans tuer pompe, pas de drain Write/SendToHost depuis callback virtualMIDI, expiry expect, plafond différé, repair F0=`10 06` |
| `5d77070` | findings revue + deferred |

**Interdit :** masquer le symptôme en faisant jeter le 1er dump par le script lab (sauf décision produit explicite de Guillaume). Le fix vise le Bridge / démarrage de session.

---

## Hypothèses à tester (ordre)

1. **Cold-start** — Post-Start IN calm / pipe prime / librarian OUT gated : le 1er dump_patch part trop tôt, ou la 1ʳᵉ réponse est perdue / non attendue.
2. **Expect / arm** — `armExpect` / `clearExpect` / fenêtre 3,5 s encore incorrecte pour le tout premier OUT après Start (moins probable pour Lab B où seul le 1er échoue, puis stable).
3. **Résidu mid-burst** — même famille `TIMEOUT last=none` que l’overnight, rare ; traiter **après** le cold-start si la gate bank n’est pas encore verte.

Fichiers suspects (point d’entrée) :

- `src/Device/DeviceSession.cpp` — Post-start IN calm, pipe prime, ouverture librarian OUT
- `src/Device/DeviceSessionHostOutbound.cpp` — expect avant flush, write outbound
- `src/Device/DeviceSessionMatrixDump.cpp` — expect / size-reject retry
- `src/Device/DeviceSessionDeviceHost.cpp` / `DeviceSessionBulkInDeliver.cpp` — livraison dump

---

## Mission

1. Investiguer le fingerprint **premier dump post-Start** avec les logs ci-dessus (+ rebuild debug si tu changes du C++).
2. Proposer / implémenter un correctif Bridge **minimal** (pas de discard lab).
3. Rejouer au minimum :
   - **Smoke cold-start** : Lab B mid (`--fresh-starts 5 --count 10 --pass-percent 100`) — gate 100 %.
   - **Gate bank** : Lab A (`--fresh-starts 20 --count 100 --pass-percent 100`) — gate 100 %.
4. Si A+B verts : option stress jour (`--fresh-starts 50` bank) — pas overnight 8 h sauf demande.
5. Pas de commit sauf demande explicite.
6. Livrer en français clair : cause retenue, diff touché, tableau Pass/Fail, chemins de logs.

### Commandes lab (référence)

```bat
cmake --build --preset debug

python scripts/lab/sysex-matrix-mid-loop.py --with-bridge --out-port "MT4 Out 1" --in-port "MT4 In 1" --pass-percent 100 --count 10 --fresh-starts 5 --bridge-exe builds\debug\Debug\Bridge.exe --log-dir tests\lab-logs\sysex-matrix-mid\post-epic2-cr-coldfix

python scripts/lab/sysex-matrix-bank-loop.py --with-bridge --out-port "MT4 Out 1" --in-port "MT4 In 1" --pass-percent 100 --count 100 --fresh-starts 20 --bridge-exe builds\debug\Debug\Bridge.exe --log-dir tests\lab-logs\sysex-matrix-bank\post-epic2-cr-coldfix
```

### Checklist matériel

- MT4 USB + Matrix sur DIN Out1↔**In** (pas Thru)
- Pas de boucle rouge ; ports libres (pas MIDI-OX / Matrix-Control)

---

## Hors scope

- Overnight 8 h Windows / Mac
- Palier 3 SysEx longs comme objectif
- Lots revue B/C (framer/clock, anneau USB) sauf preuve lab claire
- « Fix » = ignorer le 1er dump dans le harness

---

Bonne chasse — le warm path tient ; c’est le démarrage de session qu’il faut soigner.
