#ifndef DSP_RULE_EVAL_H
#define DSP_RULE_EVAL_H

#include "rule_program.h"
#include "../worldgen/hd.h"
#include "../worldgen/galaxy.h"
#include "../worldgen/star.h"
#include "../worldgen/planet.h"
#include "../worldgen/planet_props.h"
#include "../worldgen/planet_theme.h"
#include "../worldgen/planet_vein.h"
#include "../constants/enums.h"

typedef struct
{
    galaxy *gx;
    const game_desc *game;
    int spectr[DSP_MAX_STARS];
    int spectr_ready;
    /* When a proximity rule matches, it records the star index of every
     * participating system here (anchor first, then one per `systems[]`
     * entry in order) so the output can report the whole constellation, not
     * just the anchor. Reset to 0 before each top-level evaluation. */
    int match_systems[DSP_MAX_STARS];
    int match_system_count;
}
eval_context;

/* Build one star system for evaluation. Idempotent and order-independent: the
 * planet types come from the star's own RNG plus the *frozen* habitable prefix
 * (galaxy_load_types must have run first), never from a counter that mutates as
 * other stars are prepared. Preparing the same star twice -- as anchor and as a
 * proximity neighbour -- yields the identical state. gx is read-only here. */
HD static inline void prepare_star_system(star_system *sys, galaxy *gx, int index, const rule_program *prog)
{
    sys->st = star_init(gx, index);
    sys->planet_count = 0;
    sys->used_theme_count = 0;
    if (prog->needs_planets)
    {
        get_planets(sys);
    }
    if (prog->needs_themes)
    {
        int habitable;

        habitable = gx->habitable_prefix[index];
        star_system_load_types(sys, gx, &habitable);
    }
}

/* Variant for the in-order top-level scan when the program has no proximity
 * node: stars are visited exactly once, in index order, so the canonical
 * habitable prefix is simply the running count carried across calls -- no
 * separate freeze pass is needed. `running` must start at 0 for star 0 and is
 * threaded unchanged through the whole scan. Equivalent to prepare_star_system
 * after galaxy_load_types, but it fuses the count into the scan for free. */
HD static inline void prepare_star_system_seq(star_system *sys, galaxy *gx, int index,
                                              const rule_program *prog, int *running)
{
    sys->st = star_init(gx, index);
    sys->planet_count = 0;
    sys->used_theme_count = 0;
    if (prog->needs_planets)
    {
        get_planets(sys);
    }
    if (prog->needs_themes)
    {
        star_system_load_types(sys, gx, running);
    }
}

HD static inline int prog_has_proximity(const rule_program *prog)
{
    int i;

    i = 0;
    while (i < prog->node_count)
    {
        if (prog->nodes[i].kind == RULE_PROXIMITY)
        {
            return 1;
        }
        ++i;
    }
    return 0;
}

HD static inline void ensure_spectr_cache(eval_context *ctx)
{
    int i;

    if (ctx->spectr_ready)
    {
        return;
    }
    i = 0;
    while (i < ctx->gx->star_count)
    {
        star tmp;

        tmp = star_init(ctx->gx, i);
        ctx->spectr[i] = star_spectr(&tmp);
        ++i;
    }
    ctx->spectr_ready = 1;
}

HD static inline int set_contains(const rule_node *node, int target)
{
    int i;

    i = 0;
    while (i < node->value_count)
    {
        if (node->values[i] == target)
        {
            return 1;
        }
        ++i;
    }
    return 0;
}

HD static inline int eval_star_type(const rule_node *node, star_system *sys)
{
    return set_contains(node, sys->st.star_type);
}

HD static inline int eval_spectr(const rule_node *node, star_system *sys)
{
    return set_contains(node, star_spectr(&sys->st));
}

HD static inline int eval_luminosity(const rule_node *node, star_system *sys)
{
    return cond_eval(&node->cond, star_luminosity(&sys->st));
}

HD static inline int eval_dyson_radius(const rule_node *node, star_system *sys)
{
    return cond_eval(&node->cond, (float)star_dyson_radius(&sys->st));
}

HD static inline int eval_birth_distance(const rule_node *node, star_system *sys)
{
    return cond_eval(&node->cond, (float)vec3_magnitude(&sys->st.position));
}

HD static inline int eval_hive_count(const rule_node *node, star_system *sys)
{
    int count;

    count = node->flag == 1 ? star_initial_hive_count(&sys->st) : star_max_hive_count(&sys->st);
    return cond_eval(&node->cond, (float)count);
}

