# Architecture

The internal design of `term-color`: compile-time ANSI color escapes with
graceful downgrade across terminals. For usage see `README.md`; for the repo map
and invariants see `AGENTS.md`.

## Shape

Three headers, header-only, with one dependency
([`static-string`](https://github.com/Kronuz/static-string)):

```
  ansi_color.hh    ansi_color<r,g,b,bold> + the rgb()/rgba()/brgb()/brgba() macros
  colors.h         151 named static constexpr colors + NO_COLOR / CLEAR_COLOR
  color_tools.hh   hsv2rgb() + the runtime `color` class
```

`colors.h` includes `ansi_color.hh`; `color_tools.hh` includes `ansi_color.hh`
(for `ESC`). `ansi_color.hh` includes `static_string.hh`. That is the whole
dependency graph.

## Three-tier escape with graceful downgrade

`ansi_color<red, green, blue, bold>` clamps each channel to `[0, 255]` and builds
its escape, `ansi()`, by concatenating three SGR sequences:

- **truecolor** — `ESC[<bold>;38;2;r;g;bm`, the 24-bit form.
- **standard256** — `ESC[<bold>;38;5;<idx>m`, where `<idx>` maps the RGB onto the
  6×6×6 color cube (or the grayscale ramp when `r == g == b`).
- **standard16** — `ESC[<bold>;38;5;<idx>m`, where `<idx>` is one of the 16 base
  colors, picked by thresholding the normalized channels and adding 8 for the
  bright variants.

They are emitted richest-first. A terminal that understands truecolor applies it
and the cruder escapes that follow just re-set the foreground to a near-enough
approximation; a terminal that ignores the truecolor escape falls through to the
256- or 16-color one. So a single constant works across terminal capabilities
with no runtime probing of `$TERM`.

The downgrade math (the cube index, the grayscale ramp, the 16-color
thresholding) is the load-bearing part of the header. It is plain integer and
float arithmetic in a `constexpr` context, so every named color is folded to a
fixed byte sequence at compile time.

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
