#pragma once

#include "metaball/image.hpp"
#include "util/vector.hpp"

namespace metaball {

class Camera {
 public:
  static constexpr size_t ndim = 3;
  using ScalarType = Image::DataType;

  Camera();

  Image make_image(size_t height, size_t width) const;

 private:
  util::Vector<ndim, ScalarType> aperture_position_;
  util::Vector<ndim, ScalarType> image_offset_;
  util::Vector<ndim, ScalarType> image_rotation_;
};

}  // namespace metaball
