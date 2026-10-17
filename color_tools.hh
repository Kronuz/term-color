/*
 * Copyright (c) 2015-2019 Dubalu LLC
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

#pragma once

#include <cmath>             // for std::fmod
#include <cstdint>           // for uint8_t
#include <format>            // for std::format
#include <span>              // for std::span
#include <string>            // for std::string
#include <string_view>       // for std::string_view

#include "ansi_color.hh"     // for ESC


inline void
hsv2rgb(
	// input:
	double hue,        // angle in degrees between 0 and 360
	double saturation, // a fraction between 0 and 1
	double value,      // a fraction between 0 and 1
	// output:
	double& red,       // a fraction between 0 and 1
	double& green,     // a fraction between 0 and 1
	double& blue       // a fraction between 0 and 1
) {
	if (saturation <= 0.0) {
		red = value;
		green = value;
		blue = value;
		return;
	}

	if (hue >= 360.0) {
		hue = std::fmod(hue, 360.0);
	}

	hue /= 60.0;
	auto i = static_cast<long>(hue);
	auto f = hue - i;
	auto p = value * (1.0 - saturation);
	auto q = value * (1.0 - (saturation * f));
	auto t = value * (1.0 - (saturation * (1.0 - f)));

	switch (i) {
		case 0:
			red = value;
			green = t;
			blue = p;
			break;
		case 1:
			red = q;
			green = value;
			blue = p;
			break;
		case 2:
			red = p;
			green = value;
			blue = t;
			break;
		case 3:
			red = p;
			green = q;
			blue = value;
			break;
		case 4:
			red = t;
			green = p;
			blue = value;
			break;
		case 5:
		default:
			red = value;
			green = p;
			blue = q;
			break;
	}
	return;
}


class color {
	// non-constexpr version of ansi_color

	uint8_t r;
	uint8_t g;
	uint8_t b;

	auto trueColor(bool bold) {
		auto trueColor = std::format(ESC "[{};38;2;{};{};{}m", bold ? 1 : 0, r, g, b);
		return trueColor;
	}

	auto standard256(bool bold) {
		uint8_t color = static_cast<uint8_t>((r == g && g == b) ? (
			r < 6 ? 16 :
			r > 249 ? 231 :
			231 + static_cast<int>((r * 25.0f / 255.0f) + 0.5f)
		) : (
			16 +
			(static_cast<int>(r / 255.0f * 5.0f + 0.5f) * 36) +
			(static_cast<int>(g / 255.0f * 5.0f + 0.5f) * 6) +
			(static_cast<int>(b / 255.0f * 5.0f + 0.5f))
		));
		auto standard256 = std::format(ESC "[{};38;5;{}m", bold ? 1 : 0, color);
		return standard256;
	}

	auto standard16(bool bold) {
		auto _min = r < g ? r : g;
		auto min = _min < b ? _min : b;
		auto _max = r > g ? r : g;
		auto max = _max > b ? _max : b;
		uint8_t color = static_cast<uint8_t>((r == g && g == b) ? (
			r > 192 ? 15 :
			r > 128 ? 7 :
			r > 32 ? 8 :
			0
		) : (
			(max <= 32) ? (
				0
			) : (
				(
					((static_cast<int>((b - min) * 255.0f / (max - min) + 0.5f) > 128 ? 1 : 0) << 2) |
					((static_cast<int>((g - min) * 255.0f / (max - min) + 0.5f) > 128 ? 1 : 0) << 1) |
					((static_cast<int>((r - min) * 255.0f / (max - min) + 0.5f) > 128 ? 1 : 0))
				) + (max > 192 ? 8 : 0)
			)
		));
		// Map the 0-15 palette index to a real ANSI 16-color foreground SGR:
		// indices 0-7 -> 30-37 (normal), indices 8-15 -> 90-97 (bright). A
		// genuine 16-color terminal understands these; it would ignore 38;5;n.
		uint8_t sgr = static_cast<uint8_t>(color < 8 ? (30 + color) : (90 + (color - 8)));
		auto standard16 = std::format(ESC "[{};{}m", bold ? 1 : 0, sgr);
		return standard16;
	}

public:
	color(uint8_t red, uint8_t green, uint8_t blue) :
		r(red), g(green), b(blue) {}

	auto red() {
		return r;
	}

	auto green() {
		return g;
	}

	auto blue() {
		return b;
	}

	auto ansi(bool bold = false) {
		// Worst tier first, matching the compile-time ansi_color::ansi(): a
		// terminal applies each tier it understands and ends on the best it
		// supports (truecolor last on a capable terminal).
		auto ansi = (
			standard16(bold) +
			standard256(bold) +
			trueColor(bold)
		);
		return ansi;
	}
};

// A stable color for a name (a node, a peer, a voter), the same on every machine: its
// 64-bit FNV-1a hash spread around the hue wheel by the golden ratio, at saturation 0.6
// and value 0.75. Xapiand colors its nodes this way.
inline color name_color(std::string_view name) {
	std::uint64_t h = 14695981039346656037ULL;
	for (unsigned char c : name) { h = (h ^ c) * 0x100000001b3ULL; }
	double hue = static_cast<double>(h);
	hue = hue + (hue / 0.618033988749895);
	double red, green, blue;
	hsv2rgb(hue, 0.6, 0.75, red, green, blue);
	return color(static_cast<uint8_t>(red * 255), static_cast<uint8_t>(green * 255), static_cast<uint8_t>(blue * 255));
}

// A startup wordmark: each line of `art` in a color stepping from `top` to `bottom`, then
// each of `info` right-aligned under it in `bottom`'s color. Colors are stacked escapes,
// which a logger collapses to what the terminal supports.
inline std::string gradient_banner(std::span<const std::string_view> art, const uint8_t (&top)[3], const uint8_t (&bottom)[3],
	std::span<const std::string> info = {}) {
	std::size_t width = 0;
	for (auto line : art) { width = line.size() > width ? line.size() : width; }
	std::string out = "\n";
	const std::size_t n = art.size();
	for (std::size_t i = 0; i < n; ++i) {
		uint8_t c[3];
		for (int k = 0; k < 3; ++k) {
			c[k] = static_cast<uint8_t>(n > 1 ? top[k] + (int(bottom[k]) - int(top[k])) * int(i) / int(n - 1) : top[k]);
		}
		out += color(c[0], c[1], c[2]).ansi();
		out += art[i];
		out += '\n';
	}
	const std::string dim = color(bottom[0], bottom[1], bottom[2]).ansi();
	for (const auto& line : info) { out += dim + std::format("{:>{}}\n", line, width); }
	return out + ESC "[0m";
}

