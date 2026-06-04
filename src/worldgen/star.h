#ifndef DSP_STAR_H
#define DSP_STAR_H

#include "hd.h"
#include "prng.h"
#include "vector3.h"
#include "galaxy.h"
#include "dsp_math.h"
#include "../constants/enums.h"
#include "../constants/star_gen.h"
#include <math.h>

typedef struct
{
    int index;
    int star_type;
    int need_spectr;
    int name_seed;
    int planets_seed;
    int max_hive_count_modifier;
    float level;
    float age_num1;
    float age_num2;
    float age_num3;
    float mp_spectr_factor;
    double age_factor;
    double lifetime_factor;
    double radius_factor;
    double safety_factor_modifier;
    double mp_r1_1;
    double mp_r2_1;
    double mp_y;
    double mp_mass_factor;
    double hive_max_density;
    double hive_initial_colonize;
    vec3 position;
    dsp_random hive_rand;
}
star;

HD static inline int star_is_birth(const star *st)
{
    return st->index == 0;
}

/* Build one star. `with_hive` selects whether the (expensive) hive sub-generator
 * is materialised: hive_rand is a leaf PRNG -- nothing in the worldgen reads
 * back from it, and its two outputs (safety_factor_modifier,
 * max_hive_count_modifier) feed *only* the hive-count helpers (star_safety_factor
 * -> star_initial_hive_count, star_max_hive_count). So when no consumer needs the
 * hive count we can skip prng_new(...) + its two draws entirely.
 *
 * PRNG fidelity: the only parent draw involved is prng_next_seed(&rand2), which
 * seeds hive_rand. It is the LAST use of rand2 (and of rand1) in this function --
 * nothing downstream depends on rand2's post-draw state. We still issue that draw
 * in the light path so the parent generators advance identically, keeping the
 * sequence bit-for-bit unchanged; only the leaf hive_rand work is elided. */
HD static inline star star_init_ex(const galaxy *gx, int index, int with_hive)
{
    star st;
    dsp_random rand1;
    dsp_random rand2;
    double rn;
    double rt;
    int hive_seed;

    st.index = index;
    st.star_type = gx->star_types[index];
    st.need_spectr = gx->need_spectr[index];
    st.position = gx->positions[index];
    rand1 = prng_new(gx->star_seeds[index]);
    st.name_seed = prng_next_seed(&rand1);
    rand2 = prng_new(prng_next_seed(&rand1));
    prng_next_f64(&rand1);
    st.planets_seed = prng_next_seed(&rand1);
    st.mp_r1_1 = prng_next_f64(&rand2);
    st.mp_r2_1 = prng_next_f64(&rand2);
    st.age_factor = prng_next_f64(&rand2);
    rn = prng_next_f64(&rand2);
    rt = prng_next_f64(&rand2);
    st.age_num1 = (float)(rn * 0.1 + 0.95);
    st.age_num2 = (float)(rt * 0.4 + 0.8);
    st.age_num3 = (float)(rt * 9.0 + 1.0);
    st.mp_mass_factor = index == 0 ? 0.0 : prng_next_f64(&rand2);
    st.lifetime_factor = prng_next_f64(&rand2);
    st.mp_y = prng_next_f64(&rand2) * 0.4 - 0.2;
    st.radius_factor = (double)wpow((wreal)2.0, (wreal)st.mp_y);
    hive_seed = prng_next_seed(&rand2);
    if (with_hive)
    {
        st.hive_rand = prng_new(hive_seed);
        st.safety_factor_modifier = prng_next_f64(&st.hive_rand);
        st.max_hive_count_modifier = prng_next_i32(&st.hive_rand, 1000);
    }
    st.level = (float)index / (float)(gx->game.star_count - 1);
    st.mp_spectr_factor = st.need_spectr == SPECTR_TYPE_M ? SPECTR_FACTOR_M
                          : (st.need_spectr == SPECTR_TYPE_O ? SPECTR_FACTOR_O : 0.0f);
    st.hive_max_density = gx->game.hive_max_density;
    st.hive_initial_colonize = gx->game.hive_initial_colonize;
    return st;
}

HD static inline star star_init(const galaxy *gx, int index)
{
    return star_init_ex(gx, index, 1);
}