HD static inline int eval_x_distance(const rule_node *node, star_system *sys, eval_context *ctx)
{
    int any_x;
    int index;
    int matched_all;
    int matched_any;

    any_x = 0;
    matched_all = 1;
    matched_any = 0;
    index = 0;
    while (index < ctx->gx->star_count)
    {
        int t;

        t = ctx->gx->star_types[index];
        if (t == STAR_TYPE_BLACK_HOLE || t == STAR_TYPE_NEUTRON_STAR)
        {
            float dist;

            any_x = 1;
            dist = (float)vec3_distance(&sys->st.position, &ctx->gx->positions[index]);
            if (cond_eval(&node->cond, dist))
            {
                matched_any = 1;
            }
            else
            {
                matched_all = 0;
            }
        }
        ++index;
    }
    if (!any_x)
    {
        return 0;
    }
    return node->flag == 1 ? matched_all : matched_any;
}

HD static inline int eval_spectr_distance(const rule_node *node, star_system *sys, eval_context *ctx)
{
    int good_total;
    int count;
    int index;

    ensure_spectr_cache(ctx);
    good_total = 0;
    count = 0;
    index = 0;
    while (index < ctx->gx->star_count)
    {
        if (ctx->spectr[index] == node->spectr)
        {
            ++good_total;
            if (index != sys->st.index)
            {
                float dist;

                dist = (float)vec3_distance(&sys->st.position, &ctx->gx->positions[index]);
                if (cond_eval(&node->cond2, dist))
                {
                    ++count;
                }
            }
        }
        ++index;
    }
    if (good_total == 0)
    {
        return 0;
    }
    return cond_eval(&node->cond, (float)count);
}

HD static inline int eval_planet_count(const rule_node *node, star_system *sys)
{
    int len;
    int i;

    if (!node->flag)
    {
        return cond_eval(&node->cond, (float)sys->planet_count);
    }
    len = 0;
    i = 0;
    while (i < sys->planet_count)
    {
        if (!sys->planets[i].gas_giant)
        {
            ++len;
        }
        ++i;
    }
    return cond_eval(&node->cond, (float)len);
}

HD static inline int eval_satellite_count(const rule_node *node, star_system *sys)
{
    int count;
    int i;

    count = 0;
    i = 0;
    while (i < sys->planet_count)
    {
        if (planet_has_orbit_around(&sys->planets[i]))
        {
            ++count;
        }
        ++i;
    }
    return cond_eval(&node->cond, (float)count);
}

HD static inline int eval_tidal_lock_count(const rule_node *node, star_system *sys)
{
    int count;
    int i;

    count = 0;
    i = 0;
    while (i < sys->planet_count)
    {
        if (planet_is_tidal_locked(sys, i))
        {
            ++count;
        }
        ++i;
    }
    return cond_eval(&node->cond, (float)count);
}

HD static inline int eval_planet_in_dyson_count(const rule_node *node, star_system *sys)
{
    float dyson_radius;
    int count;
    int i;

    dyson_radius = (float)star_dyson_radius(&sys->st);
    count = 0;
    i = 0;
    while (i < sys->planet_count)
    {
        int eligible;

        eligible = node->flag || !sys->planets[i].gas_giant;
        if (eligible && planet_sun_distance(sys, i) * 40000.0f < dyson_radius)
        {
            ++count;
        }
        ++i;
    }
    return cond_eval(&node->cond, (float)count);
}

HD static inline int eval_gas_count(const rule_node *node, star_system *sys)
{
    int count;
    int i;

    star_system_select_all_themes(sys);
    count = 0;
    i = 0;
    while (i < sys->planet_count)
    {
        if (sys->planets[i].gas_giant)
        {
            if (node->flag == RULE_FLAG_NONE)
            {
                ++count;
            }
            else if ((THEME_PROTOS[sys->planets[i].theme_index].temperature < 0.0f) == node->flag)
            {
                ++count;
            }
        }
        ++i;
    }
    return cond_eval(&node->cond, (float)count);
}

HD static inline int eval_theme_id(const rule_node *node, star_system *sys)
{
    int i;

    star_system_select_all_themes(sys);
    i = 0;
    while (i < sys->planet_count)
    {
        if (set_contains(node, THEME_PROTOS[sys->planets[i].theme_index].id))
        {
            return 1;
        }
        ++i;
    }
    return 0;
}

HD static inline int eval_ocean_type(const rule_node *node, star_system *sys)
{
    int i;

    star_system_select_all_themes(sys);
    i = 0;
    while (i < sys->planet_count)
    {
        if (THEME_PROTOS[sys->planets[i].theme_index].water_item_id == node->ocean_type)
        {
            return 1;
        }
        ++i;
    }
    return 0;
}

HD static inline int eval_gas_rate(const rule_node *node, star_system *sys, const game_desc *game)
{
    float total;
    int i;

    star_system_select_all_themes(sys);
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
            if (items[g] == node->gas_type)
            {
                total += rates[g];
            }
            ++g;
        }
        ++i;
    }
    return cond_eval(&node->cond, total);
}

HD static inline int eval_average_vein_amount(const rule_node *node, star_system *sys, const game_desc *game)
{
    star_system_select_all_themes(sys);
    return cond_eval(&node->cond, star_system_avg_vein(sys, game, node->vein));
}

