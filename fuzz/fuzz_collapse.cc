// libFuzzer target: collapse() equals the frozen original's on arbitrary bytes at every depth.
// The first byte picks the depth, the rest is the text.
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>

#include "collapse.hh"
#include "../test/collapse_old.hh"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
	if (size == 0) {
		return 0;
	}
	const unsigned pick = data[0] % 5;
	std::string_view text(reinterpret_cast<const char*>(data) + 1, size - 1);
	const auto d = pick == 4 ? static_cast<term_color::depth>(7) : static_cast<term_color::depth>(pick);
	if (term_color::collapse(text, d) != term_color_old::collapse(text, static_cast<term_color_old::depth>(d))) {
		std::abort();
	}
	return 0;
}
