#include "metaball/scene.hpp"

#include <algorithm>
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

  // Decay factor
  // Note: Define s = x/x0 and apply decay of C*s*exp(-s). The decay
  // peaks at x=x0, i.e. s=1. With C=1, the integral of the decay over
  // [0,inf) is 1.
  const ScalarType x0 = 1;
  auto decay = [](const ScalarType& s) -> ScalarType {
    return s * std::exp(-s);
  };

  // Integral reparametrization factor
  // Note: In order to convert integral over [0,inf) to integral over
  // [0,1], reparametrize s=t/(1-t). This has x=x0 at
  // t=0.5. This reparametrization requires applying x'(t).
  auto ds = [](const ScalarType& t) -> ScalarType {
    const auto tm1 = t - 1;
    return 1 / (tm1 * tm1);
  };

  // Function to integrate
  auto integrand = [&](const ScalarType& t_) -> ScalarType {
    constexpr ScalarType max =
        1 - std::numeric_limits<ScalarType>::epsilon() / 2;
    const auto t = std::min(t_, max);
    const auto s = t / (1 - t);
    const auto x = s * x0;
    return decay(s) * ds(t) * compute_density(origin + x * orientation_unit);
  };

  return x0 * integrator(integrand);
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
    return std::make_unique<RadialSceneElement>(center, 2.);
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
  if (type == "radial sinusoid") {
    const auto center = random::randn<VectorType>();
    const auto frequency = 2 * random::rand<ScalarType>();
    const auto phase = random::rand<ScalarType>();
    return std::make_unique<RadialSinusoidSceneElement>(center, frequency,
                                                        phase, 1.);
  }
  if (type == "polar sinusoid") {
    const auto center = random::randn<VectorType>();
    const auto orientation = random::randn<VectorType>();
    const auto radial_frequency = 8 * random::rand<ScalarType>();
    const auto phase = random::rand<ScalarType>();
    return std::make_unique<PolarSinusoidSceneElement>(
        center, orientation, 0., radial_frequency, phase, 1.);
  }
  if (type == "minus exp") {
    const ScalarType dist_scale =
        (params.empty() ? static_cast<ScalarType>(1.)
                        : util::from_string<ScalarType>(params));
    return std::make_unique<MinusExpSceneElement>(VectorType{}, dist_scale);
  }
  if (type == "power decay") {
    const auto& params_split = util::split(params, ",");
    const size_t num_components =
        params.empty() ? 8 : util::from_string<size_t>(params_split[0]);
    const ScalarType frequency_cutoff =
        params_split.size() < 2
            ? 4.0
            : util::from_string<ScalarType>(params_split[1]);
    const ScalarType decay_factor =
        params_split.size() < 3
            ? 2.0
            : util::from_string<ScalarType>(params_split[2]);
    return MultiSinusoidSceneElement::make_power_spectrum_decay(
        num_components, frequency_cutoff, decay_factor);
  }
  if (type == "moire") {
    const size_t num_sinusoids =
        params.empty() ? 2 : util::from_string<size_t>(params);
    std::vector<std::tuple<VectorType, ScalarType, ScalarType>> components;
    VectorType wave_vector;
    wave_vector[0] = 32;
    components.emplace_back(wave_vector, 0., 1.);
    for (size_t i = 1; i < num_sinusoids; ++i) {
      const auto shift = random::randn<VectorType>() / 2;
      components.emplace_back(wave_vector + shift, 0., 1.);
    }
    return std::make_unique<MultiSinusoidSceneElement>(components);
  }
  if (type == "radial moire") {
    const size_t num_sinusoids =
        params.empty() ? 2 : util::from_string<size_t>(params);
    const VectorType center;
    const ScalarType frequency = 32;
    auto result = std::make_unique<MultiSceneElement>();
    result->add_element(std::make_unique<RadialSinusoidSceneElement>(
        center, frequency, 0., 1.));
    for (size_t i = 1; i < num_sinusoids; ++i) {
      const auto shift = random::randn<VectorType>() / 2;
      result->add_element(std::make_unique<RadialSinusoidSceneElement>(
          center + shift, frequency, 0., 1.));
    }
    return result;
  }
  if (type == "polar moire") {
    const size_t num_sinusoids =
        params.empty() ? 2 : util::from_string<size_t>(params);
    const VectorType center;
    VectorType orientation;
    orientation[0] = 1;
    const ScalarType frequency = 32;
    auto result = std::make_unique<MultiSceneElement>();
    result->add_element(std::make_unique<PolarSinusoidSceneElement>(
        center, orientation, 0., frequency, 0., 1.));
    for (size_t i = 1; i < num_sinusoids; ++i) {
      const auto center_shift = random::randn<VectorType>() / 2;
      const auto orientation_shift = random::randn<VectorType>() / 2;
      result->add_element(std::make_unique<PolarSinusoidSceneElement>(
          center + center_shift, orientation + orientation_shift, 0., frequency,
          0., 1.));
    }
    return result;
  }
  UTIL_ERROR("Unrecognized scene element (", type, ")");
}

