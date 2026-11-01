# collapse, color::ansi and policy: measurements

Measured 2026-10-31 at `term-color:e2e6465`. The Intel rows were measured at `term-color:3f3ab00`, which has the same library code.

## Change

- `collapse()` writes into one buffer sized to the input. It copies plain text eight bytes at a time (a has-zero-byte test on `word ^ 0x1b1b1b1b1b1b1b1b` finds the next ESC) and hands stretches of 64 bytes or more to `memchr`. It walks each run of escapes once: the escape of the chosen tier is copied as the walk goes, and the output is rewound when the run's length is not a multiple of three. It finds the end of an escape eight bytes at a time.
- `color::ansi()` writes its three escapes digit by digit into a stack buffer and copies it once. The 256-color and 16-color index arithmetic is the frozen class's.
- `resolve(mode, target, is_terminal)` returns a `policy`, and `policy::apply(s)` applies it. `apply(s, mode, target, is_terminal)` is `resolve(...).apply(s)` and gives the same bytes. `apply()` reads `NO_COLOR`, `COLORTERM` and `TERM` from the environment on every call; a sink that writes many lines resolves once.

Output is identical to the frozen code (`test/collapse_old.hh`, `test/color_tools_old.hh`) for every input the differential test covers.

## Method

- Bench: `bench/bench.cc`, target `term_color_bench` (`-DTERM_COLOR_BUILD_BENCH=ON`), `./build/term_color_bench`. No other library resolves stacked SGR escapes, so the baseline is the frozen code, and the comparators are a hand-written loop and `std::regex` (below).
- Inputs: a plain line (110 B); a log line as the logger writes it (478 B: six gradient timestamp groups, a colored priority marker and a reset around 40 to 100 characters of text, 7 colors and 8 resets); a colorful line (1366 B: 24 colored words, 24 colors and 24 resets); 64 KB of plain text; 64 KB with a color per 40 characters; one RGB value; an 11-character name.
- Each figure is the best of several alternating passes over a rotating set of inputs. "A/A" times the frozen code against itself: 0.0% to 3.6% on the Apple M4 Pro (8.2% on one 64 KB row) and 0.0% to 4.8% on the Intel Core i9-9980HK (11.4% on one 64 KB row).
- Machines: Apple M4 Pro and Intel Core i9-9980HK (macOS 15, x86_64). Apple clang 17, `-O3`, with the machine to itself.

## Results

Apple M4 Pro, time per call:

| input | call | before | after | speedup |
|---|---|---:|---:|---:|
| plain line, 110 B | `collapse` to `none` | 319.0 ns | 36.3 ns | **8.79x** |
| | `collapse` to `truecolor` | 307.3 ns | 35.6 ns | 8.62x |
| log line, 478 B | `collapse` to `none` | 316.7 ns | 143.6 ns | 2.21x |
| | `collapse` to `ansi16` | 510.3 ns | 175.1 ns | 2.91x |
| | `collapse` to `truecolor` | 502.9 ns | 172.5 ns | 2.92x |
| colorful line, 1366 B | `collapse` to `none` | 822.8 ns | 470.1 ns | 1.75x |
| | `collapse` to `ansi16` | 1423.3 ns | 739.9 ns | 1.92x |
| | `collapse` to `truecolor` | 1430.5 ns | 664.1 ns | 2.15x |
| 64 KB of plain text | `collapse` to `none` | 266.0 us | 2.06 us | 128.96x |
| 64 KB, a color per 40 characters | `collapse` to `none` | 62.8 us | 11.8 us | 5.34x |
| | `collapse` to `truecolor` | 82.3 us | 22.7 us | 3.62x |
| log line | `apply`, always + automatic tier | 657.6 ns | 329.3 ns | 2.00x |
| | `apply`, automatic + automatic | 727.4 ns | 403.3 ns | 1.80x |
| | `apply`, never (strip) | 289.7 ns | 139.5 ns | 2.08x |
| | `policy::apply`, resolved once | 720.9 ns | 173.5 ns | 4.15x |
| one RGB value | `color::ansi()` | 202.8 ns | 38.2 ns | 5.30x |
| 11-character name | `name_color(name).ansi()` | 278.5 ns | 109.9 ns | 2.53x |

Intel Core i9-9980HK:

