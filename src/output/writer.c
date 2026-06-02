#include "writer.h"
#include "names.h"
#include "../cli/config.h"
#include "../worldgen/galaxy_gen.h"
#include "../worldgen/star.h"
#include "../worldgen/planet.h"
#include "../worldgen/planet_props.h"
#include "../worldgen/planet_theme.h"
#include "../worldgen/planet_vein.h"
#include "../worldgen/vein.h"
#include "../constants/themes.h"
#include "../constants/planet_gen.h"

static int g_json_first = 1;

static void rebuild_systems(const game_desc *base_game, int seed, galaxy *gx, star_system *systems)
{
    game_desc game;
    int s;

    game = *base_game;
    game.seed = seed;
    generate_stars(&game, gx);
    s = 0;
    while (s < gx->star_count)
    {
        systems[s].st = star_init(gx, s);
        systems[s].planet_count = 0;
        systems[s].used_theme_count = 0;
        get_planets(&systems[s]);
        star_system_load_types(&systems[s], gx);
        ++s;
    }
    s = 0;
    while (s < gx->star_count)
    {
        star_system_select_all_themes(&systems[s]);
        ++s;
    }
}

static void emit_vein_json(FILE *out, const vein *v)
{
    fprintf(out, "{");
    fprintf(out, "\"veinType\":\"%s\",", vein_type_name(v->vein_type));
    fprintf(out, "\"minGroup\":%d,", v->min_group);
    fprintf(out, "\"maxGroup\":%d,", v->max_group);
    fprintf(out, "\"minPatch\":%d,", v->min_patch);
    fprintf(out, "\"maxPatch\":%d,", v->max_patch);
    fprintf(out, "\"minAmount\":%d,", v->min_amount);
    fprintf(out, "\"maxAmount\":%d", v->max_amount);
    fprintf(out, "}");
}

static void emit_veins_json(FILE *out, star_system *sys, int pidx, const game_desc *game)
{
    vein veins[MAX_VEINS_PER_PLANET];
    int count;
    int i;

    count = planet_get_veins(sys, pidx, game, veins);
    fprintf(out, "[");
    i = 0;
    while (i < count)
    {
        if (i > 0)
        {
            fprintf(out, ",");
        }
        emit_vein_json(out, &veins[i]);
        ++i;
    }
    fprintf(out, "]");
}

static void emit_gases_json(FILE *out, star_system *sys, int pidx, const game_desc *game)
{
    int items[THEME_MAX_GAS];
    float rates[THEME_MAX_GAS];
    int count;
    int i;

    count = planet_get_gases(sys, pidx, game, items, rates);
    fprintf(out, "[");
    i = 0;
    while (i < count)
    {
        if (i > 0)
        {
            fprintf(out, ",");
        }
        fprintf(out, "[%d,%.9g]", items[i], (double)rates[i]);
        ++i;
    }
    fprintf(out, "]");
}

static void emit_theme_json(FILE *out, const planet *p)
{
    const theme_proto *theme;

    theme = &THEME_PROTOS[p->theme_index];
    fprintf(out, "\"theme\":{");
    fprintf(out, "\"id\":%d,", theme->id);
    fprintf(out, "\"name\":\"%s\",", theme->name);
    fprintf(out, "\"waterItemId\":%d,", theme->water_item_id);
    fprintf(out, "\"wind\":%.9g", (double)theme->wind);
    fprintf(out, "}");
}

static void emit_planet_orbit_json(FILE *out, star_system *sys, int pidx)
{
    const planet *p;

    p = &sys->planets[pidx];
    fprintf(out, "\"index\":%d,", p->index);
    if (p->orbit_around == -1)
    {
        fprintf(out, "\"orbitAround\":null,");
    }
    else
    {
        fprintf(out, "\"orbitAround\":%d,", p->orbit_around);
    }
    fprintf(out, "\"orbitIndex\":%d,", p->orbit_index);
    fprintf(out, "\"orbitRadius\":%.9g,", (double)planet_orbital_radius(sys, pidx));
    fprintf(out, "\"orbitInclination\":%.9g,", (double)planet_orbit_inclination(sys, pidx));
    fprintf(out, "\"orbitLongitude\":%.9g,", (double)p->orbit_longitude);
    fprintf(out, "\"orbitalPeriod\":%.9g,", planet_orbital_period(sys, pidx));
    fprintf(out, "\"obliquity\":%.9g,", (double)planet_obliquity(sys, pidx));
    fprintf(out, "\"rotationPeriod\":%.9g,", planet_rotation_period(sys, pidx));
    fprintf(out, "\"sunDistance\":%.9g,", (double)planet_sun_distance(sys, pidx));
}

static void emit_planet_json(FILE *out, star_system *sys, int pidx, const game_desc *game)
{
    const planet *p;

    p = &sys->planets[pidx];
    fprintf(out, "{");
    emit_planet_orbit_json(out, sys, pidx);
    fprintf(out, "\"type\":\"%s\",", planet_type_name(THEME_PROTOS[p->theme_index].planet_type));
    fprintf(out, "\"luminosity\":%.9g,", (double)planet_luminosity(sys, pidx));
    emit_theme_json(out, p);
    fprintf(out, ",\"veins\":");
    emit_veins_json(out, sys, pidx, game);
    fprintf(out, ",\"gases\":");
    emit_gases_json(out, sys, pidx, game);
    fprintf(out, "}");
}

