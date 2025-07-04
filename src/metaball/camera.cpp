#include "metaball/camera.hpp"

#include "metaball/image.hpp"
#include "metaball/scene.hpp"
#include "util/error.hpp"
#include "util/vector.hpp"

namespace metaball {

namespace {

/*! @brief Gamma 2.2 opto-electronic transfer function
 *
 * Convert light intensity to electrical signal. Approximates sRGB
 * transfer function.
 */
inline Camera::ScalarType gamma_transfer_function(
    Camera::ScalarType intensity) {
  constexpr Camera::ScalarType min_intensity = 0;
  constexpr Camera::ScalarType max_intensity = 1;
  constexpr Camera::ScalarType reciprocal_gamma = 1 / 2.2;
  intensity = std::clamp(intensity, min_intensity, max_intensity);
  return std::pow(intensity, reciprocal_gamma);
}

}  // namespace

Camera::Camera() {
  image_offset_[image_offset_.ndim - 1] = -1;
  image_rotation_[0] = 1;
}

Image Camera::make_image(const Scene& scene, size_t height,
                         size_t width) const {
  // Check arguments
  UTIL_CHECK(height > 0, "invalid height ", height);
  UTIL_CHECK(width > 0, "invalid width ", width);

  // Spacing between pixels
  // Note: The image is a unit square that is directly facing the
  // aperture. The rotation vector is aligned to the pixel rows.
  auto image_size = std::max(height, width);
  auto shift_x = image_rotation_;
  shift_x -= util::dot(shift_x, image_offset_) * image_offset_;
  if (shift_x.norm2() > 0) {
    shift_x /= shift_x.norm() * image_size;
  }
  static_assert(VectorType::ndim == 3);
  auto shift_y = util::cross(shift_x, image_offset_);
  if (shift_y.norm2() > 0) {
    shift_y /= shift_y.norm() * image_size;
  }

  // Position of top-left pixel
  auto corner_pixel = aperture_position_ + image_offset_;
  corner_pixel += (-static_cast<ScalarType>(width - 1) / 2) * shift_x;
  corner_pixel += (-static_cast<ScalarType>(height - 1) / 2) * shift_y;

  // Calculate ray for each pixel
  // Note: Reflect image horizontally
  Image result(height, width);
  for (size_t i = 0; i < height; ++i) {
    for (size_t j = 0; j < width; ++j) {
      auto pixel = corner_pixel + i * shift_y + (width - j - 1) * shift_x;
      auto ray = aperture_position_ - pixel;
      auto intensity = scene.trace_ray(aperture_position_, ray, 8, 64);
      result.set(i, j, gamma_transfer_function(intensity * film_speed_));
    }
  }
  return result;
}

Camera::VectorType Camera::aperture_position() const {
  return aperture_position_;
}

Camera::VectorType Camera::image_offset() const { return image_offset_; }

Camera::VectorType Camera::image_rotation() const { return image_rotation_; }

Camera::ScalarType Camera::film_speed() const { return film_speed_; }

void Camera::set_aperture_position(
    const Camera::VectorType& aperture_position) {
  aperture_position_ = aperture_position;
}

void Camera::set_image_offset(const Camera::VectorType& image_offset) {
  image_offset_ = image_offset;
}

void Camera::set_image_rotation(const Camera::VectorType& image_rotation) {
  image_rotation_ = image_rotation;
}

void Camera::set_film_speed(const Camera::ScalarType& film_speed) {
  film_speed_ = film_speed;
}

}  // namespace metaball