| input | call | before | after | speedup |
|---|---|---:|---:|---:|
| plain line, 110 B | `collapse` to `none` | 433.6 ns | 81.5 ns | **5.32x** |
| | `collapse` to `truecolor` | 417.8 ns | 78.0 ns | 5.35x |
| log line, 478 B | `collapse` to `none` | 611.9 ns | 308.2 ns | 1.99x |
| | `collapse` to `ansi16` | 906.5 ns | 448.7 ns | 2.02x |
| | `collapse` to `truecolor` | 905.3 ns | 444.6 ns | 2.04x |
| colorful line, 1366 B | `collapse` to `none` | 1479.2 ns | 949.0 ns | 1.56x |
| | `collapse` to `ansi16` | 2446.1 ns | 1444.6 ns | 1.69x |
| | `collapse` to `truecolor` | 2424.7 ns | 1454.8 ns | 1.67x |
| 64 KB of plain text | `collapse` to `none` | 181.0 us | 3.00 us | 60.4x |
| 64 KB, a color per 40 characters | `collapse` to `none` | 110.7 us | 22.3 us | 4.97x |
| | `collapse` to `truecolor` | 138.0 us | 35.3 us | 3.91x |
| log line | `apply`, always + automatic tier | 1003.1 ns | 529.9 ns | 1.89x |
| | `apply`, automatic + automatic | 1068.6 ns | 595.7 ns | 1.79x |
| | `apply`, never (strip) | 434.0 ns | 303.2 ns | 1.43x |
| | `policy::apply`, resolved once | 1065.0 ns | 443.6 ns | 2.40x |
| one RGB value | `color::ansi()` | 435.8 ns | 80.4 ns | 5.42x |
| 11-character name | `name_color(name).ansi()` | 536.2 ns | 168.7 ns | 3.18x |

In the `apply` rows, the gap between the automatic and the fixed tier is the lookup of `COLORTERM` (and of `NO_COLOR` for the gate): 80 to 130 ns a call on the M4 Pro, about as much as collapsing the log line. The `policy::apply` row compares a sink that resolved the policy once with `apply` called on every line.

## Against a hand-written loop and std::regex

Measured at `term-color:fc4113d`, the best of two runs of `./build/term_color_bench`, on the Apple M4
Pro (macOS 26) and the Intel Core i9-9980HK (macOS 15), Apple clang 17, CMake Release (`-O3 -DNDEBUG`),
each with the machine to itself. The frozen original's A/A (it timed against itself) is at most 3.0%
on the M4 Pro and 9.3% on the i9 over every row of a run.

No other C++ library resolves term-color's stacked escapes, so the comparators are code a caller
would write:

- **loop**: `loop_collapse()` in `bench/bench.cc`. It finds each ESC with `std::string_view::find`
  and appends the plain stretch in one call, walks each run of `ESC [ ... m` escapes counting them,
  and then drops the run (`none`), keeps the escape of the chosen tier from each stacked triple, or
  copies a run whose length is not a multiple of three unchanged.
- **std::regex**: `regex_collapse()`: `std::regex_replace` with `\x1b\[[^m]*m` for `none` and, for a
  tier, a pattern of three such escapes whose group 1 replaces the triple. It is libc++'s `<regex>`.

Before it times anything the bench compares both with `collapse()` on every input at every depth,
byte for byte, and exits on a difference. They agree on these inputs; they are not the same
function on malformed text. At depth `none`, an escape with no `m` after it is dropped through the
end of the line by `collapse()` and the loop and left in place by the regex, and at a tier a run
whose length is not a multiple of three is left alone by `collapse()` and the loop and partly
reduced by the regex.

Nanoseconds per call (`std::regex` in microseconds):

| Input | Depth | M4 Pro term-color | loop | `std::regex` (µs) | i9 term-color | loop | `std::regex` (µs) |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Plain line, 110 B | `none` | 27.3 | 26.0 | 4.3 | 70.2 | 60.1 | 13.6 |
| | `truecolor` | 25.6 | 24.1 | 6.3 | 67.0 | 57.4 | 19.6 |
| Log line, 478 B | `none` | 159.5 | 423.7 | 17.9 | 310.9 | 464.6 | 54.1 |
| | `truecolor` | 164.7 | 495.0 | 22.5 | 379.7 | 828.9 | 69.3 |
| Colorful line, 1,366 B | `none` | 578.9 | 1,682.8 | 51.2 | 1,049.8 | 1,589.4 | 158.2 |
| | `truecolor` | 682.0 | 1,960.0 | 62.4 | 1,277.3 | 2,571.8 | 184.5 |
| 64 KB of plain text | `none` | 2,041.5 | 1,823.0 | 2,687 | 3,153.5 | 2,236.0 | 7,808 |
| | `truecolor` | 2,062.5 | 1,843.8 | 3,951 | 3,130.2 | 2,268.0 | 11,544 |
| 64 KB, a color per 40 characters | `none` | 14,135.5 | 41,416.8 | 2,508 | 25,007.8 | 36,235.2 | 7,672 |
| | `truecolor` | 19,343.8 | 57,885.2 | 3,287 | 28,886.0 | 66,022.5 | 9,912 |

