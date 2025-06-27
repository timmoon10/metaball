#pragma once

namespace util {

/*! \brief Logistic function, also known as a sigmoid function */
template <typename T>
T sigmoid(const T& x);

}  // namespace util

// Implementation
#include "util/impl/math.hpp"
