/bmad-quick-dev

## Objectif

Stress SysEx **palier 3** : prouver que le Bridge MT4 transporte sans perte des SysEx **beaucoup plus longs** que patch/master, dans les deux sens, via un outil lab automatisé (pas Matrix-Control). À faire **seulement** si paliers 1 et 2 sont à 100 %.

## Prérequis (bloquant)
- Palier 1 (275/351 o) et palier 2 (rafale banque / 100× patch) verts en lab.
- Harness `scripts/lab/` existant à prolonger.

## Intention produit
On ne dépend plus du contenu réel du Matrix pour le stress extrême :
- **Host→device** : le lab envoie un SysEx très long (payload bidon mais trame MIDI valide `F0 … F7`, octets &lt; 0x80 dans le body) sur `MT4 Output Y`.
- **Device→host** (option A, préférée si simple) : un petit **répondeur lab** sur une machine/processus qui écoute un port MIDI et, sur réception d’un dump request (ou d’un trigger lab dédié), renvoie un SysEx de longueur configurable (ex. 4 KiB, 16 KiB, 64 KiB — plafonds à trancher sous `kMaxSysexHoldBytes` / capacité Bridge).
- **Device→host** (option B) : boucle Bridge seule impossible sans second endpoint ; si pas de second port MIDI physique, utiliser teVirtualMIDI/loopback **hors** MT4 pour le répondeur, **ou** documenter que le stress device→host long se fait en injectant via un second sender sur le chemin USB seulement si l’archi lab le permet.

Recommandation BMad : **répondeur Python** (même stack mido/rtmidi que le harness) + scénario host→device long direct sur MT4 ; device→host long via répondeur branché sur DIN In du Matrix **seulement si** le lab a un second interface, sinon stress host→device long + rafale déjà prouvée au palier 2 comme preuve IN, et device→host long via répondeur sur VirtualMIDI **non-MT4** n’éprouve pas le Bridge — donc **exiger** soit Matrix+second path DIN, soit accepter que palier 3 gate = **host→device long sur MT4** + mesure Bridge counters, et reporter device→host mega-frame en Ask First hardware.

Clarifier au démarrage avec Guillaume (une question numérotée) quel hardware est dispo pour le sens device→host mega-SysEx.

## Livrables
1. Outil lab : générateur / répondeur SysEx long (script Python kebab-case sous `scripts/lab/`), longueurs CLI (`--size`, `--count`, `--interval`).
2. Harness orchestrateur `--with-bridge` qui :
   - envoie N SysEx host→device de taille S ;
   - (si mode répondeur MT4 possible) envoie trigger et attend N réponses de taille S sur Input ;
   - logs + `pass=true` à 100 %.
3. Respecter les limites Bridge documentées (hold SysEx / queue outbound) : si S dépasse un plafond volontaire, Pass = échec **anglais explicite** (pas hang silencieux) — distinguer « stress transport OK sous plafond » vs « rejet propre oversize ».
4. Spec EN + checklist EN ; ≥2 Starts frais ; barre 100 % sous le plafond choisi.
5. C++ seulement si trou lab ; sinon script-only.

## Tailles de départ suggérées
- S1 = 1024 o, S2 = 4096 o, S3 = min(16384, plafond hold documenté − marge)
- count ≥ 20 @ interval ≥ 50–100 ms (ajustable)
- Ne pas commencer à 1 Mo d’emblée.

## Hors scope
- Remplacer Matrix-Control
- Windows MIDI Services / MidiView / AMT8 / Unitor8
- Changer heartbeat MC
- Faire du palier 3 un substitut au palier 1/2 (formes Matrix réelles restent la crédibilité éditeur)

## Méthode
1. Spec single-goal ; trancher le mode device→host selon hardware.
2. Implémenter générateur/répondeur + harness ; lancer le lab toi-même.
3. Documenter plafond vs Pass ; pas de rustine timing opaque.
4. Pense à t'appuyer sur le code du driver Linux si tu dois intervenir sur le code de notre driver MT4, car à plusieurs reprises tu y as trouvé des points qui ont permi de débloquer certaines situations, le driver de Linux semble bien fait et robuste.