HD static inline float star_unmodified_mass_main(const star *st)
{
    float num8;

    if (st->mp_spectr_factor != 0.0f)
    {
        num8 = st->mp_spectr_factor;
    }
    else
    {
        float num7;
        float average_value;
        float sd;
        float num;

        num7 = lerp_f32(-0.98f, 0.88f, st->level);
        if (st->star_type == STAR_TYPE_GIANT)
        {
            average_value = st->mp_y > -0.08 ? -1.5f : 1.6f;
        }
        else
        {
            average_value = num7 >= 0.0f ? num7 + 0.65f : num7 - 0.65f;
        }
        sd = st->star_type == STAR_TYPE_GIANT ? 0.3f : 0.33f;
        num = rand_normal(average_value, sd, st->mp_r1_1, st->mp_r2_1);
        num8 = clampf(num <= 0.0f ? num : num * 2.0f, -2.4f, 4.65f);
    }
    return powf(2.0f, (float)((double)num8 + (st->mp_mass_factor - 0.5) * 0.2 + 1.0));
}

HD static inline float star_unmodified_mass(const star *st)
{
    if (star_is_birth(st))
    {
        float p1;

        p1 = clampf(rand_normal(0.0f, 0.08f, st->mp_r1_1, st->mp_r2_1), -0.2f, 0.2f);
        return powf(2.0f, p1);
    }
    if (st->star_type == STAR_TYPE_WHITE_DWARF)
    {
        return (float)(1.0 + st->mp_r2_1 * 5.0);
    }
    if (st->star_type == STAR_TYPE_NEUTRON_STAR)
    {
        return (float)(7.0 + st->mp_r1_1 * 11.0);
    }
    if (st->star_type == STAR_TYPE_BLACK_HOLE)
    {
        return (float)(18.0 + st->mp_r1_1 * st->mp_r2_1 * 30.0);
    }
    return star_unmodified_mass_main(st);
}

HD static inline float star_resource_coef(const star *st)
{
    float num1;

    if (star_is_birth(st))
    {
        return 0.6f;
    }
    num1 = (float)vec3_magnitude(&st->position) / 32.0f;
    if ((double)num1 > 1.0)
    {
        num1 = logf(logf(logf(logf(logf(num1) + 1.0f) + 1.0f) + 1.0f) + 1.0f) + 1.0f;
    }
    return powf(7.0f, num1) * 0.6f;
}

HD static inline float star_age(const star *st)
{
    float um;

    if (star_is_birth(st))
    {
        return (float)(st->age_factor * 0.4 + 0.3);
    }
    if (st->star_type == STAR_TYPE_GIANT)
    {
        return (float)(st->age_factor * 0.04 + 0.96);
    }
    if (st->star_type == STAR_TYPE_WHITE_DWARF || st->star_type == STAR_TYPE_NEUTRON_STAR
        || st->star_type == STAR_TYPE_BLACK_HOLE)
    {
        return (float)(st->age_factor * 0.4 + 1.0);
    }
    um = star_unmodified_mass(st);
    if (um >= 0.8f)
    {
        return (float)(st->age_factor * 0.7 + 0.2);
    }
    if (um >= 0.5f)
    {
        return (float)(st->age_factor * 0.4 + 0.1);
    }
    return (float)(st->age_factor * 0.12 + 0.02);
}

HD static inline float star_temperature_factor(const star *st)
{
    float p;

    p = powf(clampf(star_age(st), 0.0f, 1.0f), 20.0f);
    return (float)(1.0 - (double)p * 0.5) * star_unmodified_mass(st);
}

HD static inline float star_unmodified_temperature(const star *st)
{
    wreal f1;

    f1 = (wreal)star_temperature_factor(st);
    return (float)(wpow(f1, (wreal)0.56 + (wreal)0.14 / log_base(f1 + (wreal)4.0, (wreal)5.0)) * (wreal)4450.0 + (wreal)1300.0);
}

HD static inline wreal star_class_factor(const star *st)
{
    wreal temperature;
    wreal spectr_factor;

    temperature = (wreal)star_unmodified_temperature(st);
    spectr_factor = log_base((temperature - (wreal)1300.0) / (wreal)4500.0, (wreal)2.6) - (wreal)0.5;
    if (spectr_factor < (wreal)0.0)
    {
        spectr_factor *= (wreal)4.0;
    }
    return clampw(spectr_factor, (wreal)-4.0, (wreal)2.0);
}

HD static inline int star_is_degenerate(const star *st)
{
    return st->star_type == STAR_TYPE_WHITE_DWARF || st->star_type == STAR_TYPE_NEUTRON_STAR
           || st->star_type == STAR_TYPE_BLACK_HOLE;
}

HD static inline int star_spectr(const star *st)
{
    if (star_is_degenerate(st))
    {
        return SPECTR_TYPE_X;
    }
    return (int)round(star_class_factor(st));
}

