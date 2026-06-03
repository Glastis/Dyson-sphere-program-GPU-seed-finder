#ifndef DSP_PROGRESS_H
#define DSP_PROGRESS_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../cli/config.h"

#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_CYAN    "\033[36m"
#define C_GREEN   "\033[92m"
#define C_YELLOW  "\033[93m"
#define C_MAGENTA "\033[95m"
#define C_GREY    "\033[90m"

#define G_DIAMOND "\xe2\x97\x86"
#define G_TL      "\xe2\x95\xad"
#define G_TR      "\xe2\x95\xae"
#define G_BL      "\xe2\x95\xb0"
#define G_BR      "\xe2\x95\xaf"
#define G_TEE_L   "\xe2\x94\x9c"
#define G_TEE_R   "\xe2\x94\xa4"
#define G_V       "\xe2\x94\x82"
#define G_DASH    "\xe2\x94\x80"
#define G_DOT     "\xc2\xb7"
#define BLOCK_FULL  "\xe2\x96\x88"
#define BLOCK_EMPTY "\xe2\x96\x91"

#define PROGRESS_WINDOW 10
#define BOX_INNER       54
#define SEED_COL        14
#define BAR_WIDTH       38
#define FANCY_INTERVAL  0.06
#define PLAIN_INTERVAL  0.50

typedef struct
{
    int seed;
    char stars[24];
}
match_view;

typedef struct
{
    double last_render_elapsed;
    int rendered_lines;
    int is_plain;
    int is_tty;
    int is_started;
    match_view ring[PROGRESS_WINDOW];
    int ring_count;
}
progress_bar;

typedef struct
{
    long long processed;
    long long total;
    long long found;
    long long cursor;
    long long last_match;
    double elapsed;
}
progress_info;

typedef struct
{
    char bar[256];
    char proc[24];
    char total[24];
    char cursor[24];
    char rate[24];
    char eta[24];
    char elapsed[24];
    double pct;
}
fancy_fields;

static inline void str_app(char *buf, size_t n, int *out, const char *s)
{
    int i;

    i = 0;
    while (s[i] != '\0' && (size_t)(*out) < n - 1)
    {
        buf[*out] = s[i];
        *out += 1;
        ++i;
    }
    buf[*out] = '\0';
}

static inline void group_uint(char *buf, size_t n, long long v)
{
    char tmp[32];
    int len;
    int group;
    int out;
    int i;

    len = snprintf(tmp, sizeof(tmp), "%lld", v < 0 ? 0 : v);
    group = len % 3 == 0 ? 3 : len % 3;
    out = 0;
    i = 0;
    while (i < len && (size_t)out < n - 2)
    {
        if (i > 0 && group == 0)
        {
            buf[out++] = ',';
            group = 3;
        }
        buf[out++] = tmp[i];
        --group;
        ++i;
    }
    buf[out] = '\0';
}

static inline void fmt_duration(char *buf, size_t n, double secs)
{
    long total;
    long h;
    long m;
    long s;

    total = secs > 0.0 ? (long)(secs + 0.5) : 0;
    h = total / 3600;
    m = (total % 3600) / 60;
    s = total % 60;
    if (h > 0)
    {
        snprintf(buf, n, "%ld:%02ld:%02ld", h, m, s);
    }
    else
    {
        snprintf(buf, n, "%02ld:%02ld", m, s);
    }
}

static inline void fmt_rate(char *buf, size_t n, double rate)
{
    if (rate >= 1.0e6)
    {
        snprintf(buf, n, "%.2f M/s", rate / 1.0e6);
        return;
    }
    if (rate >= 1.0e3)
    {
        snprintf(buf, n, "%.1f k/s", rate / 1.0e3);
        return;
    }
    snprintf(buf, n, "%.0f /s", rate);
}

static const char *const BAR_PARTIALS[8] =
{
    "", "\xe2\x96\x8f", "\xe2\x96\x8e", "\xe2\x96\x8d",
    "\xe2\x96\x8c", "\xe2\x96\x8b", "\xe2\x96\x8a", "\xe2\x96\x89"
};

static inline const char *bar_partial(int eighths)
{
    return BAR_PARTIALS[eighths & 7];
}

