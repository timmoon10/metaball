#pragma once

#include <string>

namespace util {

/*! Check if file can be read */
bool file_exists(const std::string& file);

}  // namespace util

#include "util/impl/file.hpp"
