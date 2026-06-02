#ifndef DSP_CHECKPOINT_H
#define DSP_CHECKPOINT_H

#include <stdio.h>

static inline int checkpoint_save(const char *path, long long next_seed, long long found)
{
    FILE *file;

    if (path == NULL)
    {
        return 0;
    }
    file = fopen(path, "w");
    if (file == NULL)
    {
        return -1;
    }
    fprintf(file, "dsp-seed-finder-checkpoint v1\nnext_seed=%lld\nfound=%lld\n", next_seed, found);
    fclose(file);
    return 0;
}

static inline long long checkpoint_load(const char *path, long long *found_out)
{
    FILE *file;
    long long next_seed;
    long long found;

    next_seed = -1;
    found = 0;
    file = fopen(path, "r");
    if (file == NULL)
    {
        return -1;
    }
    if (fscanf(file, "dsp-seed-finder-checkpoint v1\nnext_seed=%lld\nfound=%lld", &next_seed, &found) < 1)
    {
        next_seed = -1;
    }
    fclose(file);
    if (found_out != NULL)
    {
        *found_out = found;
    }
    return next_seed;
}

#endif
