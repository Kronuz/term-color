# Architecture

The internal design of `term-color`: compile-time ANSI color escapes with
graceful downgrade across terminals. For usage see `README.md`; for the repo map
and invariants see `AGENTS.md`.

## Shape

Four headers, header-only, with one dependency
([`static-string`](https://github.com/Kronuz/static-string)):

```
  ansi_color.hh    ansi_color<r,g,b,bold> + the rgb()/rgba()/brgb()/brgba() macros
  colors.h         151 named static constexpr colors + NO_COLOR / CLEAR_COLOR
  color_tools.hh   hsv2rgb() + the runtime `color` class
  collapse.hh      collapse()/apply()/detect_depth(): resolve the stacked escapes to one tier
```

`colors.h` includes `ansi_color.hh`; `color_tools.hh` includes `ansi_color.hh`
(for `ESC`). `ansi_color.hh` includes `static_string.hh`. `collapse.hh` stands
apart: it operates on already-built escape strings, so it pulls in only the
standard library (no `static_string`, no other header here). That is the whole
dependency graph.

## Three-tier escape with graceful downgrade

`ansi_color<red, green, blue, bold>` clamps each channel to `[0, 255]` and builds
its escape, `ansi()`, by concatenating three SGR sequences, worst tier first:

- **standard16** — `ESC[<bold>;<sgr>m`, where `<sgr>` is one of the 16 base
  colors (30-37 / 90-97), picked by thresholding the normalized channels and
  adding 8 for the bright variants.
- **standard256** — `ESC[<bold>;38;5;<idx>m`, where `<idx>` maps the RGB onto the
  6×6×6 color cube (or the grayscale ramp when `r == g == b`).
- **truecolor** — `ESC[<bold>;38;2;r;g;bm`, the 24-bit form.

They are emitted worst-first, and that order is load-bearing. A terminal applies
each SGR it understands in sequence and the last one wins, so a truecolor terminal
runs through the 16- and 256-color approximations and ends on the exact truecolor;
a 256-color terminal ends on the 256 approximation and (ideally) ignores the
truecolor escape that follows. Reverse the order and a capable terminal would end
on the *crudest* tier. So the stack self-resolves on most terminals with no
runtime probing of `$TERM`.

"Most terminals" is the caveat: a terminal that mangles rather than ignores an
escape it doesn't grok would render the stack wrong. The robust path is to resolve
the stack explicitly at runtime with `collapse()` / `apply()` (see below), which
also handles `NO_COLOR`, a `--color` mode, and non-terminal sinks. The compile-time
stack is the portable default; the resolution layer is the guarantee.

The downgrade math (the cube index, the grayscale ramp, the 16-color
thresholding) is the load-bearing part of the header. It is plain integer and
float arithmetic in a `constexpr` context, so every named color is folded to a
fixed byte sequence at compile time.

## Resolving to a terminal (collapse.hh)

The stack is portable but best-effort; `collapse.hh` is where it resolves to
exactly what a sink should receive. It is deliberately independent of the rest of
the library: it reads escape *strings*, not `ansi_color` types, so a consumer can
resolve any stacked output without dragging in `static_string`.

- **`collapse(s, depth)`** is the primitive. It scans for runs of consecutive SGR
  escapes; a run whose length is a multiple of three is read as stacked triples and
  reduced to the `pick`-th escape of each triple (`0` = 16-color, `1` = 256, `2` =
  truecolor), or dropped entirely for `depth::none`. This is deliberately shape-based
  and has a consequence: a run whose length is *not* a multiple of three is not a
  term-color stack, so it is passed through untouched. A lone `ESC[1m` next to a
  color therefore makes a run of four and defeats the collapse. That is why bold
  belongs *inside* the color (`brgb` / `color::ansi(true)`), where it rides each of
  the three tiers and the run stays a clean triple.
- **`detect_depth(respect_no_color = true)`** reads the best tier from the
  environment: `NO_COLOR` (when respected) → `none`, then `COLORTERM`
  (`truecolor`/`24bit`) → truecolor, then `TERM` (`*256color*`) → 256, else 16.
- **`apply(s, mode, target, is_terminal)`** composes the two into the full policy a
  logger sink wants, so `NO_COLOR` / tty / mode handling lives here once instead of
  in every consumer. It gates on `mode` (`automatic` is tty-gated and honors
  `NO_COLOR`; `always` forces; `never` strips), then, if coloring, emits the
  `stacked` form as-is or collapses to `target` (or the detected tier for
  `automatic`).

`mode` mirrors `--color=<auto|always|never>` and `target` the requested tier; a
consumer typically maps its own `--color` flag onto them (this is exactly how the
[`logger`](https://github.com/Kronuz/logger) sink colors a line: build with the
stacked palette, then `apply()` at write time).

## Why static_string

The escapes are built with `static_string::string(ESC "[") +
static_string::explode<n>::value + ...`. `explode<n>::value` renders an integer
to its decimal digits at compile time, and `+` concatenates
`static_string`s into a longer one whose length is fixed in the type. The result
is that `RED`, and even `RED + "error" + CLEAR_COLOR`, is a single compile-time
constant with no runtime string work. That is the whole reason the color lives in
the type system instead of being formatted at runtime: a colored label costs
nothing at run time. `static_string` is therefore a real dependency, not an
implementation convenience — the compile-time concatenation is the point.

## The palette

`colors.h` is 151 named colors (`ALICE_BLUE` … `YELLOW_GREEN`, the CSS/X11 set)
plus `NO_COLOR` (the empty escape) and `CLEAR_COLOR` (the reset SGR, emitted three
times to match the three-tier structure). Each is `static constexpr auto NAME =
rgb(...)`, i.e. an `ansi_color<...>::ansi()`. They are `static constexpr`, so each
translation unit that includes the header gets its own internal-linkage copy;
unused colors fold away.

## The runtime side

`color_tools.hh` is for the case where the channels aren't known at compile time.
`hsv2rgb` converts an HSV triple (hue in degrees, saturation and value as
fractions) to RGB fractions — the standard sextant algorithm. The `color` class
is a non-`constexpr` mirror of `ansi_color`: it stores the three channels and its
`ansi(bool bold)` formats the same three-tier escape with `std::format` instead
of `static_string`. Same bytes, runtime path. (In Xapiand this used the in-house
`strings::format`, a thin `std::vformat` wrapper; standalone it is `std::format`
directly.)

## Why this shape

The library exists to push color into compile time. A named color is not a
runtime lookup or a format call — it is a constant the compiler bakes in, and the
graceful-downgrade structure means that one constant is correct on every terminal
without a capability check. Keeping it header-only and `constexpr` is what makes
that free. The runtime `color` class is the deliberate exception, for the small
fraction of cases (e.g. a color derived from live data via `hsv2rgb`) where the
value can't exist until run time.
