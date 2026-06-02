# dsp-seed-finder

A brute-force **Dyson Sphere Program** galaxy-seed finder written in **C / CUDA**.
It generates galaxies in batches on the GPU and prints the seeds whose galaxy
contains at least one star system matching a set of conditions supplied as a
readable JSON file.

The worldgen + rule core is a faithful port of the Rust/WASM reimplementation in
[`DSP-Seed-Finder`](../tmp/DSP-Seed-Finder) — the **same seeds match**.

---

## What "match" means

A seed (= one galaxy) **matches** when **at least one star system in that galaxy
satisfies all the rules**. The search keeps the first `--max-seeds` matches it
finds, in any order, then stops.

The rule tree reduces to a per-star boolean: a combinator `and`/`or` is `&&`/`||`
of its children evaluated on the same star, and the seed matches if **any** star
makes the top-level rule true. Rules are evaluated cheapest-first (a cascaded
prefilter) and short-circuited.

---

## Build

Requires **CMake ≥ 3.20**. The GPU build requires the **CUDA Toolkit ≥ 12.8**
(for `sm_120` / RTX 5070 Blackwell) with `nvcc` on `PATH`.

### GPU build (default)

```sh
mkdir build && cd build
cmake ..          # FAILS FAST with an install message if nvcc is missing
cmake --build . -j
./dsp-seed-finder --help
```

If `nvcc` is not installed, `cmake ..` stops immediately and tells you to install
the CUDA Toolkit ≥ 12.8 (target `sm_120`). It does **not** attempt to build.

### CPU reference build (no GPU, for tests / ground truth)

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

---

## Usage

```
dsp-seed-finder [options] <conditions.json>
```

| Option | Default | Meaning |
|---|---|---|
| `<conditions.json>` / `-i, --input <file>` | — | The conditions+game JSON. |
| `--max-seeds <N>` | `10` | Stop after N confirmed matches. `0` = scan whole range. |
| `--seed-start <N>` / `--seed-end <N>` | `0` / `99999999` | Search range (the in-game 8-digit range). |
| `--batch-size <N>` | auto | GPU seeds per launch. |
| `--format text\|json\|csv` | `text` | Output format. |
| `-o, --output <file>` | stdout | Output destination. |
| `--checkpoint <file>` / `--resume <file>` | — | Persist / restore search position. |
| `--validate` | — | Parse + validate the JSON and exit without searching. |
| `--plain` | — | Plain ASCII progress (no live panel, no colours). |
| `--threads <N>` | hw concurrency | CPU verifier-pool size. |
| `--reject-sample-rate <r>` | `1e-4` | Fraction of GPU rejects re-checked on the CPU. |
| `--divergence-threshold <r>` | `1e-3` | Print a reliability warning above this divergence rate. |
| `-v` / `--verbose` | — | Stackable (`-v`, `-vv`, `-vvv`). |
| `--star-count`, `--resource-mult`, `--hive-initial-colonize`, `--hive-max-density` | from JSON | Override game params (CLI wins over JSON, JSON over defaults). |
| `--help` | — | Show help. |

### Progress display

Progress goes to **stderr**; results go to **stdout** (or `--output`), so the two
never mix in a pipeline. When stderr is a terminal you get a **live panel**: a
closed, rounded box with the colour block progress bar, the seeds scanned, the
throughput (seeds/s, auto-scaled to k/s or M/s), the ETA and elapsed time, and
the running cursor (latest seed tested). Below a divider, a **rolling window of
the last 10 matching seeds** is shown as a two-column table (`seed` / `stars`,
the matching star indexes). The panel appears immediately at 0% (so the tool
never looks hung while the first GPU batch is in flight) and redraws in place.

```
╭─ ◆ DSP SEED FINDER ──────────────────────────────────╮
│  ███████████████████▋░░░░░░░░░░░░░░░░░░   43.7%       │
│  seeds   2,621,440 / 99,999,999                      │
│  rate    480.2 M/s  ·  ETA 00:03  ·  00:05           │
│  cursor  2,621,439                                   │
├─ matches (508) ──────────────────────────────────────┤
│  seed          stars                                 │
│  196,400       59                                    │
│  196,311       12, 40                                │
│  ...                                                 │
╰──────────────────────────────────────────────────────╯
```

