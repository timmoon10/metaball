#pragma once

#include <string>

namespace util {

/*! \brief Check if file can be read */
bool file_exists(const std::string& file);

}  // namespace util

// Implementation
#include "util/impl/file.hpp"
