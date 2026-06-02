#ifndef DSP_GALAXY_GEN_H
#define DSP_GALAXY_GEN_H

#include "hd.h"
#include "prng.h"
#include "vector3.h"
#include "galaxy.h"
#include "../constants/enums.h"
#include "../constants/star_gen.h"
#include <math.h>

HD static inline int check_collision(const vec3 *existing, int len, const vec3 *pt, double min_dist)
{
    double min_dist_sq;
    int index;

    min_dist_sq = min_dist * min_dist;
    index = 0;
    while (index < len)
    {
        if (vec3_distance_sq(&existing[index], pt) < min_dist_sq)
        {
            return 1;
        }
        ++index;
    }
    return 0;
}

HD static inline int pose_attempt(dsp_random *rng, const vec3 *existing, int len, vec3 base,
                                  double step_diff, double min_dist, double flatten, vec3 *out_pt)
{
    double u;
    double w;
    double v;
    double r2;
    double d;
    double mult;

    u = prng_next_f64(rng) * 2.0 - 1.0;
    w = (prng_next_f64(rng) * 2.0 - 1.0) * flatten;
    v = prng_next_f64(rng) * 2.0 - 1.0;
    r2 = prng_next_f64(rng);
    d = u * u + w * w + v * v;
    if (d > 1.0 || d < POSES_MIN_D_SQ)
    {
        return 0;
    }
    mult = (r2 * step_diff + min_dist) / sqrt(d);
    *out_pt = vec3_make(base.x + u * mult, base.y + w * mult, base.z + v * mult);
    return check_collision(existing, len, out_pt, min_dist) ? 0 : 1;
}

HD static inline int poses_first_pass(dsp_random *rng, vec3 *poses, int *len, int max_count,
                                      vec3 *drunk, int *drunk_len, int drunk_num,
                                      double step_diff, double min_dist, double flatten)
{
    int anchor;
    int attempt;
    vec3 pt;

    anchor = 0;
    while (anchor < drunk_num)
    {
        attempt = 0;
        while (attempt < POSES_ATTEMPTS)
        {
            if (pose_attempt(rng, poses, *len, vec3_zero(), step_diff, min_dist, flatten, &pt))
            {
                drunk[*drunk_len] = pt;
                ++(*drunk_len);
                poses[*len] = pt;
                ++(*len);
                if (*len >= max_count)
                {
                    return 1;
                }
                break;
            }
            ++attempt;
        }
        ++anchor;
    }
    return 0;
}

HD static inline void poses_second_pass(dsp_random *rng, vec3 *poses, int *len, int max_count,
                                        vec3 *drunk, int drunk_len,
                                        double step_diff, double min_dist, double flatten)
{
    int round;
    int anchor;
    int attempt;
    vec3 pt;

    round = 0;
    while (round < POSES_WALK_ROUNDS)
    {
        anchor = 0;
        while (anchor < drunk_len)
        {
            if (prng_next_f64(rng) <= POSES_WALK_PROB)
            {
                attempt = 0;
                while (attempt < POSES_ATTEMPTS)
                {
                    if (pose_attempt(rng, poses, *len, drunk[anchor], step_diff, min_dist, flatten, &pt))
                    {
                        drunk[anchor] = pt;
                        poses[*len] = pt;
                        ++(*len);
                        if (*len >= max_count)
                        {
                            return;
                        }
                        break;
                    }
                    ++attempt;
                }
            }
            ++anchor;
        }
        ++round;
    }
}

HD static inline int random_poses(dsp_random *rng, vec3 *poses, int max_count,
                                  double min_dist, double step_diff, double flatten)
{
    vec3 drunk[DSP_MAX_DRUNK];
    int drunk_len;
    int len;
    int drunk_num;
    double r1;

    len = 0;
    drunk_len = 0;
    poses[len++] = vec3_zero();
    r1 = prng_next_f64(rng);
    drunk_num = (int)(r1 * (double)(POSES_MAX_DRUNK_NUM - POSES_MIN_DRUNK_NUM) + (double)POSES_MIN_DRUNK_NUM);
    if (poses_first_pass(rng, poses, &len, max_count, drunk, &drunk_len, drunk_num, step_diff, min_dist, flatten))
    {
        return len;
    }
    poses_second_pass(rng, poses, &len, max_count, drunk, drunk_len, step_diff, min_dist, flatten);
    return len;
}

