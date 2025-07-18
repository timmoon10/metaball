#include "metaball/scene.hpp"

#include <chrono>
#include <memory>
#include <random>
#include <utility>

#include "util/error.hpp"
#include "util/math.hpp"

namespace metaball {

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

uint32_t seed() {
  thread_local static std::random_device dev{};
  thread_local static size_t last_seed = dev();
  last_seed = hash_combine(last_seed, dev());
  last_seed = hash_combine(last_seed, epoch_time_ns());
  return static_cast<uint32_t>(last_seed);
}

template <size_t N, typename T>
util::Vector<N, T> make_randn() {
  // RNG state
  std::mt19937 gen{seed()};
  std::normal_distribution dist{};

  // Generate Gaussian random values
  util::Vector<N, T> result;
  for (size_t i = 0; i < N; ++i) {
    result[i] = dist(gen);
  }
  return result;
}

}  // namespace

Scene::Scene() {
  // for (size_t i = 0; i < 3; ++i) {
  //   const VectorType source = make_randn<ndim, ScalarType>();
  //   add_element(std::make_unique<RadialSceneElement>(source));
  // }
  {
    std::vector<VectorType> coeffs;
    for (size_t i = 0; i < 3; ++i) {
      coeffs.emplace_back(make_randn<ndim, ScalarType>());
    }
    add_element(std::make_unique<PolynomialSceneElement>(coeffs, 1.0));
  }
}

void Scene::add_element(std::unique_ptr<SceneElement>&& element) {
  elements_.emplace_back(std::move(element));
}

SceneElement& Scene::get_element(size_t idx) {
  return const_cast<SceneElement&>(
      const_cast<const Scene&>(*this).get_element(idx));
}

const SceneElement& Scene::get_element(size_t idx) const {
  UTIL_CHECK(idx < elements_.size(), "Attempted to access scene element ", idx,
             ", but there are only ", elements_.size());
  UTIL_CHECK(elements_[idx] != nullptr, "Scene element ", idx,
             " has not been initialized");
  return *elements_[idx];
}

void Scene::remove_element(size_t idx) {
  UTIL_CHECK(idx < elements_.size(), "Attempted to remove scene element ", idx,
             ", but there are only ", elements_.size());
  elements_.erase(elements_.begin() + idx);
}

Scene::ScalarType Scene::compute_density(
    const Scene::VectorType& position) const {
  ScalarType result = 0;
  for (const auto& element : elements_) {
    result += (*element)(position);
  }
  result = util::sigmoid(32 * (result - 1));
  return result;
}

Scene::ScalarType Scene::trace_ray(const Scene::VectorType& origin,
                                   const Scene::VectorType& orientation,
                                   const Scene::ScalarType& max_distance,
                                   size_t num_evals) const {
  // Trapezoid rule
  UTIL_CHECK(orientation.norm2() > 0, "Invalid orientation (",
             static_cast<VectorType::ContainerType>(orientation), ")");
  UTIL_CHECK(num_evals >= 2, "Invalid number of evaluations (", num_evals, ")");
  const ScalarType grid_size = max_distance / (num_evals - 1);
  const VectorType grid_shift = orientation * (grid_size / orientation.norm());
  ScalarType result = compute_density(origin) / 2;
  for (size_t i = 1; i < num_evals - 1; ++i) {
    result += compute_density(origin + i * grid_shift);
  }
  result += compute_density(origin + (num_evals - 1) * grid_shift) / 2;
  result *= grid_size;
  return result;
}

RadialSceneElement::RadialSceneElement(
    const RadialSceneElement::VectorType& center)
    : center_{center} {}

RadialSceneElement::ScalarType RadialSceneElement::operator()(
    const RadialSceneElement::VectorType& position) const {
  return 1 / (1 + (position - center_).norm2());
}

PolynomialSceneElement::PolynomialSceneElement(
    std::vector<PolynomialSceneElement::VectorType> coefficients,
    const PolynomialSceneElement::ScalarType constant)
    : coefficients_{std::move(coefficients)}, constant_{constant} {}

PolynomialSceneElement::ScalarType PolynomialSceneElement::operator()(
    const PolynomialSceneElement::VectorType& position) const {
  ScalarType result = 1;
  for (const auto& coeffs : coefficients_) {
    result *= util::dot(position, coeffs);
  }
  result += constant_;
  result *= std::exp(-position.norm2() / 2);
  result = std::abs(result);
  return result;
}

}  // namespace metaball
