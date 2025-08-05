#include "metaball/image.hpp"

#include <QColor>
#include <QImage>
#include <algorithm>
#include <vector>

namespace metaball {

Image::Image(size_t height, size_t width)
    : data_(height * width * 3), height_{height}, width_{width} {}

size_t Image::height() const noexcept { return height_; }

size_t Image::width() const noexcept { return width_; }

void Image::set(size_t i, size_t j, DataType r, DataType g, DataType b) {
  const size_t offset = (i * width_ + j) * 3;
  data_[offset] = r;
  data_[offset + 1] = g;
  data_[offset + 2] = b;
}

void Image::set(size_t i, size_t j, DataType val) { set(i, j, val, val, val); }

std::array<Image::DataType, 3> Image::get(size_t i, size_t j) const {
  const size_t offset = (i * width_ + j) * 3;
  return {data_[offset], data_[offset + 1], data_[offset + 2]};
}

Image::operator QImage() const {
  QImage result(width_, height_, QImage::Format_RGB32);
  for (size_t i = 0; i < height_; ++i) {
    for (size_t j = 0; j < width_; ++j) {
      const size_t offset = (i * width_ + j) * 3;
      constexpr DataType min = 0;
      constexpr DataType max = 255;
      const auto r = std::clamp(256 * data_[offset], min, max);
      const auto g = std::clamp(256 * data_[offset + 1], min, max);
      const auto b = std::clamp(256 * data_[offset + 2], min, max);
      const auto color =
          qRgb(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b));
      result.setPixel(j, i, color);
    }
  }
  return result;
}

void Image::normalize() {
  const auto [min_it, max_it] =
      std::minmax_element(data_.cbegin(), data_.cend());
  auto min = *min_it;
  auto max = *max_it;
  DataType scale = (max > min) ? 1 / (max - min) : 0;
  DataType shift = -min * scale;
  for (auto& val : data_) {
    val = val * scale + shift;
  }
}

}  // namespace metaball
