#!/usr/bin/env python3
"""Mutation check for collapse() and the runtime color class.

For each mutant: copy the repo's headers with one deliberate bug into a scratch dir, build
test/diff_test.cc against them (with the frozen originals) and require that the test FAILS or
crashes. A mutant that survives means the differential test would not catch that bug.

usage: tools/mutants.py TREE_DIR [CXX] [only-name ...]
       env: MUT_DIR (scratch dir), STATIC_STRING (directory holding static_string.hh),
            QUIET_GATE=1 (start each mutant only while `quiet check` reports no live hold)
"""
import os
import shutil
import subprocess
import sys
import tempfile
import time

tree = os.path.abspath(sys.argv[1])
cxx = sys.argv[2] if len(sys.argv) > 2 else "c++"
only = set(sys.argv[3:])
static_string = os.environ.get("STATIC_STRING", os.path.expanduser("~/code/Kronuz/libs/static-string"))

# name: (file, old, new)
MUTANTS = {
    # ansi256 picks the truecolor escape
    "pick_256": ("collapse.hh", "d == depth::ansi256 ? 1 : 0", "d == depth::ansi256 ? 2 : 0"),
    # truecolor picks the 256-color escape
    "pick_true": ("collapse.hh", "d == depth::truecolor ? 2 :", "d == depth::truecolor ? 1 :"),
    # `none` keeps the escapes
    "strip_off": ("collapse.hh", "const bool strip = d == depth::none;", "const bool strip = false;"),
    # a lone ESC is dropped
    "lone_esc_dropped": ("collapse.hh", "*w++ = '\\033';  // a lone ESC is plain text", "(void)w;"),
    # a run whose length is not a multiple of three is dropped instead of kept
    "odd_run_dropped": ("collapse.hh", "if (!strip && count % 3 != 0) {", "if (!strip && count % 3 != 0 && count > 100) {"),
    # the multiple-of-three test is off by one
    "triple_test": ("collapse.hh", "if (!strip && count % 3 != 0) {", "if (!strip && count % 3 != 1) {"),
    # an escape loses its terminating 'm'
    "no_terminator": ("collapse.hh", "\treturn k < end ? k + 1 : end;", "\treturn k < end ? k : end;"),
    # a run needs two bytes after the last escape
    "run_end": ("collapse.hh", "while (j + 1 < end && j[0] == '\\033' && j[1] == '[') {", "while (j + 2 < end && j[0] == '\\033' && j[1] == '[') {"),
    # every escape of a triple is kept
    "keep_all": ("collapse.hh", "if (!strip && count % 3 == pick) {", "if (!strip) {"),
    # an untouched run is restored one byte short
    "restore_short": ("collapse.hh", "std::memcpy(w, p, static_cast<std::size_t>(j - p));", "std::memcpy(w, p, static_cast<std::size_t>(j - p - 1));"),
    # the result keeps its zero padding
    "no_resize": ("collapse.hh", "out.resize(static_cast<std::size_t>(w - out.data()));", ""),
    # the word scan for ESC looks for another byte
    "text_word_scan": ("collapse.hh", "0x1b1b1b1b1b1b1b1bull", "0x1c1c1c1c1c1c1c1cull"),
    # the long-stretch copy loses its last byte
    "text_long_short": ("collapse.hh", "std::memcpy(w, p, static_cast<std::size_t>(stop - p));\n\t\t\tw += stop - p;", "std::memcpy(w, p, static_cast<std::size_t>(stop - p - 1));\n\t\t\tw += stop - p - 1;"),
    # the word scan for the end of an escape looks for another byte
    "escape_word_scan": ("collapse.hh", "0x6d6d6d6d6d6d6d6dull", "0x6e6e6e6e6e6e6e6eull"),
    # a short escape copy loses its last byte
    "copy_small_short": ("collapse.hh", "\t\tfor (std::size_t i = 0; i < n; ++i) {\n\t\t\tto[i] = from[i];", "\t\tfor (std::size_t i = 0; i + 1 < n; ++i) {\n\t\t\tto[i] = from[i];"),
    # the overlapping tail of a longer escape copy reads from the wrong place
    "copy_small_tail": ("collapse.hh", "std::memcpy(to + n - 8, from + n - 8, 8);", "std::memcpy(to + n - 8, from + n - 9, 8);"),
    # a resolved policy strips where it should color
    "policy_color": ("collapse.hh", "\t\tif (!color) {\n\t\t\treturn collapse(s, depth::none);", "\t\tif (color) {\n\t\t\treturn collapse(s, depth::none);"),
    # hundreds are not written for 100
    "put_hundred": ("color_tools.hh", "if (v >= 100) {", "if (v > 100) {"),
    # tens are not written for 10
    "put_ten": ("color_tools.hh", "} else if (v >= 10) {", "} else if (v > 10) {"),
    # bold and plain swap
    "bold_swapped": ("color_tools.hh", "const char lead = bold ? '1' : '0';", "const char lead = bold ? '0' : '1';"),
    # the 256-color escape names the wrong mode
    "mode_256": ("color_tools.hh", "*w++ = '3'; *w++ = '8'; *w++ = ';'; *w++ = '5'; *w++ = ';';", "*w++ = '3'; *w++ = '8'; *w++ = ';'; *w++ = '4'; *w++ = ';';"),
    # red and green swap in the truecolor escape
    "rg_swapped": ("color_tools.hh", "w = put(w, r);\n\t\t*w++ = ';';\n\t\tw = put(w, g);", "w = put(w, g);\n\t\t*w++ = ';';\n\t\tw = put(w, r);"),
    # the gray ramp rounds differently
    "gray_ramp": ("color_tools.hh", "231 + static_cast<int>((r * 25.0f / 255.0f) + 0.5f)", "231 + static_cast<int>((r * 25.0f / 255.0f) + 0.4f)"),
    # the bright-gray threshold moves by one
    "gray_16": ("color_tools.hh", "r > 128 ? 7 :", "r > 129 ? 7 :"),
    # the tiers come out in the wrong order
    "tier_order": ("color_tools.hh", "w = put(w, sgr16());", "w = put(w, index256());"),
}

