// Benchmark: collapse(), apply() and color::ansi() against the frozen originals
// (test/collapse_old.hh, test/color_tools_old.hh).
//
// No other library resolves stacked SGR escapes, so the baseline is the frozen original. Each row
// is the best of several passes over a rotating set of inputs; "A/A" times the frozen original
// against itself and shows the noise of the run.
//
// Build: cmake -B build -DCMAKE_BUILD_TYPE=Release -DTERM_COLOR_BUILD_BENCH=ON
//        cmake --build build --target term_color_bench
// Run:   ./build/term_color_bench
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <tuple>
#include <vector>

#include "colors.h"
#include "collapse.hh"
#include "color_tools.hh"
#include "../test/collapse_old.hh"
#include "../test/color_tools_old.hh"

namespace {

std::mt19937_64 rng(42);

std::string words(std::size_t n) {
	std::string s;
	while (s.size() < n) {
		auto len = 2 + rng() % 8;
		for (std::size_t i = 0; i < len; ++i) {
			s += static_cast<char>('a' + rng() % 26);
		}
		s += ' ';
	}
	s.resize(n);
	return s;
}

const std::string_view palette[] = {RED, GREEN, BLUE, STEEL_BLUE, DARK_ORANGE, GRAY, WHITE};

// A plain log line: no escapes at all (a sink that was handed uncolored text).
std::vector<std::string> plain_lines() {
	std::vector<std::string> v(256);
	for (auto& s : v) {
		s = words(80 + rng() % 60);
	}
	return v;
}

// A decorated line as a logger writes it: a gradient timestamp (a grey triple per digit group),
// a priority marker in color, the message, a reset.
std::vector<std::string> log_lines() {
	std::vector<std::string> v(256);
	for (auto& s : v) {
		static const std::string_view greys[] = {GRAY, DARK_GRAY, DIM_GRAY, LIGHT_GRAY};
		for (int group = 0; group < 6; ++group) {
			s += greys[group % 4];
			s += std::to_string(10 + rng() % 90);
			s += group < 2 ? "-" : group < 3 ? " " : ":";
			s += CLEAR_COLOR;
		}
		s += palette[rng() % 7];
		s += " [INFO] ";
		s += CLEAR_COLOR;
		s += words(40 + rng() % 60);
		s += CLEAR_COLOR;
		s += '\n';
	}
	return v;
}

// A line that is mostly color: a colored word every few characters.
std::vector<std::string> colorful_lines() {
	std::vector<std::string> v(256);
	for (auto& s : v) {
		for (int w = 0; w < 24; ++w) {
			s += palette[rng() % 7];
			s += words(3 + rng() % 6);
			s += CLEAR_COLOR;
			s += ' ';
		}
	}
	return v;
}

std::vector<std::string> long_text(bool with_color) {
	std::vector<std::string> v(4);
	for (auto& s : v) {
		while (s.size() < 64 * 1024) {
			if (with_color) {
				s += palette[rng() % 7];
			}
			s += words(40);
			if (with_color) {
				s += CLEAR_COLOR;
			}
		}
	}
	return v;
}

volatile std::size_t sink = 0;

template <typename Fn>
double best_ns(const std::vector<std::string>& inputs, Fn&& fn, int reps) {
	double best = 1e300;
	for (int rep = 0; rep < reps; ++rep) {
		auto t0 = std::chrono::steady_clock::now();
		std::size_t s = 0;
		for (const auto& in : inputs) {
			s += fn(in);
		}
		auto t1 = std::chrono::steady_clock::now();
		sink = sink + s;
		best = std::min(best, std::chrono::duration<double, std::nano>(t1 - t0).count() / static_cast<double>(inputs.size()));
	}
	return best;
}

template <typename Old, typename New>
void row(const char* set, const char* op, const std::vector<std::string>& inputs, Old&& old_fn, New&& new_fn,
         int reps = 21) {
	// Alternate the two over rounds and keep the best pass of each; "A/A" is the spread between the
	// frozen original's best in even and odd rounds (how much the host moved during the row).
	const int rounds = 6;
	const int per_round = std::max(2, reps / 4);
	double a_even = 1e300, a_odd = 1e300, b = 1e300;
	for (int r = 0; r < rounds; ++r) {
		double a = best_ns(inputs, old_fn, per_round);
		b = std::min(b, best_ns(inputs, new_fn, per_round));
		double& slot = r % 2 ? a_odd : a_even;
		slot = std::min(slot, a);
	}
	double a = std::min(a_even, a_odd);
	double bytes = 0;
	for (const auto& s : inputs) {
		bytes += static_cast<double>(s.size());
	}
	bytes /= static_cast<double>(inputs.size());
	std::printf("%-9s %-22s %6.0f B  old %9.1f ns  new %9.1f ns  x%5.2f   A/A %4.1f%%\n", set, op, bytes, a, b, a / b,
	            100.0 * std::abs(a_even - a_odd) / a);
}

void collapse_rows(const char* set, const std::vector<std::string>& inputs, int reps = 21) {
	struct { const char* name; term_color::depth d; term_color_old::depth od; } depths[] = {
		{"collapse none", term_color::depth::none, term_color_old::depth::none},
		{"collapse ansi16", term_color::depth::ansi16, term_color_old::depth::ansi16},
		{"collapse ansi256", term_color::depth::ansi256, term_color_old::depth::ansi256},
		{"collapse truecolor", term_color::depth::truecolor, term_color_old::depth::truecolor},
	};
	for (auto& x : depths) {
		row(set, x.name, inputs,
		    [&](const std::string& s) { return term_color_old::collapse(s, x.od).size(); },
		    [&](const std::string& s) { return term_color::collapse(s, x.d).size(); }, reps);
	}
}

} // namespace

