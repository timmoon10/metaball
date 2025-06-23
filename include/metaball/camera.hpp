#pragma once

#include "metaball/image.hpp"

namespace metaball {

class Camera {
 public:
  Image make_image(size_t height, size_t width);
};

}  // namespace metaball