MultiSceneElement::MultiSceneElement() {}

void MultiSceneElement::add_element(std::unique_ptr<SceneElement>&& element) {
  elements_.emplace_back(std::move(element));
}

MultiSceneElement::ScalarType MultiSceneElement::operator()(
    const VectorType& position) const {
  ScalarType score = 0;
  for (const auto& element : elements_) {
    score += (*element)(position);
  }
  return score;
}

std::string MultiSceneElement::describe() const {
  std::string desc = "MultiSceneElement (";
  for (size_t i = 0; i < elements_.size(); ++i) {
    if (i > 0) {
      desc += ",";
    }
    desc += elements_[i]->describe();
  }
  desc += ")";
  return desc;
}

RadialSceneElement::RadialSceneElement(const VectorType& center,
                                       const ScalarType& decay)
    : center_{center}, decay_square_{decay * decay} {}

RadialSceneElement::ScalarType RadialSceneElement::operator()(
    const VectorType& position) const {
  return 1 / (1 + decay_square_ * (position - center_).norm2());
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
  result *= std::cos(two_pi * (util::dot(position, wave_vector_) + phase_));
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
              std::cos(two_pi * (util::dot(position, wave_vector) + phase));
  }
  return result;
}

std::string MultiSinusoidSceneElement::describe() const {
  return util::concat_strings(
      "MultiSinusoidSceneElement (components=", components_, ")");
}

std::unique_ptr<MultiSinusoidSceneElement>
MultiSinusoidSceneElement::make_power_spectrum_decay(
    size_t num_components, ScalarType frequency_cutoff,
    ScalarType decay_factor) {
  // Natural 2D images have been observed to have power spectra
  // roughly proportional to 1/f^alpha with alpha~2, up to a
  // resolution limit f_max. Since the power spectrum is the
  // magnitude-square of the Fourier transform, this implies a
  // frequency distribution of 1/f^(alpha/2). If such an image is a
  // projection of an N-d space, the projection-slice theorem implies
  // that the N-d space also has the same frequency distribution.
  //
  // We wish to emulate natural images based on this frequency
  // distribution. For laziness, we'll take some shortcuts in the
  // following math like dropping constants and ignoring imaginary
  // components. Now, let's apply an inverse Fourier transform to the
  // frequency distribution:
  //
  //   i(x) = integral(cos(2*pi*k*x+phi) / norm(k)^(alpha/2) * dk)
  //
  // k is the wavenumber (integrated over a ball with radius f_max)
  // and phi is the phase (a function wrt k). If we represent the
  // wavenumber k=f*omega, with frequency f and orientation omega, we
  // can convert the integral to spherical coordinates:
  //
  //   i(x) = integral(cos(2*pi*k*x+phi) / f^(alpha/2) * f^(N-1) * df * domega)
  //        = integral(cos(2*pi*k*x+phi) * f^(N-alpha/2-1) * df * domega)
  //
  // Substituting u = (f/f_max)^(N-alpha/2):
  //
  //   i(x) = integral(cos(2*pi*k*x+phi) * du * domega)
  //
  // Note that u is integrated over the unit interval and omega over
  // the unit sphere.
  //
  // We now have everything needed for a Monte Carlo integrator with
  // importance sampling. Sampling u and omega uniform-randomly:
  //
  //   i(x) = 1/n * sum(cos(2*pi*k*x+phi))
  //
  // where k=f_max*u^(1/(N-alpha/2))*omega.

  // Construct Fourier components
  std::vector<std::tuple<VectorType, ScalarType, ScalarType>> components;
  for (size_t i = 0; i < num_components; ++i) {
    // Sample frequency
    const auto rand = random::rand<ScalarType>();
    const auto rand_exponent = 1.0 / (VectorType::ndim - decay_factor / 2);
    const ScalarType frequency =
        frequency_cutoff * std::pow(rand, rand_exponent);

    // Sample orientation
    const VectorType orientation = random::randn<VectorType>().unit();

    // Sample phase
    const ScalarType phase = 2 * std::numbers::pi * random::rand<ScalarType>();

    // Construct Fourier component
    const ScalarType amplitude = 1.0 / num_components;
    components.emplace_back(frequency * orientation, phase, amplitude);
  }

  // Construct scene element with Fourier components
  return std::make_unique<MultiSinusoidSceneElement>(components);
};

