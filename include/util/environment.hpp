#pragma once

#include <concepts>
#include <cstdlib>
#include <string>
#include <string_view>

#include "util/string.hpp"

namespace util {

/*! \brief Get environment variable and convert to type */
template <typename T = std::string>
const T getenv(const std::string& variable, const T& default_value);

/*! \brief Get environment variable and convert to type
 *
 * If the environment variable is unset or empty, a falsy value is
 * returned.
 */
template <typename T = std::string>
const T getenv(const std::string& variable);

}  // namespace util

// ---------------------------------------------
// Implementation
// ---------------------------------------------

namespace util {

template <typename T>
inline const T getenv(const std::string& variable, const T& default_value) {
  const char* env = std::getenv(variable.c_str());
  if (env == nullptr || env[0] == '\0') {
    return default_value;
  }
  return from_string<T>(env);
}

template <typename T>
inline const T getenv(const std::string& variable) {
  if constexpr (std::convertible_to<T, std::string_view>) {
    return getenv<T>(variable, "");
  } else {
    return getenv<T>(variable, 0);
  }
}

}  // namespace util