static inline void make_rule(char *buf, size_t n, int cells)
{
    int out;
    int i;

    out = 0;
    buf[0] = '\0';
    i = 0;
    while (i < cells)
    {
        str_app(buf, n, &out, G_DASH);
        ++i;
    }
}

static inline void build_bar(char *buf, size_t n, double frac, int width)
{
    int filled8;
    int full;
    int rem;
    int used;
    int out;
    int i;

    if (frac < 0.0)
    {
        frac = 0.0;
    }
    if (frac > 1.0)
    {
        frac = 1.0;
    }
    filled8 = (int)(frac * (double)width * 8.0 + 0.5);
    full = filled8 / 8;
    rem = filled8 % 8;
    out = 0;
    str_app(buf, n, &out, C_GREEN);
    i = 0;
    while (i < full)
    {
        str_app(buf, n, &out, BLOCK_FULL);
        ++i;
    }
    used = full;
    if (rem > 0)
    {
        str_app(buf, n, &out, bar_partial(rem));
        used += 1;
    }
    str_app(buf, n, &out, C_GREY);
    i = used;
    while (i < width)
    {
        str_app(buf, n, &out, BLOCK_EMPTY);
        ++i;
    }
    str_app(buf, n, &out, C_RESET);
}

typedef struct
{
    char b[768];
    int len;
    int cols;
}
lb_t;

static inline void lb_init(lb_t *lb)
{
    lb->len = 0;
    lb->cols = 0;
    lb->b[0] = '\0';
}

static inline void lb_raw(lb_t *lb, const char *s)
{
    int i;

    i = 0;
    while (s[i] != '\0' && lb->len < (int)sizeof(lb->b) - 1)
    {
        lb->b[lb->len++] = s[i++];
    }
    lb->b[lb->len] = '\0';
}

static inline void lb_seg(lb_t *lb, const char *s, int cols)
{
    if (lb->cols + cols > BOX_INNER)
    {
        return;
    }
    lb_raw(lb, s);
    lb->cols += cols;
}

static inline void lb_text(lb_t *lb, const char *s)
{
    lb_seg(lb, s, (int)strlen(s));
}

static inline void lb_val(lb_t *lb, const char *color, const char *text)
{
    lb_raw(lb, color);
    lb_text(lb, text);
    lb_raw(lb, C_RESET);
}

static inline void lb_space(lb_t *lb)
{
    if (lb->cols >= BOX_INNER)
    {
        return;
    }
    lb_raw(lb, " ");
    lb->cols += 1;
}

static inline void lb_field(lb_t *lb, const char *color, const char *text, int width)
{
    int filled;

    lb_val(lb, color, text);
    filled = (int)strlen(text);
    while (filled < width)
    {
        lb_space(lb);
        ++filled;
    }
}

static inline void lb_label(lb_t *lb, const char *name, int width)
{
    lb_field(lb, C_GREY, name, width);
}

static inline void lb_dot(lb_t *lb)
{
    lb_seg(lb, "  ", 2);
    lb_seg(lb, C_GREY G_DOT C_RESET, 1);
    lb_seg(lb, "  ", 2);
}

static inline void close_body(char *dst, size_t n, lb_t *lb)
{
    while (lb->cols < BOX_INNER)
    {
        lb_space(lb);
    }
    snprintf(dst, n, C_GREY G_V C_RESET "%s" C_GREY G_V C_RESET, lb->b);
}

static inline int build_top(char *dst, size_t n)
{
    char dashes[256];

    make_rule(dashes, sizeof(dashes), BOX_INNER - 20);
    snprintf(dst, n,
        C_GREY G_TL G_DASH " " C_BOLD C_CYAN G_DIAMOND " DSP SEED FINDER " C_RESET
        C_GREY "%s" G_TR C_RESET, dashes);
    return 1;
}

static inline void build_bar_line(char *dst, size_t n, const fancy_fields *f)
{
    char pct[16];
    lb_t lb;

    snprintf(pct, sizeof(pct), "%5.1f%%", f->pct);
    lb_init(&lb);
    lb_seg(&lb, "  ", 2);
    lb_seg(&lb, f->bar, BAR_WIDTH);
    lb_seg(&lb, "   ", 3);
    lb_val(&lb, C_BOLD, pct);
    close_body(dst, n, &lb);
}

