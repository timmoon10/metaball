#include "metaball/scene.hpp"

#include "util/error.hpp"
#include "util/math.hpp"

namespace metaball {

Scene::ScalarType Scene::trace_ray(const Scene::VectorType& origin,
                                   const Scene::VectorType& orientation,
                                   const Scene::ScalarType& max_distance,
                                   size_t num_evals) const {
  // TODO Adjustable density field
  auto density_field = [](const VectorType& position) -> ScalarType {
    constexpr VectorType source1 = {0.7, 0.7, 4};
    constexpr VectorType source2 = {-0.7, -0.7, 4};
    auto density = (1 / (1 + (position - source1).norm2()) +
                    1 / (1 + (position - source2).norm2()));
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

}  // namespace metaball
