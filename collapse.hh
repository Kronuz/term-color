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
// rgb()/clear_color() emit THREE stacked SGR escapes per color (truecolor, then
// 256-color, then 16-color) so a string is portable across terminals. A terminal
// that does not understand the higher tiers ignores them, but one that
// understands all three applies them in order and the last (16-color) wins -- so
// for full color you must collapse each triple down to the single tier the
// terminal actually supports. detect_depth() reads that from the environment;
// collapse() does the reduction. (This is the counterpart to emitting the stack.)

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
//   NO_COLOR set            -> none
//   COLORTERM truecolor/24bit -> truecolor
//   TERM *256color*         -> ansi256
//   otherwise               -> ansi16
inline depth detect_depth() noexcept {
	if (std::getenv("NO_COLOR") != nullptr) {
		return depth::none;
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
// (truecolor = 1st, ansi256 = 2nd, ansi16 = 3rd), or remove all color for `none`.
// Runs whose length is not a multiple of three are left untouched (they are not
// term-color triples). Non-SGR text passes through verbatim.
inline std::string collapse(std::string_view s, depth d) {
	std::string out;
	out.reserve(s.size());
	const std::size_t pick = d == depth::truecolor ? 0 : d == depth::ansi256 ? 1 : 2;
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

}  // namespace term_color