HD static int eval_node(const rule_program *prog, int node_idx, star_system *sys, eval_context *ctx);

HD static inline int prox_neighbor_matches(const rule_program *prog, int child_idx,
                                           star_system *anchor, eval_context *ctx,
                                           float max_dist, int *used, int *out_index)
{
    int j;

    j = 0;
    while (j < ctx->gx->star_count)
    {
        float d;

        d = (float)vec3_distance(&anchor->st.position, &ctx->gx->positions[j]);
        if (!used[j] && d < max_dist)
        {
            star_system cand;

            prepare_star_system(&cand, ctx->gx, j, prog);
            if (eval_node(prog, child_idx, &cand, ctx))
            {
                used[j] = 1;
                *out_index = j;
                return 1;
            }
        }
        ++j;
    }
    return 0;
}

HD static inline int eval_proximity(const rule_program *prog, const rule_node *node,
                                    star_system *anchor, eval_context *ctx)
{
    int used[DSP_MAX_STARS];
    int found[DSP_MAX_STARS];
    int c;

    if (node->child_count < 1 || !eval_node(prog, node->children[0], anchor, ctx))
    {
        return 0;
    }
    c = 0;
    while (c < ctx->gx->star_count)
    {
        used[c] = 0;
        ++c;
    }
    used[anchor->st.index] = 1;
    found[0] = anchor->st.index;
    c = 1;
    while (c < node->child_count)
    {
        int idx;

        idx = -1;
        if (!prox_neighbor_matches(prog, node->children[c], anchor, ctx, node->cond.value, used, &idx))
        {
            return 0;
        }
        found[c] = idx;
        ++c;
    }
    /* All systems placed (each consumed a distinct star, so child_count <=
     * star_count <= DSP_MAX_STARS): publish the constellation. Done last, so a
     * nested proximity inside a child cannot clobber it mid-search. */
    c = 0;
    while (c < node->child_count)
    {
        ctx->match_systems[c] = found[c];
        ++c;
    }
    ctx->match_system_count = node->child_count;
    return 1;
}

HD static inline int eval_combinator(const rule_program *prog, const rule_node *node,
                                     star_system *sys, eval_context *ctx)
{
    int want_all;
    int i;

    want_all = node->kind == RULE_AND;
    i = 0;
    while (i < node->child_count)
    {
        int child_result;

        child_result = eval_node(prog, node->children[i], sys, ctx);
        if (want_all && !child_result)
        {
            return 0;
        }
        if (!want_all && child_result)
        {
            return 1;
        }
        ++i;
    }
    return want_all ? 1 : 0;
}

HD static inline int eval_leaf(const rule_node *node, star_system *sys, eval_context *ctx)
{
    switch (node->kind)
    {
    case RULE_BIRTH:
        return sys->st.index == 0;
    case RULE_STAR_TYPE:
        return eval_star_type(node, sys);
    case RULE_SPECTR:
        return eval_spectr(node, sys);
    case RULE_LUMINOSITY:
        return eval_luminosity(node, sys);
    case RULE_DYSON_RADIUS:
        return eval_dyson_radius(node, sys);
    case RULE_BIRTH_DISTANCE:
        return eval_birth_distance(node, sys);
    case RULE_HIVE_COUNT:
        return eval_hive_count(node, sys);
    case RULE_X_DISTANCE:
        return eval_x_distance(node, sys, ctx);
    case RULE_SPECTR_DISTANCE:
        return eval_spectr_distance(node, sys, ctx);
    case RULE_PLANET_COUNT:
        return eval_planet_count(node, sys);
    case RULE_SATELLITE_COUNT:
        return eval_satellite_count(node, sys);
    case RULE_TIDAL_LOCK_COUNT:
        return eval_tidal_lock_count(node, sys);
    case RULE_PLANET_IN_DYSON_COUNT:
        return eval_planet_in_dyson_count(node, sys);
    case RULE_GAS_COUNT:
        return eval_gas_count(node, sys);
    case RULE_THEME_ID:
        return eval_theme_id(node, sys);
    case RULE_OCEAN_TYPE:
        return eval_ocean_type(node, sys);
    case RULE_GAS_RATE:
        return eval_gas_rate(node, sys, ctx->game);
    case RULE_AVERAGE_VEIN_AMOUNT:
        return eval_average_vein_amount(node, sys, ctx->game);
    default:
        return 0;
    }
}

HD static int eval_node(const rule_program *prog, int node_idx, star_system *sys, eval_context *ctx)
{
    const rule_node *node;

    node = &prog->nodes[node_idx];
    if (node->kind == RULE_AND || node->kind == RULE_OR)
    {
        return eval_combinator(prog, node, sys, ctx);
    }
    if (node->kind == RULE_PROXIMITY)
    {
        return eval_proximity(prog, node, sys, ctx);
    }
    return eval_leaf(node, sys, ctx);
}

#endif
