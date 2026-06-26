// Smoke test for the standalone term-color library.
//
// Exercises the three headers:
//   ansi_color.hh / colors.h  the compile-time palette: named colors expand to
//                             a concatenation of truecolor + 256-color + 16-color
//                             SGR escapes, all built as static_strings so the
//                             whole sequence is a compile-time constant (checked
//                             with static_assert).
//   color_tools.hh            the runtime side: hsv2rgb() and the non-constexpr
//                             `color` class that formats the same escapes via
//                             std::format.
//
// Build via CMake: cmake -B build && cmake --build build && ctest --test-dir build
#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>

#include "ansi_color.hh"
#include "colors.h"
#include "color_tools.hh"

using namespace static_string;


// ---------------------------------------------------------------------------
// Compile-time: named colors produce the expected ANSI escape sequence.
//
// Each color is truecolor (ESC[<bold>;38;2;r;g;bm) then 256-color
// (ESC[<bold>;38;5;<idx>m) then 16-color (ESC[<bold>;<sgr>m), so a terminal
// applies the richest it understands and the cruder ones overwrite harmlessly on
// poorer terminals. The 16-color tier is a *real* ANSI 16-color SGR (indices
// 0-7 -> 30-37 normal, 8-15 -> 90-97 bright), NOT another 38;5;n 256-color
// escape: a genuine 16-color terminal that ignores 38;5; still gets a usable
// color. RED = rgb(255,0,0): truecolor 255;0;0, 256-color index 196, 16-color
// index 9 (bright red) -> SGR 91. Compared as static_strings, at compile time.
// ---------------------------------------------------------------------------

static void test_named_colors() {
	static_assert(RED == string(ESC "[0;38;2;255;0;0m" ESC "[0;38;5;196m" ESC "[0;91m"),
	              "RED escape sequence");
	static_assert(BLACK == string(ESC "[0;38;2;0;0;0m" ESC "[0;38;5;16m" ESC "[0;30m"),
	              "BLACK escape sequence");
	static_assert(WHITE == string(ESC "[0;38;2;255;255;255m" ESC "[0;38;5;231m" ESC "[0;97m"),
	              "WHITE escape sequence");
	static_assert(BLUE == string(ESC "[0;38;2;0;0;255m" ESC "[0;38;5;21m" ESC "[0;94m"),
	              "BLUE escape sequence");

	// NO_COLOR is empty; CLEAR_COLOR is the reset SGR, repeated for the three tiers.
	static_assert(NO_COLOR == string(""), "NO_COLOR is empty");
	static_assert(CLEAR_COLOR == string(ESC "[0m" ESC "[0m" ESC "[0m"), "CLEAR_COLOR resets");

	std::printf("named colors OK: RED/BLACK/WHITE/BLUE escapes, NO_COLOR, CLEAR_COLOR\n");
}


// ---------------------------------------------------------------------------
// Compile-time regression: the third (16-color downgrade) escape must be a real
// ANSI 16-color SGR, not a 256-color 38;5;n. This is the bug the fix targets --
// before the fix every one of these ended in 38;5;<idx>m, useless to a terminal
// that only speaks 16 colors. We pin the exact 16-color SGR across the normal
// range (0-7 -> 30-37), the bright range (8-15 -> 90-97), and a gray.
// ---------------------------------------------------------------------------

static void test_16color_downgrade_tier() {
	// static_string has no ends_with/starts_with, but it converts to a
	// std::string_view whose ends_with/starts_with are constexpr in C++20, so
	// these still resolve at compile time.
	constexpr std::string_view red{RED};
	constexpr std::string_view white{WHITE};
	constexpr std::string_view blue{BLUE};
	constexpr std::string_view gray{GRAY};
	constexpr std::string_view black{BLACK};
	constexpr std::string_view green{GREEN};

	// Bright range (idx 8-15 -> 90-97):
	//   RED   (255,0,0)       -> idx 9  -> 91 (bright red)
	//   WHITE (255,255,255)   -> idx 15 -> 97 (bright white, gray path)
	//   BLUE  (0,0,255)       -> idx 12 -> 94 (bright blue)
	//   GRAY  (128,128,128)   -> idx 8  -> 90 (bright black, gray path)
	static_assert(red.ends_with(ESC "[0;91m"), "RED downgrades to 16-color 91, not 38;5;9");
	static_assert(white.ends_with(ESC "[0;97m"), "WHITE downgrades to 16-color 97");
	static_assert(blue.ends_with(ESC "[0;94m"), "BLUE downgrades to 16-color 94");
	static_assert(gray.ends_with(ESC "[0;90m"), "GRAY downgrades to 16-color 90");

	// Normal range (idx 0-7 -> 30-37):
	//   BLACK (0,0,0)   -> idx 0 -> 30 (gray path)
	//   GREEN (0,128,0) -> idx 2 -> 32
	static_assert(black.ends_with(ESC "[0;30m"), "BLACK downgrades to 16-color 30");
	static_assert(green.ends_with(ESC "[0;32m"), "GREEN downgrades to 16-color 32");

	// The downgrade tier must never emit a 256-color escape.
	static_assert(!red.ends_with(ESC "[0;38;5;9m"), "RED must not end in a 256-color escape");

	// And the richer tiers are untouched: truecolor and 256-color are unchanged.
	static_assert(red.starts_with(ESC "[0;38;2;255;0;0m" ESC "[0;38;5;196m"),
	              "RED truecolor + 256-color tiers unchanged");

	std::printf("16-color downgrade OK: 30-37/90-97 SGRs (normal/bright/gray), no 38;5; fallback\n");
}