To keep the terminal clean, the panel does **not** stream every matching seed to
the scrolling terminal. The window shows the 10 most recent; when the run finds
**more than 10** and stdout is the terminal (no `--output`, not a pipe), the full
list is written to **`dsp-matches.txt`** (`.json`/`.csv` for those formats) in
the current directory, and the final summary points you to it. When stdout is a
pipe/file or `--output` is given, every match is written there as usual — the
window is purely a live preview.

Pass `--plain` for a simple, non-dynamic, **ASCII** progress line (one appended
line per update, no colours, no cursor movement, matches streamed to stdout) —
handy for logs and CI. The same plain mode is selected automatically when stderr
is **not** a terminal (a pipe or file) or when `-v`/`--verbose` is given, so
redirected output stays clean.

### Examples

```sh
# 10 seeds whose galaxy has a black hole with a lot of unipolar magnets, as text
dsp-seed-finder conditions.json

# full system details as JSON, 5 matches
dsp-seed-finder --max-seeds 5 --format json conditions.json > matches.json

# resumeable long run
dsp-seed-finder --checkpoint run.ckpt --max-seeds 0 conditions.json
# ... interrupt, later ...
dsp-seed-finder --resume run.ckpt --max-seeds 0 conditions.json

# a single system holding every rare resource that can coexist
dsp-seed-finder tests/fixtures/all-rare-resources.json

# the same system, plus a Unipolar Magnet system within reach (< 8 ly)
dsp-seed-finder tests/fixtures/promised-land.json
```

`tests/fixtures/all-rare-resources.json` asks for **one** star system that gathers
the rare resources at once, each through the mechanic that produces it: a sulfuric
acid **ocean** (`ocean: Sulfur`), Fire Ice and Deuterium off its **gas giants**
(`gasRate`), and the mineable **veins** Crude Oil, Spiniform Stalagmite Crystal,
Kimberlite Ore, Fractal Silicon, Grating Crystal and Organic Crystal
(`averageVeinAmount`). It matches roughly 1 seed in 1200.

It covers 9 of the 10 rare resources on purpose: **Unipolar Magnet** is left out
because it never lands on the *same* system as the other nine. Unipolar Magnet
only spawns on black-hole / neutron-star systems, and no such system ever also
carries the full set the query needs (the sulfuric-acid ocean, the gas-giant
gases and the five mineable rare veins) — a scan over 300 000 seeds finds zero.
Adding `Magnet` to this same `and` would make the query match nothing.

`tests/fixtures/promised-land.json` keeps that 9-resource system and asks for a
Unipolar Magnet system **nearby** rather than on the same star. Because that is a
two-system, regional query — and every other rule is judged on a *single* system
— it uses the `proximity` rule, whose `systems` array spells out one rule tree per
star and a `maxDistance` between them:

```json
{ "type": "proximity", "maxDistance": 8.0, "systems": [
  { "type": "and", "rules": [ /* the nine resources */ ] },
  { "vein": "Magnet", "op": "present" }
] }
```

This reads literally as "an all-rares system **and** a `Magnet` system within 8
light-years of it" — the resource is named, and the cross-system scope is the
structure itself, not a black-hole proxy hidden inside `xDistance`. The first
match is seed `5457`: system 4 gathers the nine resources and the black hole 7.9
ly away supplies the magnet. It matches roughly 1 seed in 5500. (Since Unipolar
Magnet only spawns on black-hole / neutron-star systems, this is equivalent to —
but far clearer than — asking for such a system within range.)

---

## Conditions JSON schema

