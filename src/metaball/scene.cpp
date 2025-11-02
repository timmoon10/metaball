#include "metaball/scene.hpp"

#include <cmath>
#include <memory>
#include <numbers>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "metaball/integrator.hpp"
#include "metaball/random.hpp"
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

Scene::Scene() {}

Scene::ScalarType Scene::density_threshold() const {
  return density_threshold_;
}

Scene::ScalarType Scene::density_threshold_width() const {
  return density_threshold_width_;
}

void Scene::set_density_threshold(const ScalarType& threshold) {
  density_threshold_ = threshold;
}

void Scene::set_density_threshold_width(const ScalarType& threshold_width) {
  density_threshold_width_ = threshold_width;
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

size_t Scene::num_elements() const { return elements_.size(); }

Scene::ScalarType Scene::compute_density(const VectorType& position) const {
  ScalarType score = 0;
  for (const auto& element : elements_) {
    score += (*element)(position);
  }
  return apply_density_threshold(score);
}

Scene::ScalarType Scene::apply_density_threshold(
    const ScalarType& score) const {
  if (density_threshold_width_ == 0) {
    return score >= density_threshold_ ? 1. : 0.;
  }
  return util::sigmoid((score - density_threshold_) / density_threshold_width_);
}

Scene::ScalarType Scene::trace_ray(const VectorType& origin,
                                   const VectorType& orientation,
                                   const Integrator& integrator) const {
  // Normalize ray orientation
  UTIL_CHECK(orientation.norm2() > 0, "Invalid orientation (",
             static_cast<VectorType::ContainerType>(orientation), ")");
  const auto orientation_unit = orientation.unit();

  // Scale integration interval to unit interval
  const ScalarType max_distance = 8;
  auto scale = max_distance;

  // Apply decay proportional to x^2/(1+x^4)
  const ScalarType decay_dist_scale = 4;  // Distance with minimum decay
  constexpr ScalarType decay_normalization = 2 * std::numbers::sqrt2 / std::numbers::pi;
  scale *= decay_normalization / decay_dist_scale;

  // Function to integrate over unit interval
  auto integrand = [&](const ScalarType& t) -> ScalarType {
    const ScalarType dist = t * max_distance;
    const auto decay_dist = dist / decay_dist_scale;
    const auto decay_dist2 = decay_dist * decay_dist;
    const auto decay = decay_dist2 / (1 + decay_dist2 * decay_dist2);
    return decay * compute_density(origin + dist * orientation_unit);
  };

  return scale * integrator(integrand);
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
    const auto center = random::randn<VectorType>();
    return std::make_unique<RadialSceneElement>(center);
  }
  if (type == "polynomial") {
    const size_t degree =
        params.empty() ? 8 : util::from_string<size_t>(params);
    std::vector<VectorType> coeffs;
    for (size_t i = 0; i < degree; ++i) {
      coeffs.emplace_back(random::randn<VectorType>());
    }
    const auto center = random::randn<VectorType>();
    return std::make_unique<PolynomialSceneElement>(coeffs, center);
  }
  if (type == "sinusoid") {
    const auto wave_vector = random::randn<VectorType>();
    const auto phase = random::rand<ScalarType>();
    return std::make_unique<SinusoidSceneElement>(wave_vector, phase, 1.);
  }
  if (type == "multi sinusoid") {
    const size_t num_sinusoids =
        params.empty() ? 8 : util::from_string<size_t>(params);
    std::vector<std::tuple<VectorType, ScalarType, ScalarType>> components;
    for (size_t i = 0; i < num_sinusoids; ++i) {
      const auto wave_vector = random::randn<VectorType>();
      const auto phase = random::rand<ScalarType>();
      const auto amplitude =
          std::abs(random::randn<ScalarType>()) / num_sinusoids;
      components.emplace_back(wave_vector, phase, amplitude);
    }
    return std::make_unique<MultiSinusoidSceneElement>(components);
  }
  if (type == "power law") {
    const size_t num_sinusoids =
        params.empty() ? 8 : util::from_string<size_t>(params);
    std::vector<std::tuple<VectorType, ScalarType, ScalarType>> components;
    for (size_t i = 0; i < num_sinusoids; ++i) {
      // Sample random frequency
      const auto frequency = 2 * random::rand<ScalarType>() + 0.25;

      // Amplitude follows power law w.r.t. frequency
      const auto amplitude = std::pow(frequency, -2) / num_sinusoids;

      // Sample orientation with bias orthogonal to y-axis
      VectorType orientation;
      do {
        orientation = random::randn<VectorType>().unit();
      } while (std::pow(1 - std::abs(orientation[1]), 4) >
               random::rand<ScalarType>());

      // Construct wave vector and phase
      auto wave_vector = orientation * frequency;
      const auto phase = random::rand<ScalarType>();
      components.emplace_back(wave_vector, phase, amplitude);
    }
    return std::make_unique<MultiSinusoidSceneElement>(components);
  }
  if (type == "moire") {
    const size_t num_sinusoids =
        params.empty() ? 2 : util::from_string<size_t>(params);
    std::vector<std::tuple<VectorType, ScalarType, ScalarType>> components;
    const auto wave_vector = 32 * random::randn<VectorType>().unit();
    components.emplace_back(wave_vector, 0., 1.);
    for (size_t i = 1; i < num_sinusoids; ++i) {
      const auto shift = random::randn<VectorType>() / 2;
      components.emplace_back(wave_vector + shift, 0., 1.);
    }
    return std::make_unique<MultiSinusoidSceneElement>(components);
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
    std::vector<VectorType> coefficients, const VectorType& center)
    : coefficients_{std::move(coefficients)}, center_{center} {}

PolynomialSceneElement::ScalarType PolynomialSceneElement::operator()(
    const VectorType& position) const {
  const auto offset = position - center_;
  ScalarType result = 1;
  for (const auto& coeffs : coefficients_) {
    result *= util::dot(coeffs, offset);
  }
  return result;
}

std::string PolynomialSceneElement::describe() const {
  return util::concat_strings("PolynomialSceneElement (coefficients=",
                              coefficients_, ", center=", center_, ")");
}

SinusoidSceneElement::SinusoidSceneElement(const VectorType& wave_vector,
                                           const ScalarType& phase,
                                           const ScalarType& amplitude)
    : wave_vector_{wave_vector}, phase_{phase}, amplitude_{amplitude} {}

SinusoidSceneElement::ScalarType SinusoidSceneElement::operator()(
    const VectorType& position) const {
  constexpr ScalarType two_pi = 2 * std::numbers::pi;
  ScalarType result = amplitude_;
  result *= std::sin(two_pi * (util::dot(position, wave_vector_) + phase_));
  return result;
}

std::string SinusoidSceneElement::describe() const {
  return util::concat_strings(
      "SinusoidSceneElement (wave_vector=", wave_vector_, ", phase=", phase_,
      ", amplitude=", amplitude_, ")");
}

MultiSinusoidSceneElement::MultiSinusoidSceneElement(
    std::vector<std::tuple<VectorType, ScalarType, ScalarType>> components)
    : components_{std::move(components)} {}

MultiSinusoidSceneElement::ScalarType MultiSinusoidSceneElement::operator()(
    const VectorType& position) const {
  ScalarType result = 0;
  constexpr ScalarType two_pi = 2 * std::numbers::pi;
  for (const auto& [wave_vector, phase, amplitude] : components_) {
    result += amplitude *
              std::sin(two_pi * (util::dot(position, wave_vector) + phase));
  }
  return result;
}

std::string MultiSinusoidSceneElement::describe() const {
  return util::concat_strings(
      "MultiSinusoidSceneElement (components=", components_, ")");
}

}  // namespace metaball
