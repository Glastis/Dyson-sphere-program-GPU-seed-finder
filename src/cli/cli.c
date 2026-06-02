#include "cli.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    cli_config *cfg;
    char *err;
    int err_size;
    int argc;
    char **argv;
    int index;
    const char *inline_value;
}
parse_ctx;

typedef int (*opt_handler)(parse_ctx *ctx);

typedef struct
{
    const char *long_name;
    const char *short_name;
    int wants_value;
    opt_handler handler;
}
opt_def;

void cli_defaults(cli_config *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->input_path = NULL;
    cfg->output_path = NULL;
    cfg->checkpoint_path = NULL;
    cfg->resume_path = NULL;
    cfg->max_seeds = 10;
    cfg->seed_start = 0;
    cfg->seed_end = 99999999;
    cfg->batch_size = 0;
    cfg->format = OUTPUT_FORMAT_TEXT;
    cfg->threads = 0;
    cfg->verbose = 0;
    cfg->is_validate_only = 0;
    cfg->is_plain_progress = 0;
    cfg->is_help = 0;
    cfg->reject_sample_rate = 1e-4;
    cfg->divergence_threshold = 1e-3;
}

static int set_err(parse_ctx *ctx, const char *prefix, const char *detail)
{
    if (ctx->err == NULL || ctx->err_size <= 0)
    {
        return 1;
    }

    if (detail != NULL)
    {
        snprintf(ctx->err, (size_t)ctx->err_size, "%s%s", prefix, detail);
    }
    else
    {
        snprintf(ctx->err, (size_t)ctx->err_size, "%s", prefix);
    }

    return 1;
}

static int take_value(parse_ctx *ctx, const char *opt_name, const char **out)
{
    if (ctx->inline_value != NULL)
    {
        *out = ctx->inline_value;
        return 0;
    }

    if (ctx->index + 1 >= ctx->argc)
    {
        return set_err(ctx, "missing value for option ", opt_name);
    }

    ctx->index += 1;
    *out = ctx->argv[ctx->index];
    return 0;
}

static int parse_ll(parse_ctx *ctx, const char *opt_name, const char *text, long long *out)
{
    char *tail;
    long long value;

    errno = 0;
    tail = NULL;
    value = strtoll(text, &tail, 10);
    if (tail == text || *tail != '\0' || errno != 0)
    {
        return set_err(ctx, "invalid integer for option ", opt_name);
    }

    *out = value;
    return 0;
}

static int parse_double(parse_ctx *ctx, const char *opt_name, const char *text, double *out)
{
    char *tail;
    double value;

    errno = 0;
    tail = NULL;
    value = strtod(text, &tail);
    if (tail == text || *tail != '\0' || errno != 0)
    {
        return set_err(ctx, "invalid number for option ", opt_name);
    }

    *out = value;
    return 0;
}

static int take_ll(parse_ctx *ctx, const char *opt_name, long long *out)
{
    const char *text;
    int status;

    text = NULL;
    status = take_value(ctx, opt_name, &text);
    if (status != 0)
    {
        return status;
    }

    return parse_ll(ctx, opt_name, text, out);
}

static int take_double(parse_ctx *ctx, const char *opt_name, double *out)
{
    const char *text;
    int status;

    text = NULL;
    status = take_value(ctx, opt_name, &text);
    if (status != 0)
    {
        return status;
    }

    return parse_double(ctx, opt_name, text, out);
}

static int handle_input(parse_ctx *ctx)
{
    const char *text;
    int status;

    text = NULL;
    status = take_value(ctx, "--input", &text);
    if (status != 0)
    {
        return status;
    }

    ctx->cfg->input_path = text;
    return 0;
}

static int handle_output(parse_ctx *ctx)
{
    const char *text;
    int status;

    text = NULL;
    status = take_value(ctx, "--output", &text);
    if (status != 0)
    {
        return status;
    }

    ctx->cfg->output_path = text;
    return 0;
}

static int handle_checkpoint(parse_ctx *ctx)
{
    const char *text;
    int status;

    text = NULL;
    status = take_value(ctx, "--checkpoint", &text);
    if (status != 0)
    {
        return status;
    }

    ctx->cfg->checkpoint_path = text;
    return 0;
}

static int handle_resume(parse_ctx *ctx)
{
    const char *text;
    int status;

    text = NULL;
    status = take_value(ctx, "--resume", &text);
    if (status != 0)
    {
        return status;
    }

    ctx->cfg->resume_path = text;
    return 0;
}

