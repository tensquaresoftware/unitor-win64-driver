# Prompt — Labs post-revues transversales Epic 1 / Epic 2 (dumps SysEx courts)

Copier-coller tel quel dans un **nouveau chat Cursor** (contexte frais).  
Skill conseillé : **bmad-quick-dev**.  
Projet : `C:\Users\Guillaume\Dev\Projects\unitor-win64-driver`

---

## Contexte de la Story

On vient de corriger des courses Bridge sur les dumps SysEx Matrix courts (rafale banque) après les revues transversales Epic 1 / Epic 2 ; il faut maintenant prouver en lab Windows que le trou overnight « 99/100 TIMEOUT aucune trame » a disparu (ou mesurer ce qui reste), avant de re-pousser le palier 3 longs ou un overnight 8 h.

---

## Qui tu es / comment parler

- Agent BMad sur **unitor-win64-driver** (Windows 10/11, Bridge WinUSB + virtualMIDI).
- Français clair, intention produit d’abord.
- Noms gardés : **MT4**, **Matrix**, **Matrix-Control**, **WinUSB**, **virtualMIDI**, **SysEx**, **Bridge**.
- Pas de commit sans demande explicite.
- Rebuild debug si besoin, puis labs — pas de redesign, pas de palier 3 comme objectif de cette passe.

Appliquer aussi : barème de clarté BMad, `conventions.md` §3, `scripts/quality/lint-touched.py` si tu modifies du C++.

---

## Pourquoi (preuve avant / correctifs)

Overnight Windows Matrix (~8 h, 2026-08-07/08) :

- Mid ≈ **95,6 %** OK ; banque ≈ **84 %** OK.
- Échec typique : une seule case manquante — `TIMEOUT … last=none` (souvent **ok=99/100**), Bridge sans plantage, `send_fail` souvent à 0.
- Journal : `tests/lab-logs/overnight-matrix/overnight-20260807T222620Z.log`
- Capsule : `docs/tests/lab-evidence/overnight-matrix-windows-2026-08-07/`

Correctifs Epic 2 lot A déjà sur `main` :

| Commit | Rôle |
|---|---|
| `835c992` | Fix Matrix short-dump races (expect avant flush, retry sans jeter le différé, abandon→retry, 2ᵉ short sans tuer la pompe, plus de drain Write/SendToHost depuis le callback virtualMIDI, expiry expect, plafond différé, repair F0 = `10 06`) |
| `5d77070` | Findings revue + deferred-work |

Références :

- Findings : `_bmad-output/implementation-artifacts/review-findings-epic2-group-a.md`
- Prompt revue Epic 2 : `docs/dev/revue-code-transverse-epic-2-transport-sysex.md`
- Prompt revue Epic 1 (joint USB/IN) : `docs/dev/revue-code-transverse-epic-1-mt4-midi.md`

**Hypothèse à valider :** le fantôme de fenêtre d’attente (~3,5 s) après une réponse livrée trop tôt pendant l’OUT explique le trou mid-rafale ; les correctifs doivent faire remonter le bank vers ~100 % sur un stress court répétable.

---

## Mission (labs seulement — gate jour)

1. Rebuild Bridge debug si le binaire n’est pas à jour avec HEAD.
2. Matériel : MT4 + Matrix sur In1/Out1 ; **pas** de boucle rouge sur ces jacks ; fermer MIDI-OX / Matrix-Control sur ces ports.
3. Lancer les labs Windows ci-dessous (**ordre imposé**).
4. Rapporter en français clair : Pass/Fail, taux, indices TIMEOUT s’il y en a, et si le Bridge a stoppé la pompe ou non.
5. Ne pas élargir au palier 3 longs sauf si tu notes une régression évidente sur le chemin court.
6. Pas de commit sauf demande explicite.

### Lab A — bank burst (cœur overnight)

```bat
cmake --build --preset debug
python scripts/lab/sysex-matrix-bank-loop.py --with-bridge --out-port "MT4 Out 1" --in-port "MT4 In 1" --pass-percent 100 --count 100 --fresh-starts 20 --bridge-exe builds\debug\Debug\Bridge.exe --log-dir tests\lab-logs\sysex-matrix-bank\post-epic2-cr
```

**Gate :** 100 % sur chaque Start (20×100 dumps). Tout `TIMEOUT last=none` = Fail à investiguer (slot, index, counters Bridge dans le log start).

### Lab B — mid (régression calme)

```bat
python scripts/lab/sysex-matrix-mid-loop.py --with-bridge --out-port "MT4 Out 1" --in-port "MT4 In 1" --pass-percent 100 --count 10 --fresh-starts 5 --bridge-exe builds\debug\Debug\Bridge.exe --log-dir tests\lab-logs\sysex-matrix-mid\post-epic2-cr
```

**Gate :** 100 % (patch + master selon le script).

### Si A+B verts — option stress jour (pas encore overnight 8 h)

Répéter Lab A avec `--fresh-starts 50`, ou enchaîner mid+bank en boucle ~30–60 min (même topo), logs sous un dossier `post-epic2-cr-stress`.

### Hors scope cette passe

- Overnight 8 h Windows / Mac (à faire **après** gate jour vert).
- Palier 3 SysEx longs comme objectif de fix.
- Lots revue B/C (framer/clock, anneau USB) sauf si le lab prouve un trou ailleurs.
- Install / Epic 3+.

---

## Livrable attendu dans le chat

1. Verdict : **vert / orange / rouge** pour Lab A et Lab B.
2. Tableau court : Starts, ok rate, nb TIMEOUT, nb `last=none`, pompe Bridge morte oui/non.
3. Si Fail : 1–3 hypothèses liées aux correctifs (expect/flush, abandon, virtualMIDI) + extrait log (`SEND` / `TIMEOUT` + counters Bridge).
4. Recommandation : overnight Windows, overnight Mac contrôle, ou nouvelle revue/code.
5. Chemins exacts des logs produits.

---

## Checklist matériel (Guillaume)

- [ ] MT4 alimentée + USB
- [ ] Matrix-1000 + DIN Out1↔In1
- [ ] Pas de câble boucle sur In1/Out1
- [ ] Ports libres (pas MIDI-OX / Matrix-Control dessus)
- [ ] Bridge rebuild debug à jour (`835c992`+)

---

Bonne lab — et bon signal avant le prochain overnight.
