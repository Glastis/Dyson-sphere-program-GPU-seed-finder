#ifndef DSP_VERIFIER_H
#define DSP_VERIFIER_H

#include <string.h>
#include "../worldgen/game_desc.h"
#include "../worldgen/galaxy.h"
#include "../worldgen/galaxy_gen.h"
#include "../rules/rule_program.h"
#include "../rules/galaxy_eval.h"
#include "../output/writer.h"

static inline int verify_seed(int seed, const game_desc *base_game, const rule_program *prog, match_record *out)
{
    game_desc game;
    galaxy gx;
    int indexes[DSP_MAX_STARS];
    int count;

    game = *base_game;
    game.seed = seed;
    generate_stars(&game, &gx);
    count = find_matching_stars(&gx, prog, indexes);
    out->seed = seed;
    out->index_count = count;
    memcpy(out->indexes, indexes, (size_t)count * sizeof(int));
    return count > 0;
}

#endif
