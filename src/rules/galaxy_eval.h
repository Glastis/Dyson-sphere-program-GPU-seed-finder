#ifndef DSP_GALAXY_EVAL_H
#define DSP_GALAXY_EVAL_H

#include "rule_program.h"
#include "rule_eval.h"
#include "../worldgen/hd.h"
#include "../worldgen/galaxy.h"
#include "../worldgen/galaxy_gen.h"
#include "../worldgen/planet.h"
#include "../worldgen/planet_theme.h"

HD static inline int find_matching_stars(galaxy *gx, const rule_program *prog, int *out_indexes)
{
    eval_context ctx;
    int count;
    int s;

    ctx.gx = gx;
    ctx.game = &gx->game;
    ctx.spectr_ready = 0;
    count = 0;
    s = 0;
    while (s < gx->star_count)
    {
        star_system sys;

        prepare_star_system(&sys, gx, s, prog);
        if (eval_node(prog, prog->root, &sys, &ctx))
        {
            out_indexes[count] = s;
            ++count;
        }
        ++s;
    }
    return count;
}

HD static inline int seed_matches(int seed, const game_desc *base_game, const rule_program *prog)
{
    game_desc game;
    galaxy gx;
    int indexes[DSP_MAX_STARS];

    game = *base_game;
    game.seed = seed;
    generate_stars(&game, &gx);
    return find_matching_stars(&gx, prog, indexes) > 0;
}

#endif
