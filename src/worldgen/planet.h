#ifndef DSP_PLANET_H
#define DSP_PLANET_H

#include "hd.h"
#include "prng.h"
#include "star.h"
#include "vein.h"
#include "dsp_math.h"
#include "../constants/enums.h"
#include "../constants/planet_gen.h"
#include "../constants/themes.h"
#include <math.h>

typedef struct
{
    int index;
    int orbit_index;
    int gas_giant;
    int seed;
    int theme_seed;
    int orbit_around;
    float radius;
    float scale;
    float orbit_longitude;
    double orbit_radius_factor;
    double orbit_inclination_factor;
    double obliquity_scale;
    double rotation_param;
    double rotation_scale;
    double habitable_factor;
    double type_factor;
    double theme_rand1;
    int planet_type;
    int theme_index;
}
planet;

typedef struct
{
    star st;
    int planet_count;
    int planets_ready;
    int used_theme_count;
    planet planets[MAX_PLANETS_PER_STAR];
    int used_theme_ids[MAX_PLANETS_PER_STAR];
}
star_system;

HD static inline void planet_init_params(planet *p, dsp_random *rng)
{
    double num3;
    double num4;
    double num8;
    double num9;
    double num10;
    double num11;

    num3 = prng_next_f64(rng);
    num4 = prng_next_f64(rng);
    p->orbit_radius_factor = num3 * (num4 - 0.5) * 0.5;
    p->orbit_inclination_factor = prng_next_f64(rng);
    p->orbit_longitude = (float)(prng_next_f64(rng) * 360.0);
    prng_next_f64(rng);
    num8 = prng_next_f64(rng);
    num9 = prng_next_f64(rng);
    p->obliquity_scale = num8 * (num9 - 0.5);
    num10 = prng_next_f64(rng);
    num11 = prng_next_f64(rng);
    p->rotation_scale = num10 * num11 * 1000.0 + 400.0;
    prng_next_f64(rng);
    p->habitable_factor = prng_next_f64(rng);
    p->type_factor = prng_next_f64(rng);
    p->theme_rand1 = prng_next_f64(rng);
    p->rotation_param = prng_next_f64(rng);
    prng_next_f64(rng);
    prng_next_f64(rng);
    prng_next_f64(rng);
    p->theme_seed = prng_next_seed(rng);
}

HD static inline planet planet_init(int info_seed, int gen_seed, int index, int orbit_index, int gas_giant)
{
    planet p;
    dsp_random rng;

    rng = prng_new(info_seed);
    planet_init_params(&p, &rng);
    p.index = index;
    p.orbit_index = orbit_index;
    p.gas_giant = gas_giant;
    p.seed = gen_seed;
    p.orbit_around = -1;
    p.radius = gas_giant ? GAS_GIANT_RADIUS : SOLID_PLANET_RADIUS;
    p.scale = gas_giant ? GAS_GIANT_SCALE : SOLID_PLANET_SCALE;
    p.planet_type = -1;
    p.theme_index = -1;
    return p;
}

HD static inline planet make_planet(dsp_random *rng, int index, int orbit_index, int gas_giant)
{
    int info_seed;
    int gen_seed;

    info_seed = prng_next_seed(rng);
    gen_seed = prng_next_seed(rng);
    return planet_init(info_seed, gen_seed, index, orbit_index, gas_giant);
}

HD static inline float planet_real_radius(const planet *p)
{
    return p->radius * p->scale;
}

HD static inline int planets_degenerate(star_system *sys, dsp_random *rng)
{
    sys->planets[0] = make_planet(rng, 0, 3, 0);
    return 1;
}

HD static inline int planets_white_dwarf(star_system *sys, dsp_random *rng, double num1, double num2)
{
    if (num1 < 0.7)
    {
        sys->planets[0] = make_planet(rng, 0, 3, 0);
        return 1;
    }
    if (num2 < 0.3)
    {
        sys->planets[0] = make_planet(rng, 0, 3, 0);
        sys->planets[1] = make_planet(rng, 1, 4, 0);
        return 2;
    }
    sys->planets[0] = make_planet(rng, 0, 4, 1);
    sys->planets[1] = make_planet(rng, 1, 1, 0);
    sys->planets[1].orbit_around = 0;
    return 2;
}

