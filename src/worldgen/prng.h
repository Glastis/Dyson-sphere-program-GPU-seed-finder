#ifndef DSP_PRNG_H
#define DSP_PRNG_H

#include "hd.h"
#include "../constants/prng.h"

typedef struct
{
    int inext;
    int inextp;
    int seed_array[PRNG_STATE_SIZE];
}
dsp_random;

HD static inline int prng_abs(int value)
{
    return value < 0 ? -value : value;
}

HD static inline int prng_wrapping_sub(int lhs, int rhs)
{
    return (int)((unsigned int)lhs - (unsigned int)rhs);
}

HD static inline void prng_seed_initial_fill(dsp_random *rng, int seed)
{
    int num1;
    int num2;
    int index2;
    int round;

    num1 = prng_wrapping_sub(PRNG_SEED_OFFSET, prng_abs(seed));
    rng->seed_array[PRNG_INIT_WRAP] = num1;
    num2 = 1;
    index2 = 0;
    round = 0;
    while (round < PRNG_INIT_ROUNDS)
    {
        index2 += PRNG_INIT_INDEX_STEP;
        if (index2 >= PRNG_INIT_WRAP)
        {
            index2 -= PRNG_INIT_WRAP;
        }
        rng->seed_array[index2] = num2;
        num2 = prng_wrapping_sub(num1, num2);
        if (num2 < 0)
        {
            num2 += PRNG_I32_MAX;
        }
        num1 = rng->seed_array[index2];
        ++round;
    }
}

HD static inline void prng_seed_mix(dsp_random *rng)
{
    int pass;
    int index;
    int other;

    pass = 0;
    while (pass < PRNG_MIXING_PASSES)
    {
        index = 1;
        while (index < PRNG_STATE_SIZE)
        {
            other = 1 + (index + PRNG_MIX_OFFSET) % PRNG_INIT_WRAP;
            rng->seed_array[index] = prng_wrapping_sub(rng->seed_array[index], rng->seed_array[other]);
            if (rng->seed_array[index] < 0)
            {
                rng->seed_array[index] += PRNG_I32_MAX;
            }
            ++index;
        }
        ++pass;
    }
}

HD static inline dsp_random prng_new(int seed)
{
    dsp_random rng;

    prng_seed_initial_fill(&rng, seed);
    prng_seed_mix(&rng);
    rng.inext = PRNG_INEXT_INIT;
    rng.inextp = PRNG_INEXTP_INIT;
    return rng;
}

HD static inline double prng_sample(dsp_random *rng)
{
    int num;

    rng->inext += 1;
    if (rng->inext >= PRNG_STATE_SIZE)
    {
        rng->inext = 1;
    }
    rng->inextp += 1;
    if (rng->inextp >= PRNG_STATE_SIZE)
    {
        rng->inextp = 1;
    }
    num = prng_wrapping_sub(rng->seed_array[rng->inext], rng->seed_array[rng->inextp]);
    if (num < 0)
    {
        num += PRNG_I32_MAX;
    }
    rng->seed_array[rng->inext] = num;
    return (double)num * (1.0 / (double)PRNG_I32_MAX);
}

HD static inline double prng_next_f64(dsp_random *rng)
{
    return prng_sample(rng);
}

HD static inline float prng_next_f32(dsp_random *rng)
{
    return (float)prng_sample(rng);
}

HD static inline int prng_next_i32(dsp_random *rng, int max_value)
{
    return (int)(prng_sample(rng) * (double)max_value);
}

HD static inline unsigned int prng_next_usize(dsp_random *rng)
{
    return (unsigned int)(prng_sample(rng) * (double)PRNG_I32_MAX);
}

HD static inline int prng_next_seed(dsp_random *rng)
{
    return (int)(prng_sample(rng) * (double)PRNG_I32_MAX);
}

#endif
