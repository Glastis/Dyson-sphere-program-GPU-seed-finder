#ifndef DSP_RULE_PROGRAM_H
#define DSP_RULE_PROGRAM_H

#include "../worldgen/hd.h"

#define RULE_MAX_NODES 128
#define RULE_MAX_VALUES 32
#define RULE_FLAG_NONE (-1)

enum cond_op
{
    COND_EQ = 0,
    COND_NEQ = 1,
    COND_LT = 2,
    COND_LTE = 3,
    COND_GT = 4,
    COND_GTE = 5,
    COND_BETWEEN = 6,
    COND_NOTBETWEEN = 7
};

enum rule_kind
{
    RULE_AND = 0,
    RULE_OR,
    RULE_BIRTH,
    RULE_STAR_TYPE,
    RULE_BIRTH_DISTANCE,
    RULE_HIVE_COUNT,
    RULE_X_DISTANCE,
    RULE_SPECTR_DISTANCE,
    RULE_LUMINOSITY,
    RULE_SPECTR,
    RULE_DYSON_RADIUS,
    RULE_PLANET_COUNT,
    RULE_SATELLITE_COUNT,
    RULE_GAS_COUNT,
    RULE_TIDAL_LOCK_COUNT,
    RULE_PLANET_IN_DYSON_COUNT,
    RULE_THEME_ID,
    RULE_OCEAN_TYPE,
    RULE_GAS_RATE,
    RULE_AVERAGE_VEIN_AMOUNT,
    RULE_PROXIMITY
};

typedef struct
{
    int op;
    float value;
    float max;
}
condition;

typedef struct
{
    int kind;
    int priority;
    condition cond;
    condition cond2;
    int values[RULE_MAX_VALUES];
    int value_count;
    int vein;
    int ocean_type;
    int gas_type;
    int spectr;
    int flag;
    int children[RULE_MAX_NODES];
    int child_count;
}
rule_node;

typedef struct
{
    rule_node nodes[RULE_MAX_NODES];
    int node_count;
    int root;
    int needs_planets;
    int needs_themes;
    int needs_hive;
}
rule_program;

static DSP_CONST const int COND_SIGN_MASK[6] = { 0x2, 0x5, 0x1, 0x3, 0x4, 0x6 };

HD static inline int cond_eval(const condition *c, float value)
{
    int sign;

    if (c->op == COND_BETWEEN)
    {
        return c->value <= value && value <= c->max;
    }
    if (c->op == COND_NOTBETWEEN)
    {
        return c->value > value || value > c->max;
    }
    sign = (value > c->value) - (value < c->value);
    return (COND_SIGN_MASK[c->op] >> (sign + 1)) & 1;
}

#endif
