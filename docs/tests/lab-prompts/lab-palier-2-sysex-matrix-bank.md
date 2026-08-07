/bmad-quick-dev

## Objectif

Stress SysEx **palier 2** sur Bridge MT4 ↔ Matrix-1000 : valider à **100 %** une rafale SysEx **device→host** de taille « banque / dump All », après succès du palier 1 (patch 275 o / master 351 o).

## Prérequis (bloquant)
- Palier 1 livré et lab **100 %** (harness Patch/Master push + dump patch/master).
- Identity first-shot déjà à 100 % (`f8b24da` / `a5d3382` / `c295835` + suite palier 1).
- Ne pas commencer si le palier 1 n’est pas vert.

## Références
- Vecteur optionnel Epic 2 #6 : ≈100× frames patch 275 o (~28 KB série), pacing ≥10 ms — `docs/tests/checklists/smoke-epic2-matrix-control-mt4.md`
- Smoke SysEx : `docs/tests/checklists/smoke-epic2-sysex-mt4.md`
- Harness palier 1 (à prolonger, pas à réécrire from scratch) sous `scripts/lab/`
- Fixtures : `tests/fixtures/sysex/Patch.syx` (forme d’une frame patch)

## Décision de forme (trancher en lab, documenter)
Deux modes acceptables — choisir **un** comme gate principal, l’autre en bonus si facile :
1. **Dump All / bank export réel** : un dump request Oberheim documenté (octets exacts depuis Matrix-Control ou capture lab) → le Matrix envoie une série de frames patch (et éventuellement master). Pass = toutes les frames complètes (chaque `F0 … F7`, taille attendue 275 o pour patch), compte de frames ≥ seuil documenté, zéro troncature, Bridge sans pump fail / restart.
2. **Rafale synthétique device→host** si le dump All exact est ambigu : N dumps patch séquentiels (`F0 10 06 04 01 <n> F7` en balayant `<n>`) avec pacing ≥10 ms, N par défaut **100** (configurable). Pass = N/N réponses 275 o exactes.

Recommandation BMad si les octets « All » ne sont pas trouvés en &lt;30 min : gate = mode 2 (100× dump patch), et noter « All réel » en Ask First / follow-up.

## Livrables
1. Extension du harness lab (ou script frère kebab-case) avec `--with-bridge`, logs sous `tests/lab-logs/` (ex. `sysex-matrix-bank/`), résumé `sent/recv/rate/pass`.
2. Réassemblage SysEx multi-fragments jusqu’à `F7` ; timeout global de rafale documenté (pas un timeout Identity de 4 s naïf).
3. Spec EN + checklist EN ; Design Notes avec stamps lab.
4. Barre : **100 %** sur ≥2 Starts frais (ex. 2×100 dumps patch, ou 2× dump All complet).
5. Patch Bridge C++ **seulement** si lab prouve une perte ; sinon script-only. `lint-touched.py` clean si C++.

## Méthode
1. Spec single-goal palier 2.
2. Implémenter harness ; lancer toi-même le lab.
3. Sur KO : compteurs taille/frame index/délai ; LEDs MT4 = Matrix actif → perte Bridge.
4. Pas de ralentissement artificiel pour « réussir » au-delà du pacing Matrix stock (≥10 ms).
5. Pense à t'appuyer sur le code du driver Linux si tu dois intervenir sur le code de notre driver MT4, car à plusieurs reprises tu y as trouvé des points qui ont permi de débloquer certaines situations, le driver de Linux semble bien fait et robuste.

## Hors scope
- Palier 3 (générateur SysEx ultra-long / echo bidon)
- Matrix-Control UI comme gate principal
- Windows MIDI Services, MidiView, AMT8 / Unitor8
- Heartbeat Matrix-Control comme fix

## Setup
- Zadig + `builds/debug/Debug/Bridge.exe --start-session --dev-zadig`
- Matrix alimenté, DIN Out1↔In1 ; fermer MIDI-OX / Matrix-Control sur ports MT4