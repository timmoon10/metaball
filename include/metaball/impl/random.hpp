#include <concepts>
#include <random>
#include <type_traits>

#include "util/vector.hpp"

namespace metaball {
namespace random {

namespace {

template <typename>
struct is_vector : std::false_type {};

template <size_t N, typename T>
struct is_vector<util::Vector<N, T>> : std::true_type {};

template <typename T>
concept IsVector = is_vector<T>::value;

}  // namespace

template <std::floating_point T>
inline T rand() {
  std::uniform_real_distribution dist{};
  return dist(generator());
}

template <IsVector T>
inline T rand() {
  std::uniform_real_distribution dist{};
  T result;
  for (size_t i = 0; i < T::ndim; ++i) {
    result[i] = dist(generator());
  }
  return result;
}

template <std::floating_point T>
inline T randn() {
  std::normal_distribution dist{};
  return dist(generator());
}

template <IsVector T>
inline T randn() {
  std::normal_distribution dist{};
  T result;
  for (size_t i = 0; i < T::ndim; ++i) {
    result[i] = dist(generator());
  }
  return result;
}

}  // namespace random
}  // namespace metaball
