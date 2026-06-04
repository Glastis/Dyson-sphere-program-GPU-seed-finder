#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "conditions.h"
#include "json_value.h"
#include "../constants/enums.h"

typedef struct
{
    rule_program *prog;
    char *err;
    int err_size;
    int has_error;
}
builder;

typedef struct
{
    const char *name;
    int value;
}
name_entry;

static const name_entry STAR_TYPES[] =
{
    { "MainSeqStar", STAR_TYPE_MAIN_SEQ },
    { "GiantStar", STAR_TYPE_GIANT },
    { "WhiteDwarf", STAR_TYPE_WHITE_DWARF },
    { "NeutronStar", STAR_TYPE_NEUTRON_STAR },
    { "BlackHole", STAR_TYPE_BLACK_HOLE }
};

static const name_entry SPECTR_TYPES[] =
{
    { "M", SPECTR_TYPE_M },
    { "K", SPECTR_TYPE_K },
    { "G", SPECTR_TYPE_G },
    { "F", SPECTR_TYPE_F },
    { "A", SPECTR_TYPE_A },
    { "B", SPECTR_TYPE_B },
    { "O", SPECTR_TYPE_O },
    { "X", SPECTR_TYPE_X }
};

/* Parse straight to the dense project index (enum dsp_ocean / dsp_gas); the
 * game's item id is recovered at eval time via the DSP_OCEAN_ID / DSP_GAS_ID
 * link tables in rule_eval.h. */
static const name_entry OCEAN_TYPES[] =
{
    { "None", DSP_OCEAN_NONE },
    { "Ice", DSP_OCEAN_ICE },
    { "Lava", DSP_OCEAN_LAVA },
    { "Water", DSP_OCEAN_WATER },
    { "Sulfur", DSP_OCEAN_SULFUR }
};

static const name_entry GAS_TYPES[] =
{
    { "None", DSP_GAS_NONE },
    { "Fireice", DSP_GAS_FIREICE },
    { "Hydrogen", DSP_GAS_HYDROGEN },
    { "Deuterium", DSP_GAS_DEUTERIUM }
};

static const name_entry VEIN_TYPES[] =
{
    { "None", VEIN_TYPE_NONE },
    { "Iron", VEIN_TYPE_IRON },
    { "Copper", VEIN_TYPE_COPPER },
    { "Silicium", VEIN_TYPE_SILICIUM },
    { "Titanium", VEIN_TYPE_TITANIUM },
    { "Stone", VEIN_TYPE_STONE },
    { "Coal", VEIN_TYPE_COAL },
    { "Oil", VEIN_TYPE_OIL },
    { "Fireice", VEIN_TYPE_FIREICE },
    { "Kimberlite", VEIN_TYPE_KIMBERLITE },
    { "Fractal", VEIN_TYPE_FRACTAL },
    { "Stalagmite", VEIN_TYPE_SPINIFORM_STALAGMITE },
    { "Grating", VEIN_TYPE_GRATING },
    { "Organic", VEIN_TYPE_ORGANIC },
    { "Magnet", VEIN_TYPE_UNIPOLAR_MAGNET }
};

static void set_error(builder *b, const char *message)
{
    if (b->has_error)
    {
        return;
    }
    b->has_error = 1;
    if (b->err != NULL && b->err_size > 0)
    {
        snprintf(b->err, (size_t)b->err_size, "%s", message);
    }
}

static void set_error_fmt(builder *b, const char *fmt, const char *arg)
{
    if (b->has_error)
    {
        return;
    }
    b->has_error = 1;
    if (b->err != NULL && b->err_size > 0)
    {
        snprintf(b->err, (size_t)b->err_size, fmt, arg);
    }
}

static int lookup_name(const name_entry *table, int count, const char *name, int *out)
{
    int index;

    index = 0;
    while (index < count)
    {
        if (strcmp(table[index].name, name) == 0)
        {
            *out = table[index].value;
            return 1;
        }
        ++index;
    }
    return 0;
}

