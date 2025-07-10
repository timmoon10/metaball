#pragma once

#include <string_view>

#include "metaball/image.hpp"
#include "metaball/scene.hpp"

namespace metaball {

class Camera {
 public:
  using VectorType = Scene::VectorType;
  using ScalarType = VectorType::ScalarType;

  Camera();

  Image make_image(const Scene& scene, size_t height, size_t width) const;

  VectorType aperture_position() const;
  VectorType aperture_orientation() const;
  VectorType row_orientation() const;
  VectorType column_orientation() const;
  ScalarType focal_length() const;
  ScalarType film_speed() const;

  void set_aperture_position(const VectorType& position);
  void set_aperture_orientation(const VectorType& orientation);
  void set_row_orientation(const VectorType& orientation);
  void set_column_orientation(const VectorType& orientation);
  void set_focal_length(const ScalarType& focal_length);
  void set_film_speed(const ScalarType& film_speed);

  void adjust_shot(const std::string_view& type, ScalarType amount);
  static bool is_adjust_shot_type(const std::string_view& type);

 private:
  VectorType aperture_position_;
  VectorType aperture_orientation_;
  VectorType row_orientation_;
  VectorType column_orientation_;

  ScalarType focal_length_ = 1;
  ScalarType film_speed_ = 1;

  void orthogonalize_orientation();
};

}  // namespace metaball
