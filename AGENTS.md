# AGENTS.md

Working notes for agents modifying this repository. For the design read
`ARCHITECTURE.md`; for usage read `README.md`. This file covers the repo layout,
how to build and test, the invariants you must not break, and the traps that are
easy to fall into.

## Repo map

```
ansi_color.hh                ansi_color<r,g,b,bold> template + rgb()/rgba()/brgb()/brgba()/clear_color()/no_color() macros + ESC. Header.
colors.h                     151 named static constexpr colors + NO_COLOR / CLEAR_COLOR. Includes ansi_color.hh.
color_tools.hh               hsv2rgb() and the runtime `color` class (std::format). Includes ansi_color.hh.
test/test.cc                 Runnable smoke test: named-color escapes (static_assert), bold/concat, hsv2rgb, the color class.
CMakeLists.txt               INTERFACE library `term_color` (+ alias term_color::term_color); FetchContent static-string; CTest test `term_color`.
LICENSE                      MIT, Copyright (c) 2015-2019 Dubalu LLC.
README.md                    What it is, install, usage.
ARCHITECTURE.md              The three-tier escape, the downgrade math, why static_string.
```

Everything is header-only. There is no `.cc` to compile except the test. The
CMake target is a pure `INTERFACE` library that adds the source dir to the
include path, requests `cxx_std_20`, and links `static_string::static_string`.

## Build and run the test

```sh
cmake -B build && cmake --build build && ctest --test-dir build
```

`FetchContent` pulls [`static-string`](https://github.com/Kronuz/static-string)
(pinned by SHA in `CMakeLists.txt`). To build against a local checkout instead of
cloning, pass `-DFETCHCONTENT_SOURCE_DIR_STATIC_STRING=/path/to/static-string`.

Expected output ends with `all term-color tests passed`, exit 0. The test target
is `term_color_test`; the registered CTest name is `term_color`.

## Conventions

- **C++20.** Required (`static_string`'s formatter and `color_tools.hh`'s
  `std::format` assume it). Don't drop below it.
- **One dependency: static-string.** The only non-stdlib include is
  `static_string.hh`, which is what the compile-time escapes are built on. Don't
  add Xapiand headers back, and don't reintroduce `strings.hh` /
  `strings::format` — `color_tools.hh` uses `std::format` on purpose.
- **No logging glue here.** The `L_*` shortcut macros that lived in Xapiand's
  `colors.h` are log glue, not color machinery, and were removed during
  extraction (see "Standalone vs. Xapiand"). Don't add logging dependencies.
- **Filenames are stable.** The headers keep their original Xapiand names
  (`ansi_color.hh`, `colors.h`, `color_tools.hh`) so a consumer that already
  `#include`s them just needs this repo on the include path. Don't rename them.
- Tabs for indentation, double quotes in code, no em dashes in prose.

## Load-bearing invariants

- **The three-tier escape and its order.** Every color emits truecolor, then
  256-color, then 16-color, in that order. That order is what gives the graceful
  downgrade: the richest escape a terminal understands wins, poorer terminals
  fall through. Don't reorder the tiers or drop one — a consumer relies on a
  single constant being correct across terminal capabilities.
- **The downgrade math is exact and tested.** The 256-color cube index, the
  grayscale ramp, and the 16-color thresholding produce specific indices (e.g.
  `RED` -> truecolor `255;0;0`, 256-color `196`, 16-color `9`). `test/test.cc`
  pins these with `static_assert`, so a regression in the arithmetic is a compile
  error, not a silent visual change. If you change the math, you are changing
  output; update the test deliberately.
- **`constexpr` is the point.** Named colors and their concatenation are
  compile-time constants built on `static_string`. The test asserts them with
  `static_assert`. Don't add a runtime path to the compile-time side — the
  runtime case already has its own home in `color_tools.hh`'s `color` class.
- **Channels are clamped.** `ansi_color` clamps r/g/b to `[0, 255]`. Keep the
  clamp; `rgb(...)` is fed arbitrary expressions (including `rgba`'s
  alpha-premultiplied values) that can land out of range.

## How to extend

- **Add a named color.** One line in `colors.h`: `static constexpr auto NAME =
  rgb(r, g, b);`. Nothing else — no macros (the `L_*` families are gone).
- **Always extend the smoke test.** `test/test.cc` is the only executable check.
  Prefer `static_assert` for anything on the compile-time side, so a regression
  is a compile error. For new escape sequences, compute the expected bytes once
  (a tiny program that dumps the `static_string`) and pin them.

## Traps

- **Don't reintroduce the `L_*` macros.** Xapiand's `colors.h` defined three
  logging shortcut macros per color (`L_UNINDENTED_<NAME>`, `L_<NAME>`,
  `L_STACKED_<NAME>`) that call its `log.h`. They are not part of a color library
  and pull in a logging dependency. They live in Xapiand now (a small header that
  includes this `colors.h` plus `log.h` and redefines them).
- **`explode<bold>` takes a bool as an int.** The leading SGR parameter is
  `static_string::explode<bold>::value`, where `bold` is `false`/`true` rendered
  as `0`/`1`. That is intentional; don't "fix" it to a literal.
- **`ESC` is defined in `ansi_color.hh`.** `color_tools.hh` uses it via that
  include. It is a plain `#define "\033"`; if you ever split headers further,
  keep `ESC` reachable.

## Standalone vs. Xapiand

This is a standalone extraction from
[Xapiand](https://github.com/Kronuz/Xapiand). The delta from the original is
decoupling, not behavior:

- **`strings::format` -> `std::format`** in `color_tools.hh`'s runtime `color`
  class. Xapiand's `strings::format` was a thin `std::vformat` wrapper, so this is
  a direct substitution and the `strings.hh` include is dropped.
- **The `L_*` logging shortcut macros were removed** from `colors.h`. Xapiand's
  version had, per color, `#define L_UNINDENTED_<NAME>(args...)
  L_UNINDENTED(LOG_NOTICE, <NAME>, args)`, `#define L_<NAME>(args...) L(LOG_NOTICE,
  <NAME>, args)`, and `#define L_STACKED_<NAME>(args...) L_STACKED(LOG_NOTICE,
  <NAME>, args)`, plus a `#pragma GCC diagnostic` guard for the variadic macros.
  Those reference Xapiand's `log.h` and are log glue, not color machinery, so they
  were stripped. The color palette is otherwise byte-for-byte the original (all
  151 `rgb(...)` values, `NO_COLOR`, `CLEAR_COLOR`).
- **The compile-time strings depend on
  [`static-string`](https://github.com/Kronuz/static-string)**, fetched via
  `FetchContent` and pinned by SHA — the same header Xapiand uses.

Keep extraction hygiene separate from behavior changes so they can be reconciled
with upstream.