static void lower_copy(char *dst, int dst_size, const char *src)
{
    int index;

    index = 0;
    while (src[index] != '\0' && index < dst_size - 1)
    {
        dst[index] = (char)tolower((unsigned char)src[index]);
        ++index;
    }
    dst[index] = '\0';
}

static const name_entry CONDITION_OPS[] =
{
    { "eq", COND_EQ }, { "neq", COND_NEQ }, { "lt", COND_LT },
    { "lte", COND_LTE }, { "gt", COND_GT }, { "gte", COND_GTE },
    { "between", COND_BETWEEN }, { "notbetween", COND_NOTBETWEEN }
};

static const name_entry PRESENCE_OPS[] =
{
    { "present", COND_GT }, { "absent", COND_LTE }
};

static int parse_op(const char *name, int *out)
{
    char lowered[32];

    lower_copy(lowered, 32, name);
    return lookup_name(CONDITION_OPS, 8, lowered, out);
}

static int apply_presence(const char *name, condition *cond)
{
    char lowered[16];
    int op;

    lower_copy(lowered, 16, name);
    if (!lookup_name(PRESENCE_OPS, 2, lowered, &op))
    {
        return 0;
    }
    cond->op = op;
    cond->value = 0.0f;
    cond->max = 0.0f;
    return 1;
}

static int allocate_node(builder *b, int kind, int priority)
{
    rule_node *node;
    int index;

    if (b->prog->node_count >= RULE_MAX_NODES)
    {
        set_error(b, "rule tree exceeds RULE_MAX_NODES");
        return -1;
    }
    index = b->prog->node_count;
    ++b->prog->node_count;
    node = &b->prog->nodes[index];
    memset(node, 0, sizeof(*node));
    node->kind = kind;
    node->priority = priority;
    node->flag = RULE_FLAG_NONE;
    return index;
}

static int get_bool_field(builder *b, const json_value *obj, const char *key, int fallback)
{
    const json_value *field;

    field = json_object_get(obj, key);
    if (field == NULL)
    {
        return fallback;
    }
    if (!json_is_bool(field))
    {
        set_error_fmt(b, "field '%s' must be a boolean", key);
        return fallback;
    }
    return json_as_bool(field);
}

static int parse_between(builder *b, const json_value *obj, condition *cond)
{
    const json_value *min_field;
    const json_value *max_field;

    min_field = json_object_get(obj, "min");
    max_field = json_object_get(obj, "max");
    if (!json_is_number(min_field) || !json_is_number(max_field))
    {
        set_error(b, "between condition requires numeric 'min' and 'max'");
        return -1;
    }
    cond->value = (float)json_as_number(min_field);
    cond->max = (float)json_as_number(max_field);
    return 0;
}

static int parse_condition(builder *b, const json_value *obj, condition *cond)
{
    const json_value *op_field;
    const json_value *value_field;
    int op;

    op_field = json_object_get(obj, "op");
    if (!json_is_string(op_field))
    {
        set_error(b, "condition missing string field 'op'");
        return -1;
    }
    if (apply_presence(json_as_string(op_field), cond))
    {
        return 0;
    }
    if (!parse_op(json_as_string(op_field), &op))
    {
        set_error_fmt(b, "unknown condition op '%s'", json_as_string(op_field));
        return -1;
    }
    cond->op = op;
    if (op == COND_BETWEEN || op == COND_NOTBETWEEN)
    {
        return parse_between(b, obj, cond);
    }
    value_field = json_object_get(obj, "value");
    if (!json_is_number(value_field))
    {
        set_error(b, "condition requires numeric 'value'");
        return -1;
    }
    cond->value = (float)json_as_number(value_field);
    cond->max = 0.0f;
    return 0;
}

static int parse_sub_condition(builder *b, const json_value *obj, const char *key, condition *cond)
{
    const json_value *sub;

    sub = json_object_get(obj, key);
    if (!json_is_object(sub))
    {
        set_error_fmt(b, "missing object field '%s'", key);
        return -1;
    }
    return parse_condition(b, sub, cond);
}