static inline void build_seeds_line(char *dst, size_t n, const fancy_fields *f)
{
    lb_t lb;

    lb_init(&lb);
    lb_seg(&lb, "  ", 2);
    lb_label(&lb, "seeds", 8);
    lb_val(&lb, C_BOLD, f->proc);
    lb_seg(&lb, " / ", 3);
    lb_val(&lb, C_BOLD, f->total);
    close_body(dst, n, &lb);
}

static inline void build_rate_line(char *dst, size_t n, const fancy_fields *f)
{
    lb_t lb;

    lb_init(&lb);
    lb_seg(&lb, "  ", 2);
    lb_label(&lb, "rate", 8);
    lb_val(&lb, C_CYAN, f->rate);
    lb_dot(&lb);
    lb_seg(&lb, "ETA ", 4);
    lb_val(&lb, C_YELLOW, f->eta);
    lb_dot(&lb);
    lb_val(&lb, C_GREY, f->elapsed);
    close_body(dst, n, &lb);
}

static inline void build_cursor_line(char *dst, size_t n, const fancy_fields *f)
{
    lb_t lb;

    lb_init(&lb);
    lb_seg(&lb, "  ", 2);
    lb_label(&lb, "cursor", 8);
    lb_val(&lb, C_YELLOW, f->cursor);
    close_body(dst, n, &lb);
}

static inline void build_divider(char *dst, size_t n, long long found)
{
    char num[24];
    char dashes[256];

    group_uint(num, sizeof(num), found);
    make_rule(dashes, sizeof(dashes), BOX_INNER - 13 - (int)strlen(num));
    snprintf(dst, n,
        C_GREY G_TEE_L G_DASH " matches (" C_RESET C_BOLD C_MAGENTA "%s" C_RESET
        C_GREY ") %s" G_TEE_R C_RESET, num, dashes);
}

static inline void build_header(char *dst, size_t n)
{
    lb_t lb;

    lb_init(&lb);
    lb_seg(&lb, "  ", 2);
    lb_label(&lb, "seed", SEED_COL);
    lb_label(&lb, "systems", 7);
    close_body(dst, n, &lb);
}

static inline void build_match_row(char *dst, size_t n, const match_view *m)
{
    char seed[24];
    lb_t lb;

    group_uint(seed, sizeof(seed), m->seed);
    lb_init(&lb);
    lb_seg(&lb, "  ", 2);
    lb_field(&lb, C_YELLOW, seed, SEED_COL);
    lb_val(&lb, C_RESET, m->stars);
    close_body(dst, n, &lb);
}

static inline void build_bottom(char *dst, size_t n)
{
    char dashes[256];

    make_rule(dashes, sizeof(dashes), BOX_INNER);
    snprintf(dst, n, C_GREY G_BL "%s" G_BR C_RESET, dashes);
}

static inline void fancy_fill(fancy_fields *f, const progress_info *info)
{
    double frac;
    double rate;

    frac = info->total > 0 ? (double)info->processed / (double)info->total : 1.0;
    rate = info->elapsed > 0.0 ? (double)info->processed / info->elapsed : 0.0;
    build_bar(f->bar, sizeof(f->bar), frac, BAR_WIDTH);
    group_uint(f->proc, sizeof(f->proc), info->processed);
    group_uint(f->total, sizeof(f->total), info->total);
    group_uint(f->cursor, sizeof(f->cursor), info->cursor);
    fmt_rate(f->rate, sizeof(f->rate), rate);
    fmt_duration(f->eta, sizeof(f->eta), rate > 0.0 ? (double)(info->total - info->processed) / rate : 0.0);
    fmt_duration(f->elapsed, sizeof(f->elapsed), info->elapsed);
    f->pct = frac * 100.0;
}

