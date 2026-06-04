#ifndef DSP_ENUMS_H
#define DSP_ENUMS_H

enum star_type
{
    STAR_TYPE_MAIN_SEQ = 0,
    STAR_TYPE_GIANT = 1,
    STAR_TYPE_WHITE_DWARF = 2,
    STAR_TYPE_NEUTRON_STAR = 3,
    STAR_TYPE_BLACK_HOLE = 4
};

enum spectr_type
{
    SPECTR_TYPE_M = -4,
    SPECTR_TYPE_K = -3,
    SPECTR_TYPE_G = -2,
    SPECTR_TYPE_F = -1,
    SPECTR_TYPE_A = 0,
    SPECTR_TYPE_B = 1,
    SPECTR_TYPE_O = 2,
    SPECTR_TYPE_X = 3
};

enum planet_type
{
    PLANET_TYPE_NONE = 0,
    PLANET_TYPE_VOLCANO = 1,
    PLANET_TYPE_OCEAN = 2,
    PLANET_TYPE_DESERT = 3,
    PLANET_TYPE_ICE = 4,
    PLANET_TYPE_GAS = 5
};

enum theme_distribute
{
    THEME_DISTRIBUTE_DEFAULT = 0,
    THEME_DISTRIBUTE_BIRTH = 1,


    THEME_DISTRIBUTE_INTERSTELLAR = 2,
    THEME_DISTRIBUTE_RARE = 3
};

enum vein_type
{
    VEIN_TYPE_NONE = 0,
    VEIN_TYPE_IRON = 1,
    VEIN_TYPE_COPPER = 2,
    VEIN_TYPE_SILICIUM = 3,
    VEIN_TYPE_TITANIUM = 4,
    VEIN_TYPE_STONE = 5,
    VEIN_TYPE_COAL = 6,
    VEIN_TYPE_OIL = 7,
    VEIN_TYPE_FIREICE = 8,
    VEIN_TYPE_KIMBERLITE = 9,
    VEIN_TYPE_FRACTAL = 10,
    VEIN_TYPE_SPINIFORM_STALAGMITE = 11,
    VEIN_TYPE_GRATING = 12,
    VEIN_TYPE_ORGANIC = 13,
    VEIN_TYPE_UNIPOLAR_MAGNET = 14,
    VEIN_TYPE_MAX = 15
};

enum ocean_type
{
    OCEAN_TYPE_NONE = 0,
    OCEAN_TYPE_ICE = -2,
    OCEAN_TYPE_LAVA = -1,
    OCEAN_TYPE_WATER = 1000,
    OCEAN_TYPE_SULFUR = 1116
};

enum gas_type
{
    GAS_TYPE_NONE = 0,
    GAS_TYPE_FIREICE = 1011,
    GAS_TYPE_HYDROGEN = 1120,
    GAS_TYPE_DEUTERIUM = 1121
};

/* The game labels gases and oceans with sparse / negative item ids (the enums
 * above -- the "dumb" ids that come straight from the game). Internally we use a
 * dense 0..N index instead, so anything that needs a lookup can just index an
 * array. The dense index is the project-side identity; the game id is recovered
 * only when we read generated game data, via the DSP_GAS_ID / DSP_OCEAN_ID link
 * tables. Keep these enums, those link tables and the name tables in lockstep. */
enum dsp_gas
{
    DSP_GAS_NONE = 0,
    DSP_GAS_FIREICE,
    DSP_GAS_HYDROGEN,
    DSP_GAS_DEUTERIUM,
    DSP_GAS_COUNT
};

enum dsp_ocean
{
    DSP_OCEAN_NONE = 0,
    DSP_OCEAN_ICE,
    DSP_OCEAN_LAVA,
    DSP_OCEAN_WATER,
    DSP_OCEAN_SULFUR,
    DSP_OCEAN_COUNT
};

#endif
