#!/usr/bin/env bash
set -u
cd "$(dirname "$0")/.."
ROOT="$(pwd)"
CC="${CC:-gcc}"
CFLAGS="-O2 -std=c11 -Wall -Wextra"
TMP="$(mktemp -d)"
fail=0

NVCC="$(command -v nvcc 2>/dev/null || true)"
if [ -z "$NVCC" ]; then
    for d in /usr/local/cuda /usr/local/cuda-13 /usr/local/cuda-12 /opt/cuda; do
        if [ -x "$d/bin/nvcc" ]; then NVCC="$d/bin/nvcc"; break; fi
    done
fi

run() { echo "==> $*"; "$@" || { echo "FAILED: $*"; fail=1; }; }

echo "### 1. C core self-test (PRNG golden, seed-0 golden values, habitable_count,"
echo "###    determinism, galaxy invariants, rule-engine sanity)"
run $CC $CFLAGS -o "$TMP/selftest" tests/reference/selftest.c -lm
"$TMP/selftest" || fail=1

echo
echo "### 2. Rule-engine extra sanity"
run $CC $CFLAGS -o "$TMP/rule_test" tests/reference/rule_test.c -lm
"$TMP/rule_test" || fail=1

echo
echo "### 3. CPU reference binary via CMake"
rm -rf "$TMP/build-cpu"; mkdir -p "$TMP/build-cpu"
( cd "$TMP/build-cpu" && cmake -DDSP_CPU_REFERENCE=ON "$ROOT" >/dev/null 2>&1 && cmake --build . >/dev/null 2>&1 ) \
    && echo "PASS: CPU reference build" || { echo "FAIL: CPU reference build"; fail=1; }

echo
echo "### 4. End-to-end: validate + search"
BIN="$TMP/build-cpu/dsp-seed-finder-cpu"
run "$BIN" --validate tests/fixtures/example.json
printf '{ "rule": { "type": "starType", "starTypes": ["BlackHole"] } }' > "$TMP/bh.json"
# The text format is a human-readable tree now; machine checks use CSV (one row
# per matched system, first column = seed) and count distinct seeds.
n=$("$BIN" --format csv --max-seeds 5 --seed-end 100000 "$TMP/bh.json" 2>/dev/null | tail -n +2 | cut -d, -f1 | sort -un | wc -l)
[ "$n" -eq 5 ] && echo "PASS: search returned 5 matches" || { echo "FAIL: expected 5, got $n"; fail=1; }
run "$BIN" --validate tests/fixtures/all-rare-resources.json
n=$("$BIN" --format csv --max-seeds 1 --seed-end 10000 tests/fixtures/all-rare-resources.json 2>/dev/null | tail -n +2 | cut -d, -f1 | sort -un | wc -l)
[ "$n" -eq 1 ] && echo "PASS: rare-resources example stays satisfiable" \
    || { echo "FAIL: rare-resources example matched nothing in 10000 seeds"; fail=1; }
run "$BIN" --validate tests/fixtures/promised-land.json
# Order-independent evaluation makes this rule correctly rare: the first true
# match is seed 89808 (constellation 0,58,63), where system 58 genuinely carries
# the 9 resources and 63 the unipolar magnet. (Before the fix the engine wrongly
# reported seed 7526 here, an order-dependent false positive.)
first=$("$BIN" --format csv --max-seeds 1 --seed-end 100000 tests/fixtures/promised-land.json 2>/dev/null | tail -n +2 | head -1 | cut -d, -f1)
[ "$first" = "89808" ] && echo "PASS: promised-land example stays satisfiable (first match seed 89808)" \
    || { echo "FAIL: promised-land expected first match seed 89808, got '$first'"; fail=1; }
run "$BIN" --validate tests/fixtures/twin-giants.json
# Permissive twin-giants constellation (birth + an O/B giant + an M/K giant within
# maxDistance 15 ly, no resources, no magnet). Matches are frequent, so a bare
# --max-seeds 1 would race across threads non-deterministically; we enumerate the
# small range and take the true minimum, which is deterministically seed 10
# (constellation 0,47,9: giant B at 47, giant M at 9).
first=$("$BIN" --format csv --max-seeds 0 --seed-end 5000 tests/fixtures/twin-giants.json 2>/dev/null | tail -n +2 | cut -d, -f1 | sort -un | head -1)
[ "$first" = "10" ] && echo "PASS: twin-giants example stays satisfiable (first match seed 10)" \
    || { echo "FAIL: twin-giants expected first match seed 10, got '$first'"; fail=1; }

