#ifndef DSP_CLI_CONFIG_H
#define DSP_CLI_CONFIG_H

#include "../worldgen/game_desc.h"

enum output_format
{
    OUTPUT_FORMAT_TEXT = 0,
    OUTPUT_FORMAT_JSON = 1,
    OUTPUT_FORMAT_CSV = 2
};

typedef struct
{
    const char *input_path;
    const char *output_path;
    const char *checkpoint_path;
    const char *resume_path;
    long long max_seeds;
    long long seed_start;
    long long seed_end;
    long long batch_size;
    int format;
    int threads;
    int verbose;
    int is_validate_only;
    int is_plain_progress;
    int is_help;
    double reject_sample_rate;
    double divergence_threshold;
    int has_star_count;
    int has_resource_mult;
    int has_hive_initial;
    int has_hive_max;
    int override_star_count;
    float override_resource_mult;
    double override_hive_initial;
    double override_hive_max;
}
cli_config;

#endif
