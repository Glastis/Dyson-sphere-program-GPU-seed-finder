#ifndef DSP_CONST_PLANET_GEN_H
#define DSP_CONST_PLANET_GEN_H

#include "enums.h"
#include "../worldgen/hd.h"

#define MAX_PLANETS_PER_STAR 6
#define MAX_VEINS_PER_PLANET 14
#define ORBIT_RADIUS_COUNT 17

#define GAS_GIANT_RADIUS 80.0f
#define GAS_GIANT_SCALE 10.0f
#define SOLID_PLANET_RADIUS 200.0f
#define SOLID_PLANET_SCALE 1.0f

#define ORBITAL_PERIOD_SATELLITE_GM 1.08308421068537e-08
#define ORBITAL_PERIOD_STAR_GM 1.35385519905204e-06

#define VEIN_AMOUNT_BASE 100000.0f
#define VEIN_AMOUNT_FLOOR 20
#define VEIN_AMOUNT_CAP 15000
#define VEIN_AMOUNT_CAP_THRESHOLD 16000

static DSP_CONST const float ORBIT_RADIUS[ORBIT_RADIUS_COUNT] = {
    0.0f, 0.4f, 0.7f, 1.0f, 1.4f, 1.9f, 2.5f, 3.3f, 4.3f, 5.5f, 6.9f, 8.4f, 10.0f, 11.7f, 13.5f, 15.4f, 17.5f
};

#define PLANET_COUNT_MAX_ENTRIES 4
#define PLANET_COUNT_RULE_COUNT 7

typedef struct
{
    double threshold;
    int count;
}
planet_count_entry;

typedef struct
{
    int spectr;
    planet_count_entry entries[PLANET_COUNT_MAX_ENTRIES];
    int entry_count;
    int fallback_count;
    int pgas_low;
    int pgas_high;
}
spectr_planet_rule;

static DSP_CONST const spectr_planet_rule SPECTR_PLANET_RULES[PLANET_COUNT_RULE_COUNT] = {
    { SPECTR_TYPE_M, { { 0.8, 4 }, { 0.3, 3 }, { 0.1, 2 } }, 3, 1, 1, 2 },
    { SPECTR_TYPE_K, { { 0.95, 5 }, { 0.7, 4 }, { 0.2, 3 }, { 0.1, 2 } }, 4, 1, 3, 4 },
    { SPECTR_TYPE_G, { { 0.9, 5 }, { 0.4, 4 } }, 2, 3, 3, 5 },
    { SPECTR_TYPE_F, { { 0.8, 5 }, { 0.35, 4 } }, 2, 3, 1, 6 },
    { SPECTR_TYPE_A, { { 0.75, 5 }, { 0.3, 4 } }, 2, 3, 1, 7 },
    { SPECTR_TYPE_B, { { 0.75, 6 }, { 0.3, 5 } }, 2, 4, 1, 8 },
    { SPECTR_TYPE_O, { { 0.5, 6 } }, 1, 5, 9, 9 }
};

static DSP_CONST const double P_GASES[10][6] = {
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
    { 0.2, 0.2, 0.0, 0.0, 0.0, 0.0 },
    { 0.0, 0.2, 0.3, 0.0, 0.0, 0.0 },
    { 0.18, 0.18, 0.0, 0.0, 0.0, 0.0 },
    { 0.0, 0.18, 0.28, 0.28, 0.0, 0.0 },
    { 0.0, 0.2, 0.3, 0.3, 0.0, 0.0 },
    { 0.0, 0.22, 0.31, 0.31, 0.0, 0.0 },
    { 0.1, 0.28, 0.3, 0.35, 0.0, 0.0 },
    { 0.1, 0.22, 0.28, 0.35, 0.35, 0.0 },
    { 0.1, 0.2, 0.25, 0.3, 0.32, 0.35 }
};

#endif
