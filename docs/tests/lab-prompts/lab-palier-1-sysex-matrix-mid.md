/bmad-quick-dev

## Objectif

Stress SysEx **palier 1** sur le chemin Bridge MT4 ↔ Matrix-1000 : valider à **100 %** l’aller-retour de SysEx de taille Matrix-Control réelle (~275 o patch / ~351 o master), dans les deux sens, après le chantier Device Inquiry first-shot (déjà à 100 % sur Identity).

## Contexte déjà livré (ne pas rediscuter)
- Device Inquiry Universal : 3×20 @ 5 s → 100 % Identity (y compris Inquiry #1 après Start frais).
- Commits sur `main` : `f8b24da` (anneau IN async 7 slots), `a5d3382` (first-shot Start), `c295835` (harness Inquiry).
- Harness modèle à prolonger : `scripts/lab/device-inquiry-loop.py` + `scripts/lab/requirements-device-inquiry.txt`.
- Fixtures déjà dans le repo :
  - `tests/fixtures/sysex/Patch.syx` (275 o) = Matrix-Control `PatchInit.syx`
  - `tests/fixtures/sysex/Master.syx` (351 o) = Matrix-Control `MasterInit.syx`
- Réf. vecteurs Epic 2 (smoke Matrix-Control) :
  - Dump request patch : `F0 10 06 04 01 <n> F7` → réponse **exactement 275 o** `F0 10 06 01 … F7` (souvent &lt; ~2 s)
  - Dump request master : `F0 10 06 04 03 00 F7` → réponse **exactement 351 o** `F0 10 06 03 … F7`
  - Docs : `docs/tests/checklists/smoke-epic2-matrix-control-mt4.md` (vecteurs #2 / #3 / #4)

## Ce qu’il faut livrer (palier 1 seulement)

### A — Harness Python automatisé (tu lances le lab)
Nouveau script lab (kebab-case), calqué sur le harness Inquiry :
- Option `--with-bridge` (start `Bridge.exe --start-session --dev-zadig`, wait ports, stop propre)
- Ports par défaut : `MT4 Output 1` / `MT4 Input 1`
- Logs sous `tests/lab-logs/` (sous-dossier dédié, ex. `sysex-matrix-mid/`) + README EN court
- Dépendances : réutiliser `scripts/lab/requirements-device-inquiry.txt` (ou fichier frère minimal si besoin)

### B — Scénarios à automatiser (100 % chacun)
1. **Host→device patch push** : envoyer le contenu binaire de `Patch.syx` ; Pass = WriteBulk/host path OK + pas de pump fail Bridge (et, si observable sans ambiguïté, pas de TIMEOUT côté script). Ne pas exiger une preuve sonore du Matrix dans le harness.
2. **Host→device master push** : idem avec `Master.syx`.
3. **Device→host patch dump** : envoyer dump request patch (`F0 10 06 04 01 <n> F7`, choisir `<n>` documenté — défaut raisonnable `00` ou slot lab documenté) ; Pass = réception d’**exactement 275 octets** commençant par `F0 10 06 01` et finissant par `F7` dans le timeout (défaut ~2–3 s + marge).
4. **Device→host master dump** : request `F0 10 06 04 03 00 F7` ; Pass = **exactement 351 octets** `F0 10 06 03 … F7`.

Répéter chaque scénario critique (≥5×, idéalement ≥10×) et au moins **2 Starts frais** Bridge pour le couple dump patch+master. Barre stricte : **100 %** (zéro TIMEOUT / troncature / taille incorrecte).

### C — Spec + checklist
- Spec Quick Dev EN sous `_bmad-output/implementation-artifacts/` (intent, ACs Given/When/Then, Design Notes avec résultats lab).
- Checklist courte EN sous `docs/tests/checklists/` (procédure one-shot + critères Pass).
- `python scripts/quality/lint-touched.py` clean si diff C++ (palier 1 = **préfère script-only** ; patch Bridge seulement si lab prouve un trou).

## Méthode attendue
1. Clarifier / spec Ready for Development (single goal : palier 1 mid-size SysEx round-trip).
2. Implémenter le harness (réassembler SysEx fragmentés WinMM/rtmidi jusqu’à `F7` ; ne pas passer au Pass sur un premier fragment).
3. Lancer toi-même le lab quand le setup est prêt (comme Inquiry) ; documenter stamps + rates.
4. Si KO : instrumenter le minimum (taille reçue, premier/dernier octet, délai) — ne pas accuser le Matrix si les LEDs MT4 prouvent l’activité DIN.
5. Pas de warm-up opaque ; pas de ralentissement artificiel pour « réussir ».

## Hors scope (paliers suivants — ne pas faire maintenant)
- Dump request **All** / rafale banque (~100× 275 o)
- Générateur SysEx très long / appli echo bidon
- Matrix-Control UI comme outil principal du gate (le harness Python est le gate)
- Windows MIDI Services, MidiView, AMT8 / Unitor8
- Changer le heartbeat Matrix-Control

## Setup lab attendu
- MT4 Zadig + Bridge Debug sous `builds/debug/Debug/Bridge.exe`
- Matrix-1000 alimenté, DIN Out1↔In1
- Fermer MIDI-OX / Matrix-Control sur les ports MT4 pendant le harness
- Commande cible (à affiner dans le script) du genre :
  `python scripts/lab/<nouveau-script>.py --with-bridge --pass-percent 100`

## Décision produit
Palier 1 = barre de crédibilité SysEx « taille éditeur » après Identity 100 %. Si 100 % tient, on ouvrira ensuite All + SysEx synthétiques longs.