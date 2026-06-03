# Optimisation exacte : grille spatiale pour la collision de poses

> Brief d'implémentation pour un agent. Objectif : accélérer la génération de
> galaxie **sans changer un seul résultat** (aucune divergence vs la référence
> FP64 actuelle), en remplaçant la boucle de collision O(n²) par une grille
> spatiale. C'est la seule piste « exacte ET plus rapide » identifiée par le
> benchmark.

## 1. Contexte et résultats du benchmark

Outil : finder de seeds DSP en C/CUDA (kernel `scan_kernel`, un thread = un seed
= une galaxie complète). Scan de référence : 99 999 999 seeds.

Faits mesurés (RTX 5070, sm_120, fixture `tests/fixtures/promised-land.json`) :

- **Le kernel GPU = 99,99 % du temps** (profil `nsys`). Transferts mémoire et
  vérification CPU négligeables. Le pipeline double-buffer overlap déjà bien.
- **Le FP64 ≈ 62 % du runtime du kernel.** Sur ce GPU grand public, le FP64
  tourne à ~1/64 du débit FP32. Une sonde « tout en FP32 » donne 2,6×–3,6× mais
  **casse l'exactitude** (voir §2), donc inacceptable.
- **Tout le coût FP64 est dans la géométrie**, pas dans le PRNG. Mesure A/B :
  géométrie en float + PRNG en double = 2,49× ; PRNG en float + géométrie en
  double = **0 % de gain**. Le chemin entier/PRNG est déjà gratuit.
- **Dans la géométrie, la boucle de collision domine à ~98 %.** Compteurs
  instrumentés sur 200 000 seeds (génération de poses uniquement) :
  - `pose_attempt`  : **678,8 / seed**
  - `sqrt(double)`  : **527,4 / seed**
  - `distance_sq`   : **55 357,7 / seed**  ← la cible
  - étendue spatiale : `max |coord| ≈ 76 ly`, `star_count = 64`
- Déjà mergé dans `main` : `-maxrregcount=96` (occupancy) → ~44 000 seeds/s
  (était ~40 300), +9 %, résultats inchangés. C'est le point de départ à battre.

En clair : ~55 k évaluations de `distance_sq` par seed, ~8 ops FP64 chacune
(≈ 443 k ops FP64/seed) ; les ~527 `sqrt` sont marginaux. Réduire le **nombre**
de `distance_sq` (sans toucher à leur précision) est le levier exact.

## 2. Pourquoi « exact » est non-négociable ici

La génération de poses contient des **décisions discrètes en cascade** : le test
de collision `distance_sq < min_dist²` accepte/rejette chaque pose candidate.
Une différence d'un ULP près du seuil inverse une collision → une pose
différente est acceptée → toute la galaxie diverge ensuite. C'est pourquoi
remplacer `double` par `float` (ou par de la virgule fixe, qui a son propre
arrondi) produit des galaxies différentes et trouve d'autres seeds.

La grille spatiale n'a PAS ce problème : elle **garde le `double`** et ne change
que *l'ensemble de poses comparées*, en prouvant que les poses ignorées ne
peuvent jamais entrer en collision (§4). Le booléen retourné est identique →
galaxie bit-identique.

## 3. Carte du code

Tout est dans des headers partagés host+device (macro `HD` = `__host__
__device__` en CUDA, vide sur CPU). Le même code sert au kernel GPU ET à la
vérification CPU exacte.

- `src/worldgen/galaxy_gen.h` — **le fichier à modifier.** Contient :
  - `check_collision(existing, len, pt, min_dist)` — la boucle linéaire O(n) à
    remplacer. Retourne 1 si une pose existante est à < `min_dist` de `pt`.
  - `pose_attempt(rng, existing, len, base, step_diff, min_dist, flatten, out_pt)`
    — tire 4 randoms, calcule une pose candidate, appelle `check_collision`.
  - `poses_first_pass`, `poses_second_pass`, `random_poses`,
    `generate_temp_poses`, `generate_stars` — orchestrent l'ajout des poses.
