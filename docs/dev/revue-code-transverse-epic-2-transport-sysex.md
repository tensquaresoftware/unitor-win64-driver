# Prompt — Revue de code transverse Epic 2 (transport studio + SysEx)

Copier-coller tel quel dans un **nouveau chat Cursor** (contexte frais), **après** (ou en s’appuyant sur) la revue Epic 1.  
Skill conseillé : **bmad-code-review** (+ edge-case / adversariale si utile).  
Projet : `C:\Users\Guillaume\Dev\Projects\unitor-win64-driver`

---

## Contexte de la Story

On cherche pourquoi, sous stress Matrix de plusieurs heures, **une réponse SysEx courte** (dump patch/master) peut encore manquer entièrement, alors que le Bridge ne plante pas et que les labs courts avaient déjà donné 100 %.

---

## Qui tu es / comment parler

- Agent BMad développeur / reviewer sur **unitor-win64-driver**.
- Français clair, intention produit d’abord.
- Noms gardés : **MT4**, **Matrix**, **Matrix-Control**, **WinUSB**, **virtualMIDI**, **SysEx**, **MIDI clock**, **MTC**.
- Pas de jargon BMad non glosé ; chemins/symboles en complément.
- **Pas de commit** sans demande explicite.
- Règles : clarity-bar, `conventions.md` §3, lint sur diff C++ touché.

---

## Pourquoi cette revue (preuve lab)

Overnight Windows Matrix (~8 h) :

- Mid ≈ **95,6 %** OK ; banque ≈ **84 %** OK.
- Échec dominant : **TIMEOUT last=none** sur **un** dump (souvent **99/100** en rafale) — la demande part, **aucune** réponse complète côté lab.
- Pushes host→Matrix en mid restent en général verts.
- Ce n’est **pas** le même symptôme que les SysEx longs (troncature au milieu / file IN géante) — mais le travail récent (drain IN pendant OUT, écriture Emagic overlapped, noms In/Out) touche aussi ce chemin : à ne pas régresser.

Journal : `tests/lab-logs/overnight-matrix/overnight-20260807T222620Z.log`  
Spec longue (contexte parallèle, pas le gate de *cette* revue) : `_bmad-output/implementation-artifacts/spec-sysex-long-loopback-2.md`

Priorité Guillaume : **trouver et corriger les failles SysEx « courts » Matrix avant de re-pousser le palier 3 longs**.

Contrôle prévu : overnight **8 h Mac M5** (pilote Apple, mêmes labs mid + bank) — utile pour séparer hardware/Matrix vs Bridge Windows.

---

## Objectif Epic 2

Epic 2 = « Studio Transport and SysEx » (sprint : **in-progress**).

But produit : clock / transport / MTC utilisables en DAW ; Matrix-Control peut faire les vecteurs SysEx minimum **sans redémarrer le Bridge** pour un usage librarian normal ; conception tenue ~4 h de session.

Stories :

1. MIDI clock + Start/Stop/Continue (realtime) — review  
2. MTC quarter-frame / full-frame — review  
3. Transport SysEx transparent + buffering de rafale — **done**  
4. Vecteurs SysEx minimum Matrix-Control — **in-progress** (cœur de la chasse overnight)  
5. Longévité session ~4 h — done  

SSOT : `_bmad-output/planning-artifacts/epics/epics-unitor-win64-driver-2026-08-04/epics.md`  
Sprint : `_bmad-output/implementation-artifacts/sprint-status.yaml`  
Specs / checklists Matrix utiles :  
`_bmad-output/implementation-artifacts/spec-sysex-matrix-mid-roundtrip.md` (et artifacts bank / mid associés),  
`docs/tests/checklists/smoke-mt4-sysex-matrix-mid.md`,  
`docs/tests/checklists/smoke-mt4-sysex-matrix-bank.md`

---

## Mission (revue transverse Epic 2)

Revue **transversale** centrée SysEx Matrix + buffering + ce qui peut faire **perdre une unique réponse dump** sous rafale et dans le temps.

### Focus (dans l’ordre)

1. **Chemin dump Matrix** (demande → attente → réponse ~275 / ~351 o) :
   - armement « on attend une réponse » ;
   - rejet taille / retry éventuel ;
   - cas où le lab voit timeout alors que le Bridge loggue peu ou pas d’échec pompe.
