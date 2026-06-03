# Bug : l'évaluation des règles dépend de l'ordre de parcours (`gx->habitable_count` mutable)

> Brief d'investigation pour un agent. Objectif : **comprendre, prouver, puis corriger** une
> incohérence du moteur de règles. Le verdict de match d'un système n'est pas auto-contenu :
> il dépend de combien d'étoiles ont été « préparées » avant lui dans la même passe, à cause
> d'un état galaxie mutable (`gx->habitable_count`) muté pendant l'évaluation.

## TL;DR

- `planet_habitable_ocean_check()` (`src/worldgen/planet_theme.h:16`) **lit** `gx->habitable_count`
  pour décider si une planète devient un océan habitable.
- `planet_unmodified_type()` (`src/worldgen/planet_theme.h:55`, via `star_system_load_types`)
  **incrémente** `gx->habitable_count` (lignes 66 et 71) — et c'est appelé depuis
  `prepare_star_system()` (`src/rules/rule_eval.h:23`) **pendant l'évaluation des règles**.
- Donc le type des planètes d'une étoile (océan/habitable → thèmes → veines → gaz) dépend de
  la valeur courante de `habitable_count`, qui s'accumule au fil des `prepare_star_system`
  successifs **dans un ordre dicté par le parcours des règles** (et peut compter plusieurs fois
  la même étoile, ou en oublier).
