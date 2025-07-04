#include <cctype>
#include <concepts>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "util/error.hpp"

// ---------------------------------------------
// to_string_like
// ---------------------------------------------

namespace util {

namespace impl {
namespace string {

template <typename T>
concept HasToString = requires(T t) {
  { std::to_string(t) } -> std::convertible_to<std::string>;
};

}  // namespace string
}  // namespace impl

inline const std::string& to_string_like(const std::string& val) noexcept {
  return val;
}

inline const std::string_view& to_string_like(
    const std::string_view& val) noexcept {
  return val;
}

inline constexpr const char* to_string_like(const char* val) noexcept {
  return val;
}

template <impl::string::HasToString T>
inline std::string to_string_like(const T& val) {
  return std::to_string(val);
}

// Forward declare cases that internally call to_string_like
template <std::ranges::range T>
std::string to_string_like(const T& range);
template <typename T1, typename T2>
std::string to_string_like(const std::pair<T1, T2>& pair);

template <std::ranges::range T>
inline std::string to_string_like(const T& range) {
  std::string str;
  str.reserve(128);  // Assume strings are <128 B
  str += "[";
  bool first = true;
  for (const auto& val : range) {
    if (!first) {
      str += ",";
    }
    str += to_string_like(val);
    first = false;
  }
  str += "]";
  return str;
}

template <typename T1, typename T2>
inline std::string to_string_like(const std::pair<T1, T2>& pair) {
  std::string str;
  str.reserve(128);  // Assume strings are <128 B
  str += "[";
  str += to_string_like(pair.first);
  str += ",";
  str += to_string_like(pair.second);
  str += "]";
  return str;
}

}  // namespace util

// ---------------------------------------------
// concat_strings
// ---------------------------------------------

namespace util {

template <typename... Ts>
inline std::string concat_strings(const Ts&... args) {
  std::string str;
  str.reserve(128);  // Assume strings are <128 B
  (..., (str += to_string_like(args)));
  return str;
}

}  // namespace util

// ---------------------------------------------
// strip/lstrip/rstrip
// ---------------------------------------------

namespace util {

inline std::string_view strip(const std::string_view& str) {
  size_t first{0}, last{str.size()};
  for (auto it = str.begin(); it != str.end() && std::isspace(*it);
       ++it, ++first) {
  }
  for (auto it = str.rbegin();
       it != str.rend() && last > first && std::isspace(*it); ++it, --last) {
  }
  return str.substr(first, last - first);
}

inline std::string_view lstrip(const std::string_view& str) {
  size_t first{0};
  for (auto it = str.begin(); it != str.end() && std::isspace(*it);
       ++it, ++first) {
  }
  return str.substr(first, str.size() - first);
}

inline std::string_view rstrip(const std::string_view& str) {
  size_t last{str.size()};
  for (auto it = str.rbegin(); it != str.rend() && std::isspace(*it);
       ++it, --last) {
  }
  return str.substr(0, last);
}

}  // namespace util

// ---------------------------------------------
// from_string
// ---------------------------------------------

namespace util {

namespace impl {
namespace string {

template <typename T>
concept HasInsertionOperator =
    requires(std::istringstream iss, T t) { iss >> t; };

template <typename T>
concept IsStringLike =
    std::convertible_to<std::string, T> || std::same_as<T, const char*>;

}  // namespace string
}  // namespace impl

// String-like types
template <typename T>
  requires(impl::string::IsStringLike<T>)
inline T from_string(const std::string& str) {
  if constexpr (std::same_as<T, const char*>) {
    return str.c_str();
  } else {
    return str;
  }
}

// bool
template <>
inline bool from_string<bool>(const std::string& str) {
  // Check for string values
  auto temp_str = std::string(strip(str));
  for (size_t i = 0; i < temp_str.size(); ++i) {
    temp_str[i] = std::tolower(temp_str[i]);
  }
  if (str == "true") {
    return true;
  } else if (str == "false") {
    return false;
  } else if (str == "yes") {
    return true;
  } else if (str == "no") {
    return false;
  }

  // Check for number values
  return static_cast<bool>(from_string<int>(str));
}

// Numeric types
template <typename T>
  requires(impl::string::HasInsertionOperator<T> &&
           !impl::string::IsStringLike<T> && !std::same_as<T, bool>)
inline T from_string(const std::string& str) {
  T value;
  std::istringstream iss(str.c_str());
  iss >> value;
  UTIL_CHECK(iss && iss.eof(),
             "Invalid conversion from string (type=", typeid(T).name(),
             ", string=\"", str, "\")");
  return value;
}

}  // namespace util
