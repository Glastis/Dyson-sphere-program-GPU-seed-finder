#include <stdio.h>
#include <stdlib.h>
#include "../../src/worldgen/galaxy_gen.h"
#include "../../src/worldgen/star.h"
#include "../../src/worldgen/planet.h"
#include "../../src/worldgen/planet_props.h"
#include "../../src/worldgen/planet_theme.h"
#include "../../src/worldgen/planet_vein.h"

static void dump_planet(star_system *sys, int pidx, const game_desc *game)
{
    planet *p;
    const theme_proto *theme;
    vein veins[MAX_VEINS_PER_PLANET];
    int gas_items[THEME_MAX_GAS];
    float gas_rates[THEME_MAX_GAS];
    int vc;
    int gc;
    int i;

    p = &sys->planets[pidx];
    theme = &THEME_PROTOS[p->theme_index];
    printf("  P %d orbit_index=%d gas=%d orbit_around=%d type=%d theme=%d sun_dist=%.5f orbital_period=%.4f rotation_period=%.4f tidal=%d\n",
           pidx, p->orbit_index, p->gas_giant, p->orbit_around, theme->planet_type, theme->id,
           planet_sun_distance(sys, pidx), planet_orbital_period(sys, pidx),
           planet_rotation_period(sys, pidx), planet_is_tidal_locked(sys, pidx));
    vc = planet_get_veins(sys, pidx, game, veins);
    i = 0;
    while (i < vc)
    {
        printf("    V type=%d group=%d-%d patch=%d-%d amount=%d-%d\n",
               veins[i].vein_type, veins[i].min_group, veins[i].max_group,
               veins[i].min_patch, veins[i].max_patch, veins[i].min_amount, veins[i].max_amount);
        ++i;
    }
    gc = planet_get_gases(sys, pidx, game, gas_items, gas_rates);
    i = 0;
    while (i < gc)
    {
        printf("    G item=%d rate=%.6f\n", gas_items[i], gas_rates[i]);
        ++i;
    }
}

int main(int argc, char **argv)
{
    game_desc game;
    galaxy gx;
    star_system sys[DSP_MAX_STARS];
    int index;

    game.seed = argc > 1 ? atoi(argv[1]) : 0;
    game.star_count = argc > 2 ? atoi(argv[2]) : 64;
    game.resource_multiplier = 1.0f;
    game.hive_initial_colonize = 1.0;
    game.hive_max_density = 1.0;

    generate_stars(&game, &gx);
    printf("seed=%d star_count=%d\n", game.seed, gx.star_count);
    index = 0;
    while (index < gx.star_count)
    {
        sys[index].st = star_init(&gx, index);
        get_planets(&sys[index]);
        star_system_load_types(&sys[index], &gx);
        star_system_select_all_themes(&sys[index]);
        ++index;
    }
    index = 0;
    while (index < gx.star_count)
    {
        star *st;
        int p;

        st = &sys[index].st;
        printf("S %d type=%d spectr=%d lum=%.4f dyson=%d planets=%d habitable_so_far\n",
               index, st->star_type, star_spectr(st), star_luminosity(st), star_dyson_radius(st),
               sys[index].planet_count);
        p = 0;
        while (p < sys[index].planet_count)
        {
            dump_planet(&sys[index], p, &game);
            ++p;
        }
        ++index;
    }
    printf("final_habitable_count=%d\n", gx.habitable_count);
    return 0;
}
