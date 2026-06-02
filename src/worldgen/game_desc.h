#ifndef DSP_GAME_DESC_H
#define DSP_GAME_DESC_H

#include "hd.h"

typedef struct
{
    int seed;
    int star_count;
    float resource_multiplier;
    double hive_initial_colonize;
    double hive_max_density;
}
game_desc;

HD static inline int game_is_infinite_resource(const game_desc *game)
{
    return game->resource_multiplier >= 99.5f;
}

HD static inline int game_is_rare_resource(const game_desc *game)
{
    return game->resource_multiplier <= 0.1001f;
}

HD static inline float game_oil_amount_multiplier(const game_desc *game)
{
    return game_is_rare_resource(game) ? 0.5f : 1.0f;
}

HD static inline float game_gas_coef(const game_desc *game)
{
    return game_is_rare_resource(game) ? 0.8f : 1.0f;
}

#endif
