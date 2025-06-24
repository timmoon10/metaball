#include "metaball/camera.hpp"

#include "metaball/image.hpp"
#include "metaball/vector.hpp"
#include "util/error.hpp"

namespace metaball {

Camera::Camera() {
  image_offset_[ndim - 1] = -1;
  image_rotation_[0] = 1;
}

Image Camera::make_image(size_t height, size_t width) const {
  // Check arguments
  UTIL_CHECK(height > 0, "invalid height ", height);
  UTIL_CHECK(width > 0, "invalid width ", width);

  // Spacing between pixels
  // Note: The image is a unit square that is directly facing the
  // aperture. The rotation vector is aligned to the pixel rows.
  auto image_size = std::max(height, width);
  auto shift_x = image_rotation_;
  shift_x -= dot(shift_x, image_offset_) * image_offset_;
  if (shift_x.norm2() > 0) {
    shift_x /= shift_x.norm() * image_size;
  }
  static_assert(ndim == 3);
  auto shift_y = cross(image_offset_, shift_x);
  if (shift_y.norm2() > 0) {
    shift_y /= shift_y.norm() * image_size;
  }

  // Position of top-left pixel
  auto corner_pixel = aperture_position_ + image_offset_;
  corner_pixel += (-static_cast<Scalar>(width - 1) / 2) * shift_x;
  corner_pixel += (-static_cast<Scalar>(height - 1) / 2) * shift_y;

  // Calculate ray for each pixel
  Image retval(height, width);
  for (size_t i = 0; i < height; ++i) {
    for (size_t j = 0; j < width; ++j) {
      auto pixel = corner_pixel + i * shift_y + j * shift_x;
      auto ray = (aperture_position_ - pixel).unit();

      /// TODO Non-trivial image
      auto ray_radial = ray - dot(ray, image_offset_) * image_offset_;
      auto val = 1.f / (ray_radial.norm2() + 1);

      retval.set(i, j, val);
    }
  }
  retval.normalize();
  return retval;
}

}  // namespace metaball
