/*
 * Copyright © 2026 Germán Méndez Bravo (Kronuz)
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
// 256-color, then truecolor) so a string is portable across terminals: a terminal
// applies each tier it understands in sequence and the last one wins, so it ends
// on the best tier it supports. On most terminals that stack already resolves on
// its own, but it is best-effort (a terminal that mangles rather than ignores an
// unknown tier renders it wrong, and the stack cannot honor NO_COLOR, a --color
// mode, or a non-terminal sink). This file is the robust path:
//
//   collapse(s, depth)                  low level: reduce each stacked triple to
//                                       one tier (or `none` to strip), no gating.
//   detect_depth()                      the best tier for this terminal, from the
//                                       environment (COLORTERM / TERM / NO_COLOR).
//   apply(s, mode, target, is_terminal) high level: the whole "color? which tier?"
//                                       policy (NO_COLOR + tty + mode), then
//                                       collapse to target. What a logger sink calls.
//   resolve(mode, target, is_terminal)  the same policy, decided once: returns a
//                                       `policy` whose apply(s) collapses a line without
//                                       reading the environment again.
//
// Most callers want apply(). Reach for collapse() when you already know the depth, and for
// resolve() when one sink writes many lines.

#pragma once

#include <cstddef>          // for std::size_t
#include <cstdint>          // for std::uint64_t
#include <cstdlib>          // for std::getenv
#include <cstring>          // for std::memchr, std::memcpy
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

namespace detail {

// True when some byte of `w` equals the byte repeated in `pattern`.
inline bool has_byte(std::uint64_t w, std::uint64_t pattern) noexcept {
	const std::uint64_t t = w ^ pattern;
	return ((t - 0x0101010101010101ull) & ~t & 0x8080808080808080ull) != 0;
}

// Copies the plain text at `p` up to the next ESC (or `end`) to `w`, advancing `w`, and returns
// where it stopped. It takes eight bytes at a time while none is ESC, and hands a stretch of
// eight words or more to memchr; the short stretches between escapes stay cheap.
inline const char* copy_plain(const char* p, const char* end, char*& w) noexcept {
	int words = 0;
	while (end - p >= 8) {
		std::uint64_t x;
		std::memcpy(&x, p, 8);
		if (has_byte(x, 0x1b1b1b1b1b1b1b1bull)) {
			break;
		}
		std::memcpy(w, p, 8);
		p += 8;
		w += 8;
		if (++words == 8) {
			const char* esc = static_cast<const char*>(std::memchr(p, '\033', static_cast<std::size_t>(end - p)));
			const char* stop = esc != nullptr ? esc : end;
			std::memcpy(w, p, static_cast<std::size_t>(stop - p));
			w += stop - p;
			return stop;
		}
	}
	while (p < end && *p != '\033') {
		*w++ = *p++;
	}
	return p;
}

// One past the 'm' that ends the escape whose parameters start at `k`, or `end` when there is none.
inline const char* escape_end(const char* k, const char* end) noexcept {
	while (end - k >= 8) {
		std::uint64_t x;
		std::memcpy(&x, k, 8);
		if (has_byte(x, 0x6d6d6d6d6d6d6d6dull)) {
			break;
		}
		k += 8;
	}
	while (k < end && *k != 'm') {
		++k;
	}
	return k < end ? k + 1 : end;
}

// Copies n bytes (an escape, up to a few dozen) without a library call.
inline void copy_small(char* to, const char* from, std::size_t n) noexcept {
	if (n < 8) {
		for (std::size_t i = 0; i < n; ++i) {
			to[i] = from[i];
		}
		return;
	}
	std::size_t i = 0;
	for (; i + 8 <= n; i += 8) {
		std::memcpy(to + i, from + i, 8);
	}
	if (i < n) {
		std::memcpy(to + n - 8, from + n - 8, 8);
	}
}

}  // namespace detail

// Reduce every run of three stacked term-color SGR escapes to just the tier `d`
// (ansi16 = 1st, ansi256 = 2nd, truecolor = 3rd -- matching ansi_color::ansi()'s
// worst-tier-first stacking order), or remove all color for `none`. Runs whose
// length is not a multiple of three are left untouched (they are not term-color
// triples). Non-SGR text passes through verbatim.
//
// Consequence of the multiple-of-three rule: a lone SGR next to a color (e.g. a
// bare `ESC[1m` bold before a color) makes a run of four and is NOT reduced, so
// the whole run passes through unresolved. Put bold *inside* the color instead --
// `brgb(r,g,b)` / `color::ansi(true)` fold it into each tier, keeping a clean triple.
inline std::string collapse(std::string_view s, depth d) {
	// The result never outgrows the input, so one allocation covers it. Each run of SGR escapes
	// is walked once, keeping the escape of the chosen tier as it goes and undoing that when the
	// run turns out not to be a multiple of three.
	std::string out(s.size(), '\0');
	char* w = out.data();
	const std::size_t pick = d == depth::truecolor ? 2 : d == depth::ansi256 ? 1 : 0;
	const bool strip = d == depth::none;
	const char* p = s.data();
	const char* const end = p + s.size();
	while (p < end) {
		p = detail::copy_plain(p, end, w);
		if (p == end) {
			break;
		}
		if (p + 1 == end || p[1] != '[') {
			*w++ = '\033';  // a lone ESC is plain text
			++p;
			continue;
		}
		// Collect a run of consecutive SGR escapes (\033[...m).
		char* const run_out = w;
		std::size_t count = 0;
		const char* j = p;
		while (j + 1 < end && j[0] == '\033' && j[1] == '[') {
			const char* k = detail::escape_end(j + 2, end);
			if (!strip && count % 3 == pick) {
				detail::copy_small(w, j, static_cast<std::size_t>(k - j));
				w += k - j;
			}
			++count;
			j = k;
		}
		if (!strip && count % 3 != 0) {
			// not a term-color triple run; leave it as-is
			w = run_out;
			std::memcpy(w, p, static_cast<std::size_t>(j - p));
			w += j - p;
		}
		p = j;
	}
	out.resize(static_cast<std::size_t>(w - out.data()));
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

// What apply() decides before it touches the text: whether to color, and at which tier.
// Deciding reads NO_COLOR, COLORTERM and TERM from the environment, so a sink that writes
// many lines resolves once and applies the policy to each (`policy::apply`).
struct policy {
	bool color = true;           // false: strip all color
	bool stacked = false;        // true: pass the stack through untouched
	depth tier = depth::ansi16;  // the tier to collapse to when coloring and not stacked

	// Terminal-ready bytes for `s`: all color stripped when gated off, the stack passed through
	// untouched for `stacked`, otherwise collapsed to `tier`.
	std::string apply(std::string_view s) const {
		if (!color) {
			return collapse(s, depth::none);
		}
		if (stacked) {
			return std::string(s);
		}
		return collapse(s, tier);
	}
};

// Resolve (mode, target, is_terminal) into a policy, reading the environment now.
inline policy resolve(mode m, target t, bool is_terminal) {
	policy p;
	if (!colorize(m, is_terminal)) {
		p.color = false;
		return p;
	}
	if (t == target::stacked) {
		p.stacked = true;
		return p;
	}
	p.tier = t == target::ansi16 ? depth::ansi16
		: t == target::ansi256 ? depth::ansi256
		: t == target::truecolor ? depth::truecolor
		: detect_depth(false);   // target::automatic
	return p;
}

// Produce terminal-ready bytes for `s` under (mode, target, is_terminal): strip all
// color when gated off, pass the stack through untouched for `stacked`, otherwise
// collapse to the requested tier (or the terminal's detected tier for `automatic`).
// NO_COLOR is handled by the gate, so the depth detection here ignores it.
inline std::string apply(std::string_view s, mode m, target t, bool is_terminal) {
	return resolve(m, t, is_terminal).apply(s);
}

}  // namespace term_color