HD static inline int planets_giant(star_system *sys, dsp_random *rng, double num1, double num2, int num3)
{
    if (num1 < 0.3)
    {
        sys->planets[0] = make_planet(rng, 0, 2 + num3, 0);
        return 1;
    }
    if (num1 < 0.8)
    {
        if (num2 < 0.25)
        {
            sys->planets[0] = make_planet(rng, 0, 2 + num3, 0);
            sys->planets[1] = make_planet(rng, 1, 3 + num3, 0);
            return 2;
        }
        sys->planets[0] = make_planet(rng, 0, 3, 1);
        sys->planets[1] = make_planet(rng, 1, 1, 0);
        sys->planets[1].orbit_around = 0;
        return 2;
    }
    if (num2 < 0.15)
    {
        sys->planets[0] = make_planet(rng, 0, 2 + num3, 0);
        sys->planets[1] = make_planet(rng, 1, 3 + num3, 0);
        sys->planets[2] = make_planet(rng, 2, 4 + num3, 0);
        return 3;
    }
    if (num2 < 0.75)
    {
        sys->planets[0] = make_planet(rng, 0, 2 + num3, 0);
        sys->planets[1] = make_planet(rng, 1, 4, 1);
        sys->planets[2] = make_planet(rng, 2, 1, 0);
        sys->planets[2].orbit_around = 1;
        return 3;
    }
    sys->planets[0] = make_planet(rng, 0, 3 + num3, 1);
    sys->planets[1] = make_planet(rng, 1, 1, 0);
    sys->planets[2] = make_planet(rng, 2, 2, 0);
    sys->planets[1].orbit_around = 0;
    sys->planets[2].orbit_around = 0;
    return 3;
}

HD static inline int select_planet_count(const star *st, double num1, const double **pgas)
{
    int spectr;
    int rule_idx;

    if (star_is_birth(st))
    {
        *pgas = P_GASES[0];
        return 4;
    }
    spectr = star_spectr(st);
    rule_idx = 0;
    while (rule_idx < PLANET_COUNT_RULE_COUNT)
    {
        if (SPECTR_PLANET_RULES[rule_idx].spectr == spectr)
        {
            const spectr_planet_rule *r;
            int count;
            int e;

            r = &SPECTR_PLANET_RULES[rule_idx];
            count = r->fallback_count;
            e = 0;
            while (e < r->entry_count)
            {
                if (num1 >= r->entries[e].threshold)
                {
                    count = r->entries[e].count;
                    break;
                }
                ++e;
            }
            *pgas = P_GASES[count <= 3 ? r->pgas_low : r->pgas_high];
            return count;
        }
        ++rule_idx;
    }
    *pgas = P_GASES[0];
    return 1;
}

HD static inline int decide_primary_gas_giant(dsp_random *rng, int is_birth, int planet_count,
                                              int index, int *num10, double num11, double pgas_val)
{
    int gas_giant;
    int broke;

    gas_giant = 0;
    if (index < planet_count - 1 && num11 < pgas_val)
    {
        gas_giant = 1;
        if (*num10 < 3)
        {
            *num10 = 3;
        }
    }
    broke = 0;
    while (!is_birth || *num10 != 3)
    {
        int num13;
        int num14;

        num13 = planet_count - index;
        num14 = 9 - *num10;
        if (num14 > num13)
        {
            float a;
            float num15;

            a = (float)num13 / (float)num14;
            num15 = a + (1.0f - a) * (*num10 <= 3 ? 0.15f : 0.45f) + 0.01f;
            if (prng_next_f64(rng) < (double)num15)
            {
                broke = 1;
                break;
            }
        }
        else
        {
            broke = 1;
            break;
        }
        *num10 += 1;
    }
    return broke ? gas_giant : 1;
}

