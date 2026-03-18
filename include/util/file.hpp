#pragma once

#include <string_view>

namespace util {

/*! \brief Check if file exists */
bool file_exists(const std::string_view& file);

}  // namespace util

// Implementation
#include "util/impl/file.hpp"
