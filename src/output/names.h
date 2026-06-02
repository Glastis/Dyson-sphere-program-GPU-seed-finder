#ifndef DSP_OUTPUT_NAMES_H
#define DSP_OUTPUT_NAMES_H

#include "../constants/enums.h"

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

static inline const char *star_type_name(int star_type)
{
    if (star_type < 0 || star_type > 4)
    {
        return "Unknown";
    }
    return STAR_TYPE_NAMES[star_type];
}

static inline const char *planet_type_name(int planet_type)
{
    if (planet_type < 0 || planet_type > 5)
    {
        return "Unknown";
    }
    return PLANET_TYPE_NAMES[planet_type];
}

static inline const char *vein_type_name(int vein_type)
{
    if (vein_type < 0 || vein_type > 15)
    {
        return "Unknown";
    }
    return VEIN_TYPE_NAMES[vein_type];
}

static inline const char *spectr_type_name(int spectr)
{
    int index;

    index = spectr + 4;
    if (index < 0 || index > 7)
    {
        return "Unknown";
    }
    return SPECTR_TYPE_NAMES[index];
}

#endif