HD static inline void apply_orbit_pairs(star_system *sys, int pairs[][2], int pair_count)
{
    int k;

    k = 0;
    while (k < pair_count)
    {
        sys->planets[pairs[k][0]].orbit_around = pairs[k][1];
        ++k;
    }
}

HD static inline int planets_main_seq(star_system *sys, dsp_random *rng, double num1)
{
    const double *pgas;
    int planet_count;
    int is_birth;
    int satellite_count;
    int orbit_around;
    int num10;
    int pairs[MAX_PLANETS_PER_STAR][2];
    int pair_count;
    int index;

    planet_count = select_planet_count(&sys->st, num1, &pgas);
    is_birth = star_is_birth(&sys->st);
    satellite_count = 0;
    orbit_around = -1;
    num10 = 1;
    pair_count = 0;
    index = 0;
    while (index < planet_count)
    {
        int info_seed;
        int gen_seed;
        double num11;
        double num12;
        int gas_giant;
        int orbit_index_value;

        info_seed = prng_next_seed(rng);
        gen_seed = prng_next_seed(rng);
        num11 = prng_next_f64(rng);
        num12 = prng_next_f64(rng);
        gas_giant = 0;
        if (orbit_around == -1)
        {
            gas_giant = decide_primary_gas_giant(rng, is_birth, planet_count, index, &num10, num11, pgas[index]);
        }
        else
        {
            satellite_count += 1;
        }
        orbit_index_value = orbit_around == -1 ? num10 : satellite_count;
        sys->planets[index] = planet_init(info_seed, gen_seed, index, orbit_index_value, gas_giant);
        if (orbit_around != -1)
        {
            pairs[pair_count][0] = index;
            pairs[pair_count][1] = orbit_around;
            ++pair_count;
        }
        num10 += 1;
        if (gas_giant)
        {
            orbit_around = index;
            satellite_count = 0;
        }
        if (satellite_count >= 1 && num12 < 0.8)
        {
            orbit_around = -1;
            satellite_count = 0;
        }
        ++index;
    }
    apply_orbit_pairs(sys, pairs, pair_count);
    return planet_count;
}

HD static inline void get_planets(star_system *sys)
{
    dsp_random rng;
    int st_type;
    double num1;
    double num2;
    int num3;

    rng = prng_new(sys->st.planets_seed);
    num1 = prng_next_f64(&rng);
    num2 = prng_next_f64(&rng);
    num3 = prng_next_f64(&rng) > 0.5 ? 1 : 0;
    prng_next_f64(&rng);
    prng_next_f64(&rng);
    prng_next_f64(&rng);
    prng_next_f64(&rng);
    sys->used_theme_count = 0;
    sys->planets_ready = 1;
    st_type = sys->st.star_type;
    if (st_type == STAR_TYPE_BLACK_HOLE || st_type == STAR_TYPE_NEUTRON_STAR)
    {
        sys->planet_count = planets_degenerate(sys, &rng);
        return;
    }
    if (st_type == STAR_TYPE_WHITE_DWARF)
    {
        sys->planet_count = planets_white_dwarf(sys, &rng, num1, num2);
        return;
    }
    if (st_type == STAR_TYPE_GIANT)
    {
        sys->planet_count = planets_giant(sys, &rng, num1, num2, num3);
        return;
    }
    sys->planet_count = planets_main_seq(sys, &rng, num1);
}

/* Lazy, idempotent planet generation. get_planets only reads star s's own
 * sub-generator (seeded from star_seeds[s]), so deferring it to the moment a
 * planet condition is evaluated changes neither s's planets nor any other star's
 * -- the result is bit-identical to generating eagerly. The planets_ready flag
 * guards against generating twice (e.g. a star tested as both anchor and a
 * proximity neighbour), which would be a needless recompute, not a wrong one. */
HD static inline void ensure_planets(star_system *sys)
{
    if (sys->planets_ready)
    {
        return;
    }
    get_planets(sys);
}

#endif

