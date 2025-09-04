#include "metaball/random.hpp"

#include <chrono>
#include <cstdint>
#include <random>
#include <thread>

namespace metaball {
namespace random {
namespace {

/*! \brief Combine two hash values
 *
 *  See
 * https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine.
 */
template <class T, class Hash = std::hash<T>>
size_t hash_combine(size_t seed, const T& val) {
  return seed ^ (Hash()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

size_t epoch_time_ns() {
  const auto now = std::chrono::high_resolution_clock::now();
  const auto epoch_time = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(epoch_time)
      .count();
}

uint32_t make_seed() {
  thread_local static std::random_device dev{};
  thread_local static size_t last_seed =
      std::hash<std::thread::id>()(std::this_thread::get_id());
  last_seed = hash_combine(last_seed, dev());
  last_seed = hash_combine(last_seed, epoch_time_ns());
  return static_cast<uint32_t>(last_seed);
}

}  // namespace

std::mt19937& generator() {
  thread_local static std::mt19937 gen{make_seed()};
  return gen;
}

}  // namespace random
}  // namespace metaball
