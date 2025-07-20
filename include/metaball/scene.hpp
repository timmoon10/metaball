#pragma once

#include <memory>
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
};

class RadialSceneElement : public SceneElement {
 public:
  RadialSceneElement(const VectorType& center = {});

  ScalarType operator()(const VectorType& position) const;

 private:
  VectorType center_;
};

class PolynomialSceneElement : public SceneElement {
 public:
  PolynomialSceneElement(std::vector<VectorType> coefficients,
                         const VectorType& center = {});

  ScalarType operator()(const VectorType& position) const;

 private:
  std::vector<VectorType> coefficients_;
  VectorType center_;
};

}  // namespace metaball
