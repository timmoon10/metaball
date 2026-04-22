#include <cctype>
#include <concepts>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <typeinfo>
#include <utility>
#include <vector>

#include "util/error.hpp"

// ---------------------------------------------
// to_string_like
// ---------------------------------------------

namespace util {

namespace impl {
namespace string {

template <typename T>
struct always_false : std::false_type {};

template <typename T>
concept convertible_to_string = requires(T t) {
  { std::to_string(t) } -> std::convertible_to<std::string>;
};

}  // namespace string
}  // namespace impl

template <typename T>
inline std::string to_string_like(const T&) {
  // Use indirect assert condition to prevent the compiler from
  // triggering a failure when the template is not instantiated.
  static_assert(impl::string::always_false<T>::value,
                "No to_string_like overload for type T");
  return T{};
}

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

template <impl::string::convertible_to_string T>
inline std::string to_string_like(const T& val) {
  return std::to_string(val);
}

template <std::ranges::range T>
inline std::string to_string_like(const T& range) {
  std::string str;
  str += "(";
  bool first = true;
  for (const auto& val : range) {
    if (!first) {
      str += ",";
    }
    str += to_string_like(val);
    first = false;
  }
  str += ")";
  return str;
}

template <typename T1, typename T2>
inline std::string to_string_like(const std::pair<T1, T2>& pair) {
  std::string str;
  str += "(";
  str += to_string_like(pair.first);
  str += ",";
  str += to_string_like(pair.second);
  str += ")";
  return str;
}

template <typename... Ts>
inline std::string to_string_like(const std::tuple<Ts...>& tuple) {
  std::string str;
  auto entries_to_string = [&]<size_t... Is>(std::index_sequence<Is...>) {
    (..., ((str += Is > 0 ? "," : "") += to_string_like(std::get<Is>(tuple))));
  };
  str += "(";
  entries_to_string(std::index_sequence_for<Ts...>());
  str += ")";
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
concept stringstream_insertable =
    requires(std::istringstream iss, T t) { iss >> t; };

}  // namespace string
}  // namespace impl

// String types
template <typename T>
  requires(std::convertible_to<std::string_view, T>)
inline T from_string(const std::string_view& str) {
  return str;
}
template <>
inline std::string from_string<std::string>(const std::string_view& str) {
  // string_view requires explicit conversion to string
  return std::string(str);
}

// bool
template <>
inline bool from_string<bool>(const std::string_view& str) {
  // Check for string values
  auto temp_str = std::string(strip(str));
  for (size_t i = 0; i < temp_str.size(); ++i) {
    temp_str[i] = std::tolower(static_cast<unsigned char>(temp_str[i]));
  }
  if (temp_str == "true" || temp_str == "yes" || temp_str == "on" ||
      temp_str == "1") {
    return true;
  } else if (temp_str == "false" || temp_str == "no" || temp_str == "off" ||
             temp_str == "0") {
    return false;
  }

  // Check for numerical values
  return static_cast<bool>(from_string<int>(temp_str));
}

// Numeric types
template <typename T>
  requires(impl::string::stringstream_insertable<T> &&
           !std::convertible_to<std::string_view, T> && !std::same_as<T, bool>)
inline T from_string(const std::string& str) {
  T value;
  std::istringstream iss(str);
  iss >> value;
  UTIL_CHECK(iss && iss.eof(),
             "Invalid conversion from string (type=", typeid(T).name(),
             ", string=\"", str, "\")");
  return value;
}
template <typename T>
  requires(impl::string::stringstream_insertable<T> &&
           !std::convertible_to<std::string_view, T> && !std::same_as<T, bool>)
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
  for (auto it = str.begin();
       it != str.end() && std::isspace(static_cast<unsigned char>(*it));
       ++it, ++first) {
  }
  for (auto it = str.rbegin(); it != str.rend() && last > first &&
                               std::isspace(static_cast<unsigned char>(*it));
       ++it, --last) {
  }
  return str.substr(first, last - first);
}

inline std::string_view lstrip(const std::string_view& str) {
  size_t first{0};
  for (auto it = str.begin();
       it != str.end() && std::isspace(static_cast<unsigned char>(*it));
       ++it, ++first) {
  }
  return str.substr(first, str.size() - first);
}

inline std::string_view rstrip(const std::string_view& str) {
  size_t last{str.size()};
  for (auto it = str.rbegin();
       it != str.rend() && std::isspace(static_cast<unsigned char>(*it));
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
