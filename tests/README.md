# Tests & validation (C only)

Run everything:

```sh
bash tests/run_tests.sh
```

Everything here is plain C compiled with the system `gcc` (plus CMake and a POSIX
shell to drive the runs). No other languages or toolchains are involved.

## What is checked

`reference/selftest.c` — the main self-test:

- **PRNG golden values** — `DspRandom` (the .NET subtractive PRNG ported in
  `src/worldgen/prng.h`) must reproduce known sequence values
  (`prng_new(42)` first sample and first `next_seed`).
- **Seed-0 golden values** — star 0 of seed 0 is a G-type `MainSeqStar` with
  `dysonRadius == 21900`, luminosity ≈ 0.988, 4 planets, and a birth Ocean home
  planet with theme id 1 ("Ocean 1"). These are stable reference values for the
  ported worldgen.
- **`habitable_count`** — the galaxy-global counter reaches the expected value
  for seeds 0 and 1 (22 and 21), proving the strict-index-order planet-type
  computation is correct.
- **Determinism** — regenerating the same seed yields the same result.
- **Galaxy invariants** over seeds 0..500 — every galaxy contains a black hole,
  and every birth system has an Ocean home planet (the `is_birth` planet).
- **Rule-engine sanity** — `starType BlackHole` and `birth` match every seed;
  `AND(starType BlackHole, luminosity >= 2.5)` matches **zero** seeds (degenerate
  stars are dim), proving the `and` combinator requires all conditions on the
  **same** star.

`reference/rule_test.c` — additional rule-engine spot checks.

`reference/gpu_kernel_stub.c` — a C stub of the `gpu_kernel.h` interface that
backs the "GPU" with the CPU reference. The test harness builds the real GPU-mode
host orchestrator (`src/gpu/gpu_search.c`: double-buffered batch pipeline,
multithreaded verifier pool, reject sampler, divergence monitor) against this stub
and confirms it produces the **same seed set** as the CPU engine — validating the
entire GPU-mode control flow without a CUDA toolchain. Only the literal CUDA
kernel launch in `gpu_kernel.cu` (which simply calls the same `seed_matches` core)
needs `nvcc` to run.

## Dev dump tools

- `reference/dump_galaxy.c` — prints a full galaxy (stars, planets, themes,
  veins, gases, `habitable_count`) for a seed; useful for eyeballing output and
  for manual spot-checks against the live site `doubleuth.github.io/DSP-Seed-Finder`.
- `reference/star_dump.c` — prints just the per-star scalar fields.
- `reference/prng_check.c` — prints raw PRNG sequences.

## Module self-tests

The JSON parser, CLI and output writers each ship a standalone C self-test
(`src/json/json_selftest.c`, `src/cli/cli_selftest.c`, `src/output/output_selftest.c`),
compiled and run during development.

## Note on numeric fidelity

The host `libm` is not bit-identical to .NET's transcendental functions, and GPU
transcendentals are not bit-identical to host `libm`. The CPU build follows the
established reference (host `libm`, as the original Rust tool does); the
reject-sampling **divergence monitor** exists to flag when GPU↔CPU generation
drifts far enough to make results untrustworthy. Vein **amounts** are estimates
(the exact in-game algorithm needs Unity) and are reproduced as the reference
estimate, not as exact in-game values.
