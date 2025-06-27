#pragma once

#include <QImage>
#include <vector>

namespace metaball {

class Image {
 public:
  using DataType = double;

  Image(size_t height, size_t width);

  size_t height() const noexcept;
  size_t width() const noexcept;

  void set(size_t i, size_t j, DataType r, DataType g, DataType b);
  void set(size_t i, size_t j, DataType val);
  std::array<DataType, 3> get(size_t i, size_t j) const;

  operator QImage() const;

  void normalize();

 private:
  std::vector<DataType> data_;
  size_t height_, width_;
};

}  // namespace metaball