- `src/worldgen/vector3.h` — `vec3` (3× `double`), `vec3_distance_sq`. **Ne pas
  changer le type.**
- `src/constants/star_gen.h` — constantes : `POSES_MIN_DIST 2.0`,
  `DSP_MAX_TEMP_POSES 256`, `POSES_ITER_COUNT 4`, `POSES_ATTEMPTS 256`,
  `POSES_WALK_ROUNDS 256`, `DSP_MAX_DRUNK 8`, `DSP_MAX_STARS 64`.
- `src/worldgen/prng.h` — PRNG (état entier exact). **Ne pas toucher.**

### Algorithme actuel (à préserver à l'identique sauf la collision)

1. `random_poses` : `poses[0] = origine`. Tire `drunk_num ∈ [6,8]` ancres.
2. `poses_first_pass` : pour chaque ancre, jusqu'à `POSES_ATTEMPTS=256`
   `pose_attempt(base=origine)`. Le premier succès est ajouté à `poses[]` ET à
   `drunk[]`. S'arrête si `len >= max_count` (= `target_count * 4`, ≤ 256).
3. `poses_second_pass` : `POSES_WALK_ROUNDS=256` rounds. Pour chaque ancre
   `drunk[a]`, avec proba `0.7`, jusqu'à 256 `pose_attempt(base=drunk[a])` ; au
   succès, `drunk[a] = pt` (marche aléatoire) et `poses[len++] = pt`.
4. `pose_attempt` : `check_collision` contre **toutes** les `len` poses déjà
   placées (`min_dist = 2.0`).
5. `trim_poses` : ne garde qu'un index sur `POSES_ITER_COUNT` (=4) jusqu'à
   `target_count`. La collision, elle, s'est faite contre l'ensemble complet.

Invariant clé : toute pose acceptée est à ≥ `min_dist` de toutes les autres.

## 4. La grille spatiale (design)

Idée : cellules de côté `c = min_dist = 2.0`. Une pose `q` peut entrer en
collision avec `p` ssi `|p − q| < min_dist`. Si `c ≥ min_dist`, alors
`|p_axe − q_axe| < c`, donc la cellule de `q` diffère de celle de `p` d'au plus
±1 sur chaque axe. **Il suffit de tester le voisinage 3×3×3 = 27 cellules**
autour de la cellule de `pt`. Toutes les poses hors de ce voisinage sont
> `min_dist` par construction → jamais en collision → les ignorer ne change
aucune décision.

Preuve d'exactitude : `check_collision` retourne un booléen et s'arrête à la
première collision ; l'ordre de parcours n'affecte pas le booléen. La grille
parcourt un sur-ensemble des poses réellement à < `min_dist`, avec exactement le
même `vec3_distance_sq` en `double`. Donc booléen identique → poses acceptées
identiques → galaxie identique. ✔

### Contrainte forte : mémoire par thread

Chaque thread GPU génère sa propre galaxie → la grille vit en **local memory par
thread**. Le kernel utilise déjà ~10 Ko de stack/thread et on vient de plafonner
les registres à 96 : ajouter une grosse structure peut **annuler le gain via une
chute d'occupancy**. La grille DOIT être compacte.

Une grille **dense** est exclue : `max |coord| ≈ 76 ly`, `c = 2.0` → ~76³ ≈
440 000 cellules. Impossible par thread. → **Utiliser une hash-grid.**

Structure suggérée (open-addressing par chaînage, indices sur 16 bits) :

