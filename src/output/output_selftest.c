#include <stdio.h>
#include "writer.h"
#include "../cli/config.h"
#include "../rules/rule_program.h"
#include "../worldgen/game_desc.h"

static void build_birth_program(rule_program *prog)
{
    rule_node *node;

    node = &prog->nodes[0];
    node->kind = RULE_BIRTH;
    node->priority = 0;
    node->cond.op = COND_EQ;
    node->cond.value = 0.0f;
    node->cond.max = 0.0f;
    node->cond2 = node->cond;
    node->value_count = 0;
    node->vein = RULE_FLAG_NONE;
    node->ocean_type = RULE_FLAG_NONE;
    node->gas_type = RULE_FLAG_NONE;
    node->spectr = RULE_FLAG_NONE;
    node->flag = RULE_FLAG_NONE;
    node->child_count = 0;
    prog->node_count = 1;
    prog->root = 0;
    prog->needs_planets = 0;
    prog->needs_themes = 0;
}

static void build_default_game(game_desc *game)
{
    game->seed = 0;
    game->star_count = 64;
    game->resource_multiplier = 1.0f;
    game->hive_initial_colonize = 1.0;
    game->hive_max_density = 1.0;
}

static void run_format(int format, const match_record *rec, const game_desc *game, const rule_program *prog)
{
    output_begin(stdout, format);
    output_record(stdout, format, rec, game, prog);
    output_end(stdout, format);
}

int main(void)
{
    rule_program prog;
    game_desc game;
    match_record rec;

    build_birth_program(&prog);
    build_default_game(&game);
    rec.seed = 0;
    rec.indexes[0] = 0;
    rec.index_count = 1;

    printf("=== TEXT ===\n");
    run_format(OUTPUT_FORMAT_TEXT, &rec, &game, &prog);
    printf("=== JSON ===\n");
    run_format(OUTPUT_FORMAT_JSON, &rec, &game, &prog);
    printf("=== CSV ===\n");
    run_format(OUTPUT_FORMAT_CSV, &rec, &game, &prog);
    return 0;
}
