#ifndef DSP_ENGINE_H
#define DSP_ENGINE_H

#include "../cli/config.h"
#include "../worldgen/game_desc.h"
#include "../rules/rule_program.h"

int run_search(const cli_config *cfg, const game_desc *game, const rule_program *prog);

#endif
