#define _POSIX_C_SOURCE 200809L
#include "engine.h"
#include "search_common.h"
#include "../checkpoint/checkpoint.h"
#include <stdlib.h>

#define ENGINE_CHUNK 512
#define ENGINE_DEFAULT_BATCH 262144

typedef struct
{
    search_state *state;
    atomic_llong *next_seed;
    long long batch_end;
}
worker_arg;

static long long resolve_batch(const cli_config *cfg)
{
    if (cfg->batch_size > 0)
    {
        return cfg->batch_size;
    }
    return ENGINE_DEFAULT_BATCH;
}

static void process_chunk(search_state *state, long long chunk_start, long long chunk_end)
{
    long long seed;

    seed = chunk_start;
    while (seed < chunk_end)
    {
        match_record rec;

        if (atomic_load(&state->should_stop))
        {
            return;
        }
        if (verify_seed((int)seed, state->game, state->prog, &rec))
        {
            emit_match(state, &rec);
        }
        ++seed;
    }
}

static void *worker_main(void *arg)
{
    worker_arg *wa;

    wa = (worker_arg *)arg;
    while (!atomic_load(&wa->state->should_stop))
    {
        long long chunk_start;
        long long chunk_end;

        chunk_start = atomic_fetch_add(wa->next_seed, ENGINE_CHUNK);
        if (chunk_start >= wa->batch_end)
        {
            break;
        }
        chunk_end = chunk_start + ENGINE_CHUNK;
        if (chunk_end > wa->batch_end)
        {
            chunk_end = wa->batch_end;
        }
        process_chunk(wa->state, chunk_start, chunk_end);
    }
    return NULL;
}

static void run_batch(search_state *state, long long start, long long end, int thread_count)
{
    pthread_t threads[256];
    worker_arg args[256];
    atomic_llong next_seed;
    int count;
    int i;

    count = thread_count > 256 ? 256 : thread_count;
    atomic_store(&next_seed, start);
    i = 0;
    while (i < count)
    {
        args[i].state = state;
        args[i].next_seed = &next_seed;
        args[i].batch_end = end;
        pthread_create(&threads[i], NULL, worker_main, &args[i]);
        ++i;
    }
    i = 0;
    while (i < count)
    {
        pthread_join(threads[i], NULL);
        ++i;
    }
}

static long long resolve_start(const cli_config *cfg, long long *found_resume)
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

static void print_summary(search_state *state, long long processed, double elapsed)
{
    fprintf(stderr, "\nDone. Scanned %lld seeds in %.2fs (%.0f seeds/s). Confirmed matches: %lld.\n",
            processed, elapsed, elapsed > 0.0 ? (double)processed / elapsed : 0.0, state->found);
    fprintf(stderr, "Engine: CPU reference (exact). GPU divergence monitoring is not applicable here.\n");
    results_summary(state);
    warn_divergence(state);
}

int run_search(const cli_config *cfg, const game_desc *game, const rule_program *prog)
{
    search_state state;
    progress_bar bar;
    FILE *out;
    long long total;
    long long batch;
    long long seed;
    long long found_resume;
    long long processed;
    double start_time;
    int thread_count;

    progress_init(&bar, cfg);
    if (!results_open(cfg, &bar, &out))
    {
        fprintf(stderr, "error: cannot open output file '%s'\n", cfg->output_path);
        return 1;
    }
    search_state_init(&state, cfg, game, prog, out);
    state.panel = &bar;
    seed = resolve_start(cfg, &found_resume);
    state.found = found_resume;
    thread_count = resolve_threads(cfg);
    batch = resolve_batch(cfg);
    total = cfg->seed_end - cfg->seed_start;
    processed = 0;
    start_time = now_seconds();
    if (out != NULL)
    {
        output_begin(out, cfg->format);
    }
    progress_tick(&bar, &state, 0, total, seed, 0.0, 0);
    while (seed < cfg->seed_end && !atomic_load(&state.should_stop))
    {
        long long batch_end;

        batch_end = seed + batch;
        if (batch_end > cfg->seed_end)
        {
            batch_end = cfg->seed_end;
        }
        run_batch(&state, seed, batch_end, thread_count);
        processed += batch_end - seed;
        seed = batch_end;
        checkpoint_save(cfg->checkpoint_path, seed, state.found);
        progress_tick(&bar, &state, processed, total, seed - 1, now_seconds() - start_time, 0);
    }
    progress_tick(&bar, &state, processed, total, seed - 1, now_seconds() - start_time, 1);
    if (out != NULL)
    {
        output_end(out, cfg->format);
    }
    results_close(&state);
    if (out != NULL && out != stdout)
    {
        fclose(out);
    }
    print_summary(&state, processed, now_seconds() - start_time);
    pthread_mutex_destroy(&state.out_mutex);
    return 0;
}
