#include "metaball/image.hpp"

#include <QColor>
#include <QImage>
#include <algorithm>
#include <vector>

#include "metaball/vector.hpp"

namespace metaball {

Image::Image(size_t height, size_t width)
    : data_(height * width), height_{height}, width_{width} {}

size_t Image::height() const noexcept { return height_; }

size_t Image::width() const noexcept { return width_; }

void Image::set(size_t i, size_t j, Scalar val) { data_[i * width_ + j] = val; }

Scalar Image::get(size_t i, size_t j) const { return data_[i * width_ + j]; }

QImage Image::make_qimage() const {
  QImage retval(width_, height_, QImage::Format_RGB32);
  for (size_t i = 0; i < height_; ++i) {
    for (size_t j = 0; j < width_; ++j) {
      int val = static_cast<int>(256 * data_[i * width_ + j]);
      val = std::max(std::min(val, 255), 0);
      retval.setPixel(j, i, qRgb(val, val, val));
    }
  }
  return retval;
}

}  // namespace metaball
