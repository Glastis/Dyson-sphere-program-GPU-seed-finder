/* Isolated-vs-nested consistency test for the eval-order bug, end to end.
 *
 * It runs the REAL rule engine on the same galaxy two ways, with genuinely
 * different traversal orders:
 *   ROOT  = nine.json        : the 9-resource `and` as a root rule. Stars are
 *           evaluated in index order; find_matching_stars returns the set R of
 *           stars that satisfy it.
 *   NEST  = nine-prox.json   : proximity(birth, <same 9-resource and>) with an
 *           effectively infinite max distance. The 9-resource system is found by
 *           the neighbour walk (a different order), and is reported as P[1].
 *
 * Invariants that must hold for every seed once evaluation is order-independent:
 *   1. NEST matches  <=>  R is non-empty            (existence agrees)
 *   2. when NEST matches, its reported 9-res system P[1] is in R   (verdict agrees)
 * Before the fix the neighbour walk accumulated a different habitable_count than
 * the in-order root pass, so a star could pass the subrule in one context and
 * fail it in the other -- exactly the seed-7526 / star-44 contradiction.
 *
 * Build: gcc -O2 -Isrc tests/reference/consistency_test.c \
 *            src/json/conditions.c src/json/json_value.c -lm
 * Run:   ./a.out <nine.json> <nine-prox.json> [num_seeds]
 */
#include <stdio.h>
#include <stdlib.h>
#include "../../src/json/conditions.h"
#include "../../src/worldgen/galaxy_gen.h"
#include "../../src/rules/galaxy_eval.h"

int main(int argc, char **argv)
{
    const char *root_path = argc > 1 ? argv[1] : "tests/fixtures/nine.json";
    const char *nest_path = argc > 2 ? argv[2] : "tests/fixtures/nine-prox.json";
    long num_seeds = argc > 3 ? atol(argv[3]) : 100000;
    game_desc game_root;
    game_desc game_nest;
    rule_program prog_root;
    rule_program prog_nest;
    char err[CONDITIONS_ERR_SIZE];
    long seed;
    long long nest_matches = 0;
    long long root_nonempty = 0;
    long long existence_mismatch = 0;
    long long verdict_mismatch = 0;

    if (parse_conditions_file(root_path, &game_root, &prog_root, err, sizeof(err)) != 0)
    {
        fprintf(stderr, "parse error (%s): %s\n", root_path, err);
        return 2;
    }
    if (parse_conditions_file(nest_path, &game_nest, &prog_nest, err, sizeof(err)) != 0)
    {
        fprintf(stderr, "parse error (%s): %s\n", nest_path, err);
        return 2;
    }
    fprintf(stderr, "consistency: %ld seeds, ROOT=%s vs NEST=%s\n", num_seeds, root_path, nest_path);

    for (seed = 0; seed < num_seeds; ++seed)
    {
        game_desc g;
        galaxy gx;
        int root_idx[DSP_MAX_STARS];
        int nest_idx[DSP_MAX_STARS];
        int root_n;
        int nest_n;
        int nest_hit;
        int root_has;

        /* ROOT and NEST share the same game block / galaxy; evaluate both. */
        g = game_root;
        g.seed = (int)seed;
        generate_stars(&g, &gx);
        root_n = find_matching_stars(&gx, &prog_root, root_idx);

        g = game_nest;
        g.seed = (int)seed;
        generate_stars(&g, &gx);
        nest_n = find_matching_stars(&gx, &prog_nest, nest_idx);

        /* The proximity anchor is star 0 (birth) and cannot be its own
         * neighbour, so the existence invariant compares against "R has a star
         * other than 0", not "R non-empty". */
        {
            int i;
            root_has = 0;
            for (i = 0; i < root_n; ++i)
            {
                if (root_idx[i] != 0)
                {
                    root_has = 1;
                    break;
                }
            }
        }
        nest_hit = nest_n > 0;
        if (root_n > 0)
        {
            ++root_nonempty;
        }
        if (nest_hit)
        {
            ++nest_matches;
        }

        /* Invariant 1: existence agrees (modulo the anchor-exclusion above). */
        if (nest_hit != root_has)
        {
            ++existence_mismatch;
            if (existence_mismatch <= 10)
            {
                fprintf(stderr, "  EXISTENCE MISMATCH seed=%ld root_has=%d nest_hit=%d\n",
                        seed, root_has, nest_hit);
            }
        }

        /* Invariant 2: the nested neighbour's 9-res system is in the root set. */
        if (nest_hit)
        {
            int neighbour;
            int in_root;
            int i;

            neighbour = nest_idx[1]; /* [anchor=birth, neighbour=9-res] */
            in_root = 0;
            for (i = 0; i < root_n; ++i)
            {
                if (root_idx[i] == neighbour)
                {
                    in_root = 1;
                    break;
                }
            }
            if (!in_root)
            {
                ++verdict_mismatch;
                if (verdict_mismatch <= 10)
                {
                    fprintf(stderr, "  VERDICT MISMATCH seed=%ld nested 9-res star=%d not in root set\n",
                            seed, neighbour);
                }
            }
        }

        if ((seed % 20000) == 0 && seed > 0)
        {
            fprintf(stderr, "  ... %ld seeds | root_nonempty=%lld nest_matches=%lld "
                            "existence_mm=%lld verdict_mm=%lld\n",
                    seed, root_nonempty, nest_matches, existence_mismatch, verdict_mismatch);
        }
    }

    printf("seeds=%ld root_nonempty=%lld nest_matches=%lld existence_mismatch=%lld verdict_mismatch=%lld\n",
           num_seeds, root_nonempty, nest_matches, existence_mismatch, verdict_mismatch);
    if (existence_mismatch == 0 && verdict_mismatch == 0)
    {
        printf("CONSISTENCY PASSED: isolated == nested for all %ld seeds\n", num_seeds);
        return 0;
    }
    printf("CONSISTENCY FAILED\n");
    return 1;
}
