# term-color

[![CI](https://github.com/Kronuz/term-color/actions/workflows/ci.yml/badge.svg)](https://github.com/Kronuz/term-color/actions/workflows/ci.yml)

![constexpr](https://img.shields.io/badge/constexpr-compile--time-blue)

A small, header-only **compile-time ANSI terminal color** library for C++20,
extracted from [Xapiand](https://github.com/Kronuz/Xapiand).

## What it is

Named colors like `RED` or `STEEL_BLUE` that expand, at compile time, into the
ANSI escape sequence that paints text in that color. Each color emits three
escapes back to back, worst first: the 16-color palette, then the 256-color
palette, then 24-bit truecolor. A terminal applies each escape it understands in
turn and ends on the last (best) one it supports, so the richest tier it can
render wins and one constant stays portable across terminals.

That stacked form is portable but best-effort (a 256-only terminal that mangles
rather than ignores an unknown truecolor escape is the awkward case). For a
guaranteed single tier, and to honor a `--color` mode, `NO_COLOR`, and
non-terminal sinks like a log file, resolve the stacked string at runtime with
`collapse()` / `apply()` from `collapse.hh`. See
[Resolving to a terminal](#resolving-to-a-terminal).

The escapes are built as [`static_string`](https://github.com/Kronuz/static-string)s,
so `RED + "error" + CLEAR_COLOR` is a single compile-time constant, not a runtime
concatenation. There is also a runtime side in `color_tools.hh`: an `hsv2rgb`
helper and a non-`constexpr` `color` class for when the channel values aren't
known until runtime.

```
  ansi_color.hh    the ansi_color<r,g,b,bold> template + the rgb()/brgb() macros
  colors.h         151 named colors (ALICE_BLUE … YELLOW_GREEN) + NO_COLOR / CLEAR_COLOR
  color_tools.hh   hsv2rgb() and the runtime `color` class
  collapse.hh      collapse()/apply() to resolve the stacked escapes to one tier
```

## Install

CMake with `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
  term_color
  GIT_REPOSITORY https://github.com/Kronuz/term-color.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(term_color)

target_link_libraries(your_target PRIVATE term_color::term_color)
```

The `term_color` target is a pure `INTERFACE` library: it compiles nothing,
requests `cxx_std_20`, puts the header directory on your include path, and pulls
in its one dependency, [`static-string`](https://github.com/Kronuz/static-string),
transitively (also via `FetchContent`). Then:

```cpp
#include "colors.h"        // named colors + the rgb()/brgb() machinery
#include "color_tools.hh"  // hsv2rgb + the runtime color class (optional)
```

Requires C++20 (the bundled `static_string` formatter and `color_tools.hh`'s
`std::format` both assume it). On macOS it builds with AppleClang/libc++, the
same toolchain Xapiand uses. The headers keep their original filenames
(`ansi_color.hh`, `colors.h`, `color_tools.hh`), so a codebase that already
`#include`s them just needs this repo on its include path.

## Usage

```cpp
#include <cstdio>
#include "colors.h"

int main() {
    // A named color is a compile-time escape sequence; wrap text and reset.
    std::printf("%s%s%s done\n", RED, "error", CLEAR_COLOR);

    // Compose at compile time: the whole thing is one static_string constant.
    constexpr auto banner = STEEL_BLUE + std::string_view("hello") + CLEAR_COLOR;
}
```

`rgb(r, g, b)` builds an arbitrary color; `brgb(r, g, b)` builds the bold
variant; `rgba` / `brgba` premultiply an alpha. `NO_COLOR` is the empty escape
and `CLEAR_COLOR` resets the SGR state. Every color converts implicitly to
`const char*` / `std::string_view`, so it drops into `printf`, `std::format`, or
stream output directly.

For runtime channels, use `color_tools.hh`:

```cpp
#include "color_tools.hh"

double r, g, b;
hsv2rgb(210.0, 0.6, 0.8, r, g, b);          // HSV degrees/fractions -> RGB fractions
color c(r * 255, g * 255, b * 255);
std::string ansi = c.ansi();                 // same three-tier escape, built at runtime
```

## Resolving to a terminal

A color is emitted as three stacked escapes (`ansi_color::ansi()` /
`color::ansi()` / `rgb()` build 16-color, then 256-color, then truecolor). Two
functions in `collapse.hh` reduce that stack to what a given sink should actually
receive. This is the piece you reach for in a logger, a CLI, or anywhere output
may go to a pipe or a `--color=never` run.

`apply()` is the one most callers want. It owns the whole "should I color, and at
what tier?" policy in one call:

```cpp
#include "collapse.hh"   // term_color::apply / collapse / detect_depth
using namespace term_color;

// mode:   automatic (tty-gated, honors NO_COLOR) | always | never
// target: automatic (detect the tier) | ansi16 | ansi256 | truecolor | stacked
std::string out = apply(line, mode::automatic, target::automatic, ::isatty(1));
std::fwrite(out.data(), 1, out.size(), stdout);
```

- `mode` mirrors a conventional `--color` flag. `automatic` colors only when the
  sink is a terminal and `NO_COLOR` is unset/empty; `always` forces it; `never`
  strips all color.
- `target` is the tier to collapse to. `automatic` detects it from `COLORTERM` /
  `TERM`; `stacked` is the escape hatch that emits all three tiers and leaves the
  terminal to pick (the portable-but-best-effort form).

`collapse()` is the lower-level primitive `apply()` is built on: reduce every
stacked triple to one `depth`, with no gating.

```cpp
std::string just_256   = collapse(line, depth::ansi256);   // one tier, always
std::string plain_text = collapse(line, depth::none);      // strip all color
depth here = detect_depth();                                // from COLORTERM / TERM
```

Use `collapse()` when you already know the depth you want (or want to force one);
use `apply()` when you want the tty / `NO_COLOR` / mode gating handled for you.

**One gotcha.** `collapse()` recognizes a color by its shape: a run of SGR escapes
whose length is a multiple of three is treated as stacked triples and reduced; any
other run is passed through untouched. So a lone `ESC[1m` (bold) placed *next to* a
color makes a run of four and defeats the collapse. Don't emit bold separately —
use `brgb(r, g, b)` (or `color::ansi(true)`), which folds bold into each of the
three tiers, so the run stays a clean triple.



```sh
cmake -B build && cmake --build build && ctest --test-dir build
```

The test checks that named colors (`RED`, `BLACK`, `WHITE`, `BLUE`) expand to the
exact expected three-tier escape sequence (with `static_assert`, so it runs at
compile time), that `brgb` flips to the bold SGR parameter and that color + text
+ reset concatenates into one `static_string`, that `hsv2rgb` returns the right
fractions for the primaries and the `s=0` gray path, and that the runtime `color`
class matches the compile-time `RED`. It prints `all term-color tests passed` and
exits 0.

## Examples

[`examples/demo.cc`](examples/demo.cc) is a runnable tour. A top-level CMake build
produces it next to the test:

```sh
cmake -B build && cmake --build build && ./build/term_color_demo
```

It prints one compile-time colored string as raw stacked bytes (16-color, then
256-color, then truecolor), then the *same* string collapsed to each depth so you
can watch one source resolve to whatever the terminal supports; a strip of named
swatches; an HSV rainbow via the runtime `color` class; the bold (`brgb`) variant;
and the gating layer (`apply()` with `--color`-style modes, a forced tier, the
un-collapsed `stacked` form, and `NO_COLOR`).

## Provenance

Extracted from [Xapiand](https://github.com/Kronuz/Xapiand), where it backed the
colored log output. The standalone delta is decoupling, not behavior:
`strings::format(...)` in the runtime `color` class became `std::format(...)`
(the Xapiand helper was a thin `std::vformat` wrapper), and the per-color `L_*`
logging shortcut macros — Xapiand log glue, not part of a color library — were
removed; they belong in Xapiand alongside its `log.h`. The color palette and the
escape machinery are unchanged. The compile-time strings are built on
[`static-string`](https://github.com/Kronuz/static-string), the one dependency.
See [ARCHITECTURE.md](ARCHITECTURE.md) for the design and [AGENTS.md](AGENTS.md)
for the repo map and invariants.

## License

MIT, Copyright (c) 2015-2019 Dubalu LLC. See [LICENSE](LICENSE) and the per-file
header.
