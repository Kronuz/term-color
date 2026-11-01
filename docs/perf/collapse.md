# collapse, color::ansi and policy: measurements

Measured 2026-10-31 at `term-color:e2e6465`. The Intel rows were measured at `term-color:3f3ab00`, which has the same library code.

## Change

- `collapse()` writes into one buffer sized to the input. It copies plain text eight bytes at a time (a has-zero-byte test on `word ^ 0x1b1b1b1b1b1b1b1b` finds the next ESC) and hands stretches of 64 bytes or more to `memchr`. It walks each run of escapes once: the escape of the chosen tier is copied as the walk goes, and the output is rewound when the run's length is not a multiple of three. It finds the end of an escape eight bytes at a time.
- `color::ansi()` writes its three escapes digit by digit into a stack buffer and copies it once. The 256-color and 16-color index arithmetic is the frozen class's.
- `resolve(mode, target, is_terminal)` returns a `policy`, and `policy::apply(s)` applies it. `apply(s, mode, target, is_terminal)` is `resolve(...).apply(s)` and gives the same bytes. `apply()` reads `NO_COLOR`, `COLORTERM` and `TERM` from the environment on every call; a sink that writes many lines resolves once.

Output is identical to the frozen code (`test/collapse_old.hh`, `test/color_tools_old.hh`) for every input the differential test covers.

## Method

- Bench: `bench/bench.cc`, target `term_color_bench` (`-DTERM_COLOR_BUILD_BENCH=ON`), `./build/term_color_bench`. No other library resolves stacked SGR escapes, so the baseline is the frozen code.
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
