/* Standalone proof driver for the eval-order / habitable_count bug.
 * Builds with: gcc -O2 -Isrc tests/reference/prove_order_bug.c -lm
 *
 * It shows, for a chosen seed and star index, that the per-star "9 resources"
 * verdict (Oil & Stalagmite & Organic veins present, ocean Sulfur) is a pure
 * function of the *starting* gx->habitable_count value -- i.e. it depends on how
 * many stars were "prepared" before it, not on the star itself. The canonical
 * ordered pass fixes a single correct prefix value per star. */
#include <stdio.h>
#include <stdlib.h>
#include "../../src/worldgen/galaxy_gen.h"
#include "../../src/worldgen/star.h"
#include "../../src/worldgen/planet.h"
#include "../../src/worldgen/planet_props.h"
#include "../../src/worldgen/planet_theme.h"
#include "../../src/worldgen/planet_vein.h"
#include "../../src/constants/enums.h"

static game_desc make_game(int seed)
{
    game_desc g;
    g.seed = seed;
    g.star_count = 64;
    g.resource_multiplier = 1.0f;
    g.hive_initial_colonize = 1.0;
    g.hive_max_density = 1.0;
    return g;
}

/* Prepare star `index` from scratch, with gx->habitable_count seeded to `start`
 * (exactly what the buggy lazy path does: load_types reads+mutates it). */
static void prepare_with_start(galaxy *gx, int index, int start, star_system *sys)
{
    int habitable = start;
    sys->st = star_init(gx, index);
    sys->planet_count = 0;
    sys->used_theme_count = 0;
    get_planets(sys);
    star_system_load_types(sys, gx, &habitable);
    star_system_select_all_themes(sys);
}

static int vein_present(star_system *sys, const game_desc *game, int vein_type)
{
    return star_system_avg_vein(sys, game, vein_type) > 0.0f;
}

static int ocean_sulfur(star_system *sys)
{
    int i;
    for (i = 0; i < sys->planet_count; ++i)
        if (THEME_PROTOS[sys->planets[i].theme_index].water_item_id == OCEAN_TYPE_SULFUR)
            return 1;
    return 0;
}

/* The discriminating part of the "9 resources" subrule. */
static int nine_res_veins(star_system *sys, const game_desc *game)
{
    return ocean_sulfur(sys)
        && vein_present(sys, game, VEIN_TYPE_OIL)
        && vein_present(sys, game, VEIN_TYPE_SPINIFORM_STALAGMITE)
        && vein_present(sys, game, VEIN_TYPE_KIMBERLITE)
        && vein_present(sys, game, VEIN_TYPE_FRACTAL)
        && vein_present(sys, game, VEIN_TYPE_GRATING)
        && vein_present(sys, game, VEIN_TYPE_ORGANIC);
}

/* Canonical ordered pass: returns total habitable_count and fills prefix[]. */
static int canonical_prefix(int seed, int *prefix)
{
    game_desc game = make_game(seed);
    galaxy gx;
    int index;
    generate_stars(&game, &gx);
    gx.habitable_count = 0;
    for (index = 0; index < gx.star_count; ++index)
    {
        star_system sys;
        prefix[index] = gx.habitable_count; /* count of oceans in stars 0..index-1 */
        sys.st = star_init(&gx, index);
        sys.planet_count = 0;
        sys.used_theme_count = 0;
        get_planets(&sys);
        star_system_load_types(&sys, &gx, &gx.habitable_count);
    }
    return gx.habitable_count;
}

int main(int argc, char **argv)
{
    int seed = argc > 1 ? atoi(argv[1]) : 7526;
    int star = argc > 2 ? atoi(argv[2]) : 44;
    game_desc game = make_game(seed);
    galaxy gx;
    int prefix[DSP_MAX_STARS];
    int total;
    int h;
    int match_values[64];
    int nmatch = 0;

    generate_stars(&game, &gx);

    total = canonical_prefix(seed, prefix);
    printf("seed=%d  star_count=%d  full_habitable_count=%d\n", seed, gx.star_count, total);
    printf("canonical prefix(star %d) = %d\n", star, prefix[star]);

    printf("\nSweeping starting habitable_count h for star %d:\n", star);
    printf("  h : oceanSulfur Oil Stalag Organic | 9res-veins?\n");
    for (h = 0; h <= 30; ++h)
    {
        star_system sys;
        int os, oil, stal, org, verdict;
        prepare_with_start(&gx, star, h, &sys);
        os = ocean_sulfur(&sys);
        oil = vein_present(&sys, &game, VEIN_TYPE_OIL);
        stal = vein_present(&sys, &game, VEIN_TYPE_SPINIFORM_STALAGMITE);
        org = vein_present(&sys, &game, VEIN_TYPE_ORGANIC);
        verdict = nine_res_veins(&sys, &game);
        printf("  %2d:     %d        %d    %d      %d    | %s\n",
               h, os, oil, stal, org, verdict ? "MATCH" : "no");
        if (verdict)
            match_values[nmatch++] = h;
    }

    {
        star_system sys;
        int verdict_canonical;
        prepare_with_start(&gx, star, prefix[star], &sys);
        verdict_canonical = nine_res_veins(&sys, &game);
        printf("\nCANONICAL verdict for star %d (h=prefix=%d): %s\n",
               star, prefix[star], verdict_canonical ? "MATCH" : "NO MATCH");
    }
    printf("h values that make star %d a 9-resource MATCH: ", star);
    {
        int i;
        for (i = 0; i < nmatch; ++i) printf("%d ", match_values[i]);
        if (nmatch == 0) printf("(none)");
        printf("\n");
    }
    return 0;
}
