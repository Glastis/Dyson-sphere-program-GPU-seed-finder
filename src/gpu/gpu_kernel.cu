#define HD __device__
#define DSP_CONST __device__

#include <cuda_runtime.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gpu_kernel.h"
#include "../rules/galaxy_eval.h"

#define GPU_BLOCK 64
#define GPU_STACK_BYTES (48 * 1024)

static void gpu_fatal_if_error(cudaError_t e, const char *where)
{
    if (e == cudaSuccess)
    {
        return;
    }
    fprintf(stderr,
            "\nfatal: CUDA error during %s: %s\n"
            "  The GPU kernel failed, so results would be wrong. Aborting.\n",
            where, cudaGetErrorString(e));
    exit(2);
}

struct gpu_context
{
    game_desc *d_game;
    rule_program *d_prog;
    unsigned char *d_hits[2];
    unsigned char *h_hits[2];
    cudaStream_t streams[2];
    long long batch;
};

__global__ static void scan_kernel(long long seed_start, int count, const game_desc *game,
                                   const rule_program *prog, unsigned char *hits)
{
    int tid;

    tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= count)
    {
        return;
    }
    hits[tid] = seed_matches((int)(seed_start + tid), game, prog) ? 1u : 0u;
}

static int gpu_alloc_buffers(gpu_context *ctx)
{
    int slot;

    slot = 0;
    while (slot < 2)
    {
        if (cudaMalloc(&ctx->d_hits[slot], (size_t)ctx->batch) != cudaSuccess)
        {
            return -1;
        }
        if (cudaHostAlloc(&ctx->h_hits[slot], (size_t)ctx->batch, cudaHostAllocDefault) != cudaSuccess)
        {
            return -1;
        }
        cudaStreamCreate(&ctx->streams[slot]);
        ++slot;
    }
    return 0;
}

extern "C" gpu_context *gpu_create(const game_desc *game, const rule_program *prog, long long batch,
                                   char *err, int err_size)
{
    gpu_context *ctx;

    ctx = (gpu_context *)calloc(1, sizeof(gpu_context));
    if (ctx == NULL)
    {
        snprintf(err, err_size, "out of host memory");
        return NULL;
    }
    ctx->batch = batch;
    if (cudaDeviceSetLimit(cudaLimitStackSize, GPU_STACK_BYTES) != cudaSuccess)
    {
        snprintf(err, err_size, "could not enlarge the CUDA per-thread stack (need %d bytes)",
                 GPU_STACK_BYTES);
        gpu_destroy(ctx);
        return NULL;
    }
    if (cudaMalloc(&ctx->d_game, sizeof(game_desc)) != cudaSuccess
        || cudaMalloc(&ctx->d_prog, sizeof(rule_program)) != cudaSuccess
        || gpu_alloc_buffers(ctx) != 0)
    {
        snprintf(err, err_size, "CUDA allocation failed (need an sm_120 GPU with enough VRAM)");
        gpu_destroy(ctx);
        return NULL;
    }
    cudaMemcpy(ctx->d_game, game, sizeof(game_desc), cudaMemcpyHostToDevice);
    cudaMemcpy(ctx->d_prog, prog, sizeof(rule_program), cudaMemcpyHostToDevice);
    return ctx;
}

extern "C" long long gpu_batch_size(const gpu_context *ctx)
{
    return ctx->batch;
}

extern "C" void gpu_launch(gpu_context *ctx, int slot, long long seed_start, int count)
{
    int blocks;

    blocks = (count + GPU_BLOCK - 1) / GPU_BLOCK;
    scan_kernel<<<blocks, GPU_BLOCK, 0, ctx->streams[slot]>>>(seed_start, count, ctx->d_game,
                                                              ctx->d_prog, ctx->d_hits[slot]);
    gpu_fatal_if_error(cudaGetLastError(), "kernel launch");
    cudaMemcpyAsync(ctx->h_hits[slot], ctx->d_hits[slot], (size_t)count,
                    cudaMemcpyDeviceToHost, ctx->streams[slot]);
}

extern "C" void gpu_fetch(gpu_context *ctx, int slot, unsigned char *out_hits, int count)
{
    gpu_fatal_if_error(cudaStreamSynchronize(ctx->streams[slot]), "kernel execution");
    memcpy(out_hits, ctx->h_hits[slot], (size_t)count);
}

/* Benchmark helper: time ONLY the scan kernel (no D2H copy, no CPU verify) over
 * a [seed_start, seed_start+count) range, averaged over `iters` launches, using
 * CUDA events. Returns the best (minimum) per-iteration kernel time in seconds.
 * This isolates pure device throughput from the CPU re-verification cost, which
 * otherwise dominates the wall time on dense rulesets and masks the FP32 kernel
 * speedup we want to measure (build once in FP32, once with DSP_FORCE_FP64_DEVICE,
 * compare the two best times). Gated behind the DSP_BENCH_KERNEL env var. */
extern "C" double gpu_bench_kernel(gpu_context *ctx, long long seed_start, int count, int iters)
{
    cudaEvent_t beg;
    cudaEvent_t end;
    int blocks;
    double best;
    int it;

    blocks = (count + GPU_BLOCK - 1) / GPU_BLOCK;
    cudaEventCreate(&beg);
    cudaEventCreate(&end);
    best = 1e30;
    /* one warmup launch (JIT, caches, clocks) excluded from the timing */
    scan_kernel<<<blocks, GPU_BLOCK, 0, ctx->streams[0]>>>(seed_start, count, ctx->d_game,
                                                           ctx->d_prog, ctx->d_hits[0]);
    cudaStreamSynchronize(ctx->streams[0]);
    it = 0;
    while (it < iters)
    {
        float ms;

        cudaEventRecord(beg, ctx->streams[0]);
        scan_kernel<<<blocks, GPU_BLOCK, 0, ctx->streams[0]>>>(seed_start, count, ctx->d_game,
                                                               ctx->d_prog, ctx->d_hits[0]);
        cudaEventRecord(end, ctx->streams[0]);
        gpu_fatal_if_error(cudaEventSynchronize(end), "bench kernel");
        ms = 0.0f;
        cudaEventElapsedTime(&ms, beg, end);
        if ((double)ms / 1000.0 < best)
        {
            best = (double)ms / 1000.0;
        }
        ++it;
    }
    cudaEventDestroy(beg);
    cudaEventDestroy(end);
    return best;
}

extern "C" void gpu_destroy(gpu_context *ctx)
{
    int slot;

    if (ctx == NULL)
    {
        return;
    }
    slot = 0;
    while (slot < 2)
    {
        if (ctx->d_hits[slot] != NULL)
        {
            cudaFree(ctx->d_hits[slot]);
        }
        if (ctx->h_hits[slot] != NULL)
        {
            cudaFreeHost(ctx->h_hits[slot]);
        }
        ++slot;
    }
    if (ctx->d_game != NULL)
    {
        cudaFree(ctx->d_game);
    }
    if (ctx->d_prog != NULL)
    {
        cudaFree(ctx->d_prog);
    }
    free(ctx);
}
