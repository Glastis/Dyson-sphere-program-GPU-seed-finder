#ifndef DSP_SEARCH_COMMON_H
#define DSP_SEARCH_COMMON_H

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>
#include "verifier.h"
#include "progress.h"
#include "../cli/config.h"
#include "../output/writer.h"

typedef struct
{
    const cli_config *cfg;
    const game_desc *game;
    const rule_program *prog;
    FILE *out;
    progress_bar *panel;
    pthread_mutex_t out_mutex;
    atomic_int should_stop;
    atomic_int last_match;
    long long found;
    long long max_seeds;
    atomic_llong rejects_sampled;
    atomic_llong divergences;
    atomic_llong false_positives;
    FILE *spill;
    int spill_open;
    int spill_failed;
    char spill_path[256];
    match_record recent[PROGRESS_WINDOW];
    int recent_count;
}
search_state;

static inline double now_seconds(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static inline int resolve_threads(const cli_config *cfg)
{
    long online;

    if (cfg->threads > 0)
    {
        return cfg->threads;
    }
    online = sysconf(_SC_NPROCESSORS_ONLN);
    return online > 0 ? (int)online : 1;
}

static inline const char *results_extension(int format)
{
    if (format == OUTPUT_FORMAT_JSON)
    {
        return "json";
    }
    if (format == OUTPUT_FORMAT_CSV)
    {
        return "csv";
    }
    return "txt";
}

static inline void search_state_init(search_state *state, const cli_config *cfg,
                                     const game_desc *game, const rule_program *prog, FILE *out)
{
    state->cfg = cfg;
    state->game = game;
    state->prog = prog;
    state->out = out;
    state->panel = NULL;
    pthread_mutex_init(&state->out_mutex, NULL);
    atomic_store(&state->should_stop, 0);
    atomic_store(&state->last_match, -1);
    state->found = 0;
    state->max_seeds = cfg->max_seeds;
    atomic_store(&state->rejects_sampled, 0);
    atomic_store(&state->divergences, 0);
    atomic_store(&state->false_positives, 0);
    state->spill = NULL;
    state->spill_open = 0;
    state->spill_failed = 0;
    state->recent_count = 0;
    snprintf(state->spill_path, sizeof(state->spill_path), "dsp-matches.%s",
             results_extension(cfg->format));
}

static inline void open_spill(search_state *state)
{
    int i;

    state->spill = fopen(state->spill_path, "w");
    if (state->spill == NULL)
    {
        state->spill_failed = 1;
        return;
    }
    state->spill_open = 1;
    output_begin(state->spill, state->cfg->format);
    i = 0;
    while (i < state->recent_count)
    {
        output_record(state->spill, state->cfg->format, &state->recent[i], state->game, state->prog);
        ++i;
    }
}

static inline void recent_push(search_state *state, const match_record *rec)
{
    if (state->recent_count < PROGRESS_WINDOW)
    {
        state->recent[state->recent_count] = *rec;
        state->recent_count += 1;
        return;
    }
    memmove(state->recent, state->recent + 1, (PROGRESS_WINDOW - 1) * sizeof(state->recent[0]));
    state->recent[PROGRESS_WINDOW - 1] = *rec;
}

static inline void emit_match_sink(search_state *state, const match_record *rec)
{
    if (state->out != NULL)
    {
        output_record(state->out, state->cfg->format, rec, state->game, state->prog);
        return;
    }
    if (!state->spill_open && !state->spill_failed && state->recent_count == PROGRESS_WINDOW)
    {
        open_spill(state);
    }
    recent_push(state, rec);
    if (state->spill_open)
    {
        output_record(state->spill, state->cfg->format, rec, state->game, state->prog);
    }
}

static inline void emit_match(search_state *state, const match_record *rec)
{
    pthread_mutex_lock(&state->out_mutex);
    if (state->max_seeds == 0 || state->found < state->max_seeds)
    {
        state->found += 1;
        atomic_store(&state->last_match, rec->seed);
        if (state->panel != NULL && !state->panel->is_plain)
        {
            mt_line lines[MT_MAX_LINES];
            int count;

            count = match_tree_render(rec, state->game, state->prog, lines, MT_MAX_LINES);
            progress_push_block(state->panel, lines, count);
        }
        emit_match_sink(state, rec);
        if (state->max_seeds != 0 && state->found >= state->max_seeds)
        {
            atomic_store(&state->should_stop, 1);
        }
    }
    pthread_mutex_unlock(&state->out_mutex);
}

static inline int results_open(const cli_config *cfg, const progress_bar *pb, FILE **out)
{
    if (cfg->output_path != NULL)
    {
        *out = fopen(cfg->output_path, "w");
        return *out != NULL;
    }
    if (!pb->is_plain && isatty(fileno(stdout)))
    {
        *out = NULL;
        return 1;
    }
    *out = stdout;
    return 1;
}

static inline void results_close(search_state *state)
{
    if (state->spill_open)
    {
        output_end(state->spill, state->cfg->format);
    }
    if (state->spill != NULL)
    {
        fclose(state->spill);
        state->spill = NULL;
    }
}

static inline void results_summary(const search_state *state)
{
    if (state->cfg->output_path != NULL)
    {
        fprintf(stderr, "Matches written to %s.\n", state->cfg->output_path);
        return;
    }
    if (state->out != NULL)
    {
        return;
    }
    if (state->found == 0)
    {
        return;
    }
    if (state->spill_open)
    {
        fprintf(stderr, "%lld matches \xe2\x80\x94 full list written to %s (most recent shown above).\n",
                state->found, state->spill_path);
    }
    else
    {
        fprintf(stderr, "%lld matches (all shown above).\n", state->found);
    }
}

static inline int confirm_and_emit(search_state *state, int seed)
{
    match_record rec;

    if (verify_seed(seed, state->game, state->prog, &rec))
    {
        emit_match(state, &rec);
        return 1;
    }
    atomic_fetch_add(&state->false_positives, 1);
    return 0;
}

static inline void sample_reject(search_state *state, int seed, unsigned int *rng)
{
    match_record rec;
    double roll;

    *rng = *rng * 1103515245u + 12345u;
    roll = (double)(*rng >> 8) / (double)(1u << 24);
    if (roll >= state->cfg->reject_sample_rate)
    {
        return;
    }
    atomic_fetch_add(&state->rejects_sampled, 1);
    if (verify_seed(seed, state->game, state->prog, &rec))
    {
        atomic_fetch_add(&state->divergences, 1);
    }
}

static inline void progress_tick(progress_bar *pb, const search_state *state, long long processed,
                                 long long total, long long cursor, double elapsed, int is_final)
{
    progress_info info;

    info.processed = processed;
    info.total = total;
    info.found = state->found;
    info.cursor = cursor;
    info.last_match = atomic_load(&state->last_match);
    info.elapsed = elapsed;
    progress_render(pb, &info, is_final);
}

static inline void warn_divergence(const search_state *state)
{
    long long sampled;
    long long diverged;
    double rate;

    sampled = atomic_load(&state->rejects_sampled);
    diverged = atomic_load(&state->divergences);
    rate = sampled > 0 ? (double)diverged / (double)sampled : 0.0;
    fprintf(stderr, "Reject sampling: %lld sampled, %lld divergences (rate %.3g). False positives: %lld.\n",
            sampled, diverged, rate, (long long)atomic_load(&state->false_positives));
    if (sampled > 0 && rate > state->cfg->divergence_threshold)
    {
        fprintf(stderr, "\n");
        fprintf(stderr, "================================================================\n");
        fprintf(stderr, "  WARNING: GPU and CPU generation diverge too much (rate %.3g >\n", rate);
        fprintf(stderr, "  threshold %.3g). The GPU search is UNRELIABLE: matching seeds\n", state->cfg->divergence_threshold);
        fprintf(stderr, "  are likely being missed. Results cannot be trusted.\n");
        fprintf(stderr, "================================================================\n");
    }
}

#endif
