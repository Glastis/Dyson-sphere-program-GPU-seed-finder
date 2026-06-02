#ifndef DSP_CONST_THEMES_H
#define DSP_CONST_THEMES_H

#include "enums.h"
#include "../worldgen/hd.h"

#define THEME_PROTO_COUNT 25
#define THEME_VEIN_SLOTS 7
#define THEME_MAX_RARE 4
#define THEME_MAX_RARE_SETTINGS 16
#define THEME_MAX_GAS 2

typedef struct
{
    int id;
    const char *name;
    float wind;
    int water_item_id;
    int distribute;
    float temperature;
    int planet_type;
    int vein_spot[THEME_VEIN_SLOTS];
    float vein_count[THEME_VEIN_SLOTS];
    float vein_opacity[THEME_VEIN_SLOTS];
    int rare_vein_count;
    int rare_veins[THEME_MAX_RARE];
    float rare_settings[THEME_MAX_RARE_SETTINGS];
    int gas_count;
    int gas_items[THEME_MAX_GAS];
    float gas_speeds[THEME_MAX_GAS];
}
theme_proto;

static DSP_CONST const theme_proto THEME_PROTOS[THEME_PROTO_COUNT] = {
    { 1, "Ocean 1", 1.0f, 1000, THEME_DISTRIBUTE_BIRTH, 0.0f, PLANET_TYPE_OCEAN,
      { 7, 5, 0, 0, 8, 11, 18 }, { 0.7f, 0.6f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f }, { 0.6f, 0.5f, 0.0f, 0.0f, 0.7f, 1.0f, 1.0f },
      1, { VEIN_TYPE_SPINIFORM_STALAGMITE }, { 0.0f, 1.0f, 0.3f, 0.3f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 2, "Gas 1", 0.0f, 0, THEME_DISTRIBUTE_DEFAULT, 2.0f, PLANET_TYPE_GAS,
      { 0, 0, 0, 0, 0, 0, 0 }, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
      0, { 0, 0, 0, 0 }, { 0.0f }, 2, { 1120, 1121 }, { 0.96f, 0.04f } },
    { 3, "Gas 2", 0.0f, 0, THEME_DISTRIBUTE_DEFAULT, 1.0f, PLANET_TYPE_GAS,
      { 0, 0, 0, 0, 0, 0, 0 }, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
      0, { 0, 0, 0, 0 }, { 0.0f }, 2, { 1120, 1121 }, { 0.96f, 0.04f } },
    { 4, "Gas 3", 0.0f, 0, THEME_DISTRIBUTE_DEFAULT, -1.0f, PLANET_TYPE_GAS,
      { 0, 0, 0, 0, 0, 0, 0 }, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
      0, { 0, 0, 0, 0 }, { 0.0f }, 2, { 1011, 1120 }, { 0.7f, 0.3f } },
    { 5, "Gas 4", 0.0f, 0, THEME_DISTRIBUTE_DEFAULT, -2.0f, PLANET_TYPE_GAS,
      { 0, 0, 0, 0, 0, 0, 0 }, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
      0, { 0, 0, 0, 0 }, { 0.0f }, 2, { 1011, 1120 }, { 0.7f, 0.3f } },
    { 6, "Desert 1", 1.5f, 0, THEME_DISTRIBUTE_DEFAULT, 2.0f, PLANET_TYPE_DESERT,
      { 3, 10, 0, 6, 10, 1, 0 }, { 0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 0.3f, 0.0f }, { 0.6f, 0.6f, 0.0f, 1.0f, 1.0f, 0.3f, 0.0f },
      1, { VEIN_TYPE_KIMBERLITE }, { 0.0f, 0.18f, 0.2f, 0.3f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 7, "Desert 2", 0.4f, 0, THEME_DISTRIBUTE_DEFAULT, -1.0f, PLANET_TYPE_DESERT,
      { 7, 2, 7, 3, 8, 1, 0 }, { 1.0f, 0.5f, 1.0f, 1.0f, 0.7f, 0.3f, 0.0f }, { 0.6f, 0.6f, 1.0f, 1.0f, 0.5f, 0.3f, 0.0f },
      2, { VEIN_TYPE_FIREICE, VEIN_TYPE_FRACTAL }, { 0.3f, 0.5f, 0.7f, 0.5f, 0.0f, 0.3f, 0.2f, 0.6f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 8, "Ocean 2", 1.0f, 1000, THEME_DISTRIBUTE_INTERSTELLAR, 0.0f, PLANET_TYPE_OCEAN,
      { 7, 2, 12, 0, 4, 10, 22 }, { 0.6f, 0.3f, 0.9f, 0.0f, 0.8f, 1.0f, 1.0f }, { 0.6f, 0.6f, 0.6f, 0.0f, 0.5f, 1.0f, 1.0f },
      2, { VEIN_TYPE_SPINIFORM_STALAGMITE, VEIN_TYPE_ORGANIC }, { 0.0f, 1.0f, 0.3f, 1.0f, 0.0f, 0.5f, 0.2f, 1.0f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 9, "Lava 1", 0.7f, -1, THEME_DISTRIBUTE_DEFAULT, 5.0f, PLANET_TYPE_VOLCANO,
      { 15, 15, 2, 9, 4, 2, 0 }, { 1.0f, 1.0f, 0.6f, 1.0f, 0.6f, 0.3f, 0.0f }, { 1.0f, 1.0f, 0.6f, 1.0f, 0.5f, 0.3f, 0.0f },
      3, { VEIN_TYPE_KIMBERLITE, VEIN_TYPE_FRACTAL, VEIN_TYPE_GRATING },
      { 0.0f, 0.2f, 0.6f, 0.7f, 0.0f, 0.2f, 0.6f, 0.7f, 0.0f, 0.1f, 0.2f, 0.8f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 10, "Ice 1", 0.7f, 1000, THEME_DISTRIBUTE_DEFAULT, -5.0f, PLANET_TYPE_ICE,
      { 5, 1, 3, 10, 2, 1, 0 }, { 0.6f, 0.2f, 0.8f, 1.0f, 0.8f, 0.2f, 0.0f }, { 1.0f, 0.5f, 1.0f, 1.0f, 1.0f, 0.3f, 0.0f },
      3, { VEIN_TYPE_FIREICE, VEIN_TYPE_FRACTAL, VEIN_TYPE_GRATING },
      { 0.3f, 1.0f, 0.8f, 1.0f, 0.0f, 0.2f, 0.6f, 0.4f, 0.0f, 0.1f, 0.2f, 0.4f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 11, "Desert 3", 0.0f, 0, THEME_DISTRIBUTE_DEFAULT, -2.0f, PLANET_TYPE_DESERT,
      { 3, 3, 3, 6, 12, 0, 0 }, { 0.5f, 0.5f, 0.5f, 1.0f, 1.2f, 0.0f, 0.0f }, { 0.6f, 0.6f, 0.9f, 0.9f, 1.5f, 0.0f, 0.0f },
      4, { VEIN_TYPE_FIREICE, VEIN_TYPE_KIMBERLITE, VEIN_TYPE_GRATING, VEIN_TYPE_ORGANIC },
      { 0.25f, 0.5f, 0.6f, 0.8f, 0.0f, 0.2f, 0.6f, 0.7f, 0.0f, 0.2f, 0.3f, 0.7f, 0.1f, 0.2f, 0.2f, 0.7f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 12, "Desert 4", 0.8f, 0, THEME_DISTRIBUTE_DEFAULT, 1.0f, PLANET_TYPE_DESERT,
      { 2, 7, 8, 0, 7, 3, 0 }, { 0.4f, 1.0f, 1.0f, 0.0f, 1.0f, 0.7f, 0.0f }, { 0.8f, 1.0f, 1.0f, 0.0f, 1.0f, 0.7f, 0.0f },
      3, { VEIN_TYPE_KIMBERLITE, VEIN_TYPE_FRACTAL, VEIN_TYPE_GRATING },
      { 0.0f, 0.25f, 0.6f, 0.6f, 0.0f, 0.25f, 0.6f, 0.6f, 0.0f, 0.1f, 0.2f, 0.5f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 13, "Volcanic 1", 0.8f, 1116, THEME_DISTRIBUTE_INTERSTELLAR, 4.0f, PLANET_TYPE_VOLCANO,
      { 10, 10, 2, 7, 4, 1, 0 }, { 1.0f, 1.0f, 0.6f, 1.0f, 0.6f, 0.3f, 0.0f }, { 1.0f, 1.0f, 0.6f, 1.0f, 0.5f, 0.3f, 0.0f },
      0, { 0, 0, 0, 0 }, { 0.0f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 14, "Ocean 3", 1.0f, 1000, THEME_DISTRIBUTE_INTERSTELLAR, 0.0f, PLANET_TYPE_OCEAN,
      { 4, 6, 0, 0, 10, 8, 12 }, { 0.7f, 0.7f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f }, { 0.5f, 0.6f, 0.0f, 0.0f, 0.8f, 1.0f, 1.0f },
      3, { VEIN_TYPE_KIMBERLITE, VEIN_TYPE_SPINIFORM_STALAGMITE, VEIN_TYPE_ORGANIC },
      { 0.0f, 0.4f, 0.3f, 0.5f, 0.0f, 1.0f, 0.3f, 0.8f, 0.0f, 0.5f, 0.2f, 0.8f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 15, "Ocean 4", 1.1f, 1000, THEME_DISTRIBUTE_INTERSTELLAR, 0.0f, PLANET_TYPE_OCEAN,
      { 7, 4, 7, 1, 2, 7, 18 }, { 0.7f, 0.6f, 0.7f, 0.4f, 0.5f, 1.0f, 1.0f }, { 0.6f, 0.5f, 0.6f, 0.5f, 0.7f, 1.0f, 1.2f },
      2, { VEIN_TYPE_SPINIFORM_STALAGMITE, VEIN_TYPE_ORGANIC }, { 0.0f, 1.0f, 0.3f, 1.0f, 0.0f, 0.5f, 0.2f, 1.0f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 16, "Ocean 5", 1.1f, 1000, THEME_DISTRIBUTE_INTERSTELLAR, 0.0f, PLANET_TYPE_OCEAN,
      { 0, 0, 0, 0, 0, 2, 10 }, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 5.0f }, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 2.0f },
      1, { VEIN_TYPE_ORGANIC }, { 1.0f, 1.0f, 1.0f, 0.9f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 17, "Desert 5", 1.1f, 0, THEME_DISTRIBUTE_DEFAULT, 1.0f, PLANET_TYPE_DESERT,
      { 2, 8, 9, 1, 3, 1, 0 }, { 1.0f, 0.8f, 0.8f, 1.0f, 0.7f, 0.3f, 0.0f }, { 0.6f, 0.6f, 1.0f, 1.0f, 0.5f, 0.3f, 0.0f },
      2, { VEIN_TYPE_KIMBERLITE, VEIN_TYPE_GRATING }, { 0.0f, 0.7f, 0.7f, 0.5f, 0.0f, 0.1f, 0.2f, 0.7f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 18, "Ocean 6", 1.0f, 1000, THEME_DISTRIBUTE_INTERSTELLAR, 0.0f, PLANET_TYPE_OCEAN,
      { 5, 6, 8, 0, 4, 8, 22 }, { 0.6f, 0.5f, 0.8f, 0.0f, 0.8f, 1.0f, 1.0f }, { 0.6f, 0.6f, 0.6f, 0.0f, 0.5f, 1.0f, 1.0f },
      2, { VEIN_TYPE_SPINIFORM_STALAGMITE, VEIN_TYPE_ORGANIC }, { 0.0f, 1.0f, 0.3f, 1.0f, 0.0f, 0.5f, 0.2f, 1.0f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 19, "Desert 6", 1.6f, 0, THEME_DISTRIBUTE_INTERSTELLAR, 1.0f, PLANET_TYPE_DESERT,
      { 2, 8, 2, 7, 4, 1, 0 }, { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.0f }, { 0.6f, 0.8f, 1.0f, 0.8f, 0.6f, 0.3f, 0.0f },
      3, { VEIN_TYPE_KIMBERLITE, VEIN_TYPE_FRACTAL, VEIN_TYPE_GRATING },
      { 0.0f, 0.25f, 0.6f, 0.6f, 0.0f, 0.25f, 0.6f, 0.6f, 0.0f, 0.4f, 0.3f, 0.9f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 20, "Desert 7", 0.7f, -2, THEME_DISTRIBUTE_DEFAULT, -2.0f, PLANET_TYPE_DESERT,
      { 5, 11, 1, 8, 3, 1, 0 }, { 0.8f, 1.0f, 0.5f, 1.0f, 0.7f, 0.3f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.3f, 0.0f },
      3, { VEIN_TYPE_FIREICE, VEIN_TYPE_KIMBERLITE, VEIN_TYPE_GRATING },
      { 0.25f, 1.0f, 0.6f, 0.7f, 0.0f, 0.2f, 0.6f, 0.9f, 0.0f, 0.3f, 0.4f, 1.0f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 21, "Gas 5", 0.0f, 0, THEME_DISTRIBUTE_INTERSTELLAR, 1.0f, PLANET_TYPE_GAS,
      { 0, 0, 0, 0, 0, 0, 0 }, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
      0, { 0, 0, 0, 0 }, { 0.0f }, 2, { 1120, 1121 }, { 0.84f, 0.16f } },
    { 22, "Desert 8", 1.1f, 1000, THEME_DISTRIBUTE_INTERSTELLAR, 0.0f, PLANET_TYPE_OCEAN,
      { 7, 4, 7, 2, 3, 6, 14 }, { 0.7f, 0.6f, 1.0f, 0.8f, 0.7f, 1.0f, 1.0f }, { 0.7f, 0.6f, 0.8f, 0.7f, 1.0f, 1.2f, 1.0f },
      2, { VEIN_TYPE_SPINIFORM_STALAGMITE, VEIN_TYPE_ORGANIC }, { 0.0f, 1.0f, 0.5f, 1.0f, 0.0f, 0.6f, 0.25f, 1.0f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 23, "Desert 9", 1.5f, 0, THEME_DISTRIBUTE_INTERSTELLAR, 0.08f, PLANET_TYPE_DESERT,
      { 13, 2, 0, 2, 0, 2, 0 }, { 1.0f, 0.5f, 0.0f, 0.7f, 0.0f, 0.6f, 0.0f }, { 1.2f, 0.8f, 0.0f, 1.0f, 0.0f, 0.5f, 0.0f },
      2, { VEIN_TYPE_SPINIFORM_STALAGMITE, VEIN_TYPE_GRATING }, { 0.0f, 0.7f, 0.2f, 0.6f, 0.0f, 1.0f, 1.0f, 0.84f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 24, "Desert 10", 1.3f, 0, THEME_DISTRIBUTE_DEFAULT, -4.0f, PLANET_TYPE_ICE,
      { 9, 2, 2, 6, 2, 1, 0 }, { 0.8f, 0.5f, 0.8f, 1.0f, 0.7f, 0.3f, 0.0f }, { 0.8f, 0.8f, 1.2f, 1.0f, 1.0f, 0.3f, 0.0f },
      3, { VEIN_TYPE_FIREICE, VEIN_TYPE_KIMBERLITE, VEIN_TYPE_GRATING },
      { 0.3f, 1.0f, 0.8f, 1.0f, 0.0f, 1.0f, 0.7f, 1.0f, 0.0f, 0.4f, 0.5f, 0.7f }, 0, { 0, 0 }, { 0.0f, 0.0f } },
    { 25, "Desert 11", 1.0f, 0, THEME_DISTRIBUTE_INTERSTELLAR, 0.0f, PLANET_TYPE_OCEAN,
      { 8, 3, 8, 1, 3, 9, 20 }, { 0.7f, 0.6f, 1.0f, 1.0f, 0.6f, 1.0f, 1.0f }, { 0.7f, 0.5f, 1.0f, 1.0f, 0.7f, 1.2f, 1.0f },
      3, { VEIN_TYPE_FRACTAL, VEIN_TYPE_SPINIFORM_STALAGMITE, VEIN_TYPE_ORGANIC },
      { 0.0f, 0.5f, 0.3f, 1.0f, 0.0f, 1.0f, 0.3f, 1.0f, 0.0f, 0.5f, 0.2f, 1.0f }, 0, { 0, 0 }, { 0.0f, 0.0f } }
};

#endif
