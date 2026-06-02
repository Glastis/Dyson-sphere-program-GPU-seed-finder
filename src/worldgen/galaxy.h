#ifndef DSP_GALAXY_H
#define DSP_GALAXY_H

#include "hd.h"
#include "vector3.h"
#include "game_desc.h"
#include "../constants/star_gen.h"

typedef struct
{
    game_desc game;
    int star_count;
    int habitable_count;
    vec3 positions[DSP_MAX_STARS];
    int star_seeds[DSP_MAX_STARS];
    int star_types[DSP_MAX_STARS];
    int need_spectr[DSP_MAX_STARS];
}
galaxy;

#endif
