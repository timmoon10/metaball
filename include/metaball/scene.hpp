#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "util/vector.hpp"

namespace metaball {

class SceneElement;

class Scene {
 public:
  static constexpr size_t ndim = 3;
  using ScalarType = double;
  using VectorType = util::Vector<ndim, ScalarType>;

  Scene();

  void add_element(std::unique_ptr<SceneElement>&& element);
  SceneElement& get_element(size_t idx);
  const SceneElement& get_element(size_t idx) const;
  void remove_element(size_t idx);

  size_t num_elements() const;

  ScalarType compute_density(const VectorType& position) const;

  ScalarType trace_ray(const VectorType& origin, const VectorType& orientation,
                       const ScalarType& max_distance, size_t num_evals) const;

 private:
  std::vector<std::unique_ptr<SceneElement>> elements_;
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
      const std::string_view& type);
};

class RadialSceneElement : public SceneElement {
 public:
  RadialSceneElement(const VectorType& center = {});

  ScalarType operator()(const VectorType& position) const;

  std::string describe() const;

 private:
  VectorType center_;
};

class PolynomialSceneElement : public SceneElement {
 public:
  PolynomialSceneElement(std::vector<VectorType> coefficients,
                         const VectorType& center = {},
                         const ScalarType& decay = 1.0);

  ScalarType operator()(const VectorType& position) const;

  std::string describe() const;

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

  ScalarType operator()(const VectorType& position) const;

  std::string describe() const;

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

  ScalarType operator()(const VectorType& position) const;

  std::string describe() const;

 private:
  std::vector<std::tuple<VectorType, ScalarType, ScalarType>> components_;
  VectorType center_;
  ScalarType decay_;
};

}  // namespace metaball