HD static inline float star_color(const star *st)
{
    if (st->star_type == STAR_TYPE_BLACK_HOLE || st->star_type == STAR_TYPE_NEUTRON_STAR)
    {
        return 1.0f;
    }
    if (st->star_type == STAR_TYPE_WHITE_DWARF)
    {
        return 0.7f;
    }
    return clampf((float)((star_class_factor(st) + 3.5) * 0.2), 0.0f, 1.0f);
}

HD static inline float star_luminosity_factor(const star *st)
{
    if (st->star_type == STAR_TYPE_BLACK_HOLE)
    {
        return 1.0f / 1000.0f * st->age_num1;
    }
    if (st->star_type == STAR_TYPE_NEUTRON_STAR)
    {
        return 0.1f * st->age_num1;
    }
    if (st->star_type == STAR_TYPE_WHITE_DWARF)
    {
        return 0.04f * st->age_num1;
    }
    if (st->star_type == STAR_TYPE_GIANT)
    {
        return 1.6f;
    }
    return 1.0f;
}

HD static inline float star_luminosity(const star *st)
{
    float real;

    real = powf(star_temperature_factor(st), 0.7f) * star_luminosity_factor(st);
    return roundf(powf(real, 0.33f) * 1000.0f) / 1000.0f;
}

HD static inline float star_radius_giant(const star *st)
{
    float num4;

    num4 = (float)(wpow((wreal)5.0, wfabs(wlog10((wreal)star_unmodified_mass(st)) - (wreal)0.7)) * (wreal)5.0);
    if (num4 > 10.0f)
    {
        num4 = (logf(num4 * 0.1f) + 1.0f) * 10.0f;
    }
    return num4 * st->age_num2;
}

HD static inline float star_radius(const star *st)
{
    float mult;

    if (st->star_type == STAR_TYPE_GIANT)
    {
        return star_radius_giant(st);
    }
    mult = st->star_type == STAR_TYPE_NEUTRON_STAR ? 0.15f
           : (st->star_type == STAR_TYPE_WHITE_DWARF ? 0.2f : 1.0f);
    return (float)(wpow((wreal)star_unmodified_mass(st), (wreal)0.4) * (wreal)st->radius_factor) * mult;
}

HD static inline float star_habitable_radius(const star *st)
{
    float factor;

    if (st->star_type == STAR_TYPE_BLACK_HOLE || st->star_type == STAR_TYPE_NEUTRON_STAR)
    {
        return 0.0f;
    }
    factor = st->star_type == STAR_TYPE_WHITE_DWARF ? 0.15f * st->age_num2
             : (st->star_type == STAR_TYPE_GIANT ? 9.0f : 1.0f);
    return (powf(1.7f, (float)star_class_factor(st) + 2.0f) + (star_is_birth(st) ? 0.2f : 0.25f)) * factor;
}

HD static inline float star_light_balance_radius(const star *st)
{
    float r;
    float factor;

    if (st->star_type == STAR_TYPE_GIANT)
    {
        return 3.0f * star_habitable_radius(st);
    }
    r = powf(1.7f, (float)star_class_factor(st) + 2.0f);
    factor = st->star_type == STAR_TYPE_BLACK_HOLE ? 0.4f * st->age_num1
             : (st->star_type == STAR_TYPE_NEUTRON_STAR ? 3.0f * st->age_num1
             : (st->star_type == STAR_TYPE_WHITE_DWARF ? 0.2f * st->age_num1 : 1.0f));
    return r * factor;
}

HD static inline float star_orbit_scaler(const star *st)
{
    float orbit_scaler;
    float mult;

    orbit_scaler = powf(1.35f, (float)star_class_factor(st) + 2.0f);
    if (orbit_scaler < 1.0f)
    {
        orbit_scaler += (1.0f - orbit_scaler) * 0.6f;
    }
    mult = st->star_type == STAR_TYPE_NEUTRON_STAR ? 1.5f * st->age_num1
           : (st->star_type == STAR_TYPE_GIANT ? 3.3f : 1.0f);
    return orbit_scaler * mult;
}

HD static inline int star_dyson_radius(const star *st)
{
    float a;
    float b;
    float best;

    a = star_orbit_scaler(st) * 0.28f;
    b = star_radius(st) * 0.045f;
    best = a > b ? a : b;
    return (int)roundf(best * 800.0f) * 100;
}