Human-readable, text keys, enum **names** (not the web UI's numeric values).

```json
{
  "game": {
    "starCount": 64,
    "resourceMultiplier": 1.0,
    "hiveInitialColonize": 1.0,
    "hiveMaxDensity": 1.0
  },
  "rule": {
    "type": "and",
    "rules": [
      { "type": "starType", "starTypes": ["BlackHole", "NeutronStar"] },
      { "type": "luminosity", "op": "gte", "value": 2.5 },
      { "type": "spectr", "spectr": ["O", "B"] },
      { "type": "averageVeinAmount", "vein": "Iron", "op": "gt", "value": 120000 },
      { "type": "planetCount", "op": "gte", "value": 4, "excludeGiant": true },
      { "type": "or", "rules": [
        { "type": "oceanType", "oceanType": "Water" },
        { "type": "tidalLockCount", "op": "gte", "value": 1 }
      ]}
    ]
  }
}
```

- `game` is optional; missing keys fall back to defaults
  (`starCount=64`, `resourceMultiplier=1.0`, `hiveInitialColonize=1.0`, `hiveMaxDensity=1.0`).
- A **condition** is inline on the rule: `{ "op": "gte", "value": 2.5 }`, or for
  ranges `{ "op": "between", "min": 1.0, "max": 3.0 }`. Ops: `eq neq lt lte gt gte between notBetween`,
  plus the value-free shortcuts `present` (same as `gt 0`) and `absent` (same as `lte 0`).
- **`type` is inferred** from a discriminating key when omitted: `{ "vein": ... }`
  ⇒ `averageVeinAmount`, `{ "gas": ... }` ⇒ `gasRate`, `{ "ocean": ... }` ⇒
  `oceanType`. So `{ "vein": "Magnet", "op": "present" }` is the short form of
  `{ "type": "averageVeinAmount", "vein": "Magnet", "op": "gt", "value": 0 }`.
- `spectrDistance` carries two nested conditions: `"count": {...}` and `"distance": {...}`.
- `proximity` is a multi-system combinator: `"systems"` is an array of rule trees
  (the first is the *anchor*, evaluated on the candidate system; each of the rest
  must be satisfied by a **distinct** other system within `"maxDistance"` of the
  anchor). With two entries it reads "system A with a system B nearby". (Every
  rule except `proximity`, `xDistance` and `spectrDistance` is judged on one
  system; those three reach across the galaxy.)

### Supported rules

`and`, `or`, `proximity` combinators, plus the simple rules:
`birth`, `starType`, `birthDistance`, `hiveCount` (`initial`),
`xDistance` (`all`), `spectrDistance`, `luminosity`, `spectr`, `dysonRadius`,
`planetCount` (`excludeGiant`), `satelliteCount`, `gasCount` (`ice`),
`tidalLockCount`, `planetInDysonCount` (`includeGiant`), `themeId`, `oceanType`,
`gasRate`, `averageVeinAmount`.

Enum names mirror the Rust `enums.rs` / web `enums.ts`
(`StarType`, `SpectrType`, `VeinType`, `OceanType`, `GasType`), except that the
rare `VeinType` names use the in-game resource names instead of the reference's
internal codes, so a query reads as the resource it finds:

| JSON `vein` | in-game resource | reference code |
| --- | --- | --- |
| `Kimberlite` | Kimberlite Ore | `Diamond` |
| `Stalagmite` | Spiniform Stalagmite Crystal | `Crysrub` |
| `Grating` | Grating Crystal | `Grat` |
| `Organic` | Organic Crystal | `Bamboo` |
| `Magnet` | Unipolar Magnet | `Mag` |

Unknown keys, bad enum names and malformed conditions are rejected with a clear
error (exercised by `--validate`).

The web UI's `composite`/`compositeAnd`/`compositeOr` count-rules are **out of
scope for v1** (the match criterion is fixed to "≥1 system satisfies the rule").

---

## Architecture

- **One GPU thread = one whole galaxy**, processed sequentially. Galaxies are
  **streamed and tested on the fly** with fixed-size local arrays — never
  materialised in full. The default launch is ~256K seeds (override with
  `--batch-size`): large enough to keep every resident thread busy, small enough
  that progress and matches stream every launch instead of arriving in one lump
  after a multi-second batch. Because
  each thread builds an entire galaxy on its own stack (a ~10 KB kernel frame),
  the per-thread device stack is enlarged with `cudaDeviceSetLimit` at startup —
  the 1 KB default is far too small and would fault. Any CUDA launch/execution
  error aborts the run with a clear message rather than silently dropping hits.
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

### Layout

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

---

## Correctness & tests

All tests are **plain C** (no other languages or toolchains). Run them with:

```sh
bash tests/run_tests.sh
```

See [`tests/README.md`](tests/README.md). In short, `tests/reference/selftest.c`
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

### Known limitation

The host `libm` is **not bit-identical** to .NET's transcendental functions, and
GPU transcendentals are not bit-identical to host `libm`. The reference itself
relies on `libm`, so the CPU build follows the established reference; the
reject-sampling **divergence monitor** exists precisely to detect when GPU↔CPU
generation drifts far enough to make results untrustworthy. Vein amounts are
**estimates** (the true in-game algorithm needs Unity) and are reproduced exactly
as the reference estimate — do not treat them as exact in-game values.
