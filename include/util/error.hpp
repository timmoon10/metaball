#pragma once

#include <iostream>
#include <stdexcept>

// Throw an exception
#define UTIL_ERROR(...)                                              \
  do {                                                               \
    throw ::std::runtime_error(::util::concat_strings(               \
        "Error in function ", __func__, " (" __FILE__ ":", __LINE__, \
        ")" __VA_OPT__(": ", ) __VA_ARGS__));                        \
  } while (false)

// Throw an exception if an expression evaluates to false
#define UTIL_CHECK(expr, ...)                                       \
  do {                                                              \
    if (!(expr)) {                                                  \
      UTIL_ERROR("Assertion failed (" #expr ")" __VA_OPT__(" - ", ) \
                     __VA_ARGS__);                                  \
    }                                                               \
  } while (false)

// Print a message to stderr
#define UTIL_WARN(...)                                                      \
  do {                                                                      \
    ::std::cerr << ::util::concat_strings(                                  \
                       "Warning in function ", __func__, " (" __FILE__ ":", \
                       __LINE__, ")" __VA_OPT__(": ", ) __VA_ARGS__, "\n")  \
                << ::std::flush;                                            \
  } while (false)

// Note: this header has a circular dependency with util/string.hpp.
// error.hpp uses concat_strings (from string.hpp) to build messages;
// string.hpp uses UTIL_CHECK (from error.hpp) for runtime assertions.
// This is intentional and safe — macros are resolved at call sites,
// not at definition time, so either header may be included first.
#include "util/string.hpp"
