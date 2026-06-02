#ifndef DSP_GPU_KERNEL_H
#define DSP_GPU_KERNEL_H

#include "../worldgen/game_desc.h"
#include "../rules/rule_program.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct gpu_context gpu_context;

gpu_context *gpu_create(const game_desc *game, const rule_program *prog, long long batch,
                        char *err, int err_size);
long long gpu_batch_size(const gpu_context *ctx);
void gpu_launch(gpu_context *ctx, int slot, long long seed_start, int count);
void gpu_fetch(gpu_context *ctx, int slot, unsigned char *out_hits, int count);
void gpu_destroy(gpu_context *ctx);

#ifdef __cplusplus
}
#endif

#endif
