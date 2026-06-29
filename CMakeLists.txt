cmake_minimum_required(VERSION 3.14)
project(term_color LANGUAGES CXX)

include(FetchContent)

# --- Dependencies ---
# The compile-time escapes are built as static_strings (ansi_color.hh does
# `static_string::string(ESC "[") + explode<...>::value + ...`), so term-color
# depends on the standalone static-string library. It is public: consumers see
# `static_string.hh` on their include path, which is what `#include
# "static_string.hh"` in ansi_color.hh resolves against. Pinned to the SHA
# term-color was extracted against.
FetchContent_Declare(
  static_string
  GIT_REPOSITORY https://github.com/Kronuz/static-string.git
  GIT_TAG        43a2e67a31a10031eb9e0d8bef61c8b6c14cd958
)
FetchContent_MakeAvailable(static_string)

# --- Library target ---
# term-color is header-only: three headers (ansi_color.hh, colors.h,
# color_tools.hh) and nothing to compile. So the target is a pure INTERFACE
# library that puts the source dir on the consumer's include path, requests
# cxx_std_20 (the bundled static_string std::formatter and color_tools.hh's
# std::format both assume C++20), and links static_string transitively.
add_library(term_color INTERFACE)
target_include_directories(term_color INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(term_color INTERFACE cxx_std_20)
target_link_libraries(term_color INTERFACE static_string::static_string)
# Alias so consumers can link `term_color::term_color` whether they add this via
# add_subdirectory or FetchContent.
add_library(term_color::term_color ALIAS term_color)

# Only build the test when this repo is the top-level project, so consumers
# (e.g. via FetchContent) don't build our tests.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  enable_testing()
  add_executable(term_color_test test/test.cc)
  target_link_libraries(term_color_test PRIVATE term_color)
  add_test(NAME term_color COMMAND term_color_test)

  # Prove the full stacked truecolor sequence is a compile-time literal embedded
  # in the binary (not reconstructed at runtime): grep the built executable for
  # the truecolor tier's bytes. If the constexpr stacking ever regressed to a
  # runtime build, these bytes would not appear as a contiguous literal.
  add_test(NAME term_color_binary_has_stack
    COMMAND grep -q "38;2;63;119;179" "$<TARGET_FILE:term_color_test>")
endif()
