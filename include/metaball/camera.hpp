#pragma once

#include "metaball/image.hpp"
#include "metaball/vector.hpp"

namespace metaball {

class Camera {
 public:
  static constexpr size_t ndim = 3;

  Camera();

  Image make_image(size_t height, size_t width) const;

 private:
  Vector<ndim> aperture_position_;
  Vector<ndim> image_offset_;
  Vector<ndim> image_rotation_;

};

}  // namespace metaball
