# Architecture

- **One GPU thread = one whole galaxy**, processed sequentially. Galaxies are
  **streamed and tested on the fly** with fixed-size local arrays — never
  materialised in full. The default launch is ~256K seeds (override with
  `--batch-size`): large enough to keep every resident thread busy, small enough
  that progress and matches stream every launch instead of arriving in one lump
  after a multi-second batch. Because each thread builds an entire galaxy on its
  own stack (a ~10 KB kernel frame), the per-thread device stack is enlarged with
  `cudaDeviceSetLimit` at startup — the 1 KB default is far too small and would
  fault. Any CUDA launch/execution error aborts the run with a clear message
  rather than silently dropping hits.
- **Shared `__host__ __device__` core** (`src/worldgen`, `src/rules`): the same
  source compiles for the GPU (device math) and the CPU (host `libm`). The CPU
  build is the reference; the GPU build is the fast path whose divergence is
  monitored.
- **`f64`/`f32` widths mirror the Rust exactly** in the PRNG, generation engine
  and threshold rules. This is the intentional exception to the no-float house
  rule; everything else (ids, indexes, counts, enum tags) is integer.
- **Cascaded prefilter**: cheap rules (star type, spectr, luminosity, position…)
  run first and short-circuit; the expensive divergent work (themes, veins,
  gases) only runs when a rule needs it and the cheaper rules already passed.
- **`habitable_count` ordering**: the only cross-star dependency. Planet types
  are computed in strict (star index, planet index) order so the galaxy-global
  habitable counter matches the reference exactly — which is what makes streaming
  generation correct.
- **Correctness pipeline**: the GPU produces candidate hits + a random sample of
  rejects; a CPU verifier pool (`--threads`) **re-verifies every hit on the CPU
  reference before output (zero false positives)** and re-checks the sampled
  rejects to track the GPU↔CPU **divergence rate**. If divergence exceeds
  `--divergence-threshold`, a prominent stderr warning declares the search
  unreliable. The verifier overlaps the next GPU batch (double-buffered) so it
  never stalls the GPU.

## Layout

```
src/
  main.c
  cli/         arg parsing, help, stackable -v
  json/        hand-rolled JSON parser + readable-schema compiler + validator
  constants/   thematic constant headers (prng, star_gen, planet_gen, vein_gen, themes, enums)
  worldgen/    __host__ __device__ core: prng, vector3, galaxy, star, planet, veins, gases
  rules/       __host__ __device__ rule program + per-star evaluation (priority cascade)
  gpu/         CUDA kernel + double-buffered batch pipeline (built only with nvcc)
  verify/      CPU reference engine + verifier pool + reject sampler + divergence monitor
  output/      text / json / csv writers
  checkpoint/  resume
tests/
  reference/   C self-test (golden values + invariants), rule tests, dev dumps, CUDA-interface stub
  fixtures/    sample condition JSON files
```

## Correctness & tests

All tests are **plain C** (no other languages or toolchains). Run them with:

```sh
bash tests/run_tests.sh
```

See [`tests/README.md`](../tests/README.md). In short, `tests/reference/selftest.c`
checks PRNG golden values, seed-0 golden star/planet/theme values,
`habitable_count`, determinism, galaxy invariants over many seeds, and
rule-engine semantics (including that `and` requires all conditions on the same
star). The GPU-mode host orchestration (double-buffered pipeline, verifier pool,
reject sampler, divergence monitor) is exercised by linking it against a C stub
of the CUDA interface backed by the CPU reference, and confirming it yields the
same seed set as the CPU engine — only the literal CUDA kernel launch needs
`nvcc`.

For an external spot-check you can compare `dsp-seed-finder-cpu --format json` (or
the `tests/reference/dump_galaxy` helper) against the live site
`doubleuth.github.io/DSP-Seed-Finder` for a given seed.

## Known limitation

The host `libm` is **not bit-identical** to .NET's transcendental functions, and
GPU transcendentals are not bit-identical to host `libm`. The reference itself
relies on `libm`, so the CPU build follows the established reference; the
reject-sampling **divergence monitor** exists precisely to detect when GPU↔CPU
generation drifts far enough to make results untrustworthy. Vein amounts are
**estimates** (the true in-game algorithm needs Unity) and are reproduced exactly
as the reference estimate — do not treat them as exact in-game values.
