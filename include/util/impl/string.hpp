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
template <typename... Ts>
std::string to_string_like(const std::tuple<Ts...>& tuple);

template <std::ranges::range T>
inline std::string to_string_like(const T& range) {
  std::string str;
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
  str += "[";
  str += to_string_like(pair.first);
  str += ",";
  str += to_string_like(pair.second);
  str += "]";
  return str;
}

template <typename... Ts>
inline std::string to_string_like(const std::tuple<Ts...>& tuple) {
  std::string str;
  auto entries_to_string = [&]<size_t... Is>(std::index_sequence<Is...>) {
    (..., ((str += Is > 0 ? "," : "") += to_string_like(std::get<Is>(tuple))));
  };
  str += "[";
  entries_to_string(std::index_sequence_for<Ts...>());
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
  (..., (str += to_string_like(args)));
  return str;
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

// string
template <>
inline std::string from_string<std::string>(const std::string_view& str) {
  return std::string(str);
}

// string_view
template <>
inline std::string_view from_string<std::string_view>(
    const std::string_view& str) {
  return str;
}

// bool
template <>
inline bool from_string<bool>(const std::string_view& str) {
  // Check for string values
  auto temp_str = std::string(strip(str));
  for (size_t i = 0; i < temp_str.size(); ++i) {
    temp_str[i] = std::tolower(temp_str[i]);
  }
  if (temp_str == "true" || temp_str == "yes" || temp_str == "on") {
    return true;
  } else if (temp_str == "false" || temp_str == "no" || temp_str == "off") {
    return false;
  }

  // Check for numerical values
  return static_cast<bool>(from_string<int>(temp_str));
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
template <typename T>
  requires(impl::string::HasInsertionOperator<T> &&
           !impl::string::IsStringLike<T> && !std::same_as<T, bool>)
inline T from_string(const std::string_view& str) {
  return from_string<T>(std::string(str));
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
// split
// ---------------------------------------------

namespace util {

inline std::vector<std::string_view> split(const std::string_view& str,
                                           const std::string_view& separator,
                                           size_t maxsplit) {
  UTIL_CHECK(separator.size() > 0, "separator string is empty");

  // Split string until reaching end of string or split limit
  std::vector<std::string_view> result;
  size_t pos = 0;
  while (pos != str.npos && (maxsplit == 0 || result.size() < maxsplit - 1)) {
    const size_t sep_pos = str.find(separator, pos);
    const size_t count = sep_pos == str.npos ? str.npos : sep_pos - pos;
    result.emplace_back(str.substr(pos, count));
    pos = sep_pos == str.npos ? str.npos : sep_pos + separator.size();
  }
  if (pos != str.npos) {
    result.emplace_back(str.substr(pos, str.npos));
  }

  return result;
}

}  // namespace util