// ---------------------------------------------------------------------------
// Compile-time: the bold variant flips the leading SGR parameter to 1, and the
// escapes still concatenate into one static_string at compile time.
// ---------------------------------------------------------------------------

static void test_bold_and_concat() {
	constexpr auto bold_red = brgb(255, 0, 0);
	static_assert(bold_red == string(ESC "[1;38;2;255;0;0m" ESC "[1;38;5;196m" ESC "[1;91m"),
	              "bold RED uses SGR parameter 1");

	// A color escape concatenated with text and a reset is itself a compile-time
	// static_string: this is the typical "wrap a label in a color" usage.
	constexpr auto labelled = RED + "error" + CLEAR_COLOR;
	static_assert(labelled.size() == RED.size() + 5 + CLEAR_COLOR.size(),
	              "concatenation length is the sum of parts");

	std::printf("bold/concat OK: brgb flips to bold, color+text+reset concatenates\n");
}


// ---------------------------------------------------------------------------
// Runtime: hsv2rgb produces the expected fractions.
// ---------------------------------------------------------------------------

static bool approx(double a, double b) {
	return std::fabs(a - b) < 1e-9;
}

static void test_hsv2rgb() {
	double r, g, b;

	hsv2rgb(0.0, 1.0, 1.0, r, g, b);   // pure red
	assert(approx(r, 1.0) && approx(g, 0.0) && approx(b, 0.0));

	hsv2rgb(120.0, 1.0, 1.0, r, g, b); // pure green
	assert(approx(r, 0.0) && approx(g, 1.0) && approx(b, 0.0));

	hsv2rgb(240.0, 1.0, 1.0, r, g, b); // pure blue
	assert(approx(r, 0.0) && approx(g, 0.0) && approx(b, 1.0));

	hsv2rgb(0.0, 0.0, 0.5, r, g, b);   // saturation 0 -> gray at `value`
	assert(approx(r, 0.5) && approx(g, 0.5) && approx(b, 0.5));

	std::printf("hsv2rgb OK: red/green/blue primaries and the gray (s=0) path\n");
}


// ---------------------------------------------------------------------------
// Runtime: the non-constexpr `color` class formats the same escapes as the
// compile-time ansi_color (via std::format) and exposes its channels.
// ---------------------------------------------------------------------------

static void test_color_class() {
	color c(255, 0, 0);
	assert(c.red() == 255 && c.green() == 0 && c.blue() == 0);

	// Same bytes as the compile-time RED, just built at runtime.
	std::string ansi = c.ansi();
	assert(ansi == std::string_view(RED));

	std::string bold = c.ansi(true);
	assert(bold == "\033[1;38;2;255;0;0m\033[1;38;5;196m\033[1;91m");

	// The runtime 16-color downgrade tier must agree with the compile-time one:
	// a real ANSI 16-color SGR (RED -> bright red, 91), never a 256-color 38;5;n.
	assert(c.ansi().ends_with("\033[0;91m"));
	assert(c.ansi().find("38;5;9m") == std::string::npos);

	// Spot-check the runtime path across normal/bright/gray ranges, matching the
	// compile-time palette: BLACK->30, GREEN->32, WHITE->97, BLUE->94, GRAY->90.
	assert(color(0, 0, 0).ansi().ends_with("\033[0;30m"));      // idx 0  -> 30
	assert(color(0, 128, 0).ansi().ends_with("\033[0;32m"));    // idx 2  -> 32
	assert(color(255, 255, 255).ansi().ends_with("\033[0;97m")); // idx 15 -> 97
	assert(color(0, 0, 255).ansi().ends_with("\033[0;94m"));    // idx 12 -> 94
	assert(color(128, 128, 128).ansi().ends_with("\033[0;90m")); // idx 8  -> 90

	std::printf("color class OK: channels, ansi() matches compile-time RED, bold variant, 16-color SGRs\n");
}


int main() {
	test_named_colors();
	test_16color_downgrade_tier();
	test_bold_and_concat();
	test_hsv2rgb();
	test_color_class();
	std::printf("all term-color tests passed\n");
	return 0;
}