static int parse_names_array(builder *b, const json_value *obj, const char *key,
                             const name_entry *table, int table_count, rule_node *node)
{
    const json_value *array;
    int size;
    int index;

    array = json_object_get(obj, key);
    if (!json_is_array(array))
    {
        set_error_fmt(b, "field '%s' must be an array", key);
        return -1;
    }
    size = json_array_size(array);
    if (size > RULE_MAX_VALUES)
    {
        set_error_fmt(b, "field '%s' has too many values", key);
        return -1;
    }
    index = 0;
    while (index < size)
    {
        const json_value *item;
        int value;

        item = json_array_get(array, index);
        if (!json_is_string(item) || !lookup_name(table, table_count, json_as_string(item), &value))
        {
            set_error_fmt(b, "unknown name in '%s'", key);
            return -1;
        }
        node->values[index] = value;
        ++index;
    }
    node->value_count = size;
    return 0;
}

static int parse_int_array(builder *b, const json_value *obj, const char *key, rule_node *node)
{
    const json_value *array;
    int size;
    int index;

    array = json_object_get(obj, key);
    if (!json_is_array(array))
    {
        set_error_fmt(b, "field '%s' must be an array", key);
        return -1;
    }
    size = json_array_size(array);
    if (size > RULE_MAX_VALUES)
    {
        set_error_fmt(b, "field '%s' has too many values", key);
        return -1;
    }
    index = 0;
    while (index < size)
    {
        const json_value *item;

        item = json_array_get(array, index);
        if (!json_is_number(item))
        {
            set_error_fmt(b, "field '%s' must contain integers", key);
            return -1;
        }
        node->values[index] = (int)json_as_number(item);
        ++index;
    }
    node->value_count = size;
    return 0;
}

static int parse_single_name(builder *b, const json_value *obj, const char *key,
                             const name_entry *table, int table_count, int *out)
{
    const json_value *field;

    field = json_object_get(obj, key);
    if (!json_is_string(field))
    {
        set_error_fmt(b, "field '%s' must be a string", key);
        return -1;
    }
    if (!lookup_name(table, table_count, json_as_string(field), out))
    {
        set_error_fmt(b, "unknown name in '%s'", key);
        return -1;
    }
    return 0;
}

static int parse_ocean(builder *b, const json_value *obj, rule_node *node)
{
    const char *key;

    key = json_object_get(obj, "ocean") != NULL ? "ocean" : "oceanType";
    return parse_single_name(b, obj, key, OCEAN_TYPES, 5, &node->ocean_type);
}

static int build_node(builder *b, const json_value *obj);

static void sort_children(builder *b, rule_node *node)
{
    int outer;

    outer = 1;
    while (outer < node->child_count)
    {
        int child;
        int priority;
        int inner;

        child = node->children[outer];
        priority = b->prog->nodes[child].priority;
        inner = outer - 1;
        while (inner >= 0 && b->prog->nodes[node->children[inner]].priority > priority)
        {
            node->children[inner + 1] = node->children[inner];
            --inner;
        }
        node->children[inner + 1] = child;
        ++outer;
    }
}

static int build_combinator(builder *b, const json_value *obj, int kind, int self_index)
{
    const json_value *rules;
    int size;
    int index;
    int max_priority;

    rules = json_object_get(obj, "rules");
    if (!json_is_array(rules))
    {
        set_error(b, "combinator rule requires array field 'rules'");
        return -1;
    }
    size = json_array_size(rules);
    max_priority = 0;
    index = 0;
    while (index < size)
    {
        int child;

        child = build_node(b, json_array_get(rules, index));
        if (child < 0)
        {
            return -1;
        }
        b->prog->nodes[self_index].children[b->prog->nodes[self_index].child_count] = child;
        ++b->prog->nodes[self_index].child_count;
        if (b->prog->nodes[child].priority > max_priority)
        {
            max_priority = b->prog->nodes[child].priority;
        }
        ++index;
    }
    b->prog->nodes[self_index].priority = max_priority;
    b->prog->nodes[self_index].kind = kind;
    sort_children(b, &b->prog->nodes[self_index]);
    return self_index;
}