static int handle_max_seeds(parse_ctx *ctx)
{
    return take_ll(ctx, "--max-seeds", &ctx->cfg->max_seeds);
}

static int handle_seed_start(parse_ctx *ctx)
{
    return take_ll(ctx, "--seed-start", &ctx->cfg->seed_start);
}

static int handle_seed_end(parse_ctx *ctx)
{
    return take_ll(ctx, "--seed-end", &ctx->cfg->seed_end);
}

static int handle_batch_size(parse_ctx *ctx)
{
    return take_ll(ctx, "--batch-size", &ctx->cfg->batch_size);
}

static int handle_threads(parse_ctx *ctx)
{
    long long value;
    int status;

    value = 0;
    status = take_ll(ctx, "--threads", &value);
    if (status != 0)
    {
        return status;
    }

    ctx->cfg->threads = (int)value;
    return 0;
}

static int handle_format(parse_ctx *ctx)
{
    const char *text;
    int status;

    text = NULL;
    status = take_value(ctx, "--format", &text);
    if (status != 0)
    {
        return status;
    }

    if (strcmp(text, "text") == 0)
    {
        ctx->cfg->format = OUTPUT_FORMAT_TEXT;
        return 0;
    }

    if (strcmp(text, "json") == 0)
    {
        ctx->cfg->format = OUTPUT_FORMAT_JSON;
        return 0;
    }

    if (strcmp(text, "csv") == 0)
    {
        ctx->cfg->format = OUTPUT_FORMAT_CSV;
        return 0;
    }

    return set_err(ctx, "unknown format (expected text|json|csv): ", text);
}

static int handle_reject_rate(parse_ctx *ctx)
{
    return take_double(ctx, "--reject-sample-rate", &ctx->cfg->reject_sample_rate);
}

static int handle_divergence(parse_ctx *ctx)
{
    return take_double(ctx, "--divergence-threshold", &ctx->cfg->divergence_threshold);
}

static int handle_star_count(parse_ctx *ctx)
{
    long long value;
    int status;

    value = 0;
    status = take_ll(ctx, "--star-count", &value);
    if (status != 0)
    {
        return status;
    }

    ctx->cfg->has_star_count = 1;
    ctx->cfg->override_star_count = (int)value;
    return 0;
}

static int handle_resource_mult(parse_ctx *ctx)
{
    double value;
    int status;

    value = 0.0;
    status = take_double(ctx, "--resource-mult", &value);
    if (status != 0)
    {
        return status;
    }

    ctx->cfg->has_resource_mult = 1;
    ctx->cfg->override_resource_mult = (float)value;
    return 0;
}

static int handle_hive_initial(parse_ctx *ctx)
{
    double value;
    int status;

    value = 0.0;
    status = take_double(ctx, "--hive-initial-colonize", &value);
    if (status != 0)
    {
        return status;
    }

    ctx->cfg->has_hive_initial = 1;
    ctx->cfg->override_hive_initial = value;
    return 0;
}

static int handle_hive_max(parse_ctx *ctx)
{
    double value;
    int status;

    value = 0.0;
    status = take_double(ctx, "--hive-max-density", &value);
    if (status != 0)
    {
        return status;
    }

    ctx->cfg->has_hive_max = 1;
    ctx->cfg->override_hive_max = value;
    return 0;
}

static int handle_validate(parse_ctx *ctx)
{
    ctx->cfg->is_validate_only = 1;
    return 0;
}

static int handle_plain(parse_ctx *ctx)
{
    ctx->cfg->is_plain_progress = 1;
    return 0;
}

static int handle_help(parse_ctx *ctx)
{
    ctx->cfg->is_help = 1;
    return 0;
}

static int handle_verbose(parse_ctx *ctx)
{
    ctx->cfg->verbose += 1;
    return 0;
}

static const opt_def OPT_TABLE[] =
{
    {"--input", "-i", 1, handle_input},
    {"--output", "-o", 1, handle_output},
    {"--checkpoint", NULL, 1, handle_checkpoint},
    {"--resume", NULL, 1, handle_resume},
    {"--max-seeds", NULL, 1, handle_max_seeds},
    {"--seed-start", NULL, 1, handle_seed_start},
    {"--seed-end", NULL, 1, handle_seed_end},
    {"--batch-size", NULL, 1, handle_batch_size},
    {"--threads", NULL, 1, handle_threads},
    {"--format", NULL, 1, handle_format},
    {"--reject-sample-rate", NULL, 1, handle_reject_rate},
    {"--divergence-threshold", NULL, 1, handle_divergence},
    {"--star-count", NULL, 1, handle_star_count},
    {"--resource-mult", NULL, 1, handle_resource_mult},
    {"--hive-initial-colonize", NULL, 1, handle_hive_initial},
    {"--hive-max-density", NULL, 1, handle_hive_max},
    {"--validate", NULL, 0, handle_validate},
    {"--plain", NULL, 0, handle_plain},
    {"--verbose", "-v", 0, handle_verbose},
    {"--help", "-h", 0, handle_help}
};

