#ifndef DSP_PLANET_PROPS_H
#define DSP_PLANET_PROPS_H

#include "hd.h"
#include "planet.h"
#include "star.h"
#include "dsp_math.h"
#include "../constants/enums.h"
#include "../constants/planet_gen.h"
#include "../constants/themes.h"
#include <math.h>

HD static inline int planet_has_orbit_around(const planet *p)
{
    return p->orbit_around != -1;
}

HD static inline int planet_is_birth(const star_system *sys, int pidx)
{
    const planet *p;

    p = &sys->planets[pidx];
    return p->orbit_index == 1 && star_is_birth(&sys->st) && planet_has_orbit_around(p);
}

HD static inline int planet_eligible_for_resonance(const planet *p)
{
    return !planet_has_orbit_around(p) && p->orbit_index <= 4 && !p->gas_giant;
}

HD static inline float planet_orbital_radius(const star_system *sys, int pidx)
{
    const planet *p;
    float a;
    float orbit_scaler;

    p = &sys->planets[pidx];
    a = powf(1.2f, (float)p->orbit_radius_factor);
    orbit_scaler = star_orbit_scaler(&sys->st);
    if (planet_has_orbit_around(p))
    {
        double parent_radius;

        parent_radius = (double)planet_real_radius(&sys->planets[p->orbit_around]);
        return (float)(((1600.0 * (double)p->orbit_index + 200.0)
                        * (double)wpow((wreal)orbit_scaler, (wreal)0.3)
                        * (double)(a + (1.0f - a) * 0.5f)
                        + parent_radius) / 40000.0);
    }
    {
        float b;
        float num16;

        b = ORBIT_RADIUS[p->orbit_index] * orbit_scaler;
        num16 = (float)((double)(a - 1.0f) / (double)fmaxf(b, 1.0f) + 1.0);
        return b * num16;
    }
}

HD static inline float planet_sun_distance(const star_system *sys, int pidx)
{
    const planet *p;

    p = &sys->planets[pidx];
    if (planet_has_orbit_around(p))
    {
        return planet_orbital_radius(sys, p->orbit_around);
    }
    return planet_orbital_radius(sys, pidx);
}

HD static inline double planet_orbital_period(const star_system *sys, int pidx)
{
    const planet *p;
    double f1;
    double gm;

    p = &sys->planets[pidx];
    f1 = (double)planet_orbital_radius(sys, pidx);
    gm = planet_has_orbit_around(p) ? ORBITAL_PERIOD_SATELLITE_GM
         : ORBITAL_PERIOD_STAR_GM * (double)star_mass(&sys->st);
    return (double)wsqrt((wreal)(4.0 * M_PI * M_PI * f1 * f1 * f1 / gm));
}

HD static inline double planet_sun_orbital_period(const star_system *sys, int pidx)
{
    const planet *p;

    p = &sys->planets[pidx];
    if (planet_has_orbit_around(p))
    {
        return planet_orbital_period(sys, p->orbit_around);
    }
    return planet_orbital_period(sys, pidx);
}

HD static inline double planet_rotation_period_mult(const star_system *sys, const planet *p)
{
    if (p->gas_giant)
    {
        return 0.2;
    }
    if (sys->st.star_type == STAR_TYPE_WHITE_DWARF)
    {
        return 0.5;
    }
    if (sys->st.star_type == STAR_TYPE_NEUTRON_STAR)
    {
        return 0.2;
    }
    if (sys->st.star_type == STAR_TYPE_BLACK_HOLE)
    {
        return 0.15;
    }
    return 1.0;
}

HD static inline double planet_rotation_period(const star_system *sys, int pidx)
{
    const planet *p;
    double rotation_period;
    double radius_mult;

    p = &sys->planets[pidx];
    if (planet_eligible_for_resonance(p))
    {
        if (p->rotation_param > 0.96)
        {
            return planet_orbital_period(sys, pidx);
        }
        if (p->rotation_param > 0.93)
        {
            return planet_orbital_period(sys, pidx) * 0.5;
        }
        if (p->rotation_param > 0.9)
        {
            return planet_orbital_period(sys, pidx) * 0.25;
        }
    }
    radius_mult = planet_has_orbit_around(p) ? 1.0 : (double)powf(planet_orbital_radius(sys, pidx), 0.25f);
    rotation_period = p->rotation_scale * planet_rotation_period_mult(sys, p) * radius_mult;
    rotation_period = 1.0 / (1.0 / planet_sun_orbital_period(sys, pidx) + 1.0 / rotation_period);
    if (p->rotation_param > 0.85 && p->rotation_param <= 0.9)
    {
        rotation_period = -rotation_period;
    }
    return rotation_period;
}

