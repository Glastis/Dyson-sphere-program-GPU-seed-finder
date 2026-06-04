#include "match_tree.h"
#include "names.h"
#include "../worldgen/galaxy_gen.h"
#include "../worldgen/star.h"
#include "../worldgen/planet_props.h"
#include "../worldgen/planet_theme.h"
#include "../worldgen/planet_vein.h"
#include "../rules/rule_eval.h"
#include "../constants/themes.h"
#include <stdio.h>
#include <string.h>

/* Tree glyphs (UTF-8). */
#define MT_DIAMOND "\xe2\x97\x86"               /* diamond */
#define MT_BR_MID  "\xe2\x94\x9c\xe2\x94\x80 "  /* tee + dash + space */
#define MT_BR_END  "\xe2\x94\x94\xe2\x94\x80 "  /* corner + dash + space */
#define MT_VBAR    "\xe2\x94\x82  "             /* vertical bar + 2 spaces */
#define MT_INDENT  "   "
#define MT_LEQ     "\xe2\x89\xa4"               /* <= */
#define MT_MIDDOT  "\xc2\xb7"                   /* middle dot */

void rebuild_systems(const game_desc *base_game, int seed, galaxy *gx, star_system *systems)
{
    game_desc game;
    int s;
    int habitable;

    game = *base_game;
    game.seed = seed;
    generate_stars(&game, gx);
    /* Resolve types in star order so the habitable count accumulates exactly as
     * the canonical pass does -- the output then agrees with the rule verdict. */
    habitable = 0;
    s = 0;
    while (s < gx->star_count)
    {
        systems[s].st = star_init(gx, s);
        systems[s].planet_count = 0;
        systems[s].used_theme_count = 0;
        get_planets(&systems[s]);
        star_system_load_types(&systems[s], gx, &habitable);
        ++s;
    }
    gx->habitable_count = habitable;
    s = 0;
    while (s < gx->star_count)
    {
        star_system_select_all_themes(&systems[s]);
        ++s;
    }
}

/* Label shown for every rule kind (also the combinator / fallback label). The
 * single source of truth for node labels; vein/gas describers append the
 * resource name to it. */
static const char *const MT_KIND_LABEL[RULE_PROXIMITY + 1] = {
    [RULE_AND]                   = "ET",
    [RULE_OR]                    = "OU",
    [RULE_BIRTH]                 = "départ",
    [RULE_STAR_TYPE]             = "type étoile",
    [RULE_BIRTH_DISTANCE]        = "dist. départ",
    [RULE_HIVE_COUNT]            = "ruches",
    [RULE_X_DISTANCE]            = "dist. trou noir",
    [RULE_SPECTR_DISTANCE]       = "voisins (classe)",
    [RULE_LUMINOSITY]            = "luminosité",
    [RULE_SPECTR]                = "classe",
    [RULE_DYSON_RADIUS]          = "rayon Dyson",
    [RULE_PLANET_COUNT]          = "planètes",
    [RULE_SATELLITE_COUNT]       = "satellites",
    [RULE_GAS_COUNT]             = "géantes gaz",
    [RULE_TIDAL_LOCK_COUNT]      = "verr. de marée",
    [RULE_PLANET_IN_DYSON_COUNT] = "planètes ds Dyson",
    [RULE_THEME_ID]              = "thème",
    [RULE_OCEAN_TYPE]            = "océan",
    [RULE_GAS_RATE]              = "gaz",
    [RULE_AVERAGE_VEIN_AMOUNT]   = "veine",
    [RULE_PROXIMITY]             = "proximité"
};

static const char *mt_kind_label(int kind)
{
    if (kind < 0 || kind > RULE_PROXIMITY || MT_KIND_LABEL[kind] == NULL)
    {
        return "?";
    }
    return MT_KIND_LABEL[kind];
}

/* --- per-leaf value describers (host-only -> a plain function-pointer table) -- */

typedef void (*mt_desc_fn)(const rule_node *node, star_system *sys,
                           const game_desc *game, char *label, size_t ln, char *val, size_t vn);

static float mt_gas_rate(star_system *sys, const game_desc *game, int gas_dense)
{
    int gid;
    float total;
    int i;

    gid = DSP_GAS_ID[gas_dense];
    total = 0.0f;
    i = 0;
    while (i < sys->planet_count)
    {
        int items[THEME_MAX_GAS];
        float rates[THEME_MAX_GAS];
        int gc;
        int g;

        gc = planet_get_gases(sys, i, game, items, rates);
        g = 0;
        while (g < gc)
        {
            if (items[g] == gid)
            {
                total += rates[g];
            }
            ++g;
        }
        ++i;
    }
    return total;
}

static void mt_desc_vein(const rule_node *node, star_system *sys, const game_desc *game,
                         char *label, size_t ln, char *val, size_t vn)
{
    snprintf(label, ln, "%s %s", mt_kind_label(node->kind), vein_type_name(node->vein));
    snprintf(val, vn, "%.2f m", (double)star_system_avg_vein(sys, game, node->vein) / 1.0e6);
}