static const opt_def *opt_table(int *count)
{
    *count = (int)(sizeof(OPT_TABLE) / sizeof(OPT_TABLE[0]));
    return OPT_TABLE;
}

static const opt_def *find_long(const char *name, int name_len)
{
    const opt_def *table;
    int count;
    int i;

    count = 0;
    table = opt_table(&count);
    i = 0;
    while (i < count)
    {
        if (strncmp(table[i].long_name, name, (size_t)name_len) == 0
            && table[i].long_name[name_len] == '\0')
        {
            return &table[i];
        }

        i += 1;
    }

    return NULL;
}

static const opt_def *find_short(const char *name)
{
    const opt_def *table;
    int count;
    int i;

    count = 0;
    table = opt_table(&count);
    i = 0;
    while (i < count)
    {
        if (table[i].short_name != NULL && strcmp(table[i].short_name, name) == 0)
        {
            return &table[i];
        }

        i += 1;
    }

    return NULL;
}

static int dispatch_long(parse_ctx *ctx, char *arg)
{
    const opt_def *def;
    const char *eq;
    int name_len;

    eq = strchr(arg, '=');
    name_len = (eq != NULL) ? (int)(eq - arg) : (int)strlen(arg);
    def = find_long(arg, name_len);
    if (def == NULL)
    {
        return set_err(ctx, "unknown option: ", arg);
    }

    ctx->inline_value = (eq != NULL) ? eq + 1 : NULL;
    if (ctx->inline_value != NULL && def->wants_value == 0)
    {
        return set_err(ctx, "option takes no value: ", arg);
    }

    return def->handler(ctx);
}

static int dispatch_verbose_cluster(parse_ctx *ctx, const char *arg)
{
    int i;

    i = 1;
    while (arg[i] != '\0')
    {
        if (arg[i] != 'v')
        {
            return -1;
        }

        ctx->cfg->verbose += 1;
        i += 1;
    }

    return 0;
}

static int dispatch_short(parse_ctx *ctx, char *arg)
{
    const opt_def *def;
    int cluster_status;

    def = find_short(arg);
    if (def != NULL)
    {
        ctx->inline_value = NULL;
        return def->handler(ctx);
    }

    cluster_status = dispatch_verbose_cluster(ctx, arg);
    if (cluster_status == 0)
    {
        return 0;
    }

    return set_err(ctx, "unknown option: ", arg);
}

static int dispatch_arg(parse_ctx *ctx, char *arg)
{
    if (arg[0] != '-' || arg[1] == '\0')
    {
        ctx->cfg->input_path = arg;
        return 0;
    }

    if (arg[1] == '-')
    {
        return dispatch_long(ctx, arg);
    }

    return dispatch_short(ctx, arg);
}

static int validate_ranges(parse_ctx *ctx)
{
    cli_config *cfg;

    cfg = ctx->cfg;
    if (cfg->seed_start < 0)
    {
        return set_err(ctx, "seed-start must be >= 0", NULL);
    }

    if (cfg->seed_end < cfg->seed_start)
    {
        return set_err(ctx, "seed-end must be >= seed-start", NULL);
    }

    if (cfg->max_seeds < 0)
    {
        return set_err(ctx, "max-seeds must be >= 0", NULL);
    }

    if (cfg->threads < 0)
    {
        return set_err(ctx, "threads must be >= 0", NULL);
    }

    if (cfg->batch_size < 0)
    {
        return set_err(ctx, "batch-size must be >= 0", NULL);
    }

    return 0;
}

static int validate_rates(parse_ctx *ctx)
{
    cli_config *cfg;

    cfg = ctx->cfg;
    if (cfg->reject_sample_rate < 0.0 || cfg->reject_sample_rate > 1.0)
    {
        return set_err(ctx, "reject-sample-rate must be in [0, 1]", NULL);
    }

    if (cfg->divergence_threshold < 0.0)
    {
        return set_err(ctx, "divergence-threshold must be >= 0", NULL);
    }

    return 0;
}

