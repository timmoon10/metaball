#pragma once

#include <random>

namespace metaball {
namespace random {

/*! Thread-local RNG */
std::mt19937& generator();

/*! Generate uniform random scalar or vector */
template <typename T>
T rand();

/*! Generate normal random scalar or vector */
template <typename T>
T randn();

}  // namespace random
}  // namespace metaball

// Implementation
#include "metaball/impl/random.hpp"