The loop beats `collapse()` on plain text on both machines (0.71 to 0.95 of its time). `collapse()`
copies plain text eight bytes at a time and hands only stretches of 64 bytes or more to `memchr`; the
loop hands every stretch to `find`. On text with escapes `collapse()` takes 0.33 to 0.38 of the loop's
time on the M4 Pro and 0.44 to 0.69 on the i9.

## Every depth

`collapse()` alone, in nanoseconds, from the same runs (the `new` column of the rows that time it
against the frozen original, best of the two runs):

| Input | M4 Pro `none` | `ansi16` | `ansi256` | `truecolor` | i9 `none` | `ansi16` | `ansi256` | `truecolor` |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Plain line, 110 B | 35.0 | 34.7 | 35.0 | 30.8 | 71.3 | 70.6 | 70.5 | 70.4 |
| Log line, 478 B | 156.9 | 176.9 | 169.3 | 166.5 | 345.8 | 402.1 | 395.7 | 395.3 |
| Colorful line, 1,366 B | 552.7 | 679.2 | 635.3 | 648.9 | 1,096.2 | 1,347.5 | 1,327.0 | 1,333.7 |
| 64 KB of plain text | 2,031.2 | 2,052.0 | 2,000.0 | 2,041.5 | 3,136.5 | 3,182.2 | 3,048.5 | 3,043.0 |
| 64 KB, a color per 40 characters | 13,416.8 | 16,583.2 | 15,927.2 | 20,208.2 | 24,993.5 | 29,044.8 | 28,535.2 | 28,704.5 |

The two blocks of a run time `collapse()` in separate loops, so the same call differs by up to 25%
between them (the 110-byte line on the M4 Pro: 27 and 35 ns); the comparison table uses one block.

## Proof

- `test/diff_test.cc` (CTest `term_color_diff`, 20M comparisons) compares `collapse()` at every depth, `apply()` and `policy::apply()` under every mode, target, terminal flag and environment the policy reads, and `color::ansi()` for every `(r, g, b)` under `--full` (16.7M triples; the default run takes the edges of each channel), against the frozen copies.
- `tools/mutants.py` breaks `collapse()`, the color class and the policy in 25 ways and requires the differential test to fail on each. A mutant that moves the overlapping tail of an escape copy by one byte writes one byte past the escape, which the next output overwrites, so it is equivalent and is not in the list; the list reads the tail copy from the wrong place instead.
- `fuzz/fuzz_collapse.cc` (`-DTERM_COLOR_FUZZ=ON`, clang) runs the same comparison under libFuzzer, ASan and UBSan.

## Decision and open items

- A call into `memchr` per stretch of plain text costs more than the byte loop it replaces when the stretches are short. The first version found every stretch with `memchr`: 10x on plain text, 1.17x on the log line, and 0.75x on a line of 48 escapes with a few characters between them (835.1 to 1116.7 ns). The eight-byte scan, with `memchr` only from 64 bytes, gives 1.7x to 2.9x on the log and colorful lines, 3.6x to 5.3x on 64 KB with a color per 40 characters, and keeps 125x on long plain text. The candidates (`memchr`, byte loops, the eight-byte scan, the eight-byte scan plus a scan for the end of each escape) were timed in one scratch program over short-stretch lines too; a long-plain-text row alone would have chosen the `memchr` version.
- Walking a run once matters on escape-dense input: counting the escapes of a run and then walking it again to pick the escape doubled the work.
- `apply(string_view, policy)` is a member of `policy`, not a free function. As a free function it was chosen as `std::apply` once a translation unit included `<tuple>`, because a non-const lvalue binds to its forwarding reference better than to `const policy&`.
- A benchmark row that reads three `getenv` calls measured 0.7 ns: the compiler had hoisted them out of the loop. Check that a row that looks too good still calls what it claims to.
- Open: the logger calls `apply()` for every line. Moving it to `resolve()` once makes an environment change visible at the next `resolve()` and not at the next line, so it is the owner's decision.