static int build_proximity(builder *b, const json_value *obj, int self_index)
{
    const json_value *systems;
    const json_value *dist;
    int size;
    int index;

    dist = json_object_get(obj, "maxDistance");
    if (!json_is_number(dist))
    {
        set_error(b, "proximity requires numeric 'maxDistance'");
        return -1;
    }
    b->prog->nodes[self_index].cond.op = COND_LT;
    b->prog->nodes[self_index].cond.value = (float)json_as_number(dist);
    systems = json_object_get(obj, "systems");
    if (!json_is_array(systems))
    {
        set_error(b, "proximity requires array field 'systems'");
        return -1;
    }
    size = json_array_size(systems);
    if (size < 2)
    {
        set_error(b, "proximity needs at least 2 entries in 'systems'");
        return -1;
    }
    index = 0;
    while (index < size)
    {
        int child;

        child = build_node(b, json_array_get(systems, index));
        if (child < 0)
        {
            return -1;
        }
        b->prog->nodes[self_index].children[b->prog->nodes[self_index].child_count] = child;
        ++b->prog->nodes[self_index].child_count;
        ++index;
    }
    b->prog->nodes[self_index].priority = 60;
    return self_index;
}

static int build_spectr_distance(builder *b, const json_value *obj, int index)
{
    rule_node *node;
    int spectr;

    if (parse_single_name(b, obj, "spectr", SPECTR_TYPES, 8, &spectr) != 0)
    {
        return -1;
    }
    node = &b->prog->nodes[index];
    node->spectr = spectr;
    if (parse_sub_condition(b, obj, "count", &node->cond) != 0)
    {
        return -1;
    }
    if (parse_sub_condition(b, obj, "distance", &node->cond2) != 0)
    {
        return -1;
    }
    return index;
}

static int build_hive_count(builder *b, const json_value *obj, int index)
{
    rule_node *node;
    int is_initial;

    is_initial = get_bool_field(b, obj, "initial", 0);
    if (b->has_error)
    {
        return -1;
    }
    node = &b->prog->nodes[index];
    node->flag = is_initial ? 1 : 0;
    node->priority = is_initial ? 23 : 13;
    if (parse_condition(b, obj, &node->cond) != 0)
    {
        return -1;
    }
    return index;
}

static int build_gas_count(builder *b, const json_value *obj, int index)
{
    rule_node *node;
    const json_value *ice;

    node = &b->prog->nodes[index];
    ice = json_object_get(obj, "ice");
    if (ice == NULL)
    {
        node->flag = RULE_FLAG_NONE;
        node->priority = 32;
    }
    else
    {
        if (!json_is_bool(ice))
        {
            set_error(b, "field 'ice' must be a boolean");
            return -1;
        }
        node->flag = json_as_bool(ice) ? 1 : 0;
        node->priority = 41;
    }
    if (parse_condition(b, obj, &node->cond) != 0)
    {
        return -1;
    }
    return index;
}

static int build_flagged_inline(builder *b, const json_value *obj, int index, const char *key)
{
    rule_node *node;

    node = &b->prog->nodes[index];
    node->flag = get_bool_field(b, obj, key, 0) ? 1 : 0;
    if (b->has_error)
    {
        return -1;
    }
    if (parse_condition(b, obj, &node->cond) != 0)
    {
        return -1;
    }
    return index;
}

static int dispatch_simple(builder *b, const json_value *obj, const char *type, int index)
{
    rule_node *node;

    node = &b->prog->nodes[index];
    if (strcmp(type, "birth") == 0)
    {
        return index;
    }
    if (strcmp(type, "starType") == 0)
    {
        return parse_names_array(b, obj, "starTypes", STAR_TYPES, 5, node) == 0 ? index : -1;
    }
    if (strcmp(type, "spectr") == 0)
    {
        return parse_names_array(b, obj, "spectr", SPECTR_TYPES, 8, node) == 0 ? index : -1;
    }
    if (strcmp(type, "themeId") == 0)
    {
        return parse_int_array(b, obj, "themeIds", node) == 0 ? index : -1;
    }
    if (strcmp(type, "oceanType") == 0)
    {
        return parse_ocean(b, obj, node) == 0 ? index : -1;
    }
    return -2;
}

