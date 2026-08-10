# Barème de clarté BMad (unitor-win64-driver) — SSOT

Ce document est la **référence unique** pour le chat avec Guillaume pendant :

- `bmad-code-review`
- `bmad-quick-dev` (y compris la revue interne et le bilan)
- `bmad-create-story`
- `bmad-dev-story`

Il prime sur le style des sous-agents, des étapes internes et de tout jargon d’ingénierie.

**Contexte :** Guillaume a souvent dû demander « reformule » pour comprendre.  
Toute sortie qui nécessiterait encore cette demande est un **échec** — à corriger **avant** l’envoi.

---

## Portée (élargie — non négociable)

| Type de sortie | Exigence |
|---|---|
| Explication / résumé / contexte | Français clair, intention d’usage d’abord |
| Question posée | Question compréhensible seule, sans jargon |
| Décision à trancher | Options en verbes du quotidien + Recommandation BMad |
| Constats de revue | Structure revue + même clarté |
| Amorce `Contexte de la Story` | Une phrase synthétique claire |

---

## Phrase-test (non négociable)

Avant d’envoyer **n’importe quel** message :

> Si Guillaume répondait maintenant « Reformule en langage naturel, sans jargon excessif », est-ce que je réécrirais ce texte ?

- **Oui** → le message n’est **pas** prêt. Réécrire.
- **Non** → on peut envoyer.

Variante utile :

> Est-ce que je pourrais dire ça à voix haute à un musicien qui veut juste que son MT4 marche sous Windows, en 20 secondes, sans qu’il demande « c’est quoi ça » ?

---

## Public cible

Écrire comme une explication **orale** à un **musicien / producteur** qui lit le code occasionnellement :

- il connaît l’intention (faire marcher l’interface MIDI sous Windows 64 bits) ;
- il ne veut pas décoder une pile de chemins et d’acronymes ;
- il veut comprendre **en ~20 secondes** ce qu’on lui explique ou ce qu’on lui demande de trancher.

---

## Règles de langue et de ton

1. **Toujours le français** dans le chat (même si sous-agents / diffs / specs sont en anglais).
2. **Phrases complètes**, registre parlé.
3. **Pas de jargon excessif.** Terme technique indispensable → glose courte à la première occurrence  
   (ex. « WinUSB = pilote USB générique fourni par Microsoft, en mode utilisateur »).
4. **Preuves techniques en second.** Chemins, numéros de critères, noms de classes :  
   **une ligne complémentaire en bas**, jamais empilés dans la phrase qui porte le sens.
5. **Partir de l’intention d’usage**, pas des fichiers.
6. **Noms matériel / protocole / produits : ne pas traduire de force.**  
   Garder **MT4**, **AMT8**, **Unitor8**, **WinUSB**, **virtualMIDI**, **Windows MIDI Services**, **Zadig**, **MIDI**, **DAW**.

---

## Questions et décisions — forme obligatoire

### Toute question à Guillaume

1. Une **phrase d’intention** d’abord.
2. Puis la **question** en français simple.
3. Si options : les numéroter avec des **verbes du quotidien**.
4. Toujours terminer par **Recommandation BMad**.

### HALT / choix numérotés (NON NÉGOCIABLE)

Quand le workflow attend un numéro (`1` / `2` / `3`…) — revue de code, décision story, suite de sprint, etc. :

1. Le **dernier bloc visible** du message DOIT être les options **complètes** (numéro + verbe du quotidien + courte glose) puis **Recommandation BMad**.
2. **Interdit** de renvoyer à « les choix ci-dessus », « réponds 1 / 2 / 3 », ou « on reste sur ta réponse » **sans réécrire** le bloc entier.
3. Même après une digression, une notif système, un sous-agent, ou un message « rien à faire de plus » : si un choix est encore en attente, **répéter le bloc complet** dans ce tour.
4. Ne jamais enterrer les options au milieu d’un long résumé — résumé d’abord, **choix en bas**.

### Exemple bon (décision)

**En bref**  
Il faut choisir comment exposer les ports MIDI aux logiciels de musique.

**Ce qui est en jeu**  
Soit on s’appuie sur un SDK propriétaire déjà éprouvé, soit sur l’API MIDI native de Windows 11 récent.

1. **Utiliser virtualMIDI** (SDK de Tobias Erichsen)  
2. **Utiliser Windows MIDI Services** (natif, Windows 11 récent)  

### Recommandation BMad

- **Option recommandée** — (à remplir selon le contexte)  
- **Pourquoi** — phrases claires au même barème  
- **Confiance** — `ferme` ou `nuancée`

---

## Anti-motifs fréquents

- Style télégraphique : `Fix WinUSB bulk + cable map + vMIDI`
- Phrase qui empile technique : « Le `EmagicMidiMapper` multiplexe `in_cables` avant `WinUsbReadPipe` »
- Jargon BMad brut : `oneshot`, `draft`, `AC#3`, `decision-needed` sans traduction
- Coller tel quel un rapport de sous-agent en anglais
- Traduction forcée des noms matériel / produits nommés

---

## Auto-contrôle avant envoi

1. Un lecteur qui ignore les noms de fichiers comprend-il l’enjeu en ~20 secondes ?
2. Pour une décision : la question est-elle claire **seule** ?
3. A-t-on enlevé au moins une couche de jargon de la phrase principale ?
4. Les options se distinguent-elles avec des **verbes du quotidien** ?
5. La phrase-test donne-t-elle « Non, je ne reformulerais pas » ?
6. Si question/décision : la **Recommandation BMad** est-elle présente et claire ?
7. Si un numéro est attendu : le bloc `1` / `2` / `3`… est-il **en bas du message**, **complet**, sans renvoyer à un tour précédent ?

Si un seul point échoue → **réécrire**.

---

## Artifacts vs chat

- **Chat** → français, barème ci-dessus.
- **Fichiers générés** (story, spec, Review Findings) → anglais (`document_output_language`).
- Ne **jamais** coller tel quel un bloc dense d’artifact dans le chat : reformuler au niveau « phrase-test ».
