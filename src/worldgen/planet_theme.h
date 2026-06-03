#ifndef DSP_PLANET_THEME_H
#define DSP_PLANET_THEME_H

#include "hd.h"
#include "planet.h"
#include "planet_props.h"
#include "star.h"
#include "galaxy.h"
#include "dsp_math.h"
#include "../constants/enums.h"
#include "../constants/themes.h"
#include "../constants/vein_gen.h"
#include "../constants/planet_gen.h"
#include <math.h>

HD static inline int planet_habitable_ocean_check(star_system *sys, int pidx, galaxy *gx, int habitable_count)
{
    int star_count;
    float num18;
    double num19;
    float a;
    float num24;
    float num25;

    star_count = gx->game.star_count;
    num18 = fmaxf(ceilf((float)star_count * 0.29f), 11.0f);
    num19 = (double)num18 - (double)habitable_count;
    a = (float)(num19 / (double)(float)(star_count - sys->st.index));
    num24 = clampf(a + (0.35f - a) * 0.5f, 0.08f, 0.8f);
    num25 = powf(clampf(planet_habitable_bias(sys, pidx) / num24, 0.0f, 1.1f), num24 * 10.0f);
    return sys->planets[pidx].habitable_factor > (double)num25;
}

HD static inline int planet_solid_type(const star_system *sys, int pidx)
{
    const planet *p;
    float f2;

    p = &sys->planets[pidx];
    f2 = planet_temperature_factor(sys, pidx);
    if (f2 < 5.0f / 6.0f)
    {
        double num26;

        num26 = fmax((double)f2 * 2.5 - 0.85, 0.15);
        return p->type_factor >= num26 ? PLANET_TYPE_VOLCANO : PLANET_TYPE_DESERT;
    }
    if (f2 < 1.2f)
    {
        return PLANET_TYPE_DESERT;
    }
    return p->type_factor >= (0.9 / (double)f2 - 0.1) ? PLANET_TYPE_ICE : PLANET_TYPE_DESERT;
}

HD static inline int planet_unmodified_type(star_system *sys, int pidx, galaxy *gx, int *habitable_count)
{
    planet *p;

    p = &sys->planets[pidx];
    if (p->gas_giant)
    {
        return PLANET_TYPE_GAS;
    }
    if (planet_is_birth(sys, pidx))
    {
        *habitable_count += 1;
        return PLANET_TYPE_OCEAN;
    }
    if (!star_is_birth(&sys->st) && planet_habitable_ocean_check(sys, pidx, gx, *habitable_count))
    {
        *habitable_count += 1;
        return PLANET_TYPE_OCEAN;
    }
    return planet_solid_type(sys, pidx);
}

/* Resolve every planet type of one star. `habitable_count` is the running tally
 * of habitable (ocean) planets seen so far; it is read by the ocean test and
 * bumped for each ocean. The caller owns it -- during rule evaluation it is a
 * local seeded from the frozen prefix (so the result is order-independent);
 * during the canonical pass it accumulates across all stars in index order. */
HD static inline void star_system_load_types(star_system *sys, galaxy *gx, int *habitable_count)
{
    int i;

    i = 0;
    while (i < sys->planet_count)
    {
        if (sys->planets[i].planet_type == -1)
        {
            sys->planets[i].planet_type = planet_unmodified_type(sys, i, gx, habitable_count);
        }
        ++i;
    }
}

/* Canonical, deterministic pass over the whole galaxy: process the stars once
 * each, in index order, resolving planet types and accumulating the habitable
 * count exactly as DSP does. It records, per star, the prefix habitable count
 * (oceans in all earlier stars) so rule evaluation can later reproduce each
 * star's verdict in isolation, and leaves gx->habitable_count at the total.
 * Must run after generate_stars and before any rule evaluation that needs
 * planet themes. One star_system lives on the stack at a time. */
HD static inline void galaxy_load_types(galaxy *gx)
{
    int index;
    int count;

    count = 0;
    index = 0;
    while (index < gx->star_count)
    {
        star_system sys;

        gx->habitable_prefix[index] = count;
        sys.st = star_init(gx, index);
        sys.planet_count = 0;
        sys.used_theme_count = 0;
        get_planets(&sys);
        star_system_load_types(&sys, gx, &count);
        ++index;
    }
    gx->habitable_count = count;
}

