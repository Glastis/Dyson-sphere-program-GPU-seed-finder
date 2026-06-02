# Usage

```
dsp-seed-finder [options] <conditions.json>
```

Results go to **stdout** (or `--output`); progress goes to **stderr**, so the two
never mix in a pipeline. Each result line is `seed` and the matching star indexes,
tab-separated:

```
5457	4
14927	59
26150	59
```

## Options

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

## Examples

```sh
# 10 seeds whose galaxy matches, as text
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

## Progress display

When stderr is a terminal you get a **live panel**: a rounded box with the colour
progress bar, seeds scanned, throughput (auto-scaled to k/s or M/s), ETA, elapsed
time, and the running cursor (latest seed tested). Below a divider, a **rolling
window of the last 10 matching seeds** is shown as a two-column table (`seed` /
`stars`). The panel appears immediately at 0% (so the tool never looks hung while
the first GPU batch is in flight) and redraws in place.

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

To keep the terminal clean, the panel does **not** stream every matching seed.
The window shows the 10 most recent; when the run finds **more than 10** and
stdout is the terminal (no `--output`, not a pipe), the full list is written to
**`dsp-matches.txt`** (`.json`/`.csv` for those formats) in the current directory,
and the final summary points you to it. When stdout is a pipe/file or `--output`
is given, every match is written there as usual — the window is purely a preview.

Pass `--plain` for a simple, non-dynamic, **ASCII** progress line (one appended
line per update, no colours, matches streamed to stdout) — handy for logs and CI.
Plain mode is also selected automatically when stderr is **not** a terminal (a
pipe or file) or when `-v`/`--verbose` is given, so redirected output stays clean.
