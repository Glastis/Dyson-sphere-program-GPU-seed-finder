#include <stdio.h>
#include <stdlib.h>
#include "../../src/worldgen/galaxy_gen.h"
#include "../../src/worldgen/star.h"

int main(int argc, char **argv)
{
    game_desc game;
    galaxy gx;
    int index;

    game.seed = argc > 1 ? atoi(argv[1]) : 0;
    game.star_count = 64;
    game.resource_multiplier = 1.0f;
    game.hive_initial_colonize = 1.0;
    game.hive_max_density = 1.0;

    generate_stars(&game, &gx);
    printf("seed=%d star_count=%d\n", game.seed, gx.star_count);
    index = 0;
    while (index < gx.star_count)
    {
        star st;

        st = star_init(&gx, index);
        printf("%d type=%d spectr=%d lum=%.4f dyson=%d mass=%.5f temp=%.2f age=%.5f maxhive=%d inithive=%d pos=%.5f,%.5f,%.5f\n",
               index, st.star_type, star_spectr(&st), star_luminosity(&st), star_dyson_radius(&st),
               star_mass(&st), star_temperature(&st), star_age(&st),
               star_max_hive_count(&st), star_initial_hive_count(&st),
               st.position.x, st.position.y, st.position.z);
        ++index;
    }
    return 0;
}
