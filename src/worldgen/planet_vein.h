#ifndef DSP_PLANET_VEIN_H
#define DSP_PLANET_VEIN_H

#include "hd.h"
#include "planet.h"
#include "planet_theme.h"
#include "star.h"
#include "vein.h"
#include "dsp_math.h"
#include "../constants/enums.h"
#include "../constants/themes.h"
#include "../constants/vein_gen.h"
#include "../constants/planet_gen.h"
#include <math.h>

#define VEIN_ARRAY_SIZE 15

HD static inline void vein_add_until(dsp_random *rng, int *value, double threshold)
{
    int i;

    i = 0;
    while (i < VEIN_ADD_UNTIL_ROUNDS)
    {
        if (prng_next_f64(rng) >= threshold)
        {
            break;
        }
        *value += 1;
        ++i;
    }
}

HD static inline void vein_init_arrays(const theme_proto *theme, int *na1, float *na2, float *na3)
{
    int i;

    i = 0;
    while (i < VEIN_ARRAY_SIZE)
    {
        if (i >= 1 && i <= THEME_VEIN_SLOTS)
        {
            na1[i] = theme->vein_spot[i - 1];
            na2[i] = theme->vein_count[i - 1];
            na3[i] = theme->vein_opacity[i - 1];
        }
        else
        {
            na1[i] = 0;
            na2[i] = 0.0f;
            na3[i] = 0.0f;
        }
        ++i;
    }
}

HD static inline void vein_apply_mod(dsp_random *rng, const vein_mod *m, int *na1, float *na2, float *na3)
{
    na1[m->index] += m->increment;
    vein_add_until(rng, &na1[m->index], (double)m->threshold);
    na2[m->index] = m->count;
    na3[m->index] = m->opacity;
}

HD static inline float vein_main_seq_p(const star *st)
{
    int spectr;
    int i;

    spectr = star_spectr(st);
    i = 0;
    while (i < VEIN_MAIN_SEQ_P_COUNT)
    {
        if (VEIN_MAIN_SEQ_P[i].spectr == spectr)
        {
            return VEIN_MAIN_SEQ_P[i].p;
        }
        ++i;
    }
    return VEIN_MAIN_SEQ_DEFAULT_P;
}

HD static inline float vein_apply_star_type(star_system *sys, int *na1, float *na2, float *na3, dsp_random *rng)
{
    int st_type;

    st_type = sys->st.star_type;
    if (st_type == STAR_TYPE_MAIN_SEQ)
    {
        return vein_main_seq_p(&sys->st);
    }
    if (st_type == STAR_TYPE_GIANT)
    {
        return VEIN_GIANT_P;
    }
    if (st_type == STAR_TYPE_WHITE_DWARF)
    {
        int i;

        i = 0;
        while (i < WHITE_DWARF_MOD_COUNT)
        {
            vein_apply_mod(rng, &WHITE_DWARF_MODS[i], na1, na2, na3);
            ++i;
        }
        return VEIN_WHITE_DWARF_P;
    }
    vein_apply_mod(rng, &DEGENERATE_MOD, na1, na2, na3);
    return st_type == STAR_TYPE_NEUTRON_STAR ? VEIN_NEUTRON_P : VEIN_BLACK_HOLE_P;
}

HD static inline float vein_resource_coef(const star *st, const game_desc *game, const theme_proto *theme)
{
    float f;

    f = star_resource_coef(st);
    if (theme->distribute == THEME_DISTRIBUTE_BIRTH)
    {
        f *= 2.0f / 3.0f;
    }
    else if (game_is_rare_resource(game))
    {
        if (f > 1.0f)
        {
            f = powf(f, 0.8f);
        }
        f *= 0.7f;
    }
    return f;
}

HD static inline void vein_apply_rares(star_system *sys, const theme_proto *theme, int *na1, float *na2,
                                       float *na3, float pval, dsp_random *rng)
{
    int is_birth;
    int idx;

    is_birth = star_is_birth(&sys->st);
    idx = 0;
    while (idx < theme->rare_vein_count)
    {
        int rare_vein;
        float num2;
        float num4;
        float num5;

        rare_vein = theme->rare_veins[idx];
        num2 = theme->rare_settings[idx * 4 + (is_birth ? 0 : 1)];
        num4 = 1.0f - powf(1.0f - num2, pval);
        num5 = 1.0f - powf(1.0f - theme->rare_settings[idx * 4 + 3], pval);
        if (prng_next_f64(rng) < (double)num4)
        {
            int k;

            na1[rare_vein] += 1;
            na2[rare_vein] = num5;
            na3[rare_vein] = num5;
            k = 0;
            while (k < VEIN_ADD_UNTIL_ROUNDS)
            {
                if (prng_next_f64(rng) >= (double)theme->rare_settings[idx * 4 + 2])
                {
                    break;
                }
                na1[rare_vein] += 1;
                ++k;
            }
        }
        ++idx;
    }
}

