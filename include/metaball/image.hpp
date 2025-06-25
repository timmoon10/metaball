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

  void set(size_t i, size_t j, DataType val);
  DataType get(size_t i, size_t j) const;

  void normalize();

  QImage make_qimage() const;

 private:
  std::vector<DataType> data_;
  size_t height_, width_;
};

}  // namespace metaball
