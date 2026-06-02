#ifndef DSP_OUTPUT_WRITER_H
#define DSP_OUTPUT_WRITER_H

#include <stdio.h>
#include "../worldgen/game_desc.h"
#include "../worldgen/galaxy.h"
#include "../rules/rule_program.h"

typedef struct
{
    int seed;
    int indexes[DSP_MAX_STARS];
    int index_count;
}
match_record;

void output_begin(FILE *out, int format);
void output_record(FILE *out, int format, const match_record *rec,
                   const game_desc *base_game, const rule_program *prog);
void output_end(FILE *out, int format);

#endif
