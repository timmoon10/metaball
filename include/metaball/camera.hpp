#pragma once

#include "metaball/image.hpp"
#include "metaball/scene.hpp"

namespace metaball {

class Camera {
 public:
  Camera();

  Image make_image(size_t height, size_t width) const;

 private:
  Scene::VectorType aperture_position_;
  Scene::VectorType image_offset_;
  Scene::VectorType image_rotation_;
};

}  // namespace metaball
