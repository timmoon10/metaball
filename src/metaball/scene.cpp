#include "metaball/scene.hpp"

#include <memory>
#include <utility>

#include "util/error.hpp"
#include "util/math.hpp"

namespace metaball {

Scene::Scene() {
  const VectorType source1 = {0.7, 0.7, 4};
  const VectorType source2 = {-0.7, -0.7, 4};
  add_element(std::make_unique<RadialSceneElement>(source1));
  add_element(std::make_unique<RadialSceneElement>(source2));
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

Scene::ScalarType Scene::trace_ray(const Scene::VectorType& origin,
                                   const Scene::VectorType& orientation,
                                   const Scene::ScalarType& max_distance,
                                   size_t num_evals) const {
  // Helper function for density field
  auto density_field = [this](const VectorType& position) -> ScalarType {
    ScalarType density = 0;
    for (const auto& element : elements_) {
      density += (*element)(position);
    }
    density = util::sigmoid(32 * (density - 1));
    return density;
  };

  // Trapezoid rule
  UTIL_CHECK(orientation.norm2() > 0, "Invalid orientation (",
             static_cast<VectorType::ContainerType>(orientation), ")");
  UTIL_CHECK(num_evals >= 2, "Invalid number of evaluations (", num_evals, ")");
  const ScalarType grid_size = max_distance / (num_evals - 1);
  const VectorType grid_shift = orientation * (grid_size / orientation.norm());
  ScalarType result = density_field(origin) / 2;
  for (size_t i = 1; i < num_evals - 1; ++i) {
    result += density_field(origin + i * grid_shift);
  }
  result += density_field(origin + (num_evals - 1) * grid_shift) / 2;
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

}  // namespace metaball
