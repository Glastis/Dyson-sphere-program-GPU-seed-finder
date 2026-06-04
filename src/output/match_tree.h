#ifndef DSP_OUTPUT_MATCH_TREE_H
#define DSP_OUTPUT_MATCH_TREE_H

#include "writer.h"
#include "../worldgen/game_desc.h"
#include "../worldgen/galaxy.h"
#include "../worldgen/planet.h"
#include "../rules/rule_program.h"

/* A confirmed match is rendered, host-side, as a tree that mirrors the rule
 * program (proximity -> systems -> ET/OU -> leaves), one leaf per line, each
 * annotated with its measured value for that seed. Generic over every rule
 * kind, so it isn't tied to "resources". Two columns per line: `left` carries
 * the tree branch + label, `right` the value. */

#define MT_LEFT_MAX   88
#define MT_RIGHT_MAX  32
#define MT_MAX_LINES  48

typedef struct
{
    char left[MT_LEFT_MAX];
    char right[MT_RIGHT_MAX];
    int is_header;
}
mt_line;

/* Canonical galaxy rebuild (ordered habitable pass). Shared with the writer so
 * the tree, the JSON/CSV output and the rule verdict all agree. */
void rebuild_systems(const game_desc *base_game, int seed, galaxy *gx, star_system *systems);

/* Renders `rec` into out[0..return-1] (<= max). Line 0 is the seed header. */
int match_tree_render(const match_record *rec, const game_desc *base_game,
                      const rule_program *prog, mt_line *out, int max);

/* Terminal display width of a UTF-8 string: count code points (our glyphs are
 * all single-width -- no combining marks, no CJK), i.e. bytes that are not
 * continuation bytes. Used by callers to align the value column. */
static inline int mt_utf8_cols(const char *s)
{
    int cols;

    cols = 0;
    while (*s != '\0')
    {
        if (((unsigned char)*s & 0xC0) != 0x80)
        {
            ++cols;
        }
        ++s;
    }
    return cols;
}

#endif