HD static inline int trim_poses(vec3 *poses, int len, int target_count)
{
    int index;

    index = len - 1;
    while (index >= 0)
    {
        if (index % POSES_ITER_COUNT != 0)
        {
            int shift;

            shift = index;
            while (shift < len - 1)
            {
                poses[shift] = poses[shift + 1];
                ++shift;
            }
            --len;
        }
        if (len <= target_count)
        {
            break;
        }
        --index;
    }
    return len;
}

HD static inline int generate_temp_poses(int seed, int target_count, vec3 *poses)
{
    dsp_random rng;
    int len;

    rng = prng_new(seed);
    len = random_poses(&rng, poses, target_count * POSES_ITER_COUNT,
                       POSES_MIN_DIST, POSES_MAX_STEP_LEN - POSES_MIN_STEP_LEN, POSES_FLATTEN);
    return trim_poses(poses, len, target_count);
}

HD static inline int star_count_value(double base, double rand_factor, double scaled, float rand_value)
{
    float total;

    total = (float)(base * scaled + (double)rand_value * rand_factor);
    return (int)ceilf(total);
}

HD static inline int placement_need_spectr(int index, int white_dwarf_start)
{
    if (index == FORCED_M_INDEX)
    {
        return SPECTR_TYPE_M;
    }
    if (index == white_dwarf_start - 1)
    {
        return SPECTR_TYPE_O;
    }
    return SPECTR_TYPE_X;
}

HD static inline int placement_star_type(int index, int black_hole_start, int neutron_star_start,
                                         int white_dwarf_start, int giant_group_num, int giant_offset)
{
    if (index >= black_hole_start)
    {
        return STAR_TYPE_BLACK_HOLE;
    }
    if (index >= neutron_star_start)
    {
        return STAR_TYPE_NEUTRON_STAR;
    }
    if (index >= white_dwarf_start)
    {
        return STAR_TYPE_WHITE_DWARF;
    }
    if (giant_group_num > 0 && index % giant_group_num == giant_offset)
    {
        return STAR_TYPE_GIANT;
    }
    return STAR_TYPE_MAIN_SEQ;
}

HD static inline void generate_stars(const game_desc *game, galaxy *out)
{
    dsp_random rng;
    vec3 poses[DSP_MAX_TEMP_POSES];
    int star_count;
    int starts[4];
    float r[4];
    int index;

    rng = prng_new(game->seed);
    star_count = generate_temp_poses(prng_next_seed(&rng), game->star_count, poses);
    index = 0;
    while (index < 4)
    {
        r[index] = prng_next_f32(&rng);
        ++index;
    }
    starts[0] = star_count - star_count_value(BLACK_HOLE_BASE, BLACK_HOLE_RAND, (double)star_count, r[0]);
    starts[1] = starts[0] - star_count_value(NEUTRON_STAR_BASE, NEUTRON_STAR_RAND, (double)star_count, r[1]);
    starts[2] = starts[1] - star_count_value(WHITE_DWARF_BASE, WHITE_DWARF_RAND, (double)star_count, r[2]);
    out->game = *game;
    out->star_count = star_count;
    out->habitable_count = 0;
    {
        int giant_num;
        int giant_group_num;
        int giant_offset;

        giant_num = star_count_value(GIANT_STAR_BASE, GIANT_STAR_RAND, (double)star_count, r[3]);
        giant_group_num = (starts[2] - 1) / giant_num;
        giant_offset = giant_group_num / 2;
        index = 0;
        while (index < star_count)
        {
            int seed;

            seed = prng_next_seed(&rng);
            out->star_seeds[index] = seed;
            out->positions[index] = poses[index];
            if (index == 0)
            {
                out->positions[index] = vec3_zero();
                out->star_types[index] = STAR_TYPE_MAIN_SEQ;
                out->need_spectr[index] = SPECTR_TYPE_X;
            }
            else
            {
                out->star_types[index] = placement_star_type(index, starts[0], starts[1], starts[2],
                                                             giant_group_num, giant_offset);
                out->need_spectr[index] = placement_need_spectr(index, starts[2]);
            }
            ++index;
        }
    }
}

#endif