- Résultat : la même étoile + la même règle peuvent donner un booléen **différent** selon le
  contexte (proximité imbriquée vs règle racine vs régénération JSON pour l'affichage).

## Symptôme reproductible (preuve)

Fixture : `tests/fixtures/promised-land.json` — une règle `proximity` à 3 systèmes :
`birth` (ancre) + un système « 9 ressources » (océan Sulfur, 2 gaz, 6 veines) + un système
« aimants » (`vein Magnet`), tous à `< 8 ly` du départ.

Build GPU (voir Environnement), puis :

1. La règle complète matche le seed **7526** et reporte les systèmes `0, 44, 63` :
   `./build/dsp-seed-finder --plain --max-seeds 5 --seed-start 0 --seed-end 300000 tests/fixtures/promised-land.json`
   → ligne `7526   0,44,63`. L'étoile **#44** est donc présentée comme « le système à 9 ressources ».

2. Mais l'étoile #44 **ne contient ni Oil, ni Stalagmite, ni Organic** dans la sortie JSON
   (le writer JSON utilise pourtant `planet_get_veins()`, la même fonction que le moteur) :
   `./build/dsp-seed-finder --plain --format json --max-seeds 1 --seed-start 7526 --seed-end 7527 tests/fixtures/promised-land.json`
   → dans `systems[1]` (l'étoile #44), agréger les `veins[].veinType` de toutes les planètes :
   on obtient {Coal, Copper, Fireice, Fractal, Grating, Iron, Kimberlite, Silicium, Stone,
   Titanium} — **Oil/Stalagmite/Organic absents**, alors que la sous-règle les exige « present ».

3. La sous-règle « 9 ressources » **seule** (sans proximité, sans birth) matche **zéro étoile**
   sur le seed 7526, mais matche très bien sur d'autres seeds (408, 22206, 120369…). Fichier de
   test `/tmp/nine.json` = juste l'`and` des 9 conditions de `promised-land.json` comme `rule`
   racine, même bloc `game`. Lancer :
   `./build/dsp-seed-finder --plain --max-seeds 5 --seed-start 0 --seed-end 300000 /tmp/nine.json`
   → matche 408, 22206… mais **rien dans le seed 7526**.

**Contradiction** : une règle *plus* restrictive (proximité + distance `< 8 ly`) trouve un
système à 9 ressources dans le seed 7526, alors que la même sous-règle *sans* contrainte n'en
trouve aucun. Impossible si l'évaluation d'une étoile était déterministe et auto-contenue.

## Cause racine (analyse, à confirmer)

`planet_habitable_ocean_check()` — `src/worldgen/planet_theme.h:16` :

```c
star_count = gx->game.star_count;
num18 = fmaxf(ceilf((float)star_count * 0.29f), 11.0f);
num19 = (double)num18 - (double)gx->habitable_count;             // budget restant
a     = (float)(num19 / (double)(float)(star_count - sys->st.index)); // réparti sur les étoiles restantes
... return sys->planets[pidx].habitable_factor > (double)num25;
```

Cette formule est clairement conçue pour **une passe unique, ordonnée, sur les étoiles 0..N-1** :
`sys->st.index` sert de « combien d'étoiles restent » et `gx->habitable_count` de « combien déjà
placées ». Elle n'a de sens que si chaque étoile est traitée **exactement une fois, dans l'ordre
de son index**.

Or le chemin d'évaluation est paresseux et désordonné :

- `seed_matches` → `find_matching_stars` (`src/rules/galaxy_eval.h`) → pour chaque étoile-candidate :
  `prepare_star_system` (`src/rules/rule_eval.h:23`) →
  `if (prog->needs_themes) star_system_load_types(sys, gx)` →
  `planet_unmodified_type` → `planet_habitable_ocean_check` (lit `habitable_count`) et
  `gx->habitable_count += 1` (mute) — lignes `src/worldgen/planet_theme.h:66` et `:71`.
- Pour une règle `proximity`, `prox_neighbor_matches` (`src/rules/rule_eval.h`) prépare en plus
  **chaque étoile-candidate voisine** essayée pendant la recherche — donc `habitable_count`
  accumule des étoiles qui ne font même pas partie du match, dans l'ordre de la recherche.
- Certaines étoiles sont préparées plusieurs fois (ancre + candidate), d'autres jamais
  (court-circuit `and`/`or`). `get_planets` remet `planet_type = -1` à chaque prepare, donc le
  type est re-dérivé à chaque fois avec la valeur **courante** (et fausse) de `habitable_count`.

Conséquence : le type des planètes de l'étoile #44 (et donc ses thèmes/veines/gaz) diffère selon
le nombre d'étoiles préparées avant elle. D'où le match dans un contexte et pas dans l'autre.

### Référence du comportement correct

`tests/reference/selftest.c:34` `full_habitable_count(seed)` calcule la valeur **canonique** en
faisant exactement la bonne passe : `generate_stars`, puis boucle `index = 0..star_count-1`,
`get_planets` + `star_system_load_types` **une fois par étoile, dans l'ordre**. Cela donne
`habitable_count == 22` pour le seed 0 (golden testé `selftest.c:98`) et `21` pour le seed 1.
C'est le modèle que la correction doit reproduire : les types de planètes / `habitable_count`
doivent être figés par cette passe ordonnée **avant** toute évaluation de règle.

## Pourquoi le moniteur de divergence GPU↔CPU reste vert (donc n'a rien attrapé)

Pour un seed donné, le GPU (`seed_matches`) et la vérif CPU (`verify_seed`,
`src/verify/verifier.h`) exécutent **le même code dans le même ordre** → la même accumulation
foireuse de `habitable_count` → le même booléen. Le bug est déterministe *à ordre de parcours
fixé* ; il ne se manifeste qu'en **changeant** l'ordre (règle isolée, régénération JSON pour
l'affichage, ou simplement un autre jeu de règles). C'est aussi pour ça que les matches existants
sont stables run-à-run mais potentiellement **faux** (faux positifs / faux négatifs).

## Ce n'est PAS causé par les changements récents

Le dépôt vient de gagner (commits sur `main`) : une hash-grid de collision exacte, un
`-maxrregcount=96`, et une feature « lister tous les systèmes participants d'un match `proximity` »
(reporter `[ancre, voisin1, voisin2, …]` dans `find_matching_stars`/`eval_proximity`). Le booléen
de match est **identique** à l'éval d'origine — la feature « participants » n'a fait que **révéler**
le bug en exposant l'étoile #44. Le bug d'ordre préexiste à ces changements.

## Le correctif (direction, à toi de concevoir)

Principe : `habitable_count` et les `planet_type` doivent être des **propriétés figées de la
galaxie générée**, calculées une seule fois dans une passe ordonnée déterministe (façon
`full_habitable_count`), pas un effet de bord de l'évaluation des règles.

Pistes (non exhaustives, choisis la plus propre et la moins coûteuse) :

1. Après `generate_stars`, faire une passe ordonnée `0..star_count-1` qui fixe `planet_type` de
   chaque planète et calcule `habitable_count` une fois pour toutes ; mémoriser les `planet_type`
   dans la galaxie (ou un cache par étoile) ; rendre `planet_habitable_ocean_check`/
   `planet_unmodified_type` **purs en lecture** côté évaluation (ne plus muter `gx`).
2. Veiller à l'**idempotence** : `prepare_star_system` ne doit plus modifier `habitable_count`.
   Un même étoile préparée 2 fois doit donner exactement le même état.
3. Attention au **coût GPU** : c'est dans le kernel (headers `HD` partagés). Une passe complète
   des planètes de 64 étoiles par seed peut être chère. Idéalement, ne calculer les types que si
   `prog->needs_themes`/`needs_planets`, et ne pas exploser le stack (déjà ~16 Ko/thread, REG
   plafonné à 96). Mesurer REG/STACK via `cuobjdump --dump-resource-usage` et le débit avant/après.
4. La correction **changera l'ensemble des seeds qui matchent** `promised-land` (et toute règle
   touchant océan/habitable/thèmes/veines). C'est **attendu et voulu** : les anciens matches
   étaient en partie faux. Il faudra régénérer les goldens (voir Validation).

## Validation (obligatoire)

1. **Invariant canonique** : `full_habitable_count(0) == 22`, `(1) == 21` doivent rester vrais
   (`tests/reference/selftest.c`). Lancer `bash tests/run_tests.sh` — tout doit passer (le golden
   du premier match `promised-land` y est codé en dur, ligne ~48 : il devra probablement être
   **mis à jour** vers le nouveau premier match après correction ; recalcule-le et ajuste).
2. **Cohérence isolé == imbriqué** (le cœur du bug) : pour un échantillon de seeds, l'ensemble des
   étoiles qui satisfont la sous-règle « 9 ressources » seule (`/tmp/nine.json`) doit être
   **cohérent** avec les systèmes reportés par `promised-land.json`. Plus aucun système reporté ne
   doit échouer la même sous-règle évaluée isolément. Écris un script qui vérifie ça sur ≥100 000
   seeds.
3. **Cohérence sortie/verdict** : pour chaque match, le système reporté comme « 9 ressources »
   doit réellement contenir les veines/gaz/océan exigés dans la sortie JSON (Oil, Stalagmite,
   Organic, etc. présents). Plus de contradiction comme l'étoile #44 ci-dessus.
4. **Déterminisme & ordre-indépendance** : le verdict d'un seed ne doit plus dépendre de l'ordre
   de parcours ni du jeu de règles englobant. Idée de test : évaluer une même condition mono-système
   sur une étoile (a) en racine, (b) imbriquée dans une `proximity` — même résultat.
5. **GPU == référence CPU** : `tests/run_tests.sh` section 7 (vrai kernel GPU vs CPU) doit rester
   verte ; lancer aussi le moniteur de divergence sur ≥1 M seeds avec
   `--reject-sample-rate 0.05` → `0 divergences`.
6. **Perf** : pas de régression notable du débit ; documenter REG/STACK et seeds/s avant/après
   (baseline actuelle sur cette fixture lourde ≈ 36–40 k seeds/s GPU-only, sampling off ;
   la grille de collision donnait ≈ 155 k/s sur des règles légères).

## Carte du code

- `src/worldgen/planet_theme.h` — `planet_habitable_ocean_check` (:16, lit `habitable_count`),
  `planet_unmodified_type` (:55, mute `habitable_count` :66/:71), `star_system_load_types` (:77),
  `planet_get_theme` (:179, mute `used_theme_ids`/`used_theme_count`),
  `star_system_select_all_themes` (:206).
- `src/worldgen/galaxy_gen.h` — `generate_stars` (:344), `out->habitable_count = 0` (:366).
- `src/worldgen/galaxy.h` — `int habitable_count;` (:13).
- `src/worldgen/planet_vein.h` — `star_system_avg_vein` (:297, somme les patches de
  `planet_get_veins`), `planet_get_veins` (:249). « vein present » = `averageVeinAmount` avec
  `op=present` = `COND_GT 0` (mapping `src/json/conditions.c:148`).
- `src/rules/rule_eval.h` — `prepare_star_system` (:23, appelle load_types pendant l'éval),
  `eval_proximity`, `prox_neighbor_matches`, `eval_ocean_type`, `eval_gas_*`,
  `eval_average_vein_amount`.
- `src/rules/galaxy_eval.h` — `find_matching_stars`, `seed_matches`.
- `src/verify/verifier.h` — `verify_seed` (produit le `match_record` pour la sortie).
- `src/output/writer.c` — `emit_veins_json`/`emit_gases_json` (utilisent `planet_get_veins`/
  `planet_get_gases`), `rebuild_systems`.
- `tests/reference/selftest.c` — `full_habitable_count` (:34, la passe canonique) et goldens.

## Environnement

- GPU RTX 5070 (Blackwell, sm_120), CUDA 13.x. `export PATH=/usr/local/cuda/bin:$PATH`.
- Build GPU : `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)`.
- Build référence CPU (kernel stubbé, pour vérité terrain et tests) :
  `cmake -S . -B build-cpu -DDSP_CPU_REFERENCE=ON && cmake --build build-cpu -j$(nproc)`.
- `ncu`/Nsight Compute est **bloqué** (`RmProfilingAdminOnly: 1`) même en sudo (GPU d'affichage).
  Utiliser `nsys`, `cuobjdump --dump-resource-usage` / `--dump-sass`, et des A/B chronométrés.
- Pour compiler un petit driver host qui inclut directement le worldgen : `gcc -O2 -Isrc …`
  (NB : `cc` est shimmé bizarrement sur cette machine, utiliser `gcc`).

## Contraintes

- **Exactitude d'abord** : le but est de reproduire la génération DSP correcte (la passe ordonnée
  qui donne `habitable_count(0)==22`), pas juste « rendre cohérent ». Si un doute subsiste sur le
  comportement DSP de référence, le code source décompilé DSP (Knuth PRNG, drunk-walk, etc.) fait
  foi ; les goldens du selftest sont la vérité locale.
- Ne pas casser les autres goldens (PRNG, seed-0 star 0, dyson radius, déterminisme).
- Garder le `double` partout (pas de passage FP32 : ça casserait l'exactitude des décisions).
- Ne rien commit sans accord ; présenter le diff, l'analyse confirmée, les chiffres de validation
  (cohérence isolé==imbriqué, 0 divergence, perf avant/après) et les goldens régénérés.