int main() {
	auto plain = plain_lines();
	auto logs = log_lines();
	auto colorful = colorful_lines();
	auto long_plain = long_text(false);
	auto long_color = long_text(true);

	std::printf("term_color bench\n");
	collapse_rows("plain", plain);
	collapse_rows("log line", logs);
	collapse_rows("colorful", colorful);
	collapse_rows("64K plain", long_plain, 9);
	collapse_rows("64K color", long_color, 9);

	// apply(): the gating, the environment lookups and the collapse, as a logger sink calls it.
	setenv("COLORTERM", "truecolor", 1);
	unsetenv("NO_COLOR");
	for (auto [name, m, t] : {std::tuple{"apply always+auto", term_color::mode::always, term_color::target::automatic},
	                          std::tuple{"apply auto+auto", term_color::mode::automatic, term_color::target::automatic},
	                          std::tuple{"apply always+256", term_color::mode::always, term_color::target::ansi256}}) {
		row("log line", name, logs,
		    [&](const std::string& s) {
			    return term_color_old::apply(s, static_cast<term_color_old::mode>(m), static_cast<term_color_old::target>(t), true).size();
		    },
		    [&](const std::string& s) { return term_color::apply(s, m, t, true).size(); });
	}
	row("log line", "apply never (strip)", logs,
	    [&](const std::string& s) { return term_color_old::apply(s, term_color_old::mode::never, term_color_old::target::automatic, true).size(); },
	    [&](const std::string& s) { return term_color::apply(s, term_color::mode::never, term_color::target::automatic, true).size(); });
	// A sink that resolves the policy once and applies it to every line.
	const auto resolved = term_color::resolve(term_color::mode::automatic, term_color::target::automatic, true);
	row("log line", "apply(resolved) auto", logs,
	    [&](const std::string& s) {
		    return term_color_old::apply(s, term_color_old::mode::automatic, term_color_old::target::automatic, true).size();
	    },
	    [&](const std::string& s) { return resolved.apply(s).size(); });
	row("plain", "apply never (strip)", plain,
	    [&](const std::string& s) { return term_color_old::apply(s, term_color_old::mode::never, term_color_old::target::automatic, true).size(); },
	    [&](const std::string& s) { return term_color::apply(s, term_color::mode::never, term_color::target::automatic, true).size(); });

	// The runtime color class: three escapes for one RGB value.
	std::vector<std::string> names(256);
	for (auto& s : names) {
		s = words(6 + rng() % 12);
	}
	row("runtime", "color::ansi", names,
	    [](const std::string& s) { return color_tools_old::color(static_cast<std::uint8_t>(s[0] * 7), static_cast<std::uint8_t>(s[1] * 13), static_cast<std::uint8_t>(s[2] * 29)).ansi().size(); },
	    [](const std::string& s) { return color(static_cast<std::uint8_t>(s[0] * 7), static_cast<std::uint8_t>(s[1] * 13), static_cast<std::uint8_t>(s[2] * 29)).ansi().size(); });
	row("runtime", "name_color().ansi", names,
	    [](const std::string& s) { return color_tools_old::name_color(s).ansi().size(); },
	    [](const std::string& s) { return name_color(s).ansi().size(); });
	return 0;
}