if len(MUTANTS) < 15:
    raise SystemExit("mutant table shrank")

work = os.environ.get("MUT_DIR") or tempfile.mkdtemp(prefix="tc_mutants_")
os.makedirs(work, exist_ok=True)


def wait_for_quiet():
    """With QUIET_GATE=1, start each mutant only while `quiet check` reports no live hold."""
    if os.environ.get("QUIET_GATE") != "1":
        return
    while subprocess.run(["quiet", "check"], capture_output=True).returncode == 1:
        time.sleep(2)


survivors = []
for name, (file, old, new) in MUTANTS.items():
    if only and name not in only:
        continue
    wait_for_quiet()
    src = open(os.path.join(tree, file), encoding="utf-8").read()
    if src.count(old) != 1:
        print(f"{name}: pattern matches {src.count(old)} times in {file}, fix the table")
        survivors.append(name)
        continue
    d = os.path.join(work, name)
    os.makedirs(d, exist_ok=True)
    for h in ("ansi_color.hh", "colors.h", "collapse.hh", "color_tools.hh"):
        shutil.copy(os.path.join(tree, h), d)
    with open(os.path.join(d, file), "w", encoding="utf-8") as f:
        f.write(src.replace(old, new))
    exe = os.path.join(d, "diff_test")
    cmd = [cxx, "-std=c++20", "-O1", "-I" + d, "-I" + static_string, "-I" + os.path.join(tree, "test"),
           os.path.join(tree, "test/diff_test.cc"), "-o", exe]
    built = subprocess.run(cmd, capture_output=True, text=True)
    if built.returncode != 0:
        print(f"{name}: does not compile\n{built.stderr[:400]}")
        survivors.append(name)
        continue
    try:
        ran = subprocess.run([exe], capture_output=True, text=True, timeout=120)
    except subprocess.TimeoutExpired:
        print(f"{name}: killed (the test does not finish)")
        shutil.rmtree(d, ignore_errors=True)
        continue
    if ran.returncode == 0:
        print(f"{name}: SURVIVED")
        survivors.append(name)
    else:
        print(f"{name}: killed (exit {ran.returncode})")
    shutil.rmtree(d, ignore_errors=True)

if survivors:
    print("survivors:", ", ".join(survivors))
    sys.exit(1)
print("all mutants killed")