2. **Rafale banque** (100 dumps, intervalle court) : file outbound, pacing, interaction avec hold SysEx IN, risque qu’une réponse soit écrasée, finalisée trop tôt, ou jamais livrée à virtualMIDI.
3. **Realtime (clock / MTC)** pendant SysEx : un message temps réel ou un status parasite peut-il **casser** un SysEx en cours (framer) et produire « aucune trame » ou une trame invalide ignorée ?
4. **Longévité session** (story 2.5) : compteurs, fuites, états qui se dégradent après des centaines de Starts (le overnight relance Bridge à chaque cycle mid/bank — regarder aussi le coût Start/Stop).
5. **Joint avec Epic 1** : s’appuyer sur les findings Epic 1 (verrou USB, anneau IN, demux) — ne pas les redécouvrir en silo ; dire ce qui est **spécifique Epic 2**.

### Travail récent à ne pas casser (si tu lis le diff autour de `7d34b63`)

- Demux / frame IN **entre** paquets OUT Emagic, SendToHost **différé** jusqu’à fin d’OUT.
- Write Emagic **overlapped** + drain pendant l’attente.
- Plafond chunk OUT Emagic 32 o.
- Noms ports **MT4 In / Out** (sérigraphie).

Ces changements aident surtout les **longs** SysEx ; valider qu’ils n’introduisent pas de courses sur les dumps courts.

### Hors focus

- Palier 3 longs comme *objectif de fix immédiat* (OK de noter interactions, pas d’en faire le ticket).  
- Install / Epic 3+ / marketing.  
- Redesign large « pour AMT8 ».

### Livrable attendu

1. Bilan français clair, findings classés **Bloquant / Sérieux / Léger**, max ~10, **priorisés pour expliquer 99/100 bank ou 9/10 mid**.  
2. Pour chacun : symptôme lab, lieu code, lien possible avec overnight, fix **minimal**.  
3. **Plan de repro** court (commande lab exacte) pour coincer le trou en journée.  
4. **Top 3** prochaines actions avant le prochain overnight Windows.  
5. Mentionner ce que le **overnight Mac** devra confirmer / infirmer.  
6. Code seulement si finding net ; lint clean ; **pas de commit** sans demande.

### Méthode suggérée

- Lire intent Epic 2 + stories `2-*` / specs Matrix.  
- Code : `DeviceSessionHostOutbound*`, `DeviceSessionDeviceHost*`, `DeviceSessionBulkInDeliver*`, framer SysEx, chemins « expect dump / Matrix », labs `scripts/lab/sysex-matrix-*.py`.  
- Échantillonner 2–3 cycles overnight en échec sous `tests/lab-logs/overnight-matrix/cycle-****-bank/` (chercher `TIMEOUT` + `ok=99`).  
- Revue transverse (skills BMad), pas seulement le dernier commit.

### Commande d’amorce (exemple)

```text
/bmad-code-review

Revue TRANSPARENTALE Epic 2 (transport + SysEx Matrix), focus dumps courts / rafale banque.
Symptôme : overnight Windows ~84 % bank, échec typique 99/100 TIMEOUT aucune trame.
Appuie-toi sur la revue Epic 1 si déjà faite. Lis le prompt Bureau : revue-code-transverse-epic-2-transport-sysex.md
SysEx courts avant palier 3. Pas de commit. Français clair.
```

---

## Checklist matériel pour Guillaume (après-midi / nuit Mac)

Emporter / vérifier :

- [ ] Mac M5 + chargeur  
- [ ] PC BootCamp (si tu continues Windows aussi) + chargeur  
- [ ] MT4 + alimentation USB/secteur selon ton setup  
- [ ] Matrix-1000 + alimentation  
- [ ] Câbles DIN (au moins Out1↔In1 pour Matrix)  
- [ ] Câble USB MT4  
- [ ] (Option) câble rouge boucle pour plus tard — **pas** sur In1/Out1 pendant Matrix  

Overnight Mac (rappel) :

```bash
# Exemple — adapter chemins / noms de ports Apple (list-ports d’abord)
python3 scripts/lab/sysex-matrix-mid-loop.py --list-ports
# Puis boucle manuelle ou petit wrapper : mid + bank, 8 h, sans --with-bridge
```

Logs Mac : préférer `tests/lab-logs/sysex-matrix-*-macos/` (comme les labs déjà documentés).

Bonne revue — et bon trajet.
