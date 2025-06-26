#pragma once

#include "util/vector.hpp"

namespace metaball {

class Scene {
 public:
  static constexpr size_t ndim = 3;
  using ScalarType = double;
  using VectorType = util::Vector<ndim, ScalarType>;

  ScalarType trace_ray(const VectorType& origin,
                       const VectorType& orientation,
                       const ScalarType& max_distance,
                       size_t num_evals) const;
};

}  // namespace metaball