static void mt_desc_gas(const rule_node *node, star_system *sys, const game_desc *game,
                        char *label, size_t ln, char *val, size_t vn)
{
    snprintf(label, ln, "%s %s", mt_kind_label(node->kind), gas_type_name(node->gas_type));
    snprintf(val, vn, "%.2f /s", (double)mt_gas_rate(sys, game, node->gas_type));
}

static void mt_desc_ocean(const rule_node *node, star_system *sys, const game_desc *game,
                          char *label, size_t ln, char *val, size_t vn)
{
    (void)sys;
    (void)game;
    snprintf(label, ln, "%s", mt_kind_label(node->kind));
    snprintf(val, vn, "%s", ocean_type_name(node->ocean_type));
}

static void mt_desc_star_type(const rule_node *node, star_system *sys, const game_desc *game,
                              char *label, size_t ln, char *val, size_t vn)
{
    (void)game;
    snprintf(label, ln, "%s", mt_kind_label(node->kind));
    snprintf(val, vn, "%s", star_type_name(sys->st.star_type));
}

static void mt_desc_spectr(const rule_node *node, star_system *sys, const game_desc *game,
                           char *label, size_t ln, char *val, size_t vn)
{
    (void)game;
    snprintf(label, ln, "%s", mt_kind_label(node->kind));
    snprintf(val, vn, "%s", spectr_type_name(star_spectr(&sys->st)));
}

static void mt_desc_luminosity(const rule_node *node, star_system *sys, const game_desc *game,
                               char *label, size_t ln, char *val, size_t vn)
{
    (void)game;
    snprintf(label, ln, "%s", mt_kind_label(node->kind));
    snprintf(val, vn, "%.2f", (double)star_luminosity(&sys->st));
}

static void mt_desc_dyson(const rule_node *node, star_system *sys, const game_desc *game,
                          char *label, size_t ln, char *val, size_t vn)
{
    (void)game;
    snprintf(label, ln, "%s", mt_kind_label(node->kind));
    snprintf(val, vn, "%d", star_dyson_radius(&sys->st));
}

static void mt_desc_birth_dist(const rule_node *node, star_system *sys, const game_desc *game,
                               char *label, size_t ln, char *val, size_t vn)
{
    (void)game;
    snprintf(label, ln, "%s", mt_kind_label(node->kind));
    snprintf(val, vn, "%.1f ly", vec3_magnitude(&sys->st.position));
}

static const mt_desc_fn MT_DESC[RULE_PROXIMITY + 1] = {
    [RULE_STAR_TYPE]           = mt_desc_star_type,
    [RULE_BIRTH_DISTANCE]      = mt_desc_birth_dist,
    [RULE_LUMINOSITY]          = mt_desc_luminosity,
    [RULE_SPECTR]              = mt_desc_spectr,
    [RULE_DYSON_RADIUS]        = mt_desc_dyson,
    [RULE_OCEAN_TYPE]          = mt_desc_ocean,
    [RULE_GAS_RATE]            = mt_desc_gas,
    [RULE_AVERAGE_VEIN_AMOUNT] = mt_desc_vein
};

static void mt_describe(const rule_program *prog, int node_idx, star_system *sys,
                        const game_desc *game, char *label, size_t ln, char *val, size_t vn)
{
    const rule_node *node;
    mt_desc_fn fn;

    node = &prog->nodes[node_idx];
    fn = NULL;
    if (node->kind >= 0 && node->kind <= RULE_PROXIMITY)
    {
        fn = MT_DESC[node->kind];
    }
    if (fn != NULL && sys != NULL)
    {
        fn(node, sys, game, label, ln, val, vn);
        return;
    }
    snprintf(label, ln, "%s", mt_kind_label(node->kind));
    val[0] = '\0';
}

/* "#12 BH   6.1 ly" -- star id, compact type tag, distance from the start. */
static const char *const MT_STAR_TAG[5] = {
    [STAR_TYPE_MAIN_SEQ]     = "",
    [STAR_TYPE_GIANT]        = "GG",
    [STAR_TYPE_WHITE_DWARF]  = "WD",
    [STAR_TYPE_NEUTRON_STAR] = "NS",
    [STAR_TYPE_BLACK_HOLE]   = "BH"
};

static void mt_star_info(const star_system *sys, char *buf, size_t n)
{
    const char *tag;

    if (sys == NULL)
    {
        snprintf(buf, n, "%s", "?");
        return;
    }
    tag = MT_STAR_TAG[sys->st.star_type];
    if (tag[0] == '\0')
    {
        tag = spectr_type_name(star_spectr(&sys->st));
    }
    snprintf(buf, n, "#%-3d %-2s %4.1f ly", sys->st.index, tag,
             vec3_magnitude(&sys->st.position));
}

