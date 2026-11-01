# term-color

Part of [Kronuz C++ Libraries](https://github.com/Kronuz/cpp-libraries), a collection of small, reusable C++ libraries.

[![CI](https://github.com/Kronuz/term-color/actions/workflows/ci.yml/badge.svg)](https://github.com/Kronuz/term-color/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Header-only](https://img.shields.io/badge/header--only-yes-green.svg)

Compile-time ANSI terminal colors for C++20: 151 named colors that expand to
escape sequences at compile time, portable across 16-color, 256-color, and
truecolor terminals, plus a runtime layer that honors `--color`, `NO_COLOR`, and
pipes. For colored logs and command-line tools.

![term-color: the demo's output in a truecolor terminal](docs/term-color.svg)

## ✨ Highlights

- **Zero runtime cost.** `RED + "error" + CLEAR_COLOR` is one compile-time
  constant.
- **151 named colors** from `ALICE_BLUE` to `YELLOW_GREEN`, plus `rgb()` and bold
  `brgb()` for your own.
- **Portable by default.** Each color carries 16-color, 256-color, and truecolor
  escapes; the terminal keeps the best one it understands.
- **Correct for pipes and logs.** `apply()` resolves colors to one tier, or strips
  them, by `--color` mode, `NO_COLOR`, and whether the sink is a terminal.
- **Runtime colors too.** `hsv2rgb` and a `color` class for values known only at
  runtime, a stable color per name (`name_color`), and gradient startup banners.

## 📦 Installation

Requires a C++20 compiler and CMake 3.14 or newer; it fetches
[static-string](https://github.com/Kronuz/static-string). CI builds it on Linux
(x86_64, arm64), macOS (arm64, x86_64), and FreeBSD, plus ASan, UBSan, and TSan
builds.

```cmake
include(FetchContent)
FetchContent_Declare(term_color
  GIT_REPOSITORY https://github.com/Kronuz/term-color.git
  GIT_TAG        main)
FetchContent_MakeAvailable(term_color)

target_link_libraries(my_app PRIVATE term_color::term_color)
```

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
	std::fwrite(out.data(), 1, out.size(), stdout);
}
```

[`examples/demo.cc`](examples/demo.cc) prints swatches, an HSV rainbow, and one
string collapsed to each depth.

## 🎯 When to use it

- ✅ Colored output from a CLI or a logger, with `--color` and `NO_COLOR` handled.
- ❌ Full-screen terminal UIs: this colors text; it does not move the cursor or
  manage the screen.
- ❌ Windows consoles without virtual-terminal sequences enabled.

## ⚡ Performance

Resolving a stacked color line is one pass over it. On an Apple M4 Pro (Apple clang 17,
`-O3`), from `bench/bench.cc`:

| input | `collapse` to `none` | `collapse` to `truecolor` |
| --- | ---: | ---: |
| plain line, 110 B | 36 ns | 36 ns |
| log line, 478 B | 144 ns | 173 ns |
| colorful line, 1366 B | 470 ns | 664 ns |
| 64 KB of plain text | 2.1 µs | 2.1 µs |

`color::ansi()` for one RGB value takes 38 ns. `apply()` reads `NO_COLOR`, `COLORTERM` and
`TERM` on every call, 80 to 130 ns each here; a sink that writes many lines calls `resolve()`
once and `policy::apply()` per line (174 ns for the log line above, against 721 ns for
`apply()` on every line in the same loop).

On an Intel i9-9980HK the same rows take 82 and 78 ns (plain line), 308 and 445 ns (log line),
949 and 1455 ns (colorful line) and 3.0 µs (64 KB of plain text) for `none` and `truecolor`;
`color::ansi()` takes 80 ns, and `policy::apply()` 444 ns on the log line against 1065 ns for
`apply()` on every line.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DTERM_COLOR_BUILD_BENCH=ON && cmake --build build
./build/term_color_bench
```

[docs/perf/collapse.md](docs/perf/collapse.md) has the method, the numbers by input and the variants that did not win. [CHANGELOG.md](CHANGELOG.md) lists the changes with their measurements.

## 📚 Documentation

- [docs/API.md](docs/API.md): named colors, `rgb`/`brgb`, the runtime `color`
  class, and resolving with `apply()` and `collapse()`.
- [ARCHITECTURE.md](ARCHITECTURE.md): how the three stacked tiers are built and
  collapsed.
- [AGENTS.md](AGENTS.md): repository map, invariants, and traps.

## 🛠️ Building and testing

```sh
cmake -B build && cmake --build build && ctest --test-dir build
./build/term_color_demo
```

## 🤝 Contributing

Issues and pull requests are welcome. Read [AGENTS.md](AGENTS.md) first. CI must
pass on every platform.

## 📄 License

MIT, Copyright (c) 2015-2019 Dubalu LLC. See [LICENSE](LICENSE). Extracted from
[Xapiand](https://github.com/Kronuz/Xapiand), where it colors the logs.