HD static inline int is_theme_used(const star_system *sys, int theme_id)
{
    int i;

    i = 0;
    while (i < sys->used_theme_count)
    {
        if (sys->used_theme_ids[i] == theme_id)
        {
            return 1;
        }
        ++i;
    }
    return 0;
}

HD static inline int theme_passes_filter(const star_system *sys, int pidx, const theme_proto *theme, int planet_type)
{
    float temperature_bias;
    int is_star_birth;
    int flag2;

    is_star_birth = star_is_birth(&sys->st);
    if (is_star_birth && planet_type == PLANET_TYPE_OCEAN)
    {
        return theme->distribute == THEME_DISTRIBUTE_BIRTH;
    }
    temperature_bias = planet_temperature_bias(sys, pidx);
    if (fabsf(theme->temperature) < 0.5f && theme->planet_type == PLANET_TYPE_DESERT)
    {
        flag2 = (double)fabsf(temperature_bias) < (double)fabsf(theme->temperature) + 0.1;
    }
    else
    {
        flag2 = (double)theme->temperature * (double)temperature_bias >= -0.1;
    }
    if (theme->planet_type != planet_type || !flag2)
    {
        return 0;
    }
    if (is_star_birth)
    {
        return theme->distribute == THEME_DISTRIBUTE_DEFAULT;
    }
    return theme->distribute == THEME_DISTRIBUTE_DEFAULT || theme->distribute == THEME_DISTRIBUTE_INTERSTELLAR;
}

HD static inline int collect_potential_themes(star_system *sys, int pidx, int *potential)
{
    int pcount;
    int planet_type;
    int i;

    pcount = 0;
    planet_type = sys->planets[pidx].planet_type;
    i = 0;
    while (i < THEME_PROTO_COUNT)
    {
        if (!is_theme_used(sys, THEME_PROTOS[i].id)
            && theme_passes_filter(sys, pidx, &THEME_PROTOS[i], planet_type))
        {
            potential[pcount++] = i;
        }
        ++i;
    }
    return pcount;
}

HD static inline int collect_desert_themes(star_system *sys, int *potential, int require_unused)
{
    int pcount;
    int i;

    pcount = 0;
    i = 0;
    while (i < THEME_PROTO_COUNT)
    {
        if ((!require_unused || !is_theme_used(sys, THEME_PROTOS[i].id))
            && THEME_PROTOS[i].planet_type == PLANET_TYPE_DESERT)
        {
            potential[pcount++] = i;
        }
        ++i;
    }
    return pcount;
}

HD static inline int planet_get_theme(star_system *sys, int pidx)
{
    planet *p;
    int potential[THEME_PROTO_COUNT];
    int pcount;
    int chosen;

    p = &sys->planets[pidx];
    if (p->theme_index != -1)
    {
        return p->theme_index;
    }
    pcount = collect_potential_themes(sys, pidx, potential);
    if (pcount == 0)
    {
        pcount = collect_desert_themes(sys, potential, 1);
    }
    if (pcount == 0)
    {
        pcount = collect_desert_themes(sys, potential, 0);
    }
    chosen = potential[((int)(p->theme_rand1 * (double)pcount)) % pcount];
    sys->used_theme_ids[sys->used_theme_count++] = THEME_PROTOS[chosen].id;
    p->theme_index = chosen;
    return chosen;
}

HD static inline void star_system_select_all_themes(star_system *sys)
{
    int i;

    i = 0;
    while (i < sys->planet_count)
    {
        planet_get_theme(sys, i);
        ++i;
    }
}

HD static inline int planet_get_gases(star_system *sys, int pidx, const game_desc *game, int *items, float *rates)
{
    planet *p;
    dsp_random rng;
    const theme_proto *theme;
    float gas_coef;
    float coef;
    int n;

    p = &sys->planets[pidx];
    if (!p->gas_giant)
    {
        return 0;
    }
    gas_coef = game_gas_coef(game);
    rng = prng_new(p->theme_seed);
    theme = &THEME_PROTOS[planet_get_theme(sys, pidx)];
    coef = powf(star_resource_coef(&sys->st), 0.3f);
    n = 0;
    while (n < theme->gas_count)
    {
        float num2;

        num2 = theme->gas_speeds[n] * (prng_next_f32(&rng) * 21.0f / 110.0f + 10.0f / 11.0f) * gas_coef;
        items[n] = theme->gas_items[n];
        rates[n] = num2 * coef;
        ++n;
    }
    return theme->gas_count;
}

#endif
