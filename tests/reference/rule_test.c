#include <stdio.h>
#include <stdlib.h>
#include "../../src/rules/rule_program.h"
#include "../../src/rules/galaxy_eval.h"

static game_desc default_game(void)
{
    game_desc game;

    game.seed = 0;
    game.star_count = 64;
    game.resource_multiplier = 1.0f;
    game.hive_initial_colonize = 1.0;
    game.hive_max_density = 1.0;
    return game;
}

static int count_matches(const game_desc *game, const rule_program *prog, int start, int end, int *first_hits, int max_hits, int *hit_count)
{
    int total;
    int seed;

    total = 0;
    *hit_count = 0;
    seed = start;
    while (seed < end)
    {
        if (seed_matches(seed, game, prog))
        {
            if (*hit_count < max_hits)
            {
                first_hits[(*hit_count)++] = seed;
            }
            ++total;
        }
        ++seed;
    }
    return total;
}

int main(void)
{
    game_desc game;
    rule_program prog;
    int hits[10];
    int hit_count;
    int total;

    game = default_game();

    prog.node_count = 1;
    prog.root = 0;
    prog.needs_planets = 0;
    prog.needs_themes = 0;
    prog.nodes[0].kind = RULE_STAR_TYPE;
    prog.nodes[0].priority = 11;
    prog.nodes[0].value_count = 1;
    prog.nodes[0].values[0] = STAR_TYPE_BLACK_HOLE;
    prog.nodes[0].child_count = 0;
    total = count_matches(&game, &prog, 0, 2000, hits, 10, &hit_count);
    printf("starType=BlackHole over 0..2000: %d matches\n", total);

    prog.nodes[0].kind = RULE_AVERAGE_VEIN_AMOUNT;
    prog.nodes[0].priority = 51;
    prog.nodes[0].vein = VEIN_TYPE_IRON;
    prog.nodes[0].cond.op = COND_GT;
    prog.nodes[0].cond.value = 150000.0f;
    prog.needs_planets = 1;
    prog.needs_themes = 1;
    total = count_matches(&game, &prog, 0, 2000, hits, 10, &hit_count);
    printf("avgVein Iron>150000 over 0..2000: %d matches, first: ", total);
    {
        int i;

        i = 0;
        while (i < hit_count)
        {
            printf("%d ", hits[i]);
            ++i;
        }
        printf("\n");
    }

    prog.node_count = 3;
    prog.root = 0;
    prog.nodes[0].kind = RULE_AND;
    prog.nodes[0].priority = 51;
    prog.nodes[0].child_count = 2;
    prog.nodes[0].children[0] = 1;
    prog.nodes[0].children[1] = 2;
    prog.nodes[1].kind = RULE_STAR_TYPE;
    prog.nodes[1].priority = 11;
    prog.nodes[1].value_count = 2;
    prog.nodes[1].values[0] = STAR_TYPE_BLACK_HOLE;
    prog.nodes[1].values[1] = STAR_TYPE_NEUTRON_STAR;
    prog.nodes[1].child_count = 0;
    prog.nodes[2].kind = RULE_LUMINOSITY;
    prog.nodes[2].priority = 20;
    prog.nodes[2].cond.op = COND_GTE;
    prog.nodes[2].cond.value = 2.5f;
    prog.nodes[2].child_count = 0;
    prog.needs_planets = 0;
    prog.needs_themes = 0;
    total = count_matches(&game, &prog, 0, 2000, hits, 10, &hit_count);
    printf("AND(starType in {BH,NS}, lum>=2.5) over 0..2000: %d matches, first: ", total);
    {
        int i;

        i = 0;
        while (i < hit_count)
        {
            printf("%d ", hits[i]);
            ++i;
        }
        printf("\n");
    }
    return 0;
}