static int dispatch_inline(builder *b, const json_value *obj, const char *type, int index)
{
    rule_node *node;

    node = &b->prog->nodes[index];
    if (strcmp(type, "birthDistance") == 0 || strcmp(type, "luminosity") == 0
        || strcmp(type, "xDistance") == 0 || strcmp(type, "dysonRadius") == 0
        || strcmp(type, "satelliteCount") == 0 || strcmp(type, "tidalLockCount") == 0)
    {
        if (strcmp(type, "xDistance") == 0)
        {
            node->flag = get_bool_field(b, obj, "all", 0) ? 1 : 0;
            if (b->has_error)
            {
                return -1;
            }
        }
        return parse_condition(b, obj, &node->cond) == 0 ? index : -1;
    }
    return -2;
}

static int build_named_inline(builder *b, const json_value *obj, int index, const char *key,
                              const name_entry *table, int table_count, int *target)
{
    if (parse_single_name(b, obj, key, table, table_count, target) != 0)
    {
        return -1;
    }
    return parse_condition(b, obj, &b->prog->nodes[index].cond) == 0 ? index : -1;
}

static int dispatch_special(builder *b, const json_value *obj, const char *type, int index)
{
    rule_node *node;

    node = &b->prog->nodes[index];
    if (strcmp(type, "spectrDistance") == 0)
    {
        return build_spectr_distance(b, obj, index);
    }
    if (strcmp(type, "hiveCount") == 0)
    {
        return build_hive_count(b, obj, index);
    }
    if (strcmp(type, "gasCount") == 0)
    {
        return build_gas_count(b, obj, index);
    }
    if (strcmp(type, "planetCount") == 0)
    {
        return build_flagged_inline(b, obj, index, "excludeGiant");
    }
    if (strcmp(type, "planetInDysonCount") == 0)
    {
        return build_flagged_inline(b, obj, index, "includeGiant");
    }
    if (strcmp(type, "gasRate") == 0)
    {
        return build_named_inline(b, obj, index, "gas", GAS_TYPES, 4, &node->gas_type);
    }
    if (strcmp(type, "averageVeinAmount") == 0)
    {
        return build_named_inline(b, obj, index, "vein", VEIN_TYPES, 15, &node->vein);
    }
    return -2;
}

typedef struct
{
    const char *type;
    int kind;
    int priority;
}
type_spec;

static const type_spec TYPE_SPECS[] =
{
    { "and", RULE_AND, 0 }, { "or", RULE_OR, 0 },
    { "birth", RULE_BIRTH, 10 }, { "starType", RULE_STAR_TYPE, 11 },
    { "birthDistance", RULE_BIRTH_DISTANCE, 12 }, { "hiveCount", RULE_HIVE_COUNT, 13 },
    { "xDistance", RULE_X_DISTANCE, 14 }, { "spectrDistance", RULE_SPECTR_DISTANCE, 15 },
    { "luminosity", RULE_LUMINOSITY, 20 }, { "spectr", RULE_SPECTR, 21 },
    { "dysonRadius", RULE_DYSON_RADIUS, 22 }, { "planetCount", RULE_PLANET_COUNT, 30 },
    { "satelliteCount", RULE_SATELLITE_COUNT, 31 }, { "gasCount", RULE_GAS_COUNT, 32 },
    { "tidalLockCount", RULE_TIDAL_LOCK_COUNT, 33 }, { "planetInDysonCount", RULE_PLANET_IN_DYSON_COUNT, 34 },
    { "themeId", RULE_THEME_ID, 40 }, { "oceanType", RULE_OCEAN_TYPE, 42 },
    { "gasRate", RULE_GAS_RATE, 50 }, { "averageVeinAmount", RULE_AVERAGE_VEIN_AMOUNT, 51 },
    { "proximity", RULE_PROXIMITY, 60 }
};

static const type_spec *find_type_spec(const char *type)
{
    int index;

    index = 0;
    while (index < (int)(sizeof(TYPE_SPECS) / sizeof(TYPE_SPECS[0])))
    {
        if (strcmp(TYPE_SPECS[index].type, type) == 0)
        {
            return &TYPE_SPECS[index];
        }
        ++index;
    }
    return NULL;
}