HD static inline float star_mass(const star *st)
{
    float um;

    um = star_unmodified_mass(st);
    if (st->star_type == STAR_TYPE_BLACK_HOLE)
    {
        return um * 2.5f * st->age_num2;
    }
    if (st->star_type == STAR_TYPE_NEUTRON_STAR || st->star_type == STAR_TYPE_WHITE_DWARF)
    {
        return um * 0.2f * st->age_num1;
    }
    if (st->star_type == STAR_TYPE_GIANT)
    {
        return um * (1.0f - powf(star_age(st), 30.0f) * 0.5f);
    }
    return um;
}

HD static inline float star_temperature(const star *st)
{
    float temperature;

    if (st->star_type == STAR_TYPE_BLACK_HOLE)
    {
        return 0.0f;
    }
    if (st->star_type == STAR_TYPE_NEUTRON_STAR)
    {
        return st->age_num3 * 1e+7f;
    }
    if (st->star_type == STAR_TYPE_WHITE_DWARF)
    {
        return st->age_num2 * 150000.0f;
    }
    temperature = star_unmodified_temperature(st);
    if (st->star_type == STAR_TYPE_GIANT)
    {
        return temperature * (1.0f - powf(star_age(st), 30.0f) * 0.5f);
    }
    return temperature;
}

HD static inline float star_lifetime(const star *st)
{
    float um;
    double d;
    double mass_multiplier;
    double lifetime_delta;
    double lifetime;

    um = star_unmodified_mass(st);
    d = um < 2.0f ? (2.0 + 0.4 * (1.0 - (double)um)) : 5.0;
    mass_multiplier = st->star_type == STAR_TYPE_GIANT ? 0.58 : 0.5;
    lifetime_delta = st->star_type == STAR_TYPE_WHITE_DWARF ? 10000.0
                     : (st->star_type == STAR_TYPE_NEUTRON_STAR ? 1000.0 : 0.0);
    lifetime = 10000.0 * (double)wpow((wreal)0.1, log_base((wreal)((double)um * mass_multiplier), (wreal)d) + (wreal)1.0)
               * (st->lifetime_factor * 0.2 + 0.9) + lifetime_delta;
    if (star_is_birth(st))
    {
        return (float)lifetime;
    }
    {
        float age;
        float num9;

        age = star_age(st);
        num9 = (float)lifetime * age;
        if (num9 > 5000.0f)
        {
            num9 = (float)(((double)logf(num9 / 5000.0f) + 1.0) * 5000.0);
        }
        if (num9 > 8000.0f)
        {
            float inner;

            inner = logf(logf(logf(num9 / 8000.0f) + 1.0f) + 1.0f);
            num9 = (float)(((double)inner + 1.0) * 8000.0);
        }
        return num9 / age;
    }
}

HD static inline float star_safety_factor_b(const star *st)
{
    float b2;

    if (st->star_type == STAR_TYPE_BLACK_HOLE)
    {
        return 5.0f;
    }
    if (st->star_type == STAR_TYPE_NEUTRON_STAR)
    {
        return 1.7f;
    }
    if (st->star_type == STAR_TYPE_WHITE_DWARF)
    {
        return 1.2f;
    }
    b2 = powf(star_color(st), 1.3f);
    if (st->star_type == STAR_TYPE_GIANT)
    {
        return b2 > 0.6f ? b2 : 0.6f;
    }
    if (star_spectr(st) == SPECTR_TYPE_O)
    {
        return b2 + 0.05f;
    }
    return b2;
}

HD static inline float star_safety_factor(const star *st)
{
    float f1;
    float f2;
    float b;
    float result;

    if (star_is_birth(st))
    {
        return (float)(0.847 + st->safety_factor_modifier * 0.026);
    }
    f1 = clampf((float)((vec3_magnitude(&st->position) - 2.0) / 20.0), 0.0f, 2.5f);
    if (f1 > 1.0f)
    {
        f1 = logf(logf(f1) + 1.0f) + 1.0f;
    }
    f2 = f1 / 1.4f;
    b = star_safety_factor_b(st);
    result = (float)(1.0 - (double)powf(b * 0.9f + 0.07f, 0.73f) * (double)powf(f2, 0.27f)
                     + st->safety_factor_modifier * 0.08 - 0.04);
    return clampf(result, 0.0f, 1.0f);
}

HD static inline int star_max_hive_count(const star *st)
{
    double num14;

    num14 = (st->star_type == STAR_TYPE_BLACK_HOLE || st->star_type == STAR_TYPE_NEUTRON_STAR) ? 2.0 : 1.0;
    return (int)(st->hive_max_density * num14 * 1000.0 + (double)st->max_hive_count_modifier + 0.5) / 1000;
}

