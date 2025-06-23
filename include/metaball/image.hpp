#pragma once

#include <QImage>
#include <vector>

#include "metaball/vector.hpp"

namespace metaball {

class Image {
 public:
  using DataType = Scalar;

  Image(size_t height, size_t width);

  size_t height() const noexcept;
  size_t width() const noexcept;

  void set(size_t i, size_t j, Scalar val);
  Scalar get(size_t i, size_t j) const;

  void normalize();

  QImage make_qimage() const;

 private:
  std::vector<Scalar> data_;
  size_t height_, width_;
};

}  // namespace metaball