RadialSinusoidSceneElement::RadialSinusoidSceneElement(
    const VectorType& center, const ScalarType& frequency,
    const ScalarType& phase, const ScalarType& amplitude)
    : center_{center},
      frequency_{frequency},
      phase_{phase},
      amplitude_{amplitude} {}

RadialSinusoidSceneElement::ScalarType RadialSinusoidSceneElement::operator()(
    const VectorType& position) const {
  constexpr ScalarType two_pi = 2 * std::numbers::pi;
  const auto& r = (position - center_).norm();
  return amplitude_ * std::cos(two_pi * frequency_ * r + phase_);
}

std::string RadialSinusoidSceneElement::describe() const {
  return util::concat_strings("RadialSinusoidSceneElement (center=", center_,
                              ", frequency=", frequency_, ", phase=", phase_,
                              ", amplitude=", amplitude_, ")");
}

PolarSinusoidSceneElement::PolarSinusoidSceneElement(
    const VectorType& center, const VectorType& orientation,
    const ScalarType& radial_frequency, const ScalarType& polar_frequency,
    const ScalarType& phase, const ScalarType& amplitude)
    : center_{center},
      orientation_{orientation.unit()},
      radial_frequency_{radial_frequency},
      polar_frequency_{polar_frequency},
      phase_{phase},
      amplitude_{amplitude} {}

PolarSinusoidSceneElement::ScalarType PolarSinusoidSceneElement::operator()(
    const VectorType& position) const {
  constexpr ScalarType one = 1;
  constexpr ScalarType two_pi = 2 * std::numbers::pi;
  const auto& pos = position - center_;
  const auto& r = pos.norm();
  const auto& cos_theta = util::dot(pos / r, orientation_);
  const auto& theta = std::acos(std::clamp(cos_theta, -one, one));
  return amplitude_ *
         std::cos(two_pi * (radial_frequency_ * r + polar_frequency_ * theta) +
                  phase_);
}

std::string PolarSinusoidSceneElement::describe() const {
  return util::concat_strings(
      "PolarSinusoidSceneElement (center=", center_,
      ", orientation=", orientation_, ", radial frequency=", radial_frequency_,
      ", polar frequency=", polar_frequency_, ", phase=", phase_,
      ", amplitude=", amplitude_, ")");
}

MinusExpSceneElement::MinusExpSceneElement(const VectorType& center,
                                           const ScalarType& dist_scale)
    : center_{center}, dist_scale_square_{dist_scale * dist_scale} {}

MinusExpSceneElement::ScalarType MinusExpSceneElement::operator()(
    const VectorType& position) const {
  return -std::expm1(dist_scale_square_ * (position - center_).norm2());
}

std::string MinusExpSceneElement::describe() const {
  return util::concat_strings("MinusExpSceneElement (center=", center_,
                              ", dist_scale_square=", dist_scale_square_, ")");
}

}  // namespace metaball
