# Changelog

Notable changes to term-color, newest first. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/); entries are dated because the
library has no releases.

## 2026-11-01

### Added

- `docs/perf/collapse.md`: the method, the numbers on two machines, and the variants that did not win.

### Fixed

- `tools/mutants.py` finds `static_string.hh` in the build directory's fetched copy (or in `STATIC_STRING`) and no longer reads a fixed path in a home directory.

## 2026-10-31

### Changed

- `collapse()` writes into one buffer sized to the input. It copies plain text eight bytes at
  a time (handing stretches of 64 bytes or more to `memchr`), walks each run of escapes once,
  copying the escape of the chosen tier as it goes and rewinding a run whose length is not a
  multiple of three, and finds the end of an escape eight bytes at a time. Before, it appended
  plain text one byte at a time and walked each run of escapes twice, once to count it and once
  to pick from it. Output is identical. (`c797664`)
- The first version of this change found each stretch of plain text with `memchr`. It was slower
  than the old code on text made mostly of short stretches between escapes (0.75x when stripping
  the "colorful" lines below) and was replaced by the eight-byte scan.
- `color::ansi()` writes its three escapes digit by digit into a stack buffer and copies it
  once. Before, it built them with `std::format` and joined them with two concatenations. The
  256-color and 16-color index arithmetic is unchanged. Output is identical. (`c797664`)

### Added

- `resolve(mode, target, is_terminal)` returns a `policy`, and `policy::apply(s)` applies it.
  `apply(s, mode, target, is_terminal)` is now `resolve(...).apply(s)` and gives the same bytes.
  `apply()` reads `NO_COLOR`, `COLORTERM` and `TERM` from the environment on every call; a sink
  that writes many lines resolves once.
- `test/diff_test.cc` compares `collapse()` at every depth, `apply()` and `policy::apply()` under
  every mode, target, terminal flag and environment the policy reads, and `color::ansi()` for
  every `(r, g, b)` (`--full`; the default run takes the edges of each channel), with frozen
  copies of the old headers (20M comparisons, CTest `term_color_diff`).
- `tools/mutants.py` breaks `collapse()`, the color class and the policy in 25 ways; the
  differential test has to fail on each.
- `fuzz/fuzz_collapse.cc`, a libFuzzer target of the same comparison (`TERM_COLOR_FUZZ`).
- `bench/bench.cc` (`TERM_COLOR_BUILD_BENCH`) times all of it against the frozen copies.

Time per call on an Apple M4 Pro (Apple clang 17, `-O3`), from `bench/bench.cc` with the machine
to itself. Before is the frozen code in `test/collapse_old.hh` and
`test/color_tools_old.hh`; each figure is the best of several alternating passes, and the frozen
code run against itself differs by 0.0% to 3.6% (8.2% on one 64K row):

| input | call | before | after | speedup |
| --- | --- | ---: | ---: | ---: |
| plain line, 110 B | `collapse` to `none` | 319.0 ns | 36.3 ns | 8.79x |
| | `collapse` to `truecolor` | 307.3 ns | 35.6 ns | 8.62x |
| log line, 478 B, 15 stacked triples | `collapse` to `none` | 316.7 ns | 143.6 ns | 2.21x |
| | `collapse` to `ansi16` | 510.3 ns | 175.1 ns | 2.91x |
| | `collapse` to `truecolor` | 502.9 ns | 172.5 ns | 2.92x |
| colorful line, 1366 B, 48 stacked triples | `collapse` to `none` | 822.8 ns | 470.1 ns | 1.75x |
| | `collapse` to `ansi16` | 1423.3 ns | 739.9 ns | 1.92x |
| | `collapse` to `truecolor` | 1430.5 ns | 664.1 ns | 2.15x |
| 64 KB of plain text | `collapse` to `none` | 266.0 µs | 2.06 µs | 128.96x |
| 64 KB with a color per 40 characters | `collapse` to `none` | 62.8 µs | 11.8 µs | 5.34x |
| | `collapse` to `truecolor` | 82.3 µs | 22.7 µs | 3.62x |
| log line | `apply`, always + automatic tier | 657.6 ns | 329.3 ns | 2.00x |
| | `apply`, automatic + automatic | 727.4 ns | 403.3 ns | 1.80x |
| | `apply`, never (strip) | 289.7 ns | 139.5 ns | 2.08x |
| | `policy::apply`, resolved once | 720.9 ns | 173.5 ns | 4.15x |
| `color` of one RGB value | `ansi()` | 202.8 ns | 38.2 ns | 5.30x |
| 11-character name | `name_color(name).ansi()` | 278.5 ns | 109.9 ns | 2.53x |

In the `apply` rows, the difference between the automatic and the fixed tier is the lookup of
`COLORTERM` (and of `NO_COLOR` for the gate): about 80 to 130 ns a call on this host. The
`policy::apply` row compares a sink that resolved the policy once with the old `apply` called on
every line.

On an Intel i9-9980HK (Apple clang 17, `-O3`, machine to itself; the frozen code run against itself
differs by 0.0% to 4.8%, and 11.4% on one 64K row):

| input | call | before | after | speedup |
| --- | --- | ---: | ---: | ---: |
| plain line, 110 B | `collapse` to `none` | 433.6 ns | 81.5 ns | 5.32x |
| | `collapse` to `truecolor` | 417.8 ns | 78.0 ns | 5.35x |
| log line, 478 B | `collapse` to `none` | 611.9 ns | 308.2 ns | 1.99x |
| | `collapse` to `ansi16` | 906.5 ns | 448.7 ns | 2.02x |
| | `collapse` to `truecolor` | 905.3 ns | 444.6 ns | 2.04x |
| colorful line, 1366 B | `collapse` to `none` | 1479.2 ns | 949.0 ns | 1.56x |
| | `collapse` to `ansi16` | 2446.1 ns | 1444.6 ns | 1.69x |
| | `collapse` to `truecolor` | 2424.7 ns | 1454.8 ns | 1.67x |
| 64 KB of plain text | `collapse` to `none` | 181.0 µs | 3.00 µs | 60.4x |
| 64 KB with a color per 40 characters | `collapse` to `none` | 110.7 µs | 22.3 µs | 4.97x |
| | `collapse` to `truecolor` | 138.0 µs | 35.3 µs | 3.91x |
| log line | `apply`, always + automatic tier | 1003.1 ns | 529.9 ns | 1.89x |
| | `apply`, automatic + automatic | 1068.6 ns | 595.7 ns | 1.79x |
| | `apply`, never (strip) | 434.0 ns | 303.2 ns | 1.43x |
| | `policy::apply`, resolved once | 1065.0 ns | 443.6 ns | 2.40x |
| `color` of one RGB value | `ansi()` | 435.8 ns | 80.4 ns | 5.42x |
| 11-character name | `name_color(name).ansi()` | 536.2 ns | 168.7 ns | 3.18x |

The "log line" is a decorated line as the logger writes it: six gradient timestamp groups,
a colored priority marker and a reset around 40 to 100 characters of text (7 colors and
8 resets). The "colorful line" has 24 colored words (24 colors and 24 resets).
