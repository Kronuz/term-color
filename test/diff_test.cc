// Differential test: collapse(), apply() and color::ansi() against the frozen originals
// (collapse_old.hh, color_tools_old.hh). Every result must equal the original's byte for byte:
// exhaustive strings over the characters that matter, random mixes of whole, truncated and
// stacked escapes, the named palette, every gating combination of apply(), and every
// (r, g, b) the runtime color class can be built from.
//
// Run: term_color_diff_test [--full]     (--full checks all 2^24 colors; takes a few minutes)
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "colors.h"
#include "collapse.hh"
#include "color_tools.hh"
#include "collapse_old.hh"
#include "color_tools_old.hh"

namespace {

std::size_t checked = 0;
std::size_t failures = 0;

std::string show(std::string_view s) {
	std::string out;
	for (unsigned char c : s.substr(0, 96)) {
		char buf[8];
		if (c == '\033') {
			out += "\\e";
		} else if (c >= 0x20 && c < 0x7f && c != '\\') {
			out += static_cast<char>(c);
		} else {
			std::snprintf(buf, sizeof buf, "\\x%02x", c);
			out += buf;
		}
	}
	return out;
}

void fail(const char* what, std::string_view in, const std::string& want, const std::string& got) {
	if (++failures <= 20) {
		std::printf("FAIL %s of \"%s\" (%zu bytes): want \"%s\" got \"%s\"\n", what, show(in).c_str(), in.size(),
		            show(want).c_str(), show(got).c_str());
	}
}

constexpr term_color::depth depths[] = {
	term_color::depth::none, term_color::depth::ansi16, term_color::depth::ansi256, term_color::depth::truecolor,
	static_cast<term_color::depth>(7),  // not an enumerator: reads as the first tier
};

void check_collapse(std::string_view in) {
	for (auto d : depths) {
		++checked;
		auto want = term_color_old::collapse(in, static_cast<term_color_old::depth>(d));
		auto got = term_color::collapse(in, d);
		if (got != want) {
			fail("collapse", in, want, got);
		}
	}
}

struct Rng {
	std::uint64_t s;
	std::uint64_t operator()() {
		std::uint64_t z = (s += 0x9e3779b97f4a7c15ULL);
		z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
		z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
		return z ^ (z >> 31);
	}
	std::size_t below(std::size_t n) { return static_cast<std::size_t>((*this)() % n); }
};

// Every string up to max_len over `alphabet`.
void exhaustive(std::string_view alphabet, std::size_t max_len) {
	std::string s;
	std::vector<std::size_t> digit;
	for (std::size_t len = 0; len <= max_len; ++len) {
		s.assign(len, alphabet[0]);
		digit.assign(len, 0);
		for (;;) {
			check_collapse(s);
			std::size_t k = 0;
			while (k < len && ++digit[k] == alphabet.size()) {
				digit[k] = 0;
				s[k] = alphabet[0];
				++k;
			}
			if (k == len) {
				break;
			}
			s[k] = alphabet[digit[k]];
		}
	}
}

void test_collapse() {
	// The characters that decide anything: ESC, '[', the terminator 'm', a digit, text.
	exhaustive("\033[ma;1", 8);
	exhaustive(std::string_view("\033[m\0a", 5), 8);
	// Whole escapes, truncated escapes, lone ESCs, text and stacked runs, in random order.
	static const char* const pieces[] = {
		"\033[0;91m", "\033[0;38;5;196m", "\033[0;38;2;255;0;0m", "\033[1;30m", "\033[m", "\033[0m",
		"\033[0;38;2;12", "\033[", "\033[0;9", "\033", "[", "m", "text", " ", "a longer piece of plain text, ", "\n",
		"\033[0;91m\033[0;38;5;196m\033[0;38;2;255;0;0m",
		"\033[0;91m\033[0;38;5;196m",
		"\033[1m\033[0;91m\033[0;38;5;196m\033[0;38;2;255;0;0m",
		"\033[0;91m\033[0;38;5;196m\033[0;38;2;255;0;0m\033[0;91m\033[0;38;5;196m\033[0;38;2;255;0;0m",
		"\033\033[0;91m", "\033[0;91m\033", "\033[0;91m\033[", "\033[\033[0;91m",
		"\xc3\xa9", "\xff", "\033[38;2;\xc3\xa9m",
	};
	constexpr std::size_t count = sizeof(pieces) / sizeof(*pieces);
	Rng rng{0xc0102};
	for (std::size_t n = 0; n < 300000; ++n) {
		std::string s;
		auto parts = rng.below(14);
		for (std::size_t k = 0; k < parts; ++k) {
			s += pieces[rng.below(count)];
		}
		check_collapse(s);
		// Cut it at every possible byte now and then: truncated input of every shape.
		if (n % 50 == 0) {
			for (std::size_t cut = 0; cut <= s.size(); ++cut) {
				check_collapse(std::string_view(s).substr(0, cut));
			}
		}
	}
	// The named palette, plain and bold, as a logger line is built.
	const std::string_view colors[] = {RED, GREEN, BLUE, WHITE, BLACK, GRAY, STEEL_BLUE, DARK_ORANGE, CLEAR_COLOR};
	for (std::size_t n = 0; n < 100000; ++n) {
		std::string s;
		auto parts = rng.below(12);
		for (std::size_t k = 0; k < parts; ++k) {
			if (rng.below(2)) {
				s += colors[rng.below(sizeof(colors) / sizeof(*colors))];
			} else {
				s += pieces[12 + rng.below(count - 12)];
			}
		}
		check_collapse(s);
	}
	// Long inputs, past every buffer size.
	for (std::size_t n = 0; n < 40; ++n) {
		std::string s;
		auto target = 1000 + rng.below(100000);
		while (s.size() < target) {
			s += pieces[rng.below(count)];
		}
		check_collapse(s);
	}
	std::printf("collapse: %zu comparisons so far\n", checked);
}

// apply(): every mode, target and terminal flag under every environment the policy reads.
void test_apply() {
	static const char* const no_color[] = {nullptr, "", "1", "anything"};
	static const char* const colorterm[] = {nullptr, "", "truecolor", "24bit", "xterm"};
	static const char* const term[] = {nullptr, "", "xterm", "xterm-256color", "dumb"};
	const std::string line = std::string(RED) + "error: " + CLEAR_COLOR + "disk " + std::string(brgb(1, 2, 3)) +
		"full\n" + "\033[1m" + std::string(GREEN) + "ok" + CLEAR_COLOR;
	constexpr term_color::mode modes[] = {term_color::mode::automatic, term_color::mode::always, term_color::mode::never};
	constexpr term_color::target targets[] = {
		term_color::target::automatic, term_color::target::ansi16, term_color::target::ansi256,
		term_color::target::truecolor, term_color::target::stacked,
	};
	auto set = [](const char* name, const char* value) {
		if (value == nullptr) {
			unsetenv(name);
		} else {
			setenv(name, value, 1);
		}
	};
	for (const char* nc : no_color) {
		for (const char* ct : colorterm) {
			for (const char* t : term) {
				set("NO_COLOR", nc);
				set("COLORTERM", ct);
				set("TERM", t);
				for (auto m : modes) {
					for (auto tg : targets) {
						for (bool tty : {false, true}) {
							for (std::string_view s : {std::string_view(line), std::string_view(""), std::string_view("plain")}) {
								++checked;
								auto want = term_color_old::apply(s,
									static_cast<term_color_old::mode>(m), static_cast<term_color_old::target>(tg), tty);
								auto got = term_color::apply(s, m, tg, tty);
								if (got != want) {
									fail("apply", s, want, got);
								}
								auto via_policy = term_color::resolve(m, tg, tty).apply(s);
								if (via_policy != want) {
									fail("apply(policy)", s, want, via_policy);
								}
							}
						}
					}
				}
				for (bool respect : {false, true}) {
					++checked;
					if (static_cast<int>(term_color::detect_depth(respect)) !=
					    static_cast<int>(term_color_old::detect_depth(respect))) {
						fail("detect_depth", "", "", "");
					}
				}
			}
		}
	}
	unsetenv("NO_COLOR");
	unsetenv("COLORTERM");
	unsetenv("TERM");
	std::printf("apply: %zu comparisons so far\n", checked);
}

void check_color(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
	for (bool bold : {false, true}) {
		++checked;
		auto want = color_tools_old::color(r, g, b).ansi(bold);
		auto got = color(r, g, b).ansi(bold);
		if (got != want) {
			char what[64];
			std::snprintf(what, sizeof what, "color(%d,%d,%d).ansi(%d)", r, g, b, bold);
			fail(what, "", want, got);
		}
	}
}

void test_color(bool full) {
	if (full) {
		for (unsigned r = 0; r < 256; ++r) {
			for (unsigned g = 0; g < 256; ++g) {
				for (unsigned b = 0; b < 256; ++b) {
					check_color(static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g), static_cast<std::uint8_t>(b));
				}
			}
		}
	} else {
		static const unsigned edges[] = {0, 1, 5, 6, 7, 31, 32, 33, 64, 100, 127, 128, 129, 191, 192, 193, 249, 250, 254, 255};
		for (unsigned r = 0; r < 256; ++r) {
			for (unsigned g = 0; g < 256; ++g) {
				for (unsigned b : edges) {
					check_color(static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g), static_cast<std::uint8_t>(b));
				}
				check_color(static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g), static_cast<std::uint8_t>(r));
				check_color(static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g), static_cast<std::uint8_t>(g));
			}
			check_color(static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(r));
		}
		Rng rng{0xc0102};
		for (std::size_t n = 0; n < 200000; ++n) {
			auto v = rng();
			check_color(static_cast<std::uint8_t>(v), static_cast<std::uint8_t>(v >> 8), static_cast<std::uint8_t>(v >> 16));
		}
	}
	// name_color and gradient_banner build on color::ansi().
	for (std::string_view name : {"", "a", "node-1", "node-2", "Xapiand", "10.0.0.1:8890", "a much longer node name for the hash"}) {
		++checked;
		auto want = color_tools_old::name_color(name).ansi();
		auto got = name_color(name).ansi();
		if (got != want) {
			fail("name_color", name, want, got);
		}
	}
	const std::string_view art[] = {"  ____  ", " / __ \\ ", "| |  | |", " \\____/ "};
	const std::string info[] = {"version 1", "build abc"};
	const std::uint8_t top[3] = {255, 120, 0}, bottom[3] = {20, 40, 200};
	++checked;
	if (gradient_banner(art, top, bottom, info) != color_tools_old::gradient_banner(art, top, bottom, info)) {
		fail("gradient_banner", "", "", "differs");
	}
	std::printf("color: %zu comparisons so far\n", checked);
}

}  // namespace

int main(int argc, char** argv) {
	bool full = argc > 1 && std::strcmp(argv[1], "--full") == 0;
	test_collapse();
	test_apply();
	test_color(full);
	if (failures != 0) {
		std::printf("%zu of %zu comparisons DIFFER from the frozen originals\n", failures, checked);
		return 1;
	}
	std::printf("all term-color differential tests passed (%zu comparisons)\n", checked);
	return 0;
}
