// A runnable tour of term-color.
//
// Build (when this repo is the top-level project):
//   cmake -B build && cmake --build build && ./build/term_color_demo
//
// The one idea worth taking away: a colored string is built ONCE, at compile
// time, carrying all three SGR tiers stacked (16-color, then 256-color, then
// truecolor). At runtime you collapse that one string to whatever the terminal
// in front of you supports. This demo shows the stacked bytes, the same string
// resolved at each depth, the named palette, an HSV rainbow, and the gating
// layer (--color modes / NO_COLOR).
#include <cstdio>
#include <string>
#include <string_view>

#include "colors.h"         // the named palette + rgb()/brgb()
#include "color_tools.hh"   // hsv2rgb + the runtime `color` class
#include "collapse.hh"      // depth / collapse / detect_depth / apply / mode / target

static void rule(const char* title) {
	std::printf("\n\033[1m── %s ──\033[0m\n", title);
}

// Print a string with the ESC byte shown as "\e", so stacked escapes are legible
// as text instead of being acted on by the terminal.
static void show_bytes(std::string_view s) {
	for (char c : s) {
		if (c == '\033') {
			std::fputs("\\e", stdout);
		} else {
			std::putc(c, stdout);
		}
	}
	std::putc('\n', stdout);
}

int main() {
	// The terminal's best depth, from COLORTERM / TERM (and NO_COLOR).
	const term_color::depth here = term_color::detect_depth();
	const char* here_name =
		here == term_color::depth::truecolor ? "truecolor" :
		here == term_color::depth::ansi256   ? "256-color" :
		here == term_color::depth::ansi16    ? "16-color"  : "none";

	std::printf("term-color demo  (this terminal detected as: %s)\n", here_name);

	// --- 1. one compile-time string, three stacked tiers ---------------------
	rule("one string, built at compile time, carries every tier");
	// RED is a static_string constant: rgb(255,0,0) stacked 16 -> 256 -> truecolor,
	// concatenated by the compiler. Here are its raw bytes:
	constexpr auto sample = RED + "the quick brown fox" + CLEAR_COLOR;
	std::fputs("  raw bytes : ", stdout);
	show_bytes(std::string_view(sample));

	// --- 2. the same string, collapsed to each depth -------------------------
	rule("collapse() resolves it per terminal");
	std::string_view s{sample};
	std::printf("  truecolor : %s   (38;2;r;g;b)\n", term_color::collapse(s, term_color::depth::truecolor).c_str());
	std::printf("  256-color : %s   (38;5;n)\n",     term_color::collapse(s, term_color::depth::ansi256).c_str());
	std::printf("  16-color  : %s   (30-37/90-97)\n", term_color::collapse(s, term_color::depth::ansi16).c_str());
	std::printf("  none      : %s   (stripped)\n",    term_color::collapse(s, term_color::depth::none).c_str());
	std::puts("  (each line is the SAME source string, reduced to one tier)");

	// --- 3. the named palette ------------------------------------------------
	rule("153 named colors (collapsed to this terminal)");
	const std::pair<std::string_view, std::string_view> swatches[] = {
		{"RED", RED}, {"ORANGE", ORANGE}, {"GOLD", GOLD}, {"GREEN", GREEN},
		{"TEAL", TEAL}, {"DODGER_BLUE", DODGER_BLUE}, {"BLUE_VIOLET", BLUE_VIOLET},
		{"HOT_PINK", HOT_PINK}, {"GRAY", GRAY}, {"WHITE", WHITE},
	};
	std::fputs("  ", stdout);
	for (auto& [name, esc] : swatches) {
		std::string cell = std::string(esc) + "██ " + std::string(std::string_view(CLEAR_COLOR));
		std::fputs(term_color::collapse(cell, here).c_str(), stdout);
	}
	std::putc('\n', stdout);
	std::fputs("  ", stdout);
	for (auto& [name, esc] : swatches) {
		std::string cell = std::string(esc) + std::string(name) + " " + std::string(std::string_view(CLEAR_COLOR));
		std::fputs(term_color::collapse(cell, here).c_str(), stdout);
	}
	std::putc('\n', stdout);

	// --- 4. an HSV rainbow, via the runtime color class ----------------------
	rule("hsv2rgb sweep (runtime color, collapsed to this terminal)");
	std::fputs("  ", stdout);
	for (int i = 0; i < 60; ++i) {
		double r, g, b;
		hsv2rgb(i * 360.0 / 60.0, 1.0, 1.0, r, g, b);
		color c(static_cast<uint8_t>(r * 255), static_cast<uint8_t>(g * 255), static_cast<uint8_t>(b * 255));
		std::string cell = c.ansi() + "█" + std::string(std::string_view(CLEAR_COLOR));
		std::fputs(term_color::collapse(cell, here).c_str(), stdout);
	}
	std::putc('\n', stdout);

	// --- 5. bold + the runtime color's channels ------------------------------
	rule("bold (brgb) and the runtime color class");
	constexpr auto bold = brgb(255, 215, 0) + "bold gold (brgb)" + CLEAR_COLOR;
	std::printf("  %s\n", term_color::collapse(std::string_view(bold), here).c_str());
	color teal(0, 128, 128);
	std::printf("  color(0,128,128) -> r=%d g=%d b=%d, ansi() %zu bytes stacked\n",
		teal.red(), teal.green(), teal.blue(), teal.ansi().size());

	// --- 6. the gating layer: --color modes + NO_COLOR -----------------------
	rule("apply(): gate by mode / NO_COLOR, then collapse");
	std::string line = std::string(std::string_view(DODGER_BLUE)) + "status: ok" + std::string(std::string_view(CLEAR_COLOR));
	const bool tty = true;  // pretend a terminal for the demo
	std::printf("  mode=always  : %s\n", term_color::apply(line, term_color::mode::always, term_color::target::automatic, tty).c_str());
	std::printf("  mode=never   : %s   (stripped)\n", term_color::apply(line, term_color::mode::never, term_color::target::automatic, tty).c_str());
	std::printf("  target=16    : %s\n", term_color::apply(line, term_color::mode::always, term_color::target::ansi16, tty).c_str());
	std::printf("  target=stacked (all tiers, terminal resolves): ");
	show_bytes(term_color::apply(line, term_color::mode::always, term_color::target::stacked, tty));
	std::printf("  NO_COLOR is %s right now\n", term_color::no_color_set() ? "set -> color suppressed in auto mode" : "unset");

	std::puts("\ndone.");
	return 0;
}
