# Prompt — Correct Course : Cap hobby / open source (archived)

> **Archived 2026-08-10.** Historical prompt used to run Correct Course. Outcome SSOT:
> [`_bmad-output/planning-artifacts/sprint-change-proposal-2026-08-10.md`](../../_bmad-output/planning-artifacts/sprint-change-proposal-2026-08-10.md)
> and the root [`README.md`](../../README.md). Kept for traceability only — do not treat as live product messaging.

---

```text
/bmad-correct-course

## Contexte de la Story

On réoriente le projet : Bridge libre et gratuit sur GitHub pour usage personnel et communauté, sans certificat de signature dans cette ligne de projet — tout en gardant une install possible pour un musicien Windows peu à l’aise avec l’informatique.

## Qui tu es / comment parler

- Agent BMad (correct-course) sur **unitor-win64-driver**.
- Français clair, intention produit d’abord (musicien MIDI / Matrix-Control / communauté).
- Noms gardés : **MT4**, **WinUSB**, **virtualMIDI**, **Bridge**, **SmartScreen**, **Zadig**, **loopMIDI**, **rtpMIDI**, **GitHub**.
- Pas de jargon BMad non glosé ; chemins / IDs en complément.
- **Pas de commit** sans demande explicite de Guillaume.
- Appliquer clarity-bar + `conventions.md` §3.
- Ne pas publier de récit privé sur des tiers (éditeurs SDK, contacts, pressentiments) dans les docs du dépôt.

## Décision produit (non négociable — point de départ)

Guillaume construit ce Bridge **surtout pour le plaisir et son usage personnel** (projet Matrix-Control). Cette ligne de projet **ne fournit pas** de certificat Authenticode / catalogue. Il veut :

1. **Donner** le projet librement sur GitHub (coût projet proche de zéro hors temps / machine).
2. Que des **musiciens lambda** (peu d’informatique) puissent **installer seuls** le Bridge pour leur interface Emagic sous Windows 10/11 64 bits, **sans s’arracher les cheveux**.
3. **Ne plus** viser un parcours « produit commercial signé + article magazine type install pro » comme objectif V1 (magazines possibles plus tard seulement avec un discours hobby / open source honnête).

### Ce qui est déjà vrai en lab (ne pas inventer le contraire)

- Sur PC Win10 **propre**, le Public Installer actuel **échoue** à l’association WinUSB (`0xE000022F`) sans catalogue de confiance — Fail documenté dans `docs/tests/smoke-epic4-public-installer-mt4.md`.
- La story **4.4** a livré la **politique** Authenticode / SmartScreen ; **OQ-3** = pas de certificat dans cette ligne hobby.
- Lab `installer/sign-lab-package.ps1` = auto-signé **local**, pas confiance communautaire publique.
- Zadig = chemin WinUSB guidé documenté ; virtualMIDI = loopMIDI/rtpMIDI installé par l’utilisateur (pas d’embed MSI ; Releases liées au SDK virtualMIDI hors scope community).

### Ce qu’il faut trancher / produire (mission Correct Course)

Utilise le workflow **bmad-correct-course** pour proposer le **bon véhicule** de changement (ne présuppose pas la réponse) :

- Correct Course seul → maj PRD / architecture / epics / sprint ?
- Nouvel épic (ex. community install without paid cert / WMS) ?
- Stories de finition sous épic 4 / avant ou après épic 5 ?
- Combinaison (ex. Correct Course + stories d’implémentation + maj docs user) ?

Objectif de sortie attendu :

1. **Verdict feuille de route** : où ranger ce pivot + ordre recommandé vs **épic 5** (ne pas bloquer l’épic 5 sur le certificat).
2. **Nouveau contrat produit « hobby install »** pour musicien lambda (SmartScreen + WinUSB guidé ; pas de promesse Setup-seul magique sur PC neuf).
3. **Impact docs** : surfaces à réécrire pour rester honnêtes sans récit privé sur des tiers.
4. **Impact code / packaging** si besoin — scope minimal.
5. **OQ-1 / OQ-3** : OQ-3 = pas de certificat dans cette ligne ; OQ-1 = Releases virtualMIDI-linked hors scope community (WMS ensuite).
6. **Livrables BMad** et ordre.
7. Bloc de choix numérotés + **Recommandation BMad** si besoin.

## Hors scope (ne pas élargir)

- Comparer des devis de certificats.
- Refonte protocole MIDI / Epic 2 / Epic 3 sauf si l’install les casse.
- Implémenter l’épic 5 dans ce chat.
- Contourner en mentant (« Setup clean marche » alors que le Fail lab dit le contraire).

## Ton de succès

Guillaume sait quel skill / story lancer ensuite, quel parcours d’install on documente **sans cert**, et quelles docs changer — sans se comparer à une release commerciale signée qu’il ne vise pas.

Français clair. Pas de commit.
```
