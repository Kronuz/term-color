# term-color

Part of [Kronuz C++ Libraries](https://github.com/Kronuz/cpp-libraries), a collection of small, reusable C++ libraries.

[![CI](https://github.com/Kronuz/term-color/actions/workflows/ci.yml/badge.svg)](https://github.com/Kronuz/term-color/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Header-only](https://img.shields.io/badge/header--only-yes-green.svg)

Compile-time ANSI terminal colors for C++20: 151 named colors, each carrying 16-color,
256-color, and truecolor escapes. A runtime layer collapses a line to the one tier the
terminal understands, or strips the colors, by `--color`, `NO_COLOR`, and whether the
output is a terminal. Reach for it for colored logs and command-line tools.

![term-color: the demo's output in a truecolor terminal](docs/term-color.svg)

## ✨ Highlights

- **No runtime cost to build a color.** `RED + "error" + CLEAR_COLOR` is one
  compile-time constant.
- **151 named colors** from `ALICE_BLUE` to `YELLOW_GREEN`, plus `rgb()` and bold
  `brgb()` for your own.
- **Portable by default.** Each color carries 16-color, 256-color, and truecolor
  escapes; a terminal keeps the best one it understands.
- **Correct for pipes and logs.** `apply()` resolves colors to one tier, or strips them,
  by `--color` mode, `NO_COLOR`, and whether the sink is a terminal.
- **One pass to resolve.** `collapse()` reduces a 478-byte log line in 144 ns on an
  Apple M4 Pro (see [Performance](#-performance)).
- **Runtime colors too.** `hsv2rgb` and a `color` class for values known only at
  runtime, a stable color per name (`name_color`), and gradient startup banners.

## 🚀 Quick start

```cpp
#include "colors.h"
#include "collapse.hh"

#include <cstdio>
#include <string>
#include <unistd.h>

int main() {
	constexpr auto line = RED + "error: " + CLEAR_COLOR;   // compile time

	// One tier for this terminal; plain text when piped or NO_COLOR is set.
	std::string out = term_color::apply(std::string(line) + "disk full\n",
		term_color::mode::automatic, term_color::target::automatic, ::isatty(1));
	std::fwrite(out.data(), 1, out.size(), stdout);   // prints error: disk full
}
```

[`examples/demo.cc`](examples/demo.cc) prints the swatches, the HSV sweep, and one
string collapsed to each tier, as in the picture above.

## 📦 Installation

Requires a C++20 compiler and CMake 3.14 or newer. It brings
[static-string](https://github.com/Kronuz/static-string) through FetchContent. CI builds
and tests it with GCC and Clang on Linux (x86_64, arm64), macOS (arm64, x86_64), and
FreeBSD, plus ASan, UBSan, and TSan builds.

```cmake
include(FetchContent)
FetchContent_Declare(term_color
  GIT_REPOSITORY https://github.com/Kronuz/term-color.git
  GIT_TAG        454ae039d85578c629fd0cc8d59c88a8d1a27580)  # or main, which tracks the tip
FetchContent_MakeAvailable(term_color)

target_link_libraries(my_app PRIVATE term_color::term_color)
```

Then `#include "colors.h"` for the named colors and `#include "collapse.hh"` for
`apply()` and `collapse()`.

## 🎯 When to use it

Colored output from a command-line tool or a logger, with `--color` and `NO_COLOR`
handled and pipes kept clean.

Not for:

- **Full-screen terminal interfaces.** It colors text; it does not move the cursor or
  manage the screen.
- **Windows consoles without virtual-terminal sequences enabled.** The output is ANSI
  escapes.

## ⚡ Performance

Time for `collapse()` to resolve one stacked line, from `bench/bench.cc`:

| Input | Apple M4 Pro, to `none` | to `truecolor` | Intel Core i9-9980HK, to `none` | to `truecolor` |
| --- | ---: | ---: | ---: | ---: |
| Plain line, 110 B | 36 ns | 36 ns | 82 ns | 78 ns |
| Log line, 478 B | 144 ns | 173 ns | 308 ns | 445 ns |
| Colorful line, 1,366 B | 470 ns | 664 ns | 949 ns | 1,455 ns |
| 64 KB of plain text | 2.1 µs | 2.1 µs | 3.0 µs | 3.0 µs |

`color::ansi()` for one RGB value takes 38 ns on the M4 Pro and 80 ns on the i9.
`apply()` reads `NO_COLOR`, `COLORTERM`, and `TERM` on every call; a sink that writes
many lines calls `resolve()` once and `policy::apply()` per line, 174 ns for the log
line on the M4 Pro (444 ns on the i9) against 721 ns (1,065 ns) for `apply()` on every
line.

Apple clang 17, `-O3`, macOS 15 on both machines:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DTERM_COLOR_BUILD_BENCH=ON
cmake --build build
./build/term_color_bench
```

[docs/perf/collapse.md](docs/perf/collapse.md) has the method and the numbers by input.

## 📚 Documentation

- [docs/API.md](docs/API.md): named colors, `rgb`/`brgb`, the runtime `color` class,
  and resolving with `apply()` and `collapse()`.
- [ARCHITECTURE.md](ARCHITECTURE.md): how the three stacked tiers are built and
  collapsed.
- [docs/perf/collapse.md](docs/perf/collapse.md): the benchmark method and numbers.
- [CHANGELOG.md](CHANGELOG.md): notable changes, with their measurements.
- [AGENTS.md](AGENTS.md): repository map, invariants, and traps.
- [Catalog entry](https://github.com/Kronuz/cpp-libraries/blob/main/AGENTS.md#term-color).

## 🛠️ Building and testing

```sh
cmake -B build
cmake --build build
ctest --test-dir build
./build/term_color_demo
```

`-DTERM_COLOR_BUILD_BENCH=ON` builds the benchmark.

## 🤝 Contributing

Issues and pull requests are welcome. Read [AGENTS.md](AGENTS.md) first, and the
[catalog's AGENTS.md](https://github.com/Kronuz/cpp-libraries/blob/main/AGENTS.md) for
the family's rules. CI must pass on every platform.

## 📄 License

MIT; see [LICENSE](LICENSE). Extracted from [Xapiand](https://github.com/Kronuz/Xapiand),
where it colors the logs.