static void init_ctx(parse_ctx *ctx, int argc, char **argv, cli_config *cfg,
    char *err, int err_size)
{
    ctx->cfg = cfg;
    ctx->err = err;
    ctx->err_size = err_size;
    ctx->argc = argc;
    ctx->argv = argv;
    ctx->index = 1;
    ctx->inline_value = NULL;
}

static int run_validation(parse_ctx *ctx)
{
    int status;

    if (ctx->cfg->is_help)
    {
        return 0;
    }

    status = validate_ranges(ctx);
    if (status != 0)
    {
        return status;
    }

    return validate_rates(ctx);
}

int parse_cli(int argc, char **argv, cli_config *cfg, char *err, int err_size)
{
    parse_ctx ctx;
    int status;

    cli_defaults(cfg);
    if (err != NULL && err_size > 0)
    {
        err[0] = '\0';
    }

    init_ctx(&ctx, argc, argv, cfg, err, err_size);
    while (ctx.index < argc)
    {
        status = dispatch_arg(&ctx, argv[ctx.index]);
        if (status != 0)
        {
            return status;
        }

        ctx.index += 1;
    }

    return run_validation(&ctx);
}

static void print_usage_block(const char *prog_name, FILE *out)
{
    fprintf(out, "Usage: %s [options] <conditions.json>\n\n", prog_name);
    fprintf(out, "Brute-force Dyson Sphere Program galaxy-seed finder. Reads a\n");
    fprintf(out, "human-readable JSON conditions file, generates galaxies in GPU\n");
    fprintf(out, "batches, and prints the seeds that match the requested conditions.\n\n");
    fprintf(out, "Game parameters may be set inside the JSON conditions file and\n");
    fprintf(out, "overridden on the command line with --star-count, --resource-mult,\n");
    fprintf(out, "--hive-initial-colonize and --hive-max-density.\n\n");
    fprintf(out, "Arguments:\n");
    fprintf(out, "  <conditions.json>            Path to the JSON conditions file.\n\n");
}

static void print_io_options(FILE *out)
{
    fprintf(out, "Input/output options:\n");
    fprintf(out, "  -i, --input <file>           Conditions JSON file (same as positional).\n");
    fprintf(out, "  -o, --output <file>          Write results to file instead of stdout.\n");
    fprintf(out, "  --checkpoint <file>          Periodically save progress to this file.\n");
    fprintf(out, "  --resume <file>              Resume a previous run from this file.\n");
    fprintf(out, "  --format <text|json|csv>     Output format (default: text).\n\n");
}

static void print_scan_options(FILE *out)
{
    fprintf(out, "Scan options:\n");
    fprintf(out, "  --max-seeds <N>              Stop after N matches; 0 scans the\n");
    fprintf(out, "                               whole range (default: 10).\n");
    fprintf(out, "  --seed-start <N>             First seed to scan (default: 0).\n");
    fprintf(out, "  --seed-end <N>               Last seed to scan (default: 99999999).\n");
    fprintf(out, "  --batch-size <N>             GPU batch size; 0 = auto (default: 0).\n");
    fprintf(out, "  --threads <N>                Worker threads; 0 = hardware\n");
    fprintf(out, "                               concurrency (default: 0).\n\n");
}

static void print_tuning_options(FILE *out)
{
    fprintf(out, "Tuning options:\n");
    fprintf(out, "  --reject-sample-rate <r>     Sampling rate in [0, 1] (default: 1e-4).\n");
    fprintf(out, "  --divergence-threshold <r>   Divergence threshold >= 0 (default: 1e-3).\n\n");
    fprintf(out, "Game parameter overrides:\n");
    fprintf(out, "  --star-count <N>             Override the galaxy star count.\n");
    fprintf(out, "  --resource-mult <f>          Override the resource multiplier.\n");
    fprintf(out, "  --hive-initial-colonize <f>  Override the hive initial colonize value.\n");
    fprintf(out, "  --hive-max-density <f>       Override the hive maximum density.\n\n");
}

static void print_misc_options(FILE *out)
{
    fprintf(out, "Other options:\n");
    fprintf(out, "  --validate                   Validate the conditions file and exit.\n");
    fprintf(out, "  --plain                      Plain ASCII progress (no live panel/colours).\n");
    fprintf(out, "  -v, --verbose                Increase verbosity (stackable, e.g. -vv).\n");
    fprintf(out, "  -h, --help                   Show this help and exit.\n");
}

void print_help(const char *prog_name, FILE *out)
{
    print_usage_block(prog_name, out);
    print_io_options(out);
    print_scan_options(out);
    print_tuning_options(out);
    print_misc_options(out);
}