typedef struct
{
    const char *key;
    const char *type;
}
infer_spec;

static const infer_spec INFER_SPECS[] =
{
    { "vein", "averageVeinAmount" },
    { "gas", "gasRate" },
    { "ocean", "oceanType" }
};

static const char *infer_type(const json_value *obj)
{
    int count;
    int index;

    count = (int)(sizeof(INFER_SPECS) / sizeof(INFER_SPECS[0]));
    index = 0;
    while (index < count)
    {
        if (json_object_get(obj, INFER_SPECS[index].key) != NULL)
        {
            return INFER_SPECS[index].type;
        }
        ++index;
    }
    return NULL;
}

static int dispatch_node(builder *b, const json_value *obj, const char *type, int kind, int index)
{
    int result;

    if (kind == RULE_AND || kind == RULE_OR)
    {
        return build_combinator(b, obj, kind, index);
    }
    if (kind == RULE_PROXIMITY)
    {
        return build_proximity(b, obj, index);
    }
    result = dispatch_simple(b, obj, type, index);
    if (result != -2)
    {
        return result;
    }
    result = dispatch_inline(b, obj, type, index);
    if (result != -2)
    {
        return result;
    }
    return dispatch_special(b, obj, type, index);
}

static int build_node(builder *b, const json_value *obj)
{
    const json_value *type_field;
    const char *type_name;
    const type_spec *spec;
    int index;

    if (b->has_error)
    {
        return -1;
    }
    if (!json_is_object(obj))
    {
        set_error(b, "rule must be an object");
        return -1;
    }
    type_field = json_object_get(obj, "type");
    type_name = json_is_string(type_field) ? json_as_string(type_field) : infer_type(obj);
    if (type_name == NULL)
    {
        set_error(b, "rule missing 'type' and no inferable key (vein/gas/ocean)");
        return -1;
    }
    spec = find_type_spec(type_name);
    if (spec == NULL)
    {
        set_error_fmt(b, "unknown rule type '%s'", type_name);
        return -1;
    }
    index = allocate_node(b, spec->kind, spec->priority);
    if (index < 0)
    {
        return -1;
    }
    return dispatch_node(b, obj, spec->type, spec->kind, index);
}

static int kind_needs_planets(int kind)
{
    return kind == RULE_PLANET_COUNT || kind == RULE_SATELLITE_COUNT
        || kind == RULE_TIDAL_LOCK_COUNT || kind == RULE_PLANET_IN_DYSON_COUNT
        || kind == RULE_GAS_COUNT || kind == RULE_THEME_ID || kind == RULE_OCEAN_TYPE
        || kind == RULE_GAS_RATE || kind == RULE_AVERAGE_VEIN_AMOUNT;
}

static int kind_needs_themes(int kind)
{
    return kind == RULE_THEME_ID || kind == RULE_OCEAN_TYPE
        || kind == RULE_GAS_RATE || kind == RULE_AVERAGE_VEIN_AMOUNT;
}

/* Only the hive-count rule reads the per-star hive sub-generator; when no such
 * rule is present, star_init can skip materialising it (see star_init_ex). */
static int kind_needs_hive(int kind)
{
    return kind == RULE_HIVE_COUNT;
}

static void compute_needs(rule_program *prog)
{
    int index;

    prog->needs_planets = 0;
    prog->needs_themes = 0;
    prog->needs_hive = 0;
    index = 0;
    while (index < prog->node_count)
    {
        int kind;

        kind = prog->nodes[index].kind;
        if (kind_needs_planets(kind))
        {
            prog->needs_planets = 1;
        }
        if (kind_needs_themes(kind))
        {
            prog->needs_themes = 1;
        }
        if (kind_needs_hive(kind))
        {
            prog->needs_hive = 1;
        }
        if (kind == RULE_GAS_COUNT && prog->nodes[index].flag != RULE_FLAG_NONE)
        {
            prog->needs_themes = 1;
        }
        ++index;
    }
}

