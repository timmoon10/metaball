#pragma once

#include <stdexcept>

#define UTIL_ERROR(...)                                              \
  do {                                                               \
    throw ::std::runtime_error(::util::concat_strings(               \
        "Error in function ", __func__, " (" __FILE__ ":", __LINE__, \
        ")" __VA_OPT__(": ", ) __VA_ARGS__));                        \
  } while (false)

#define UTIL_CHECK(expr, ...)                                      \
  do {                                                             \
    if (!(expr)) {                                                 \
      UTIL_ERROR("Assertion failed (" #expr ")" __VA_OPT__(". ", ) \
                     __VA_ARGS__);                                 \
    }                                                              \
  } while (false)

#include "util/string.hpp"
