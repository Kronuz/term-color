# term-color API reference

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

`name_color(name)` gives a name a stable color, the same on every machine: its 64-bit
FNV-1a hash spread around the hue wheel by the golden ratio. Xapiand colors its nodes this
way. `gradient_banner(art, top, bottom, info)` renders a startup wordmark whose lines step
from `top` to `bottom`, with `info` lines right-aligned under it.

```cpp
std::string node = name_color("node1").ansi() + "node1";
const std::string_view art[] = {" _  _ ", "| \\| |", "|_|\\_|"};
const uint8_t top[3] = {144, 238, 144}, bottom[3] = {0, 96, 0};
const std::string info[] = {"v1.0.0"};
std::string banner = gradient_banner(art, top, bottom, info);
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
