#include <stdio.h>
#include "../../src/worldgen/prng.h"

int main(void)
{
    int seeds[7];
    int count;
    int s;

    seeds[0] = 0;
    seeds[1] = 1;
    seeds[2] = 42;
    seeds[3] = 12345;
    seeds[4] = 99999999;
    seeds[5] = -7;
    seeds[6] = 2147483646;
    count = 7;

    s = 0;
    while (s < count)
    {
        dsp_random rng;
        dsp_random rng2;
        int i;

        rng = prng_new(seeds[s]);
        printf("%d", seeds[s]);
        i = 0;
        while (i < 8)
        {
            printf(" %.17f", prng_sample(&rng));
            ++i;
        }
        rng2 = prng_new(seeds[s]);
        i = 0;
        while (i < 4)
        {
            printf(" %d", prng_next_seed(&rng2));
            ++i;
        }
        printf("\n");
        ++s;
    }
    return 0;
}
