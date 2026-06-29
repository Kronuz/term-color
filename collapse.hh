/*
 * Copyright (c) 2015-2026 Dubalu LLC / Germán Méndez Bravo (Kronuz)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// Resolving term-color's depth-portable output to a terminal.
//
// rgb()/clear_color() emit THREE stacked SGR escapes per color (16-color, then
// 256-color, then truecolor) so a string is portable across terminals. A terminal
// that does not understand the higher tiers ignores them and applies the best one
// it supports last, so on most terminals the stack already resolves to the right
// tier on its own. collapse() is the explicit counterpart: it reduces each triple
// to a single tier, both to strip color entirely (`none`) and to emit clean,
// single-tier output for a known depth (or for terminals that render rather than
// ignore the escapes they do not understand). detect_depth() reads the best depth
// from the environment.

#pragma once

#include <cstddef>          // for std::size_t
#include <cstdlib>          // for std::getenv
#include <string>           // for std::string
#include <string_view>      // for std::string_view

namespace term_color {

enum class depth {
	none,       // strip all color
	ansi16,     // 16-color  (\033[..;30-37/90-97m)
	ansi256,    // 256-color (\033[..;38;5;Nm)
	truecolor,  // 24-bit    (\033[..;38;2;R;G;Bm)
};

// Best color depth for the current terminal, from the environment:
//   NO_COLOR set & non-empty  -> none   (honoured only when respect_no_color)
//   COLORTERM truecolor/24bit -> truecolor
//   TERM *256color*           -> ansi256
//   otherwise                 -> ansi16
// Per the NO_COLOR convention (https://no-color.org), the variable disables color
// when present and NOT an empty string; its actual value does not matter. Pass
// respect_no_color=false to detect the terminal's depth while a caller handles the
// NO_COLOR gating itself (e.g. so an explicit "force color" flag can override it).
inline depth detect_depth(bool respect_no_color = true) noexcept {
	if (respect_no_color) {
		const char* nc = std::getenv("NO_COLOR");
		if (nc != nullptr && nc[0] != '\0') {
			return depth::none;
		}
	}
	if (const char* ct = std::getenv("COLORTERM")) {
		std::string_view s(ct);
		if (s.find("truecolor") != std::string_view::npos || s.find("24bit") != std::string_view::npos) {
			return depth::truecolor;
		}
	}
	if (const char* t = std::getenv("TERM")) {
		if (std::string_view(t).find("256color") != std::string_view::npos) {
			return depth::ansi256;
		}
	}
	return depth::ansi16;
}

// Reduce every run of three stacked term-color SGR escapes to just the tier `d`
// (ansi16 = 1st, ansi256 = 2nd, truecolor = 3rd -- matching ansi_color::ansi()'s
// worst-tier-first stacking order), or remove all color for `none`. Runs whose
// length is not a multiple of three are left untouched (they are not term-color
// triples). Non-SGR text passes through verbatim.
inline std::string collapse(std::string_view s, depth d) {
	std::string out;
	out.reserve(s.size());
	const std::size_t pick = d == depth::truecolor ? 2 : d == depth::ansi256 ? 1 : 0;
	std::size_t i = 0;
	while (i < s.size()) {
		if (s[i] == '\033' && i + 1 < s.size() && s[i + 1] == '[') {
			// Collect a run of consecutive SGR escapes (\033[...m).
			std::size_t run_begin = i;
			std::size_t count = 0;
			std::size_t j = i;
			while (j < s.size() && s[j] == '\033' && j + 1 < s.size() && s[j + 1] == '[') {
				std::size_t k = j + 2;
				while (k < s.size() && s[k] != 'm') {
					++k;
				}
				if (k < s.size()) {
					++k;  // include the terminating 'm'
				}
				++count;
				j = k;
			}
			if (d == depth::none) {
				// drop the whole run
			} else if (count % 3 == 0) {
				// keep the `pick`-th escape of each triple
				std::size_t p = run_begin;
				for (std::size_t t = 0; t < count; ++t) {
					std::size_t kk = p + 2;
					while (kk < s.size() && s[kk] != 'm') {
						++kk;
					}
					if (kk < s.size()) {
						++kk;
					}
					if (t % 3 == pick) {
						out.append(s.substr(p, kk - p));
					}
					p = kk;
				}
			} else {
				// not a term-color triple run; leave it as-is
				out.append(s.substr(run_begin, j - run_begin));
			}
			i = j;
		} else {
			out.push_back(s[i]);
			++i;
		}
	}
	return out;
}


// --- gating + one-call resolution --------------------------------------------
//
// detect_depth() and collapse() are the primitives. These compose them into the
// whole "should I color, and at what tier?" policy a consumer (e.g. a logger sink)
// needs, so the NO_COLOR / tty / mode handling lives here in one place rather than
// being re-implemented by every consumer.

// When to colorize, mirroring the conventional `--color=<mode>` flag.
enum class mode {
	automatic,  // color only when the sink is a terminal and NO_COLOR is unset/empty
	always,     // force color, overriding the tty check and NO_COLOR
	never,      // never color
};

// What to target when colorizing.
enum class target {
	automatic,  // detect the tier from the terminal (COLORTERM / TERM)
	ansi16,
	ansi256,
	truecolor,
	stacked,    // do not collapse: emit all three stacked tiers; the terminal picks
};

// NO_COLOR disables color when present and not an empty string; its value does not
// matter (https://no-color.org). (Named no_color_set rather than no_color to avoid
// ansi_color.hh's function-like `no_color()` macro.)
inline bool no_color_set() noexcept {
	const char* nc = std::getenv("NO_COLOR");
	return nc != nullptr && nc[0] != '\0';
}

// Whether to colorize at all, from the mode, NO_COLOR, and whether the sink is a
// terminal. `automatic` is tty-gated and honors NO_COLOR; `always` forces color and
// overrides both; `never` never colors.
inline bool colorize(mode m, bool is_terminal) noexcept {
	switch (m) {
		case mode::never:  return false;
		case mode::always: return true;
		case mode::automatic:
		default:           return !no_color_set() && is_terminal;
	}
}

// Produce terminal-ready bytes for `s` under (mode, target, is_terminal): strip all
// color when gated off, pass the stack through untouched for `stacked`, otherwise
// collapse to the requested tier (or the terminal's detected tier for `automatic`).
// NO_COLOR is handled by the gate, so the depth detection here ignores it.
inline std::string apply(std::string_view s, mode m, target t, bool is_terminal) {
	if (!colorize(m, is_terminal)) {
		return collapse(s, depth::none);
	}
	if (t == target::stacked) {
		return std::string(s);
	}
	depth d = t == target::ansi16 ? depth::ansi16
		: t == target::ansi256 ? depth::ansi256
		: t == target::truecolor ? depth::truecolor
		: detect_depth(false);   // target::automatic
	return collapse(s, d);
}

}  // namespace term_color