HD static inline int vein_make_amount(int amount, int is_oil, const game_desc *game)
{
    float x1;
    float mult;
    int x2;

    x1 = roundf((float)amount * 1.1f);
    mult = is_oil ? game_oil_amount_multiplier(game) : game->resource_multiplier;
    x2 = (int)roundf(x1 * mult);
    return x2 > 1 ? x2 : 1;
}

HD static inline void vein_set_amount(vein *v, const float *na2, const float *na3,
                                      int index3, float f, const game_desc *game)
{
    int is_oil;

    is_oil = index3 == VEIN_TYPE_OIL;
    if (is_oil)
    {
        v->min_patch = 1;
        v->max_patch = 1;
    }
    else
    {
        v->min_patch = (int)roundf(na2[index3] * 20.0f);
        v->max_patch = (int)roundf(na2[index3] * 24.0f);
    }
    if (game_is_infinite_resource(game) && !is_oil)
    {
        v->min_amount = 1;
        v->max_amount = 1;
    }
    else
    {
        float num16;
        int num17;
        int num18;

        num16 = is_oil ? powf(f, 0.5f) : f;
        num17 = (int)roundf(na3[index3] * VEIN_AMOUNT_BASE * num16);
        if (num17 < VEIN_AMOUNT_FLOOR)
        {
            num17 = VEIN_AMOUNT_FLOOR;
        }
        num18 = num17 < VEIN_AMOUNT_CAP_THRESHOLD ? (int)floorf((float)num17 * (15.0f / 16.0f)) : VEIN_AMOUNT_CAP;
        v->min_amount = vein_make_amount(num17 - num18, is_oil, game);
        v->max_amount = vein_make_amount(num17 + num18, is_oil, game);
    }
}

HD static inline int vein_build(const int *na1, const float *na2, const float *na3, float f,
                                const game_desc *game, vein *out)
{
    int count;
    int index3;

    count = 0;
    index3 = 1;
    while (index3 < VEIN_ARRAY_SIZE)
    {
        if (na1[index3] > 0)
        {
            vein v;

            v.vein_type = index3;
            v.min_group = na1[index3] - 1;
            v.max_group = na1[index3] + 1;
            vein_set_amount(&v, na2, na3, index3, f, game);
            out[count++] = v;
        }
        ++index3;
    }
    return count;
}

HD static inline int planet_get_veins(star_system *sys, int pidx, const game_desc *game, vein *out)
{
    planet *p;
    dsp_random rng;
    const theme_proto *theme;
    int na1[VEIN_ARRAY_SIZE];
    float na2[VEIN_ARRAY_SIZE];
    float na3[VEIN_ARRAY_SIZE];
    float pval;
    float f;
    int discard;

    p = &sys->planets[pidx];
    if (p->gas_giant)
    {
        return 0;
    }
    rng = prng_new(p->seed);
    discard = 0;
    while (discard < VEIN_DISCARD_DRAWS)
    {
        prng_next_f64(&rng);
        ++discard;
    }
    theme = &THEME_PROTOS[planet_get_theme(sys, pidx)];
    vein_init_arrays(theme, na1, na2, na3);
    pval = vein_apply_star_type(sys, na1, na2, na3, &rng);
    f = vein_resource_coef(&sys->st, game, theme);
    vein_apply_rares(sys, theme, na1, na2, na3, pval, &rng);
    return vein_build(na1, na2, na3, f, game, out);
}

HD static inline int theme_has_rare(const theme_proto *theme, int vein_type)
{
    int i;

    i = 0;
    while (i < theme->rare_vein_count)
    {
        if (theme->rare_veins[i] == vein_type)
        {
            return 1;
        }
        ++i;
    }
    return 0;
}

HD static inline float star_system_avg_vein(star_system *sys, const game_desc *game, int vein_type)
{
    float count;
    int is_rare;
    int pi;

    if (vein_type == VEIN_TYPE_UNIPOLAR_MAGNET && sys->st.star_type != STAR_TYPE_BLACK_HOLE
        && sys->st.star_type != STAR_TYPE_NEUTRON_STAR)
    {
        return 0.0f;
    }
    count = 0.0f;
    is_rare = vein_is_rare(vein_type);
    pi = 0;
    while (pi < sys->planet_count)
    {
        if (!sys->planets[pi].gas_giant
            && !(is_rare && !theme_has_rare(&THEME_PROTOS[sys->planets[pi].theme_index], vein_type)))
        {
            vein veins[MAX_VEINS_PER_PLANET];
            int vc;
            int vi;

            vc = planet_get_veins(sys, pi, game, veins);
            vi = 0;
            while (vi < vc)
            {
                if (veins[vi].vein_type == vein_type)
                {
                    count += (float)(veins[vi].min_patch + veins[vi].max_patch)
                             * (float)(veins[vi].min_group + veins[vi].max_group)
                             * (float)(veins[vi].min_amount + veins[vi].max_amount) / 8.0f;
                }
                ++vi;
            }
        }
        ++pi;
    }
    return count;
}

#endif
