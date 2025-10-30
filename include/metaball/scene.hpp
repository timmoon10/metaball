#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "metaball/integrator.hpp"
#include "util/vector.hpp"

namespace metaball {

class SceneElement;

class Scene {
 public:
  static constexpr size_t ndim = 3;
  using ScalarType = Integrator::ScalarType;
  using VectorType = util::Vector<ndim, ScalarType>;

  Scene();

  ScalarType density_threshold() const;
  ScalarType density_threshold_width() const;

  void set_density_threshold(const ScalarType& threshold);
  void set_density_threshold_width(const ScalarType& width);

  void add_element(std::unique_ptr<SceneElement>&& element);
  SceneElement& get_element(size_t idx);
  const SceneElement& get_element(size_t idx) const;
  void remove_element(size_t idx);

  size_t num_elements() const;

  void set_integrator(std::unique_ptr<Integrator>&& integrator);
  Integrator& get_integrator();
  const Integrator& get_integrator() const;

  ScalarType compute_density(const VectorType& position) const;

  ScalarType trace_ray(const VectorType& origin, const VectorType& orientation,
                       const ScalarType& max_distance) const;

 private:
  ScalarType apply_density_threshold(const ScalarType& score) const;

  std::vector<std::unique_ptr<SceneElement>> elements_;
  ScalarType density_threshold_ = 0.25;
  ScalarType density_threshold_width_ = 0.;
  std::unique_ptr<Integrator> integrator_ =
      Integrator::make_integrator("trapezoid");
};

class SceneElement {
 public:
  static constexpr size_t ndim = Scene::ndim;
  using ScalarType = Scene::ScalarType;
  using VectorType = Scene::VectorType;

  virtual ~SceneElement() = default;

  virtual ScalarType operator()(const VectorType& position) const = 0;

  virtual std::string describe() const = 0;

  static std::unique_ptr<SceneElement> make_element(
      const std::string_view& config);
};

class RadialSceneElement : public SceneElement {
 public:
  RadialSceneElement(const VectorType& center = {});

  ScalarType operator()(const VectorType& position) const override;

  std::string describe() const override;

 private:
  VectorType center_;
};

class PolynomialSceneElement : public SceneElement {
 public:
  PolynomialSceneElement(std::vector<VectorType> coefficients,
                         const VectorType& center = {},
                         const ScalarType& decay = 1.0);

  ScalarType operator()(const VectorType& position) const override;

  std::string describe() const override;

 private:
  std::vector<VectorType> coefficients_;
  VectorType center_;
  ScalarType decay_;
};

class SinusoidSceneElement : public SceneElement {
 public:
  SinusoidSceneElement(const VectorType& wave_vector,
                       const ScalarType& phase = 0.,
                       const ScalarType& amplitude = 1.,
                       const VectorType& center = {},
                       const ScalarType& decay = 1.);

  ScalarType operator()(const VectorType& position) const override;

  std::string describe() const override;

 private:
  VectorType wave_vector_;
  ScalarType phase_;
  ScalarType amplitude_;
  VectorType center_;
  ScalarType decay_;
};

class MultiSinusoidSceneElement : public SceneElement {
 public:
  MultiSinusoidSceneElement(
      std::vector<std::tuple<VectorType, ScalarType, ScalarType>> components,
      const VectorType& center = {}, const ScalarType& decay = 1.);

  ScalarType operator()(const VectorType& position) const override;

  std::string describe() const override;

 private:
  std::vector<std::tuple<VectorType, ScalarType, ScalarType>> components_;
  VectorType center_;
  ScalarType decay_;
};

}  // namespace metaball
