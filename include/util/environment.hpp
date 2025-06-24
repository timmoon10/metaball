#pragma once

#include <string>

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

#include "util/impl/environment.hpp"
