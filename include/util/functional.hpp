#pragma once

namespace util {

template <typename Func, typename... Args>
auto apply_reversed_args(Func&& func, Args&&... args);

}  // namespace util

// Implementation
#include "util/impl/functional.hpp"
