# Conditions JSON schema

The conditions file is human-readable: text keys, enum **names** (not the web UI's
numeric values).

## What "match" means

A seed (= one galaxy) **matches** when **at least one star system in that galaxy
satisfies all the rules**. The search keeps the first `--max-seeds` matches it
finds, then stops.

The rule tree reduces to a per-star boolean: a combinator `and`/`or` is `&&`/`||`
of its children evaluated on the same star, and the seed matches if **any** star
makes the top-level rule true. Rules are evaluated cheapest-first (a cascaded
prefilter) and short-circuited.

## Full example

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
      { "vein": "Iron", "op": "gt", "value": 120000 },
      { "type": "planetCount", "op": "gte", "value": 4, "excludeGiant": true },
      { "type": "or", "rules": [
        { "ocean": "Water" },
        { "type": "tidalLockCount", "op": "gte", "value": 1 }
      ]}
    ]
  }
}
```

## Conditions & shortcuts

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

```json
{ "type": "proximity", "maxDistance": 8.0, "systems": [
  { "type": "and", "rules": [ /* the nine resources */ ] },
  { "vein": "Magnet", "op": "present" }
] }
```

## Supported rules

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

## Worked examples

[`tests/fixtures/all-rare-resources.json`](../tests/fixtures/all-rare-resources.json)
asks for **one** star system that gathers nine rare resources at once, each
through the mechanic that produces it: a sulfuric-acid **ocean**, Fire Ice and
Deuterium off its **gas giants**, and the mineable **veins** Crude Oil, Spiniform
Stalagmite Crystal, Kimberlite Ore, Fractal Silicon, Grating Crystal and Organic
Crystal. It matches roughly 1 seed in 1200.

It covers 9 of the 10 rare resources on purpose: **Unipolar Magnet** is left out
because it never lands on the *same* system as the other nine — it only spawns on
black-hole / neutron-star systems, and no such system ever also carries the full
set (a scan over 300 000 seeds finds zero). Adding `Magnet` to this same `and`
would make the query match nothing.

[`tests/fixtures/promised-land.json`](../tests/fixtures/promised-land.json) keeps
that 9-resource system and asks for a Unipolar Magnet system **nearby** instead,
via the `proximity` rule shown above. It reads literally as "an all-rares system
**and** a `Magnet` system within 8 light-years of it". The first match is seed
`5457`: system 4 gathers the nine resources and the black hole 7.9 ly away
supplies the magnet. It matches roughly 1 seed in 5500.
