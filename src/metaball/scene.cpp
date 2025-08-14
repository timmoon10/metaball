#include "metaball/scene.hpp"

#include <chrono>
#include <cmath>
#include <memory>
#include <numbers>
#include <random>
#include <string>
#include <string_view>
#include <utility>

#include "util/error.hpp"
#include "util/math.hpp"
#include "util/string.hpp"

namespace util {

template <size_t N, typename T>
inline std::string to_string_like(const Vector<N, T>& val) {
  return to_string_like(static_cast<Vector<N, T>::ContainerType>(val));
}

}  // namespace util

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

Scene::Scene() {}

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

size_t Scene::num_elements() const { return elements_.size(); }

Scene::ScalarType Scene::compute_density(const VectorType& position) const {
  ScalarType result = 0;
  for (const auto& element : elements_) {
    result += (*element)(position);
  }
  result = util::sigmoid(32 * (result - 1));
  return result;
}

Scene::ScalarType Scene::trace_ray(const VectorType& origin,
                                   const VectorType& orientation,
                                   const ScalarType& max_distance,
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

std::unique_ptr<SceneElement> SceneElement::make_element(
    const std::string_view& config) {
  const auto config_parsed = util::split(config, "=", 2);
  UTIL_CHECK(config_parsed.size() == 1 || config_parsed.size() == 2,
             "error parsing config (", config, ")");
  const auto& type = util::strip(config_parsed[0]);
  const auto& params =
      config_parsed.size() > 1 ? util::strip(config_parsed[1]) : "";
  if (type == "radial") {
    const auto center = make_randn<ndim, ScalarType>();
    return std::make_unique<RadialSceneElement>(center);
  }
  if (type == "polynomial") {
    const size_t degree =
        params.empty() ? 8 : util::from_string<size_t>(params);
    std::vector<VectorType> coeffs;
    for (size_t i = 0; i < degree; ++i) {
      coeffs.emplace_back(make_randn<ndim, ScalarType>());
    }
    const auto center = make_randn<ndim, ScalarType>();
    return std::make_unique<PolynomialSceneElement>(coeffs, center);
  }
  if (type == "sinusoid") {
    const auto wave_vector = make_randn<ndim, ScalarType>();
    const auto center = make_randn<ndim, ScalarType>();
    return std::make_unique<SinusoidSceneElement>(wave_vector, center);
  }
  if (type == "multi sinusoid") {
    const size_t num_sinusoids =
        params.empty() ? 8 : util::from_string<size_t>(params);
    std::vector<VectorType> wave_vectors;
    for (size_t i = 0; i < num_sinusoids; ++i) {
      wave_vectors.emplace_back(make_randn<ndim, ScalarType>());
    }
    const auto center = make_randn<ndim, ScalarType>();
    return std::make_unique<MultiSinusoidSceneElement>(wave_vectors, center);
  }
  UTIL_ERROR("Unrecognized scene element (", type, ")");
}

RadialSceneElement::RadialSceneElement(const VectorType& center)
    : center_{center} {}

RadialSceneElement::ScalarType RadialSceneElement::operator()(
    const VectorType& position) const {
  return 1 / (1 + (position - center_).norm2());
}

std::string RadialSceneElement::describe() const {
  return util::concat_strings("RadialSceneElement (center=", center_, ")");
}

PolynomialSceneElement::PolynomialSceneElement(
    std::vector<VectorType> coefficients, const VectorType& center,
    const ScalarType& decay)
    : coefficients_{std::move(coefficients)}, center_{center}, decay_{decay} {}

PolynomialSceneElement::ScalarType PolynomialSceneElement::operator()(
    const VectorType& position) const {
  const auto offset = position - center_;
  const auto log_envelope = -decay_ * offset.norm2();
  if (log_envelope < -16) {
    return 0;
  }
  ScalarType result = std::exp(log_envelope);
  for (const auto& coeffs : coefficients_) {
    result *= util::dot(coeffs, offset);
  }
  return result;
}

std::string PolynomialSceneElement::describe() const {
  return util::concat_strings(
      "PolynomialSceneElement (coefficients=", coefficients_,
      ", center=", center_, ", decay=", decay_, ")");
}

SinusoidSceneElement::SinusoidSceneElement(const VectorType& wave_vector,
                                           const VectorType& center,
                                           const ScalarType& amplitude,
                                           const ScalarType& decay)
    : wave_vector_{wave_vector},
      center_{center},
      amplitude_{amplitude},
      decay_{decay} {}

SinusoidSceneElement::ScalarType SinusoidSceneElement::operator()(
    const VectorType& position) const {
  const auto offset = position - center_;
  const auto log_envelope = -decay_ * offset.norm2();
  if (log_envelope < -8) {
    return 0;
  }
  ScalarType result = std::exp(log_envelope);
  constexpr ScalarType two_pi = 2 * std::numbers::pi;
  result *= amplitude_ * std::sin(two_pi * util::dot(offset, wave_vector_));
  return result;
}

std::string SinusoidSceneElement::describe() const {
  return util::concat_strings(
      "SinusoidSceneElement (wave_vector=", wave_vector_, ", center=", center_,
      ", amplitude=", amplitude_, ", decay=", decay_, ")");
}

MultiSinusoidSceneElement::MultiSinusoidSceneElement(
    std::vector<VectorType> wave_vectors, const VectorType& center,
    const ScalarType& amplitude, const ScalarType& decay)
    : wave_vectors_{std::move(wave_vectors)},
      center_{center},
      amplitude_{amplitude},
      decay_{decay} {}

MultiSinusoidSceneElement::ScalarType MultiSinusoidSceneElement::operator()(
    const VectorType& position) const {
  const auto offset = position - center_;
  const auto log_envelope = -decay_ * offset.norm2();
  if (log_envelope < -8) {
    return 0;
  }
  ScalarType result = 0;
  constexpr ScalarType two_pi = 2 * std::numbers::pi;
  for (const auto& wave_vector : wave_vectors_) {
    result += std::sin(two_pi * util::dot(offset, wave_vector));
  }
  result *= amplitude_ * std::exp(log_envelope);
  return result;
}

std::string MultiSinusoidSceneElement::describe() const {
  return util::concat_strings(
      "MultiSinusoidSceneElement (wave_vectors=", wave_vectors_,
      ", center=", center_, ", amplitude=", amplitude_, ", decay=", decay_,
      ")");
}

}  // namespace metaball
