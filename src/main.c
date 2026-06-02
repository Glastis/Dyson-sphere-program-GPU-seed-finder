#include <stdio.h>
#include <string.h>
#include "cli/cli.h"
#include "cli/config.h"
#include "json/conditions.h"
#include "verify/engine.h"
#include "worldgen/game_desc.h"
#include "constants/star_gen.h"
#include "rules/rule_program.h"

static void apply_overrides(const cli_config *cfg, game_desc *game)
{
    if (cfg->has_star_count)
    {
        game->star_count = cfg->override_star_count;
    }
    if (cfg->has_resource_mult)
    {
        game->resource_multiplier = cfg->override_resource_mult;
    }
    if (cfg->has_hive_initial)
    {
        game->hive_initial_colonize = cfg->override_hive_initial;
    }
    if (cfg->has_hive_max)
    {
        game->hive_max_density = cfg->override_hive_max;
    }
}

static int validate_game(const game_desc *game, char *err, int err_size)
{
    if (game->star_count < 1 || game->star_count > DSP_MAX_STARS)
    {
        snprintf(err, err_size, "starCount must be between 1 and %d", DSP_MAX_STARS);
        return -1;
    }
    return 0;
}

static int load_program(const cli_config *cfg, game_desc *game, rule_program *prog)
{
    char err[CONDITIONS_ERR_SIZE];

    if (parse_conditions_file(cfg->input_path, game, prog, err, CONDITIONS_ERR_SIZE) != 0)
    {
        fprintf(stderr, "error: %s\n", err);
        return -1;
    }
    apply_overrides(cfg, game);
    if (validate_game(game, err, CONDITIONS_ERR_SIZE) != 0)
    {
        fprintf(stderr, "error: %s\n", err);
        return -1;
    }
    return 0;
}

static int run_validate(const game_desc *game, const rule_program *prog)
{
    printf("valid: parsed %d rule node(s); root kind=%d; needs_planets=%d; needs_themes=%d\n",
           prog->node_count, prog->nodes[prog->root].kind, prog->needs_planets, prog->needs_themes);
    printf("game: starCount=%d resourceMultiplier=%g hiveInitialColonize=%g hiveMaxDensity=%g\n",
           game->star_count, (double)game->resource_multiplier,
           game->hive_initial_colonize, game->hive_max_density);
    return 0;
}

int main(int argc, char **argv)
{
    cli_config cfg;
    game_desc game;
    rule_program prog;
    char err[256];

    cli_defaults(&cfg);
    if (parse_cli(argc, argv, &cfg, err, sizeof(err)) != 0)
    {
        fprintf(stderr, "error: %s\n", err);
        fprintf(stderr, "try '%s --help'\n", argv[0]);
        return 2;
    }
    if (cfg.is_help)
    {
        print_help(argv[0], stdout);
        return 0;
    }
    if (cfg.input_path == NULL)
    {
        fprintf(stderr, "error: no conditions file given\n");
        fprintf(stderr, "try '%s --help'\n", argv[0]);
        return 2;
    }
    if (load_program(&cfg, &game, &prog) != 0)
    {
        return 1;
    }
    if (cfg.is_validate_only)
    {
        return run_validate(&game, &prog);
    }
    return run_search(&cfg, &game, &prog);
}
