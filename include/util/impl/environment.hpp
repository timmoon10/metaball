#include <concepts>
#include <cstdlib>
#include <string>
#include <string_view>

#include "util/string.hpp"

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