echo
echo "### 5. GPU-mode orchestration (kernel stubbed by the CPU reference) == CPU engine"
printf '{ "rule": {"type":"and","rules":[{"type":"spectr","spectr":["O"]},{"type":"oceanType","oceanType":"Water"}]} }' > "$TMP/ow.json"
run $CC $CFLAGS -I"$ROOT/src" -o "$TMP/gpu_sim" \
    src/main.c src/cli/cli.c src/json/conditions.c src/json/json_value.c src/output/writer.c \
    src/output/match_tree.c \
    src/gpu/gpu_search.c tests/reference/gpu_kernel_stub.c -lpthread -lm
"$BIN" --format csv --max-seeds 0 --seed-end 8000 "$TMP/ow.json" 2>/dev/null | tail -n +2 | cut -d, -f1 | sort -un > "$TMP/cpu.txt"
"$TMP/gpu_sim" --format csv --max-seeds 0 --seed-end 8000 "$TMP/ow.json" 2>/dev/null | tail -n +2 | cut -d, -f1 | sort -un > "$TMP/gpu.txt"
diff -q "$TMP/cpu.txt" "$TMP/gpu.txt" >/dev/null \
    && echo "PASS: GPU orchestration matches CPU engine ($(wc -l < "$TMP/cpu.txt") seeds)" \
    || { echo "FAIL: GPU orchestration diverges"; fail=1; }

echo
echo "### 6. nvcc fail-fast (default build must error without nvcc)"
if [ -n "$NVCC" ]; then
    echo "SKIP: nvcc present ($NVCC)"
else
    rm -rf "$TMP/build-gpu"; mkdir -p "$TMP/build-gpu"
    ( cd "$TMP/build-gpu" && cmake "$ROOT" >/dev/null 2>&1 )
    [ $? -ne 0 ] && echo "PASS: default build fails fast without nvcc" || { echo "FAIL: should have errored"; fail=1; }
fi

echo
echo "### 7. Real GPU kernel vs CPU reference (only when nvcc is available)"
echo "###    7a: FP64-device build is BIT-EXACT (logic guardrail)."
echo "###    7b: FP32-device build (production) has ~0 false negatives."
if [ -z "$NVCC" ]; then
    echo "SKIP: nvcc not found"
else
    export PATH="$(dirname "$NVCC"):$PATH"
    printf '{ "rule": {"type":"and","rules":[{"type":"spectr","spectr":["O"]},{"type":"averageVeinAmount","vein":"Iron","op":"gt","value":120000}]} }' > "$TMP/heavy.json"
    # Ground truth: the FP64 CPU reference. The kernel runs the worldgen in FP32
    # on the GPU (src/worldgen/dsp_math.h), so the device path can diverge from
    # this by float precision -- which is exactly what 7a/7b pin down.
    "$BIN" --format csv --max-seeds 0 --seed-end 60000 "$TMP/heavy.json" 2>/dev/null | tail -n +2 | cut -d, -f1 | sort -un > "$TMP/cpu7.txt"

    # 7a. Recompile the device in FP64 (DSP_FORCE_FP64_DEVICE) and demand exact
    #     equality: this asserts the GPU *logic* is correct independently of the
    #     FP32 rounding, so a real logic regression cannot hide behind precision.
    rm -rf "$TMP/build-gpu64"; mkdir -p "$TMP/build-gpu64"
    if ( cd "$TMP/build-gpu64" && cmake -DDSP_FORCE_FP64_DEVICE=ON "$ROOT" >/dev/null 2>&1 && cmake --build . -j >/dev/null 2>&1 ); then
        "$TMP/build-gpu64/dsp-seed-finder" --format csv --max-seeds 0 --seed-end 60000 "$TMP/heavy.json" 2>/dev/null | tail -n +2 | cut -d, -f1 | sort -un > "$TMP/gpu7_64.txt"
        diff -q "$TMP/cpu7.txt" "$TMP/gpu7_64.txt" >/dev/null \
            && echo "PASS: 7a FP64 GPU kernel matches CPU reference exactly ($(wc -l < "$TMP/cpu7.txt") seeds, heavy vein rule)" \
            || { echo "FAIL: 7a FP64 GPU kernel diverges from CPU reference (logic bug)"; fail=1; }
    else
        echo "FAIL: 7a FP64 GPU build failed"; fail=1
    fi

    # 7b. The production FP32 device build. The CPU re-verifies every GPU hit in
    #     FP64, so emitted matches are a subset of the truth (false positives are
    #     impossible); the only risk is a precision-induced miss (false negative).
    #     Keeping vec3 distances in FP64 should drive the FN rate to ~0.
    rm -rf "$TMP/build-gpu"; mkdir -p "$TMP/build-gpu"
    if ( cd "$TMP/build-gpu" && cmake "$ROOT" >/dev/null 2>&1 && cmake --build . -j >/dev/null 2>&1 ); then
        "$TMP/build-gpu/dsp-seed-finder" --format csv --max-seeds 0 --seed-end 60000 "$TMP/heavy.json" 2>/dev/null | tail -n +2 | cut -d, -f1 | sort -un > "$TMP/gpu7_32.txt"
        fp=$(awk 'NR==FNR{c[$0]=1;next} !($0 in c)' "$TMP/cpu7.txt" "$TMP/gpu7_32.txt" | wc -l | tr -d ' ')
        fn=$(awk 'NR==FNR{g[$0]=1;next} !($0 in g)' "$TMP/gpu7_32.txt" "$TMP/cpu7.txt" | wc -l | tr -d ' ')
        tot=$(wc -l < "$TMP/cpu7.txt" | tr -d ' ')
        if [ "$fp" -ne 0 ]; then
            echo "FAIL: 7b FP32 GPU emitted $fp seeds the CPU rejects (re-verify path broken)"; fail=1
        elif [ "$fn" -eq 0 ]; then
            echo "PASS: 7b FP32 GPU kernel has 0 false negatives vs CPU ($tot truths)"
        elif awk "BEGIN{exit !($fn/$tot <= 0.0001)}"; then
            echo "PASS: 7b FP32 GPU false-negative rate $(awk "BEGIN{printf \"%.5f%%\", 100*$fn/$tot}") within 0.01% ($fn/$tot)"
        else
            echo "FAIL: 7b FP32 GPU false-negative rate $(awk "BEGIN{printf \"%.5f%%\", 100*$fn/$tot}") exceeds 0.01% ($fn/$tot)"; fail=1
        fi
    else
        echo "FAIL: 7b FP32 GPU build failed"; fail=1
    fi
