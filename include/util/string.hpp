#pragma once

#include <string>
#include <string_view>

namespace util {

/*! \brief Convert to a C-style or C++-style string */
template <typename T>
std::string to_string_like(const T& val);

/*! \brief Convert arguments to strings and concatenate */
template <typename... Ts>
std::string concat_strings(const Ts&... args);

/*! \brief Remove leading and trailing whitespace */
std::string_view strip(const std::string_view& str);
/*! \brief Remove leading whitespace */
std::string_view lstrip(const std::string_view& str);
/*! \brief Remove trailing whitespace */
std::string_view rstrip(const std::string_view& str);

/*! \brief Convert from a string */
template <typename T>
T from_string(const std::string& str);

}  // namespace util

#include "util/impl/string.hpp"
