#ifndef DSP_CONST_VEIN_GEN_H
#define DSP_CONST_VEIN_GEN_H

#include "enums.h"
#include "../worldgen/hd.h"

#define VEIN_GIANT_P 2.5f
#define VEIN_WHITE_DWARF_P 3.5f
#define VEIN_NEUTRON_P 4.5f
#define VEIN_BLACK_HOLE_P 5.0f
#define VEIN_MAIN_SEQ_DEFAULT_P 1.0f

#define VEIN_DISCARD_DRAWS 6
#define VEIN_ADD_UNTIL_ROUNDS 11

typedef struct
{
    int spectr;
    float p;
}
vein_p_entry;

#define VEIN_MAIN_SEQ_P_COUNT 5

static DSP_CONST const vein_p_entry VEIN_MAIN_SEQ_P[VEIN_MAIN_SEQ_P_COUNT] = {
    { SPECTR_TYPE_M, 2.5f },
    { SPECTR_TYPE_G, 0.7f },
    { SPECTR_TYPE_F, 0.6f },
    { SPECTR_TYPE_B, 0.4f },
    { SPECTR_TYPE_O, 1.6f }
};

typedef struct
{
    int index;
    int increment;
    float threshold;
    float count;
    float opacity;
}
vein_mod;

#define WHITE_DWARF_MOD_COUNT 3

static DSP_CONST const vein_mod WHITE_DWARF_MODS[WHITE_DWARF_MOD_COUNT] = {
    { 9, 2, 0.45f, 0.7f, 1.0f },
    { 10, 2, 0.45f, 0.7f, 1.0f },
    { 12, 1, 0.5f, 0.7f, 0.3f }
};

static DSP_CONST const vein_mod DEGENERATE_MOD = { 14, 1, 0.65f, 0.7f, 0.3f };

#endif