HD static inline int star_initial_hive_count_birth(const star *st, int max_hive_count)
{
    int num8;
    float num9;
    float sd;
    dsp_random rng;
    int attempt;
    int count;

    num8 = st->hive_initial_colonize * (double)max_hive_count < 0.7 ? 0 : 1;
    num9 = 0.6f * (float)st->hive_initial_colonize * (float)max_hive_count;
    sd = 0.5f;
    if (num9 < 1.0f)
    {
        sd = (float)((double)sqrtf(num9) * 0.29 + 0.21);
    }
    else if (num9 > (float)max_hive_count)
    {
        num9 = (float)max_hive_count;
    }
    rng = st->hive_rand;
    count = -1;
    attempt = 0;
    while (attempt < 17)
    {
        double r1_2;
        double r2_2;

        r1_2 = prng_next_f64(&rng);
        r2_2 = prng_next_f64(&rng);
        count = (int)(rand_normal(num9, sd, r1_2, r2_2) + 0.5f);
        if (count >= 0 && count <= max_hive_count)
        {
            break;
        }
        ++attempt;
    }
    if (count < num8)
    {
        count = num8;
    }
    if (count > max_hive_count)
    {
        count = max_hive_count;
    }
    return count;
}

HD static inline float star_initial_hive_count_base(const star *st, int max_hive_count)
{
    float t;
    float a;

    t = clampf(star_safety_factor(st) * 1.05f - 0.15f, 0.0f, 1.0f);
    a = clampf((float)(1.0 - (double)powf(t, 0.82f) - (double)(max_hive_count - 1) * 0.05), 0.0f, 1.0f)
        * (float)(1.1 - (double)max_hive_count * 0.1);
    if (st->hive_initial_colonize > 1.0)
    {
        return lerp_f32(a, (float)(1.0 + (st->hive_initial_colonize - 1.0) * 0.2),
                        (float)((st->hive_initial_colonize - 1.0) * 0.5));
    }
    return a * (float)st->hive_initial_colonize;
}

HD static inline float star_initial_hive_count_sd(float num16)
{
    if (num16 <= 0.01f)
    {
        return 0.0f;
    }
    if (num16 < 1.0f)
    {
        return sqrtf(num16) * 2.9f + 2.1f;
    }
    if (num16 > 1.0f)
    {
        return 0.3f + 0.2f * num16;
    }
    return 0.5f;
}

HD static inline float star_hive_type_mult(const star *st, float num15a)
{
    if (st->star_type == STAR_TYPE_GIANT)
    {
        return num15a * 1.2f;
    }
    if (st->star_type == STAR_TYPE_WHITE_DWARF)
    {
        return num15a * 1.4f;
    }
    if (st->star_type == STAR_TYPE_NEUTRON_STAR)
    {
        return num15a * 1.6f;
    }
    if (st->star_type == STAR_TYPE_BLACK_HOLE)
    {
        return num15a * 1.8f;
    }
    if (star_spectr(st) == SPECTR_TYPE_O)
    {
        return num15a * 1.1f;
    }
    return num15a;
}

HD static inline int star_initial_hive_count(const star *st)
{
    int max_hive_count;
    float num15;
    float num16;
    float sd2;
    float cap;
    dsp_random rng;
    int attempt;
    int count;

    if (st->hive_initial_colonize < 0.015)
    {
        return 0;
    }
    max_hive_count = star_max_hive_count(st);
    if (star_is_birth(st))
    {
        return star_initial_hive_count_birth(st, max_hive_count);
    }
    num15 = star_hive_type_mult(st, star_initial_hive_count_base(st, max_hive_count));
    cap = (float)max_hive_count + 0.75f;
    num16 = num15 * (float)max_hive_count;
    if (num16 > cap)
    {
        num16 = cap;
    }
    sd2 = star_initial_hive_count_sd(num16);
    rng = st->hive_rand;
    count = -1;
    attempt = 0;
    while (attempt < 65)
    {
        double r1_2;
        double r2_2;

        r1_2 = prng_next_f64(&rng);
        r2_2 = prng_next_f64(&rng);
        count = (int)(rand_normal(num16, sd2, r1_2, r2_2) + 0.5f);
        if (count >= 0 && count <= max_hive_count)
        {
            break;
        }
        ++attempt;
    }
    count = count < 0 ? 0 : (count > max_hive_count ? max_hive_count : count);
    if (st->star_type == STAR_TYPE_BLACK_HOLE)
    {
        int bh_min;

        bh_min = (int)(st->hive_max_density * 1000.0 + (double)st->max_hive_count_modifier + 0.5) / 1000;
        count = count > bh_min ? count : bh_min;
        count = count > 1 ? count : 1;
    }
    return count;
}

#endif
