# Building

Requires **CMake ≥ 3.20**. The GPU build also requires the **CUDA Toolkit ≥ 12.8**
(for `sm_120` / RTX 5070 Blackwell) with `nvcc` on `PATH`.

## GPU build (default)

```sh
mkdir build && cd build
cmake ..          # FAILS FAST with an install message if nvcc is missing
cmake --build . -j
./dsp-seed-finder --help
```

If `nvcc` is not installed, `cmake ..` stops immediately and tells you to install
the CUDA Toolkit ≥ 12.8 (target `sm_120`). It does **not** attempt to build.

## CPU reference build (no GPU, for tests / ground truth)

The same `__host__ __device__` core compiles as plain C for the CPU. This build
is the **reference / ground truth** (closest to the Rust/.NET behaviour) and is
what the GPU pipeline verifies against. It needs no CUDA:

```sh
mkdir build-cpu && cd build-cpu
cmake -DDSP_CPU_REFERENCE=ON ..
cmake --build . -j
./dsp-seed-finder-cpu --help
```

The CPU build is fully functional (multithreaded, all rules, all output formats,
checkpoint/resume); it is simply slower than the GPU build.