static void set_game_defaults(game_desc *game)
{
    game->seed = 0;
    game->star_count = 64;
    game->resource_multiplier = 1.0f;
    game->hive_initial_colonize = 1.0;
    game->hive_max_density = 1.0;
}

static void apply_game_fields(const json_value *block, game_desc *game)
{
    const json_value *field;

    field = json_object_get(block, "starCount");
    if (field != NULL)
    {
        game->star_count = (int)json_as_number(field);
    }
    field = json_object_get(block, "resourceMultiplier");
    if (field != NULL)
    {
        game->resource_multiplier = (float)json_as_number(field);
    }
    field = json_object_get(block, "hiveInitialColonize");
    if (field != NULL)
    {
        game->hive_initial_colonize = json_as_number(field);
    }
    field = json_object_get(block, "hiveMaxDensity");
    if (field != NULL)
    {
        game->hive_max_density = json_as_number(field);
    }
}

static int parse_game_block(builder *b, const json_value *root, game_desc *game)
{
    const json_value *block;

    set_game_defaults(game);
    block = json_object_get(root, "game");
    if (block == NULL)
    {
        return 0;
    }
    if (!json_is_object(block))
    {
        set_error(b, "top-level 'game' must be an object");
        return -1;
    }
    apply_game_fields(block, game);
    return 0;
}

static int check_top_level_keys(builder *b, const json_value *root)
{
    int index;

    index = 0;
    while (index < root->member_count)
    {
        const char *key;

        key = root->members[index].key;
        if (strcmp(key, "game") != 0 && strcmp(key, "rule") != 0)
        {
            set_error_fmt(b, "unknown top-level key '%s'", key);
            return -1;
        }
        ++index;
    }
    return 0;
}

int parse_conditions_text(const char *text, game_desc *game, rule_program *prog,
                          char *err, int err_size)
{
    builder b;
    json_value *root;
    const json_value *rule;

    if (err != NULL && err_size > 0)
    {
        err[0] = '\0';
    }
    b.prog = prog;
    b.err = err;
    b.err_size = err_size;
    b.has_error = 0;
    prog->node_count = 0;
    prog->root = -1;
    root = json_parse(text);
    if (root == NULL || !json_is_object(root))
    {
        json_free(root);
        set_error(&b, "input is not a valid JSON object");
        return 1;
    }
    if (check_top_level_keys(&b, root) != 0 || parse_game_block(&b, root, game) != 0)
    {
        json_free(root);
        return 1;
    }
    rule = json_object_get(root, "rule");
    if (rule == NULL)
    {
        json_free(root);
        set_error(&b, "missing required top-level key 'rule'");
        return 1;
    }
    prog->root = build_node(&b, rule);
    json_free(root);
    if (b.has_error || prog->root < 0)
    {
        return 1;
    }
    compute_needs(prog);
    return 0;
}

static char *read_whole_file(const char *path, char *err, int err_size)
{
    FILE *file;
    long size;
    char *buffer;
    size_t read_count;

    file = fopen(path, "rb");
    if (file == NULL)
    {
        snprintf(err, (size_t)err_size, "cannot open file '%s'", path);
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0 || fseek(file, 0, SEEK_SET) != 0)
    {
        snprintf(err, (size_t)err_size, "cannot read file '%s'", path);
        fclose(file);
        return NULL;
    }
    buffer = malloc((size_t)size + 1);
    if (buffer == NULL)
    {
        snprintf(err, (size_t)err_size, "out of memory reading '%s'", path);
        fclose(file);
        return NULL;
    }
    read_count = fread(buffer, 1, (size_t)size, file);
    fclose(file);
    buffer[read_count] = '\0';
    return buffer;
}

int parse_conditions_file(const char *path, game_desc *game, rule_program *prog,
                          char *err, int err_size)
{
    char *buffer;
    int result;

    if (err != NULL && err_size > 0)
    {
        err[0] = '\0';
    }
    buffer = read_whole_file(path, err, err_size);
    if (buffer == NULL)
    {
        return 1;
    }
    result = parse_conditions_text(buffer, game, prog, err, err_size);
    free(buffer);
    return result;
}
