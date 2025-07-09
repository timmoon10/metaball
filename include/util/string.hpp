#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace util {

/*! \brief Convert to a C-style or C++-style string */
template <typename T>
std::string to_string_like(const T& val);

/*! \brief Convert arguments to strings and concatenate */
template <typename... Ts>
std::string concat_strings(const Ts&... args);

/*! \brief Convert from a string view */
template <typename T>
T from_string(const std::string_view& str);

/*! \brief Remove leading and trailing whitespace */
std::string_view strip(const std::string_view& str);
/*! \brief Remove leading whitespace */
std::string_view lstrip(const std::string_view& str);
/*! \brief Remove trailing whitespace */
std::string_view rstrip(const std::string_view& str);

/*! \brief Split string into a list */
std::vector<std::string_view> split(const std::string_view& str,
                                    const std::string_view& separator,
                                    size_t maxsplit = 0);

}  // namespace util

// Implementation
#include "util/impl/string.hpp"
