#include <stdio.h>
#include <math.h>
#include "../../src/worldgen/galaxy_gen.h"
#include "../../src/worldgen/star.h"
#include "../../src/worldgen/planet.h"
#include "../../src/worldgen/planet_props.h"
#include "../../src/worldgen/planet_theme.h"
#include "../../src/rules/rule_program.h"
#include "../../src/rules/galaxy_eval.h"

static int g_fail = 0;

static void check(int condition, const char *name)
{
    printf("%s %s\n", condition ? "PASS" : "FAIL", name);
    if (!condition)
    {
        g_fail = 1;
    }
}

static game_desc default_game(void)
{
    game_desc game;

    game.seed = 0;
    game.star_count = 64;
    game.resource_multiplier = 1.0f;
    game.hive_initial_colonize = 1.0;
    game.hive_max_density = 1.0;
    return game;
}

static int full_habitable_count(int seed)
{
    game_desc game;
    galaxy gx;
    int index;

    game = default_game();
    game.seed = seed;
    generate_stars(&game, &gx);
    index = 0;
    while (index < gx.star_count)
    {
        star_system sys;

        sys.st = star_init(&gx, index);
        sys.planet_count = 0;
        sys.used_theme_count = 0;
        get_planets(&sys);
        star_system_load_types(&sys, &gx);
        ++index;
    }
    return gx.habitable_count;
}

static void test_prng_golden(void)
{
    dsp_random rng;
    dsp_random rng2;

    rng = prng_new(42);
    rng2 = prng_new(42);
    check(fabs(prng_sample(&rng) - 0.66887221376824757) < 1e-15, "prng seed 42 first sample golden");
    check(prng_next_seed(&rng2) == 1436392141, "prng seed 42 first next_seed golden");
}

static void test_seed0_golden(void)
{
    game_desc game;
    galaxy gx;
    star st0;
    star_system sys0;

    game = default_game();
    generate_stars(&game, &gx);
    check(gx.star_count == 64, "seed 0 star_count == 64");
    st0 = star_init(&gx, 0);
    check(st0.star_type == STAR_TYPE_MAIN_SEQ, "seed 0 star 0 is MainSeqStar");
    check(star_spectr(&st0) == SPECTR_TYPE_G, "seed 0 star 0 spectr is G");
    check(star_dyson_radius(&st0) == 21900, "seed 0 star 0 dysonRadius == 21900");
    check(fabsf(star_luminosity(&st0) - 0.988f) < 1e-3f, "seed 0 star 0 luminosity ~= 0.988");
    sys0.st = st0;
    sys0.planet_count = 0;
    sys0.used_theme_count = 0;
    get_planets(&sys0);
    star_system_load_types(&sys0, &gx);
    star_system_select_all_themes(&sys0);
    check(sys0.planet_count == 4, "seed 0 star 0 has 4 planets");
    check(sys0.planets[1].orbit_index == 1 && sys0.planets[1].orbit_around == 0, "seed 0 birth planet is moon at orbit 1");
    check(THEME_PROTOS[sys0.planets[1].theme_index].id == 1, "seed 0 birth planet theme id == 1 (Ocean 1)");
    check(THEME_PROTOS[sys0.planets[1].theme_index].planet_type == PLANET_TYPE_OCEAN, "seed 0 birth planet is Ocean");
}

static void test_habitable_golden(void)
{
    check(full_habitable_count(0) == 22, "seed 0 habitable_count == 22");
    check(full_habitable_count(1) == 21, "seed 1 habitable_count == 21");
}

static void test_determinism(void)
{
    check(full_habitable_count(12345) == full_habitable_count(12345), "regeneration is deterministic");
}

static void test_invariants(void)
{
    int seed;
    int all_have_blackhole;
    int all_birth_ocean;

    all_have_blackhole = 1;
    all_birth_ocean = 1;
    seed = 0;
    while (seed < 500)
    {
        game_desc game;
        galaxy gx;
        star_system sys0;
        int has_bh;
        int has_birth_ocean;
        int i;

        game = default_game();
        game.seed = seed;
        generate_stars(&game, &gx);
        has_bh = 0;
        i = 0;
        while (i < gx.star_count)
        {
            if (gx.star_types[i] == STAR_TYPE_BLACK_HOLE)
            {
                has_bh = 1;
            }
            ++i;
        }
        if (!has_bh)
        {
            all_have_blackhole = 0;
        }
        sys0.st = star_init(&gx, 0);
        sys0.planet_count = 0;
        sys0.used_theme_count = 0;
        get_planets(&sys0);
        star_system_load_types(&sys0, &gx);
        has_birth_ocean = 0;
        i = 0;
        while (i < sys0.planet_count)
        {
            if (planet_is_birth(&sys0, i) && sys0.planets[i].planet_type == PLANET_TYPE_OCEAN)
            {
                has_birth_ocean = 1;
            }
            ++i;
        }
        if (!has_birth_ocean)
        {
            all_birth_ocean = 0;
        }
        ++seed;
    }
    check(all_have_blackhole, "every galaxy (0..500) has a black hole");
    check(all_birth_ocean, "every birth system (0..500) has an Ocean home planet");
}

static rule_program single_rule(int kind)
{
    rule_program prog;

    prog.node_count = 1;
    prog.root = 0;
    prog.needs_planets = 0;
    prog.needs_themes = 0;
    prog.nodes[0].kind = kind;
    prog.nodes[0].child_count = 0;
    prog.nodes[0].value_count = 0;
    return prog;
}

static int count_matches(const rule_program *prog, int start, int end)
{
    game_desc game;
    int total;
    int seed;

    game = default_game();
    total = 0;
    seed = start;
    while (seed < end)
    {
        if (seed_matches(seed, &game, prog))
        {
            ++total;
        }
        ++seed;
    }
    return total;
}

static void test_rules(void)
{
    rule_program prog;

    prog = single_rule(RULE_STAR_TYPE);
    prog.nodes[0].priority = 11;
    prog.nodes[0].value_count = 1;
    prog.nodes[0].values[0] = STAR_TYPE_BLACK_HOLE;
    check(count_matches(&prog, 0, 1000) == 1000, "starType BlackHole matches all 1000 (BH always present)");

    prog = single_rule(RULE_BIRTH);
    prog.nodes[0].priority = 10;
    check(count_matches(&prog, 0, 1000) == 1000, "birth matches all 1000");

    prog.node_count = 3;
    prog.root = 0;
    prog.nodes[0].kind = RULE_AND;
    prog.nodes[0].child_count = 2;
    prog.nodes[0].children[0] = 1;
    prog.nodes[0].children[1] = 2;
    prog.nodes[1].kind = RULE_STAR_TYPE;
    prog.nodes[1].value_count = 1;
    prog.nodes[1].values[0] = STAR_TYPE_BLACK_HOLE;
    prog.nodes[1].child_count = 0;
    prog.nodes[2].kind = RULE_LUMINOSITY;
    prog.nodes[2].child_count = 0;
    prog.nodes[2].cond.op = COND_GTE;
    prog.nodes[2].cond.value = 2.5f;
    check(count_matches(&prog, 0, 2000) == 0, "AND(starType BH, lum>=2.5) matches 0 (degenerate stars are dim)");
}

int main(void)
{
    test_prng_golden();
    test_seed0_golden();
    test_habitable_golden();
    test_determinism();
    test_invariants();
    test_rules();
    printf(g_fail ? "\nSELFTEST FAILED\n" : "\nSELFTEST PASSED\n");
    return g_fail;
}
