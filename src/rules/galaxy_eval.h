#ifndef DSP_GALAXY_EVAL_H
#define DSP_GALAXY_EVAL_H

#include "rule_program.h"
#include "rule_eval.h"
#include "../worldgen/hd.h"
#include "../worldgen/galaxy.h"
#include "../worldgen/galaxy_gen.h"
#include "../worldgen/planet.h"
#include "../worldgen/planet_theme.h"

/* Scan all stars of a prepared galaxy and record every matching star index into
 * out_indexes. When stop_at_first is set the scan returns as soon as the first
 * match is found -- enough to answer "does this seed match at all?". This is
 * sound because a star's verdict depends only on itself plus the FROZEN
 * habitable prefix of earlier stars (never on later stars), so stopping early
 * cannot change whether count > 0. The full scan (stop_at_first == 0) is what
 * verify_seed needs: it must list the complete set of matching systems. */
HD static inline int scan_stars(galaxy *gx, const rule_program *prog, int *out_indexes,
                                int stop_at_first)
{
    eval_context ctx;
    int count;
    int s;
    int use_freeze;
    int running;

    ctx.gx = gx;
    ctx.game = &gx->game;
    ctx.spectr_ready = 0;
    /* The habitable-ocean test needs the canonical count of oceans in earlier
     * stars. A proximity rule jumps to arbitrary neighbour stars out of index
     * order, so it needs every prefix frozen up front (one ordered pass). Any
     * other rule visits the stars exactly once, in index order, below -- so the
     * prefix is just the running count threaded through the scan, computed for
     * free with no extra planet generation. */
    use_freeze = prog->needs_themes && prog_has_proximity(prog);
    if (use_freeze)
    {
        galaxy_load_types(gx);
    }
    count = 0;
    running = 0;
    s = 0;
    while (s < gx->star_count)
    {
        star_system sys;

        ctx.match_system_count = 0;
        if (use_freeze)
        {
            prepare_star_system(&sys, gx, s, prog);
        }
        else
        {
            prepare_star_system_seq(&sys, gx, s, prog, &running);
        }
        if (eval_node(prog, prog->root, &sys, &ctx))
        {
            if (ctx.match_system_count > 0)
            {
                /* A proximity matched: report the whole constellation (anchor
                 * + neighbours) and stop -- one representative match is enough. */
                int k;

                k = 0;
                while (k < ctx.match_system_count)
                {
                    out_indexes[count] = ctx.match_systems[k];
                    ++count;
                    ++k;
                }
                return count;
            }
            out_indexes[count] = s;
            ++count;
            if (stop_at_first)
            {
                return count;
            }
        }
        ++s;
    }
    return count;
}

/* Full scan: lists every matching system. Used by verify_seed for reporting. */
HD static inline int find_matching_stars(galaxy *gx, const rule_program *prog, int *out_indexes)
{
    return scan_stars(gx, prog, out_indexes, 0);
}

HD static inline int seed_matches(int seed, const game_desc *base_game, const rule_program *prog)
{
    game_desc game;
    galaxy gx;
    int indexes[DSP_MAX_STARS];

    game = *base_game;
    game.seed = seed;
    generate_stars(&game, &gx);
    /* Only the yes/no verdict matters here, so stop at the first match. */
    return scan_stars(&gx, prog, indexes, 1) > 0;
}

#endif
