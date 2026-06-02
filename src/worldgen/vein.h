#ifndef DSP_VEIN_H
#define DSP_VEIN_H

#include "hd.h"
#include "../constants/enums.h"

typedef struct
{
    int vein_type;
    int min_group;
    int max_group;
    int min_patch;
    int max_patch;
    int min_amount;
    int max_amount;
}
vein;

HD static inline int vein_is_rare(int vein_type)
{
    return vein_type == VEIN_TYPE_FIREICE || vein_type == VEIN_TYPE_KIMBERLITE
           || vein_type == VEIN_TYPE_FRACTAL || vein_type == VEIN_TYPE_SPINIFORM_STALAGMITE
           || vein_type == VEIN_TYPE_GRATING || vein_type == VEIN_TYPE_ORGANIC;
}

#endif
