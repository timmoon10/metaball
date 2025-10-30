#include "metaball/camera.hpp"

#include <omp.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

#include "metaball/image.hpp"
#include "metaball/scene.hpp"
#include "util/error.hpp"
#include "util/vector.hpp"

namespace metaball {

namespace {

template <typename T>
T degrees_to_radians(const T& degrees) {
  return degrees * (std::numbers::pi / 180);
}

/*! @brief Rotate two orthonormal vectors within their spanning plane */
template <size_t N, typename T>
void rotate_plane_basis(util::Vector<N, T>& x, util::Vector<N, T>& y,
                        const T& radians) {
  const auto sin = std::sin(radians);
  const auto cos = std::cos(radians);
  const auto x_tmp = cos * x + sin * y;
  y = -sin * x + cos * y;
  x = x_tmp;
}

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
  aperture_position_[aperture_orientation_.ndim - 1] = -4;
  aperture_orientation_[aperture_orientation_.ndim - 1] = 1;
  row_orientation_[0] = 1;
  column_orientation_[1] = -1;
}

Image Camera::make_image(const Scene& scene, size_t height,
                         size_t width) const {
  Image result(height, width);
  const auto corner_pixel_and_offsets_ =
      corner_pixel_and_offsets(height, width);
  const auto& corner_pixel = corner_pixel_and_offsets_[0];
  const auto& shift_x = corner_pixel_and_offsets_[1];
  const auto& shift_y = corner_pixel_and_offsets_[2];
#pragma omp parallel for
  for (size_t i = 0; i < height; ++i) {
    for (size_t j = 0; j < width; ++j) {
      auto pixel = corner_pixel + i * shift_y + j * shift_x;
      auto ray = aperture_position_ - pixel;
      auto intensity = scene.trace_ray(aperture_position_, ray, 16);
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

void Camera::set_aperture_position(const VectorType& position) {
  aperture_position_ = position;
}

void Camera::set_aperture_orientation(const VectorType& orientation) {
  aperture_orientation_ = orientation;
  util::make_orthonormal(column_orientation_, row_orientation_,
                         aperture_orientation_);
}

void Camera::set_row_orientation(const VectorType& orientation) {
  row_orientation_ = orientation;
  util::make_orthonormal(column_orientation_, aperture_orientation_,
                         row_orientation_);
}

void Camera::set_column_orientation(const VectorType& orientation) {
  column_orientation_ = orientation;
  util::make_orthonormal(row_orientation_, aperture_orientation_,
                         column_orientation_);
}

void Camera::set_focal_length(const ScalarType& focal_length) {
  UTIL_CHECK(focal_length > 0, "Focal length must be positive, but got ",
             focal_length);
  focal_length_ = focal_length;
}

void Camera::set_film_speed(const ScalarType& film_speed) {
  UTIL_CHECK(film_speed >= 0, "Film speed must be non-negative, but got ",
             film_speed);
  film_speed_ = film_speed;
}

void Camera::set_orientation(const VectorType& aperture_orientation,
                             const VectorType& row_orientation,
                             const VectorType& column_orientation) {
  aperture_orientation_ = aperture_orientation;
  row_orientation_ = row_orientation;
  column_orientation_ = column_orientation;
  util::make_orthonormal(column_orientation_, row_orientation_,
                         aperture_orientation_);
}

Camera::VectorType Camera::pixel_orientation(size_t row, size_t col,
                                             size_t height,
                                             size_t width) const {
  UTIL_CHECK(row < height && col < width, "Attempted to access pixel (", row,
             ",", col, ") in image with height ", height, " and width ", width);
  const auto [corner_pixel, shift_x, shift_y] =
      corner_pixel_and_offsets(height, width);
  const auto pixel = corner_pixel + row * shift_y + col * shift_x;
  return (aperture_position_ - pixel).unit();
}

void Camera::set_pixel_orientation(size_t row, size_t col, size_t height,
                                   size_t width,
                                   const VectorType& orientation) {
  // Check inputs
  UTIL_CHECK(row < height && col < width, "Attempted to access pixel (", row,
             ",", col, ") in image with height ", height, " and width ", width);

  // First basis vector is current pixel orientation
  const auto src_basis1 = pixel_orientation(row, col, height, width);

  // Second basis vector points toward target pixel orientation
  auto cos_theta = util::dot(src_basis1, orientation);
  auto src_basis2 = orientation - cos_theta * src_basis1;
  if (src_basis2.norm2() == 0) {
    return;
  }
  src_basis2 = src_basis2.unit();

  // Third basis vector is orthogonal
  VectorType src_basis3;
  do {
    auto make_orthogonal = [&](VectorType vec) -> VectorType {
      vec -= util::dot(src_basis1, vec) * src_basis1;
      vec -= util::dot(src_basis2, vec) * src_basis2;
      return vec;
    };
    src_basis3 = make_orthogonal(column_orientation_);
    if (src_basis3.norm2() > 0) {
      break;
    }
    src_basis3 = make_orthogonal(row_orientation_);
    if (src_basis3.norm2() > 0) {
      break;
    }
    src_basis3 = make_orthogonal(aperture_orientation_);
    if (src_basis3.norm2() > 0) {
      break;
    }
    UTIL_ERROR("Could not construct orthonormal basis");
  } while (false);
  src_basis3 = src_basis3.unit();

  // Rotate basis so first basis vector is target pixel orientation
  auto dst_basis1 = src_basis1;
  auto dst_basis2 = src_basis2;
  const auto dst_basis3 = src_basis3;
  const auto theta = std::acos(std::clamp(
      cos_theta, static_cast<ScalarType>(-1), static_cast<ScalarType>(1)));
  rotate_plane_basis(dst_basis1, dst_basis2, theta);

  // Apply rotation to camera orientation vectors
  auto rotate = [&](VectorType& vec) {
    const auto x1 = util::dot(vec, src_basis1);
    const auto x2 = util::dot(vec, src_basis2);
    const auto x3 = util::dot(vec, src_basis3);
    vec = x1 * dst_basis1 + x2 * dst_basis2 + x3 * dst_basis3;
  };
  rotate(aperture_orientation_);
  rotate(row_orientation_);
  rotate(column_orientation_);
  util::make_orthonormal(column_orientation_, row_orientation_,
                         aperture_orientation_);
}

void Camera::adjust_shot(const std::string_view& type, ScalarType amount) {
  util::make_orthonormal(column_orientation_, row_orientation_,
                         aperture_orientation_);
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
  } else if (type == "rotate up" || type == "rotate down") {
    if (type == "rotate up") {
      amount *= -1;
    }
    rotate_plane_basis(aperture_orientation_, column_orientation_,
                       degrees_to_radians(amount));
  } else if (type == "rotate left" || type == "rotate right") {
    if (type == "rotate left") {
      amount *= -1;
    }
    rotate_plane_basis(aperture_orientation_, row_orientation_,
                       degrees_to_radians(amount));
  } else if (type == "rotate clockwise" || type == "rotate counterclockwise") {
    if (type == "rotate counterclockwise") {
      amount *= -1;
    }
    rotate_plane_basis(row_orientation_, column_orientation_,
                       degrees_to_radians(amount));
  } else {
    UTIL_ERROR("Unsupported shot adjustment (", type, ")");
  }
  util::make_orthonormal(column_orientation_, row_orientation_,
                         aperture_orientation_);
}

bool Camera::is_adjust_shot_type(const std::string_view& type) {
  static std::unordered_set<std::string> types = {
      "move forward",     "move backward",
      "move right",       "move left",
      "move up",          "move down",
      "zoom in",          "zoom out",
      "rotate up",        "rotate down",
      "rotate left",      "rotate right",
      "rotate clockwise", "rotate counterclockwise"};
  return types.count(std::string(type)) > 0;
}

std::array<Camera::VectorType, 3> Camera::corner_pixel_and_offsets(
    size_t height, size_t width) const {
  // Check arguments
  UTIL_CHECK(height > 0, "Invalid height ", height);
  UTIL_CHECK(width > 0, "Invalid width ", width);

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

  return {std::move(corner_pixel), std::move(shift_x), std::move(shift_y)};
}

}  // namespace metaball