- `int16_t head[HASH_SIZE]` initialisé à −1 (têtes de listes par bucket).
- `int16_t next[DSP_MAX_TEMP_POSES]` (chaînage des poses d'un même bucket).
- `int32_t cellkey[DSP_MAX_TEMP_POSES]` : la clé de cellule entière empaquetée
  de chaque pose (pour confirmer l'égalité de cellule exacte, car le hash mélange
  des cellules différentes).
- Hash : `(ix,iy,iz)` → cellule via `floor(coord / c)` (attention aux
  coordonnées négatives : `floor`, pas une troncature) ; clé empaquetée p.ex.
  `((ix+OFFS)&0x3FF) | ((iy+OFFS)&0x3FF)<<10 | ((iz+OFFS)&0x3FF)<<20` avec
  `OFFS=512` (couvre ±512 cellules = ±1024 ly, large) ; `bucket = (key * 2654435761u) % HASH_SIZE`.
- `HASH_SIZE` modeste (p.ex. 256 ou 512), à régler. Coût : `head` doit être
  remis à −1 **à chaque seed** (par `generate_temp_poses`). Garder `HASH_SIZE`
  petit pour amortir ce reset.

Requête de collision pour un candidat `pt` :
- calculer sa cellule `(cx,cy,cz)` ;
- pour les 27 voisins `(cx+dx,cy+dy,cz+dz)`, dx,dy,dz ∈ {−1,0,1} : hash → bucket,
  parcourir la chaîne `head[bucket] → next → …`, et pour chaque pose dont la
  `cellkey` correspond exactement au voisin courant, faire le `vec3_distance_sq`
  en `double` et tester `< min_dist²`. Retourner 1 à la première collision.

Insertion : **uniquement après acceptation** d'une pose (au même endroit où le
code actuel fait `poses[len++] = pt`), insérer son index dans la grille.

Alternative plus simple à essayer d'abord (peut suffire) : un seul `vec3` array
trié n'aide pas ; mais comme les poses sont ajoutées incrémentalement et jamais
retirées pendant une passe, le chaînage ci-dessus est direct. Si l'occupancy
souffre trop, réduire `HASH_SIZE`, passer `cellkey` sur 16 bits, ou stocker la
grille en `__shared__` partagée par bloc est à étudier (mais 64 threads/bloc ×
structure = budget shared serré, ~768 o/thread — probablement trop petit).

### Détails à NE PAS casser

- Ne change pas l'ordre ni le nombre de tirages PRNG. `pose_attempt` doit tirer
  exactement les mêmes 4 valeurs et accepter/rejeter exactement comme avant. La
  grille ne remplace QUE le scan de `check_collision`.
