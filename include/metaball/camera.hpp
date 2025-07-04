#pragma once

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
  VectorType image_offset() const;
  VectorType image_rotation() const;
  ScalarType film_speed() const;

  void set_aperture_position(const VectorType& aperture_position);
  void set_image_offset(const VectorType& image_offset);
  void set_image_rotation(const VectorType& image_rotation);
  void set_film_speed(const ScalarType& film_speed);

 private:
  VectorType aperture_position_;
  VectorType image_offset_;
  VectorType image_rotation_;

  ScalarType film_speed_ = 1;
};

}  // namespace metaball
