#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include "gpu_kernel.h"
#include "../verify/engine.h"
#include "../verify/search_common.h"
#include "../checkpoint/checkpoint.h"

#define GPU_MAX_THREADS 256
#define GPU_DEFAULT_BATCH (1 << 18)

typedef struct
{
    search_state *state;
    long long seed_start;
    const unsigned char *hits;
    int begin;
    int end;
    unsigned int rng;
}
verify_arg;

static void *verify_worker(void *arg)
{
    verify_arg *va;
    int i;

    va = (verify_arg *)arg;
    i = va->begin;
    while (i < va->end)
    {
        int seed;

        if (atomic_load(&va->state->should_stop) && va->state->max_seeds != 0)
        {
            break;
        }
        seed = (int)(va->seed_start + i);
        if (va->hits[i])
        {
            confirm_and_emit(va->state, seed);
        }
        else
        {
            sample_reject(va->state, seed, &va->rng);
        }
        ++i;
    }
    return NULL;
}

static void verify_batch(search_state *state, long long seed_start, int count,
                         const unsigned char *hits, int thread_count)
{
    pthread_t threads[GPU_MAX_THREADS];
    verify_arg args[GPU_MAX_THREADS];
    int slice;
    int i;

    slice = (count + thread_count - 1) / thread_count;
    i = 0;
    while (i < thread_count)
    {
        args[i].state = state;
        args[i].seed_start = seed_start;
        args[i].hits = hits;
        args[i].begin = i * slice;
        args[i].end = (i + 1) * slice > count ? count : (i + 1) * slice;
        args[i].rng = (unsigned int)seed_start ^ (0x9e3779b9u * (unsigned int)(i + 1));
        pthread_create(&threads[i], NULL, verify_worker, &args[i]);
        ++i;
    }
    i = 0;
    while (i < thread_count)
    {
        pthread_join(threads[i], NULL);
        ++i;
    }
}

typedef struct
{
    long long seed_start;
    int count;
    int slot;
    int is_pending;
}
batch_ref;

typedef struct
{
    search_state *state;
    gpu_context *gpu;
    progress_bar *bar;
    unsigned char *hbuf;
    int thread_count;
    long long processed;
    long long cursor;
    double start_time;
}
pipeline;

static void consume_prev(pipeline *pl, const batch_ref *prev)
{
    const cli_config *cfg;
    long long total;

    cfg = pl->state->cfg;
    gpu_fetch(pl->gpu, prev->slot, pl->hbuf, prev->count);
    verify_batch(pl->state, prev->seed_start, prev->count, pl->hbuf, pl->thread_count);
    pl->processed += prev->count;
    pl->cursor = prev->seed_start + prev->count - 1;
    total = cfg->seed_end - cfg->seed_start;
    checkpoint_save(cfg->checkpoint_path, prev->seed_start + prev->count, pl->state->found);
    progress_tick(pl->bar, pl->state, pl->processed, total, pl->cursor, now_seconds() - pl->start_time, 0);
}

static int next_count(const cli_config *cfg, long long seed, long long batch)
{
    long long end;

    end = seed + batch > cfg->seed_end ? cfg->seed_end : seed + batch;
    return (int)(end - seed);
}

static void run_pipeline(pipeline *pl, long long seed)
{
    const cli_config *cfg;
    batch_ref prev;
    long long batch;
    int slot;

    cfg = pl->state->cfg;
    batch = gpu_batch_size(pl->gpu);
    prev.is_pending = 0;
    slot = 0;
    while (seed < cfg->seed_end && !atomic_load(&pl->state->should_stop))
    {
        int count;

        count = next_count(cfg, seed, batch);
        gpu_launch(pl->gpu, slot, seed, count);
        if (prev.is_pending)
        {
            consume_prev(pl, &prev);
        }
        prev.seed_start = seed;
        prev.count = count;
        prev.slot = slot;
        prev.is_pending = 1;
        seed += count;
        slot ^= 1;
    }
    if (prev.is_pending)
    {
        consume_prev(pl, &prev);
    }
}

static long long resolve_seed_start(const cli_config *cfg, long long *found_resume)
{
    long long resume;

    *found_resume = 0;
    if (cfg->resume_path == NULL)
    {
        return cfg->seed_start;
    }
    resume = checkpoint_load(cfg->resume_path, found_resume);
    return resume < cfg->seed_start ? cfg->seed_start : resume;
}

int run_search(const cli_config *cfg, const game_desc *game, const rule_program *prog)
{
    search_state state;
    progress_bar bar;
    pipeline pl;
    FILE *out;
    gpu_context *gpu;
    char err[256];
    long long batch_default;
    long long seed;
    long long found_resume;

    batch_default = cfg->batch_size > 0 ? cfg->batch_size : GPU_DEFAULT_BATCH;
    gpu = gpu_create(game, prog, batch_default, err, sizeof(err));
    if (gpu == NULL)
    {
        fprintf(stderr, "error: %s\n", err);
        return 1;
    }
    progress_init(&bar, cfg);
    if (!results_open(cfg, &bar, &out))
    {
        fprintf(stderr, "error: cannot open output file '%s'\n", cfg->output_path);
        gpu_destroy(gpu);
        return 1;
    }
    search_state_init(&state, cfg, game, prog, out);
    state.panel = &bar;
    seed = resolve_seed_start(cfg, &found_resume);
    state.found = found_resume;
    pl.state = &state;
    pl.gpu = gpu;
    pl.bar = &bar;
    pl.hbuf = (unsigned char *)malloc((size_t)gpu_batch_size(gpu));
    pl.thread_count = resolve_threads(cfg) > GPU_MAX_THREADS ? GPU_MAX_THREADS : resolve_threads(cfg);
    pl.processed = 0;
    pl.cursor = seed > 0 ? seed - 1 : 0;
    pl.start_time = now_seconds();
    if (out != NULL)
    {
        output_begin(out, cfg->format);
    }
    progress_tick(&bar, &state, 0, cfg->seed_end - cfg->seed_start, seed, 0.0, 0);
    run_pipeline(&pl, seed);
    progress_tick(&bar, &state, pl.processed, cfg->seed_end - cfg->seed_start, pl.cursor,
                  now_seconds() - pl.start_time, 1);
    if (out != NULL)
    {
        output_end(out, cfg->format);
    }
    results_close(&state);
    fprintf(stderr, "\nDone. Scanned %lld seeds in %.2fs. Confirmed matches: %lld.\n",
            pl.processed, now_seconds() - pl.start_time, state.found);
    results_summary(&state);
    warn_divergence(&state);
    free(pl.hbuf);
    if (out != NULL && out != stdout)
    {
        fclose(out);
    }
    gpu_destroy(gpu);
    return 0;
}
