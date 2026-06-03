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
    /* Frozen, order-independent input to the habitable-ocean test: for star i,
     * habitable_prefix[i] is the canonical number of habitable (ocean) planets
     * placed in stars 0..i-1 by the ordered pass `galaxy_load_types`. Reading
     * this instead of a running, traversal-order-dependent counter makes every
     * star's planet-type verdict self-contained. */
    int habitable_prefix[DSP_MAX_STARS];
}
galaxy;

#endif