fi

echo
echo "### 8. Progress: redirected stderr is plain ASCII, --plain forces it"
printf '{ "rule": { "type": "starType", "starTypes": ["BlackHole"] } }' > "$TMP/bhp.json"
"$BIN" --max-seeds 3 --seed-end 50000 "$TMP/bhp.json" >/dev/null 2>"$TMP/prog.err"
if grep -qa "$(printf '\033')" "$TMP/prog.err"; then
    echo "FAIL: ANSI escape leaked into redirected (non-tty) stderr"; fail=1
elif grep -qa "seeds |" "$TMP/prog.err"; then
    echo "PASS: redirected progress is plain ASCII"
else
    echo "FAIL: expected plain progress line not found"; fail=1
fi
"$BIN" --plain --max-seeds 3 --seed-end 50000 "$TMP/bhp.json" >/dev/null 2>"$TMP/prog2.err"
if grep -qa "cursor" "$TMP/prog2.err" && ! grep -qa "$(printf '\033')" "$TMP/prog2.err"; then
    echo "PASS: --plain forces clean ASCII progress"
else
    echo "FAIL: --plain did not produce clean ASCII progress"; fail=1
fi

echo
echo "### 9. Live panel: rolling window shows 10, full list spills to file (TTY only)"
SCRIPT="$(command -v script 2>/dev/null || true)"
if [ -z "$SCRIPT" ]; then
    echo "SKIP: 'script' (pty) not available"
else
    printf '{ "rule": { "type": "starType", "starTypes": ["BlackHole"] } }' > "$TMP/bh9.json"
    PIPED=$("$BIN" --max-seeds 0 --seed-end 4000 "$TMP/bh9.json" 2>/dev/null | wc -l | tr -d ' ')
    WORK="$TMP/panel9"; rm -rf "$WORK"; mkdir -p "$WORK"
    ( cd "$WORK" && "$SCRIPT" -qec "$BIN --max-seeds 0 --seed-end 4000 $TMP/bh9.json" /dev/null >/dev/null 2>&1 )
    SPILLED=$( [ -f "$WORK/dsp-matches.txt" ] && wc -l < "$WORK/dsp-matches.txt" | tr -d ' ' || echo 0 )
    if [ "$PIPED" -gt 10 ] && [ "$SPILLED" -eq "$PIPED" ]; then
        echo "PASS: spill file holds the complete match list ($SPILLED == $PIPED) while the window shows 10"
    else
        echo "FAIL: spill=$SPILLED expected to equal piped=$PIPED (and >10)"; fail=1
    fi
    WORK2="$TMP/panel9b"; rm -rf "$WORK2"; mkdir -p "$WORK2"
    ( cd "$WORK2" && "$SCRIPT" -qec "$BIN --max-seeds 5 --seed-end 4000 $TMP/bh9.json" /dev/null >/dev/null 2>&1 )
    if [ -f "$WORK2/dsp-matches.txt" ]; then
        echo "FAIL: <=10-match run must not create a spill file"; fail=1
    else
        echo "PASS: a run with <=10 matches creates no spill file"
    fi
fi

echo
rm -rf "$TMP"
[ "$fail" -eq 0 ] && echo "ALL TESTS PASSED" || echo "SOME TESTS FAILED"
exit $fail
