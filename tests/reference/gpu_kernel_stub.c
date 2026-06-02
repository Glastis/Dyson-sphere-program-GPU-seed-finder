#include <stdlib.h>
#include <string.h>
#include "../../src/gpu/gpu_kernel.h"
#include "../../src/verify/verifier.h"

struct gpu_context
{
    game_desc game;
    rule_program prog;
    long long batch;
    unsigned char *hits[2];
};

gpu_context *gpu_create(const game_desc *game, const rule_program *prog, long long batch,
                        char *err, int err_size)
{
    gpu_context *ctx;

    (void)err;
    (void)err_size;
    ctx = (gpu_context *)malloc(sizeof(gpu_context));
    ctx->game = *game;
    ctx->prog = *prog;
    ctx->batch = batch;
    ctx->hits[0] = (unsigned char *)malloc((size_t)batch);
    ctx->hits[1] = (unsigned char *)malloc((size_t)batch);
    return ctx;
}

long long gpu_batch_size(const gpu_context *ctx)
{
    return ctx->batch;
}

void gpu_launch(gpu_context *ctx, int slot, long long seed_start, int count)
{
    int i;

    i = 0;
    while (i < count)
    {
        match_record rec;

        ctx->hits[slot][i] = verify_seed((int)(seed_start + i), &ctx->game, &ctx->prog, &rec) ? 1u : 0u;
        ++i;
    }
}

void gpu_fetch(gpu_context *ctx, int slot, unsigned char *out_hits, int count)
{
    memcpy(out_hits, ctx->hits[slot], (size_t)count);
}

void gpu_destroy(gpu_context *ctx)
{
    free(ctx->hits[0]);
    free(ctx->hits[1]);
    free(ctx);
}
