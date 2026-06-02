#ifndef DSP_CLI_H
#define DSP_CLI_H

#include <stdio.h>

#include "config.h"

void cli_defaults(cli_config *cfg);

int parse_cli(int argc, char **argv, cli_config *cfg, char *err, int err_size);

void print_help(const char *prog_name, FILE *out);

#endif