static void emit_star_fields_json(FILE *out, const star *st)
{
    fprintf(out, "\"index\":%d,", st->index);
    fprintf(out, "\"position\":[%.9g,%.9g,%.9g],", st->position.x, st->position.y, st->position.z);
    fprintf(out, "\"mass\":%.9g,", (double)star_mass(st));
    fprintf(out, "\"lifetime\":%.9g,", (double)star_lifetime(st));
    fprintf(out, "\"age\":%.9g,", (double)star_age(st));
    fprintf(out, "\"temperature\":%.9g,", (double)star_temperature(st));
    fprintf(out, "\"type\":\"%s\",", star_type_name(st->star_type));
    fprintf(out, "\"spectr\":\"%s\",", spectr_type_name(star_spectr(st)));
    fprintf(out, "\"luminosity\":%.9g,", (double)star_luminosity(st));
    fprintf(out, "\"radius\":%.9g,", (double)star_radius(st));
    fprintf(out, "\"dysonRadius\":%d,", star_dyson_radius(st));
    fprintf(out, "\"initialHiveCount\":%d,", star_initial_hive_count(st));
    fprintf(out, "\"maxHiveCount\":%d,", star_max_hive_count(st));
    fprintf(out, "\"color\":%.9g,", (double)star_color(st));
}

static void emit_star_json(FILE *out, star_system *sys, const game_desc *game)
{
    int i;

    fprintf(out, "{");
    emit_star_fields_json(out, &sys->st);
    fprintf(out, "\"planets\":[");
    i = 0;
    while (i < sys->planet_count)
    {
        if (i > 0)
        {
            fprintf(out, ",");
        }
        emit_planet_json(out, sys, i, game);
        ++i;
    }
    fprintf(out, "]}");
}

static void emit_record_json(FILE *out, const match_record *rec, const game_desc *base_game)
{
    galaxy gx;
    star_system systems[DSP_MAX_STARS];
    int i;

    rebuild_systems(base_game, rec->seed, &gx, systems);
    if (!g_json_first)
    {
        fprintf(out, ",\n");
    }
    g_json_first = 0;
    fprintf(out, "{\"seed\":%d,\"stars\":[", rec->seed);
    i = 0;
    while (i < rec->index_count)
    {
        if (i > 0)
        {
            fprintf(out, ",");
        }
        emit_star_json(out, &systems[rec->indexes[i]], &gx.game);
        ++i;
    }
    fprintf(out, "]}");
}

static void emit_record_text(FILE *out, const match_record *rec)
{
    int i;

    fprintf(out, "%d", rec->seed);
    if (rec->index_count > 0)
    {
        fprintf(out, "\t");
        i = 0;
        while (i < rec->index_count)
        {
            if (i > 0)
            {
                fprintf(out, ",");
            }
            fprintf(out, "%d", rec->indexes[i]);
            ++i;
        }
    }
    fprintf(out, "\n");
}

static int count_ocean_planets(const star_system *sys)
{
    int count;
    int i;

    count = 0;
    i = 0;
    while (i < sys->planet_count)
    {
        if (THEME_PROTOS[sys->planets[i].theme_index].planet_type == PLANET_TYPE_OCEAN)
        {
            ++count;
        }
        ++i;
    }
    return count;
}

static void emit_csv_row(FILE *out, int seed, const star_system *sys)
{
    const star *st;
    int ocean_count;

    st = &sys->st;
    ocean_count = count_ocean_planets(sys);
    fprintf(out, "%d,%d,", seed, st->index);
    fprintf(out, "%s,%s,", star_type_name(st->star_type), spectr_type_name(star_spectr(st)));
    fprintf(out, "%.9g,%d,", (double)star_luminosity(st), star_dyson_radius(st));
    fprintf(out, "%.9g,%.9g,", (double)star_mass(st), (double)star_age(st));
    fprintf(out, "%.9g,%.9g,", (double)star_temperature(st), (double)star_radius(st));
    fprintf(out, "%d,%d,", star_initial_hive_count(st), star_max_hive_count(st));
    fprintf(out, "%d,%d\n", sys->planet_count, ocean_count);
}

static void emit_record_csv(FILE *out, const match_record *rec, const game_desc *base_game)
{
    galaxy gx;
    star_system systems[DSP_MAX_STARS];
    int i;

    rebuild_systems(base_game, rec->seed, &gx, systems);
    i = 0;
    while (i < rec->index_count)
    {
        emit_csv_row(out, rec->seed, &systems[rec->indexes[i]]);
        ++i;
    }
}

void output_begin(FILE *out, int format)
{
    if (format == OUTPUT_FORMAT_JSON)
    {
        g_json_first = 1;
        fprintf(out, "[\n");
    }
    else if (format == OUTPUT_FORMAT_CSV)
    {
        fprintf(out, "seed,starIndex,starType,spectr,luminosity,dysonRadius,mass,age,");
        fprintf(out, "temperature,radius,initialHiveCount,maxHiveCount,planetCount,oceanPlanetCount\n");
    }
}

void output_record(FILE *out, int format, const match_record *rec,
                   const game_desc *base_game, const rule_program *prog)
{
    (void)prog;
    if (format == OUTPUT_FORMAT_JSON)
    {
        emit_record_json(out, rec, base_game);
    }
    else if (format == OUTPUT_FORMAT_CSV)
    {
        emit_record_csv(out, rec, base_game);
    }
    else
    {
        emit_record_text(out, rec);
    }
}

void output_end(FILE *out, int format)
{
    if (format == OUTPUT_FORMAT_JSON)
    {
        fprintf(out, "\n]\n");
    }
}
