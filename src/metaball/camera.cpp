#include "metaball/camera.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <unordered_set>

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
  aperture_orientation_[aperture_orientation_.ndim - 1] = 1;
  row_orientation_[0] = 1;
  column_orientation_[1] = -1;
}

Image Camera::make_image(const Scene& scene, size_t height,
                         size_t width) const {
  // Check arguments
  UTIL_CHECK(height > 0, "invalid height ", height);
  UTIL_CHECK(width > 0, "invalid width ", width);

  // Spacing between pixels
  // Note: The aperture projects a flipped image onto a unit square
  // centered at the focal point.
  auto image_size = std::max(height, width);
  auto shift_x = -row_orientation_;
  if (shift_x.norm2() > 0) {
    shift_x /= shift_x.norm() * image_size;
  }
  auto shift_y = -column_orientation_;
  if (shift_y.norm2() > 0) {
    shift_y /= shift_y.norm() * image_size;
  }

  // Position of top-left pixel
  auto corner_pixel =
      aperture_position_ - focal_length_ * aperture_orientation_;
  corner_pixel += (-static_cast<ScalarType>(width - 1) / 2) * shift_x;
  corner_pixel += (-static_cast<ScalarType>(height - 1) / 2) * shift_y;

  // Calculate ray for each pixel
  Image result(height, width);
  for (size_t i = 0; i < height; ++i) {
    for (size_t j = 0; j < width; ++j) {
      auto pixel = corner_pixel + i * shift_y + j * shift_x;
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

Camera::VectorType Camera::aperture_orientation() const {
  return aperture_orientation_;
}

Camera::VectorType Camera::row_orientation() const { return row_orientation_; }

Camera::VectorType Camera::column_orientation() const {
  return column_orientation_;
}

Camera::ScalarType Camera::focal_length() const { return focal_length_; }

Camera::ScalarType Camera::film_speed() const { return film_speed_; }

void Camera::set_aperture_position(const Camera::VectorType& position) {
  aperture_position_ = position;
}

void Camera::set_aperture_orientation(const Camera::VectorType& orientation) {
  aperture_orientation_ = orientation;
  orthogonalize_orientation();
}

void Camera::set_row_orientation(const Camera::VectorType& orientation) {
  row_orientation_ = orientation;
  orthogonalize_orientation();
}

void Camera::set_column_orientation(const Camera::VectorType& orientation) {
  column_orientation_ = orientation;
  orthogonalize_orientation();
}

void Camera::set_focal_length(const Camera::ScalarType& focal_length) {
  UTIL_CHECK(focal_length > 0, "Focal length must be positive, but got ",
             focal_length);
  focal_length_ = focal_length;
}

void Camera::set_film_speed(const Camera::ScalarType& film_speed) {
  UTIL_CHECK(film_speed >= 0, "Film speed must be non-negative, but got ",
             film_speed);
  film_speed_ = film_speed;
}

void Camera::adjust_shot(const std::string_view& type,
                         Camera::ScalarType amount) {
  if (type == "move forward" || type == "move backward") {
    if (type == "move backward") {
      amount *= -1;
    }
    aperture_position_ += amount * aperture_orientation_;
  } else if (type == "move right" || type == "move left") {
    if (type == "move left") {
      amount *= -1;
    }
    aperture_position_ += amount * row_orientation_;
  } else if (type == "move up" || type == "move down") {
    if (type == "move up") {
      amount *= -1;
    }
    aperture_position_ += amount * column_orientation_;
  } else if (type == "zoom in") {
    UTIL_CHECK(amount > 0, "Zoom amount must be positive, but got ", amount);
    focal_length_ *= amount;
  } else if (type == "zoom out") {
    UTIL_CHECK(amount > 0, "Zoom amount must be positive, but got ", amount);
    focal_length_ /= amount;
  } else {
    UTIL_ERROR("Unsupported shot adjustment (", type, ")");
  }
  orthogonalize_orientation();
}

bool Camera::is_adjust_shot_type(const std::string_view& type) {
  static std::unordered_set<std::string> types = {
      "move forward", "move backward", "move right", "move left",
      "move up",      "move down",     "zoom in",    "zoom out"};
  return types.count(std::string(type)) > 0;
}

void Camera::orthogonalize_orientation() {
  auto& x1 = aperture_orientation_;
  auto& x2 = row_orientation_;
  auto& x3 = column_orientation_;
  x1 = x1.unit();
  x2 -= util::dot(x1, x2) * x1;
  x2 = x2.unit();
  x3 -= util::dot(x1, x3) * x1 + util::dot(x2, x3) * x2;
  x3 = x3.unit();
}

}  // namespace metaball