HD static inline int planet_is_tidal_locked(const star_system *sys, int pidx)
{
    return planet_rotation_period(sys, pidx) == planet_orbital_period(sys, pidx);
}

HD static inline float planet_temperature_factor(const star_system *sys, int pidx)
{
    const planet *p;
    float habitable_radius;

    p = &sys->planets[pidx];
    if (p->gas_giant)
    {
        return 0.0f;
    }
    habitable_radius = star_habitable_radius(&sys->st);
    if (habitable_radius > 0.0f)
    {
        return planet_sun_distance(sys, pidx) / habitable_radius;
    }
    return 1000.0f;
}

HD static inline float planet_habitable_bias(const star_system *sys, int pidx)
{
    const planet *p;
    float habitable_radius;
    float num21;
    float num22;

    p = &sys->planets[pidx];
    if (p->gas_giant)
    {
        return 1000.0f;
    }
    habitable_radius = star_habitable_radius(&sys->st);
    num21 = habitable_radius > 0.0f ? fabsf(logf(planet_sun_distance(sys, pidx) / habitable_radius)) : 1000.0f;
    num22 = clampf(sqrtf(habitable_radius), 1.0f, 2.0f) - 0.04f;
    return num21 * num22;
}

HD static inline float planet_temperature_bias(const star_system *sys, int pidx)
{
    const planet *p;

    p = &sys->planets[pidx];
    if (p->gas_giant)
    {
        return 0.0f;
    }
    return (float)(1.2 / ((double)planet_temperature_factor(sys, pidx) + 0.2) - 1.0);
}

HD static inline float planet_orbit_inclination(const star_system *sys, int pidx)
{
    const planet *p;
    float orbit_inclination;

    p = &sys->planets[pidx];
    orbit_inclination = (float)(p->orbit_inclination_factor * 16.0 - 8.0);
    if (planet_has_orbit_around(p))
    {
        orbit_inclination *= 2.2f;
    }
    if (sys->st.star_type == STAR_TYPE_NEUTRON_STAR)
    {
        orbit_inclination += orbit_inclination > 0.0f ? 3.0f : -3.0f;
    }
    return orbit_inclination;
}

HD static inline float planet_obliquity_resonant(const planet *p, float obliquity)
{
    if (p->rotation_param > 0.96)
    {
        return obliquity * 0.01f;
    }
    if (p->rotation_param > 0.93)
    {
        return obliquity * 0.1f;
    }
    if (p->rotation_param > 0.9)
    {
        return obliquity * 0.2f;
    }
    return obliquity;
}

HD static inline float planet_obliquity(const star_system *sys, int pidx)
{
    const planet *p;
    float obliquity;

    p = &sys->planets[pidx];
    if (p->rotation_param < 0.04)
    {
        obliquity = (float)(p->obliquity_scale * 39.9);
        return obliquity + (obliquity < 0.0f ? -70.0f : 70.0f);
    }
    if (p->rotation_param < 0.1)
    {
        obliquity = (float)(p->obliquity_scale * 80.0);
        return obliquity + (obliquity < 0.0f ? -30.0f : 30.0f);
    }
    obliquity = (float)(p->obliquity_scale * 60.0);
    if (planet_eligible_for_resonance(p))
    {
        return planet_obliquity_resonant(p, obliquity);
    }
    return obliquity;
}

HD static inline float planet_luminosity(const star_system *sys, int pidx)
{
    float luminosity;

    luminosity = powf(star_light_balance_radius(&sys->st) / (planet_sun_distance(sys, pidx) + 0.01f), 0.6f);
    if (luminosity > 1.0f)
    {
        luminosity = logf(logf(logf(luminosity) + 1.0f) + 1.0f) + 1.0f;
    }
    return roundf(luminosity * 100.0f) / 100.0f;
}

#endif
