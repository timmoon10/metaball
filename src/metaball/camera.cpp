#include "metaball/camera.hpp"

#include "metaball/image.hpp"

namespace metaball {

Image Camera::make_image(size_t height, size_t width) {
  /// TODO Make non-trivial image
  Image retval(height, width);
  for (size_t i = 0; i < height; ++i) {
    const auto y = static_cast<Image::DataType>(i) / height;
    for (size_t j = 0; j < width; ++j) {
      const auto x = static_cast<Image::DataType>(j) / width;
      auto val = 0.5f / (x * x + y * y);
      retval.set(i, j, val);
    }
  }
  return retval;
}

}  // namespace metaball
