#ifndef DSP_OUTPUT_NAMES_H
#define DSP_OUTPUT_NAMES_H

#include "../constants/enums.h"

static const char *const DSP_NAME_UNKNOWN = "Unknown";

static const char *const STAR_TYPE_NAMES[5] = {
    "MainSeqStar", "GiantStar", "WhiteDwarf", "NeutronStar", "BlackHole"
};

static const char *const PLANET_TYPE_NAMES[6] = {
    "None", "Volcano", "Ocean", "Desert", "Ice", "Gas"
};

static const char *const VEIN_TYPE_NAMES[16] = {
    "None", "Iron", "Copper", "Silicium", "Titanium", "Stone", "Coal", "Oil",
    "Fireice", "Kimberlite", "Fractal", "Stalagmite", "Grating", "Organic", "Magnet", "Max"
};

static const char *const SPECTR_TYPE_NAMES[8] = {
    "M", "K", "G", "F", "A", "B", "O", "X"
};

static inline const char *star_type_name(enum star_type star_type)
{
    if ((int)star_type < 0 || star_type > STAR_TYPE_BLACK_HOLE)
    {
        return DSP_NAME_UNKNOWN;
    }
    return STAR_TYPE_NAMES[star_type];
}

static inline const char *planet_type_name(enum planet_type planet_type)
{
    if ((int)planet_type < 0 || planet_type > PLANET_TYPE_GAS)
    {
        return DSP_NAME_UNKNOWN;
    }
    return PLANET_TYPE_NAMES[planet_type];
}

static inline const char *vein_type_name(enum vein_type vein_type)
{
    if ((int)vein_type < 0 || vein_type > VEIN_TYPE_MAX)
    {
        return DSP_NAME_UNKNOWN;
    }
    return VEIN_TYPE_NAMES[vein_type];
}

static inline const char *spectr_type_name(enum spectr_type spectr)
{
    if (spectr < SPECTR_TYPE_M || spectr > SPECTR_TYPE_X)
    {
        return DSP_NAME_UNKNOWN;
    }
    return SPECTR_TYPE_NAMES[spectr - SPECTR_TYPE_M];
}

/* Gas and ocean are named off the dense project index (enum dsp_gas /
 * dsp_ocean), so the lookup is a plain array index -- the game's sparse item
 * ids never reach this table. */
static const char *const GAS_NAMES[DSP_GAS_COUNT] = {
    [DSP_GAS_NONE]      = "None",
    [DSP_GAS_FIREICE]   = "Fireice",
    [DSP_GAS_HYDROGEN]  = "Hydrogen",
    [DSP_GAS_DEUTERIUM] = "Deuterium"
};

static const char *const OCEAN_NAMES[DSP_OCEAN_COUNT] = {
    [DSP_OCEAN_NONE]   = "None",
    [DSP_OCEAN_ICE]    = "Ice",
    [DSP_OCEAN_LAVA]   = "Lava",
    [DSP_OCEAN_WATER]  = "Water",
    [DSP_OCEAN_SULFUR] = "Sulfur"
};

static inline const char *gas_type_name(enum dsp_gas gas)
{
    if ((int)gas < 0 || gas >= DSP_GAS_COUNT)
    {
        return DSP_NAME_UNKNOWN;
    }
    return GAS_NAMES[gas];
}

static inline const char *ocean_type_name(enum dsp_ocean ocean)
{
    if ((int)ocean < 0 || ocean >= DSP_OCEAN_COUNT)
    {
        return DSP_NAME_UNKNOWN;
    }
    return OCEAN_NAMES[ocean];
}

#endif