static inline int build_panel(const progress_bar *pb, const fancy_fields *f,
                              const progress_info *info, char lines[][800])
{
    int n;
    int i;

    n = 0;
    build_top(lines[n++], 800);
    build_bar_line(lines[n++], 800, f);
    build_seeds_line(lines[n++], 800, f);
    build_rate_line(lines[n++], 800, f);
    build_cursor_line(lines[n++], 800, f);
    build_divider(lines[n++], 800, info->found);
    build_header(lines[n++], 800);
    i = 0;
    while (i < pb->ring_count)
    {
        build_match_row(lines[n++], 800, &pb->ring[i]);
        ++i;
    }
    build_bottom(lines[n++], 800);
    return n;
}

static inline void panel_blit(progress_bar *pb, char lines[][800], int n)
{
    int i;

    if (pb->is_started)
    {
        fprintf(stderr, "\033[%dA\r", pb->rendered_lines);
    }
    i = 0;
    while (i < n)
    {
        fprintf(stderr, "%s\033[K\n", lines[i]);
        ++i;
    }
    fflush(stderr);
    pb->rendered_lines = n;
    pb->is_started = 1;
}

static inline void render_fancy(progress_bar *pb, const progress_info *info)
{
    fancy_fields f;
    char lines[24][800];
    int n;

    fancy_fill(&f, info);
    n = build_panel(pb, &f, info, lines);
    panel_blit(pb, lines, n);
}

static inline void render_plain(progress_bar *pb, const progress_info *info)
{
    char g_proc[24];
    char g_total[24];
    char g_cursor[24];
    char s_rate[24];
    char s_eta[24];
    char s_last[24];
    double frac;
    double rate;

    frac = info->total > 0 ? (double)info->processed / (double)info->total : 1.0;
    rate = info->elapsed > 0.0 ? (double)info->processed / info->elapsed : 0.0;
    group_uint(g_proc, sizeof(g_proc), info->processed);
    group_uint(g_total, sizeof(g_total), info->total);
    group_uint(g_cursor, sizeof(g_cursor), info->cursor);
    fmt_rate(s_rate, sizeof(s_rate), rate);
    fmt_duration(s_eta, sizeof(s_eta), rate > 0.0 ? (double)(info->total - info->processed) / rate : 0.0);
    if (info->last_match >= 0)
    {
        group_uint(s_last, sizeof(s_last), info->last_match);
    }
    else
    {
        snprintf(s_last, sizeof(s_last), "%s", "-");
    }
    fprintf(stderr, "[%5.1f%%] %s/%s seeds | %s | hits %lld | last #%s | cursor %s | ETA %s\n",
            frac * 100.0, g_proc, g_total, s_rate, info->found, s_last, g_cursor, s_eta);
    fflush(stderr);
    pb->is_started = 1;
}

static inline void progress_init(progress_bar *pb, const cli_config *cfg)
{
    pb->last_render_elapsed = -1.0;
    pb->rendered_lines = 0;
    pb->is_tty = isatty(fileno(stderr)) ? 1 : 0;
    pb->is_plain = (cfg->is_plain_progress || cfg->verbose >= 1 || !pb->is_tty) ? 1 : 0;
    pb->is_started = 0;
    pb->ring_count = 0;
}

static inline void progress_push_match(progress_bar *pb, int seed, const char *stars)
{
    match_view *slot;

    if (pb->ring_count < PROGRESS_WINDOW)
    {
        slot = &pb->ring[pb->ring_count];
        pb->ring_count += 1;
    }
    else
    {
        memmove(pb->ring, pb->ring + 1, (PROGRESS_WINDOW - 1) * sizeof(pb->ring[0]));
        slot = &pb->ring[PROGRESS_WINDOW - 1];
    }
    slot->seed = seed;
    snprintf(slot->stars, sizeof(slot->stars), "%s", stars);
}

static inline void progress_render(progress_bar *pb, const progress_info *info, int is_final)
{
    double min_interval;

    min_interval = pb->is_plain ? PLAIN_INTERVAL : FANCY_INTERVAL;
    if (!is_final && pb->is_started && info->elapsed - pb->last_render_elapsed < min_interval)
    {
        return;
    }
    pb->last_render_elapsed = info->elapsed;
    if (pb->is_plain)
    {
        render_plain(pb, info);
    }
    else
    {
        render_fancy(pb, info);
    }
}

#endif