static void mt_group_uint(char *buf, size_t n, long long v)
{
    char tmp[32];
    int len;
    int group;
    int out;
    int i;

    len = snprintf(tmp, sizeof(tmp), "%lld", v < 0 ? 0 : v);
    group = len % 3 == 0 ? 3 : len % 3;
    out = 0;
    i = 0;
    while (i < len && (size_t)out < n - 2)
    {
        if (i > 0 && group == 0)
        {
            buf[out++] = ',';
            group = 3;
        }
        buf[out++] = tmp[i];
        --group;
        ++i;
    }
    buf[out] = '\0';
}

/* Emits one proximity-system subtree (a system header line, plus one line per
 * leaf when the system rule is a combinator). Returns the line count. */
static int mt_emit_system(const rule_program *prog, int node_idx, star_system *sys,
                          const game_desc *game, mt_line *out, int max, int is_last)
{
    const rule_node *node;
    const char *branch;
    char sinfo[MT_RIGHT_MAX];
    int n;

    if (max <= 0)
    {
        return 0;
    }
    node = &prog->nodes[node_idx];
    branch = is_last ? MT_BR_END : MT_BR_MID;
    mt_star_info(sys, sinfo, sizeof(sinfo));
    n = 0;

    if (node->kind == RULE_AND || node->kind == RULE_OR)
    {
        int j;

        snprintf(out[n].left, MT_LEFT_MAX, "%s%s", branch, mt_kind_label(node->kind));
        snprintf(out[n].right, MT_RIGHT_MAX, "%s", sinfo);
        out[n].is_header = 0;
        ++n;
        j = 0;
        while (j < node->child_count && n < max)
        {
            char label[MT_LEFT_MAX];
            char val[MT_RIGHT_MAX];
            const char *cont;
            const char *cbr;

            cont = is_last ? MT_INDENT : MT_VBAR;
            cbr = (j == node->child_count - 1) ? MT_BR_END : MT_BR_MID;
            mt_describe(prog, node->children[j], sys, game, label, sizeof(label), val, sizeof(val));
            snprintf(out[n].left, MT_LEFT_MAX, "%s%s%s", cont, cbr, label);
            snprintf(out[n].right, MT_RIGHT_MAX, "%s", val);
            out[n].is_header = 0;
            ++n;
            ++j;
        }
        return n;
    }

    if (node->kind == RULE_BIRTH)
    {
        snprintf(out[n].left, MT_LEFT_MAX, "%s%s", branch, mt_kind_label(RULE_BIRTH));
        snprintf(out[n].right, MT_RIGHT_MAX, "%s", sinfo);
        out[n].is_header = 0;
        return 1;
    }

    /* Bare-leaf system: the header line carries the star info and the value. */
    {
        char label[MT_LEFT_MAX];
        char val[MT_RIGHT_MAX];

        mt_describe(prog, node_idx, sys, game, label, sizeof(label), val, sizeof(val));
        snprintf(out[n].left, MT_LEFT_MAX, "%s%s", branch, label);
        if (val[0] != '\0')
        {
            snprintf(out[n].right, MT_RIGHT_MAX, "%s " MT_MIDDOT " %s", sinfo, val);
        }
        else
        {
            snprintf(out[n].right, MT_RIGHT_MAX, "%s", sinfo);
        }
        out[n].is_header = 0;
        return 1;
    }
}

int match_tree_render(const match_record *rec, const game_desc *base_game,
                      const rule_program *prog, mt_line *out, int max)
{
    galaxy gx;
    star_system systems[DSP_MAX_STARS];
    const rule_node *root;
    char seedbuf[24];
    int n;

    if (max <= 0)
    {
        return 0;
    }
    rebuild_systems(base_game, rec->seed, &gx, systems);
    root = &prog->nodes[prog->root];

    mt_group_uint(seedbuf, sizeof(seedbuf), rec->seed);
    snprintf(out[0].left, MT_LEFT_MAX, MT_DIAMOND " %s", seedbuf);
    if (root->kind == RULE_PROXIMITY)
    {
        snprintf(out[0].right, MT_RIGHT_MAX, "proximité " MT_LEQ " %g ly", (double)root->cond.value);
    }
    else
    {
        out[0].right[0] = '\0';
    }
    out[0].is_header = 1;
    n = 1;

    if (root->kind == RULE_PROXIMITY)
    {
        int c;

        c = 0;
        while (c < root->child_count && n < max)
        {
            star_system *sys;

            sys = (c < rec->index_count) ? &systems[rec->indexes[c]] : NULL;
            n += mt_emit_system(prog, root->children[c], sys, base_game, out + n, max - n,
                                c == root->child_count - 1);
            ++c;
        }
        return n;
    }

    /* Non-proximity root: render it against the first matching star. */
    {
        star_system *sys;

        sys = (rec->index_count > 0) ? &systems[rec->indexes[0]] : NULL;
        n += mt_emit_system(prog, prog->root, sys, base_game, out + n, max - n, 1);
    }
    return n;
}
