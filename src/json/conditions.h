#ifndef DSP_JSON_CONDITIONS_H
#define DSP_JSON_CONDITIONS_H

#include "../worldgen/game_desc.h"
#include "../rules/rule_program.h"

#define CONDITIONS_ERR_SIZE 512

int parse_conditions_file(const char *path, game_desc *game, rule_program *prog,
                          char *err, int err_size);

int parse_conditions_text(const char *text, game_desc *game, rule_program *prog,
                          char *err, int err_size);

#endif