- `pose[0] = origine` est ajouté avant la marche → l'insérer aussi dans la grille.
- `min_dist` est toujours `2.0` ici, mais garde la fonction générique (la grille
  doit utiliser le `min_dist` reçu pour la taille de cellule, pas une constante
  en dur, au cas où d'autres appels surviennent).
- Reset de la grille par seed (dans `generate_temp_poses`, avant `random_poses`).

## 5. Protocole de validation (obligatoire — prouver 0 divergence)

L'optimisation est inutile si elle change ne serait-ce qu'un seed. Faire les deux :

1. **Diff bit-à-bit de la sortie galaxie.** Un petit driver CPU (modèle :
   `/tmp/dsp_count/count_main.c`, qui inclut directement `galaxy_gen.h`) qui,
   pour chaque seed d'une large plage (≥ 5 000 000), hash l'intégralité de
   `gx.positions[]`, `gx.star_seeds[]`, `gx.star_types[]`, `gx.star_count`, et
   compare le hash AVANT vs APRÈS la modif. **Exiger 0 différence.** C'est le
   test le plus fort (compare l'ensemble complet de poses, pas juste les matchs).
2. **Diff des résultats du finder.** Construire la **référence CPU** avant ET
   après, lancer sur une plage avec une condition, diff les fichiers de sortie :
   doivent être identiques.
   `cmake -S . -B build-cpu -DDSP_CPU_REFERENCE=ON && cmake --build build-cpu -j$(nproc)`
   → binaire `dsp-seed-finder-cpu` (pas besoin de CUDA, build rapide).
3. **Moniteur de divergence GPU.** Lancer le build GPU sur une large plage avec
   `--reject-sample-rate 0.05` ; le résumé doit afficher `0 divergences`.
4. Lancer la suite de tests existante dans `tests/`.

## 6. Protocole de benchmark

Baseline à battre : **~44 000 seeds/s** (main actuel, regs=96). Plafond
théorique (FP32, casse l'exactitude) : ~150 000 — la grille ne l'atteindra pas
mais doit dépasser 44 k de façon nette puisqu'elle coupe ~55 k → quelques k
`distance_sq`/seed. Gain net **à mesurer** : la mémoire ajoutée peut réduire
l'occupancy et manger une partie du gain. Si régression, ajuster `HASH_SIZE` /
packing.

Mesure du débit (1 M seeds, lire la ligne « in Xs ») :

```
./build/dsp-seed-finder --plain --max-seeds 0 --seed-start 0 --seed-end 1000000 \
  -o /tmp/out.txt tests/fixtures/promised-land.json
```

Ressources kernel (registres / stack — surveiller la pression mémoire) :

```
cuobjdump --dump-resource-usage build/dsp-seed-finder | grep -E "REG:|STACK:"
```

Profil timeline si besoin (kernel vs reste) :

```
nsys profile --trace=cuda -o /tmp/p --force-overwrite=true ./build/dsp-seed-finder \
  --plain --max-seeds 0 --seed-end 400000 -o /tmp/o.txt tests/fixtures/promised-land.json
nsys stats --report cuda_gpu_kern_sum /tmp/p.nsys-rep
```

## 7. Environnement

- GPU : RTX 5070, sm_120. CUDA 13.3 à `/usr/local/cuda/bin` → `export
  PATH=/usr/local/cuda/bin:$PATH`.
- Build GPU : `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build
  build -j$(nproc)`.
- `nvcc` requis pour le build GPU ; `DSP_CPU_REFERENCE=ON` pour le build CPU sans
  CUDA.
- **`ncu` (Nsight Compute) est bloqué** par `RmProfilingAdminOnly: 1`, y compris
  en `sudo` (GPU d'affichage). Se reposer sur `nsys` + `cuobjdump` + chrono A/B.
- Driver d'instrumentation de référence : `/tmp/dsp_count/count_main.c`
  (compteurs `distance_sq` / `sqrt` / `pose_attempt`, build `gcc -O2 -lm`).

## 8. Critère de succès

- 0 différence sur ≥ 5 M seeds (test §5.1) ET référence CPU identique (§5.2) ET
  `0 divergences` GPU (§5.3).
- Débit > 44 000 seeds/s (idéalement nettement, viser 60–100 k selon l'occupancy).
- `REG`/`STACK` documentés avant/après pour expliquer tout écart au gain attendu.
- Si le gain net est négatif (occupancy), documenter et soit régler les
  paramètres, soit conclure que la grille n'est pas rentable sur ce kernel.

## 9. Résultats de l'implémentation (livré)

Implémenté dans `src/worldgen/galaxy_gen.h` : une hash-grid par thread
(`pose_grid`) remplace `check_collision`. `grid_collision` ne teste que le
voisinage 3×3×3 ; `grid_insert` est appelé à chaque acceptation de pose (et pour
`poses[0]`) ; `grid_reset` est fait une fois par seed dans `generate_temp_poses`.
Tout reste en `double` ; aucun tirage PRNG ni logique accept/reject n'a changé.

Structure (indices 16 bits, clé de cellule 32 bits) :

```
int16_t head[HASH_SIZE];          /* têtes de bucket, -1 = vide          */
int16_t next[DSP_MAX_TEMP_POSES]; /* chaînage des poses d'un bucket      */
int32_t cellkey[DSP_MAX_TEMP_POSES]; /* cellule empaquetée 3×10 bits     */
```

### Exactitude (0 divergence — preuve)

- **§5.1 digest bit-à-bit, 5 000 000 seeds** (driver `/tmp/dsp_validate/hash_main.c`,
  hash FNV-1a 128 bits de `positions`/`star_seeds`/`star_types`/`star_count`,
  320 M étoiles) : AVANT == APRÈS, digest `4e8c3291f73a56d1 0d4bc0937ec71cb2`.
  **0 différence.**
- **§5.2 référence CPU** (`spectr O ∧ ocean Water`, 2 M seeds, 843 496 matchs) :
  liste de matchs bit-identique avant/après.
- **§5.3 moniteur GPU** (`--reject-sample-rate 0.05`, 2 M seeds) :
  `57729 sampled, 0 divergences`. (1 *false positive* — artefact float GPU/CPU
  pré-existant du code planète/océan aval, **identique** avec le `galaxy_gen.h`
  d'origine ; sans rapport avec la grille.)
- **§5.4 suite `tests/`** : tout passe, dont le test critique #7
  « real GPU kernel matches CPU reference ». Seul échoue
  `promised-land … seed 5457`, à cause de la **modification pré-existante du
  fixture** (`promised-land.json` a une règle `birth` ajoutée non commitée) : avec
  le fixture d'origine, le build hash-grid retrouve exactement seed 5457.

### Débit et ressources (RTX 5070, sm_120, 1 M seeds)

| Config | REG | STACK (o) | 1 M seeds | seeds/s | vs baseline |
|---|---|---|---|---|---|
| Baseline `main` (O(n²), regs=96) | 96 | 10 448 | ~23,2 s | ~43 100 | 1,00× |
| Hash-grid 256 buckets | 96 | 12 480 | 9,30 s | ~107 500 | 2,49× |
| Hash-grid 512 buckets | 96 | 12 992 | 8,50 s | ~117 600 | 2,73× |
| Hash-grid 1024 buckets | 96 | 14 016 | 7,97 s | ~125 500 | 2,91× |
| **Hash-grid 2048 buckets (livré)** | **96** | **16 064** | **~7,48 s** | **~133 700** | **3,10×** |
| Hash-grid 4096 buckets | 96 | 20 160 | 7,49 s | ~133 500 | 3,10× |
| Hash-grid 8192 buckets | 96 | 28 352 | 7,25–7,58 s | ~134 000 | ~3,1× |

`HASH_SIZE` retenu : **2048** (`DSP_GRID_HASH_BITS 11`). C'est le genou de la
courbe : au-delà, chaque doublement de `HASH_SIZE` (donc de la stack via `head[]`)
n'achète plus que < 2,5 % de débit. À 2048 on capte l'essentiel du gain avec une
stack à 16 Ko (33 % du budget de 48 Ko/thread), large marge.

Notes :
- **REG inchangé (96, plafonné).** L'occupancy est bornée par les registres et le
  shared (0), pas par la stack : ajouter ~5,6 Ko de local memory n'a pas réduit
  l'occupancy. Pas de cliff.
- Contre-intuitif : un `HASH_SIZE` **plus grand** accélère ici. Avec ≤ 256 poses,
  le filtre `cellkey` garantit le même ensemble de `distance_sq` quel que soit
  `HASH_SIZE` ; agrandir la table réduit les collisions de bucket entre les 27
  cellules voisines (moins de parcours de chaîne redondants), ce qui domine le
  surcoût du reset `O(HASH_SIZE)`/seed tant que `HASH_SIZE` reste modéré.
- L'exactitude est **indépendante de `HASH_SIZE`** (le filtre `cellkey` fixe
  l'ensemble scanné) ; le digest 5 M a été re-vérifié sur la config 2048 livrée.

**Conclusion : gain net ~3,1× (43 k → ~134 k seeds/s), résultats bit-identiques,
sans régression d'occupancy.** La grille spatiale est rentable sur ce kernel.
