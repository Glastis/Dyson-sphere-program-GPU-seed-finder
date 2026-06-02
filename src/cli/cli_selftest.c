#include <stdio.h>

#include "cli.h"

static const char *format_name(int format)
{
    if (format == OUTPUT_FORMAT_JSON)
    {
        return "json";
    }

    if (format == OUTPUT_FORMAT_CSV)
    {
        return "csv";
    }

    return "text";
}

static void dump_config(const cli_config *cfg)
{
    printf("  input_path=%s\n", cfg->input_path ? cfg->input_path : "(null)");
    printf("  output_path=%s\n", cfg->output_path ? cfg->output_path : "(null)");
    printf("  checkpoint_path=%s\n", cfg->checkpoint_path ? cfg->checkpoint_path : "(null)");
    printf("  resume_path=%s\n", cfg->resume_path ? cfg->resume_path : "(null)");
    printf("  max_seeds=%lld seed_start=%lld seed_end=%lld\n",
        cfg->max_seeds, cfg->seed_start, cfg->seed_end);
    printf("  batch_size=%lld threads=%d verbose=%d\n",
        cfg->batch_size, cfg->threads, cfg->verbose);
    printf("  format=%s is_validate_only=%d is_plain_progress=%d is_help=%d\n",
        format_name(cfg->format), cfg->is_validate_only, cfg->is_plain_progress, cfg->is_help);
    printf("  reject_sample_rate=%g divergence_threshold=%g\n",
        cfg->reject_sample_rate, cfg->divergence_threshold);
    printf("  has_star_count=%d override_star_count=%d\n",
        cfg->has_star_count, cfg->override_star_count);
    printf("  has_resource_mult=%d override_resource_mult=%g\n",
        cfg->has_resource_mult, (double)cfg->override_resource_mult);
    printf("  has_hive_initial=%d override_hive_initial=%g\n",
        cfg->has_hive_initial, cfg->override_hive_initial);
    printf("  has_hive_max=%d override_hive_max=%g\n",
        cfg->has_hive_max, cfg->override_hive_max);
}

static void run_case(const char *label, int argc, char **argv)
{
    cli_config cfg;
    char err[256];
    int status;

    printf("=== case: %s ===\n", label);
    status = parse_cli(argc, argv, &cfg, err, (int)sizeof(err));
    printf("  parse_cli -> %d\n", status);
    if (status != 0)
    {
        printf("  error: %s\n", err);
        printf("\n");
        return;
    }

    dump_config(&cfg);
    printf("\n");
}

static void run_basic(void)
{
    char *argv[] = {"prog", "-vv", "--max-seeds", "5", "--format", "csv", "cond.json"};
    int argc;

    argc = (int)(sizeof(argv) / sizeof(argv[0]));
    run_case("-vv --max-seeds 5 --format csv cond.json", argc, argv);
}

static void run_help(void)
{
    char *argv[] = {"prog", "--help"};
    int argc;

    argc = (int)(sizeof(argv) / sizeof(argv[0]));
    run_case("--help", argc, argv);
}

static void run_bad_range(void)
{
    char *argv[] = {"prog", "--seed-start", "10", "--seed-end", "5", "x.json"};
    int argc;

    argc = (int)(sizeof(argv) / sizeof(argv[0]));
    run_case("--seed-start 10 --seed-end 5 x.json (expect error)", argc, argv);
}

static void run_eq_forms(void)
{
    char *argv[] = {"prog", "--input=galaxy.json", "--format=json", "--threads=8",
        "--resource-mult=2.5", "--star-count=64", "-vvv"};
    int argc;

    argc = (int)(sizeof(argv) / sizeof(argv[0]));
    run_case("--input=... --format=json --threads=8 --resource-mult=2.5 --star-count=64 -vvv",
        argc, argv);
}

static void run_overrides(void)
{
    char *argv[] = {"prog", "--hive-initial-colonize", "0.4", "--hive-max-density", "0.9",
        "-o", "out.csv", "--checkpoint", "ckpt.bin", "--validate", "c.json"};
    int argc;

    argc = (int)(sizeof(argv) / sizeof(argv[0]));
    run_case("hive overrides + -o + --checkpoint + --validate", argc, argv);
}

static void run_unknown(void)
{
    char *argv[] = {"prog", "--nope", "c.json"};
    int argc;

    argc = (int)(sizeof(argv) / sizeof(argv[0]));
    run_case("--nope (expect unknown option error)", argc, argv);
}

static void run_missing_value(void)
{
    char *argv[] = {"prog", "--max-seeds"};
    int argc;

    argc = (int)(sizeof(argv) / sizeof(argv[0]));
    run_case("--max-seeds with no value (expect error)", argc, argv);
}

static void run_bad_number(void)
{
    char *argv[] = {"prog", "--max-seeds", "12x", "c.json"};
    int argc;

    argc = (int)(sizeof(argv) / sizeof(argv[0]));
    run_case("--max-seeds 12x (expect invalid integer)", argc, argv);
}

static void run_bad_rate(void)
{
    char *argv[] = {"prog", "--reject-sample-rate", "2.0", "c.json"};
    int argc;

    argc = (int)(sizeof(argv) / sizeof(argv[0]));
    run_case("--reject-sample-rate 2.0 (expect out-of-range error)", argc, argv);
}

static void run_plain(void)
{
    char *argv[] = {"prog", "--plain", "--max-seeds", "0", "cond.json"};
    int argc;

    argc = (int)(sizeof(argv) / sizeof(argv[0]));
    run_case("--plain --max-seeds 0 cond.json", argc, argv);
}

int main(void)
{
    run_basic();
    run_help();
    run_bad_range();
    run_eq_forms();
    run_overrides();
    run_plain();
    run_unknown();
    run_missing_value();
    run_bad_number();
    run_bad_rate();

    printf("=== print_help output ===\n");
    print_help("dsp-seed-finder", stdout);
    return 0;
}
