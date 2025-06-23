#pragma once

#include <array>
#include <cmath>
#include <vector>

// Macro for loop unroll
#define DO_PRAGMA(arg) _Pragma(#arg)
#if defined(__clang__)
#define LOOP_UNROLL(n) DO_PRAGMA(unroll(n))
#elif defined(__GNUC__)
#define LOOP_UNROLL(n) DO_PRAGMA(GCC unroll(n))
#else
#define LOOP_UNROLL(n) DO_PRAGMA(unroll(n))
#endif

namespace metaball {

/*! Real number */
using Scalar = double;
/*! List of real numbers */
using ScalarList = std::vector<Scalar>;

/*! Real vector */
template <size_t N>
class Vector {
 public:
  /*! Number of vector dimensions */
  static constexpr size_t ndim = N;

  constexpr Vector(Scalar value = 0) noexcept;
  constexpr Vector(Scalar x, Scalar y) noexcept;

  /*! Swap data between two vectors */
  template <size_t n>
  friend constexpr void swap(Vector<n>& a, Vector<n>& b) noexcept;

  /*! Get vector element */
  constexpr Scalar& operator[](size_t i);
  /*! Get vector element */
  constexpr const Scalar& operator[](size_t i) const;

  constexpr Vector operator-() const noexcept;
  constexpr Vector operator+(const Vector& other) const noexcept;
  constexpr Vector operator-(const Vector& other) const noexcept;
  constexpr Vector operator*(const Scalar& other) const noexcept;
  constexpr Vector operator/(const Scalar& other) const noexcept;
  template <size_t n>
  friend constexpr Vector<n> operator*(const Scalar& a, const Vector<n>& b) noexcept;

  constexpr Vector& operator+=(const Vector& other) noexcept;
  constexpr Vector& operator-=(const Vector& other) noexcept;
  constexpr Vector& operator*=(const Scalar& other) noexcept;
  constexpr Vector& operator/=(const Scalar& other) noexcept;

  /*! Set all vector elements to value */
  constexpr void fill(Scalar value) noexcept;
  /*! Set all vector elements to zero */
  constexpr void zero() noexcept;

  /*! 2-norm */
  Scalar norm() const;
  /*! Square of 2-norm */
  constexpr Scalar norm2() const noexcept;

  /*! Whether all vector elements are finite */
  bool isfinite() const noexcept;

  Vector unit() const;

 private:
  std::array<Scalar, N> data_;
};

/*! Entry-wise maximum */
template <size_t N>
constexpr Vector<N> max(const Vector<N>& a, const Vector<N>& b) noexcept;
/*! Entry-wise minimum */
template <size_t N>
constexpr Vector<N> min(const Vector<N>& a, const Vector<N>& b) noexcept;

/*! Dot product */
template <size_t N>
constexpr Scalar dot(const Vector<N>& a, const Vector<N>& b) noexcept;

/*! Cross product */
constexpr Vector<3> cross(const Vector<3>& a, const Vector<3>& b) noexcept;

}  // namespace metaball

// ---------------------------------------------
// Implementation
// ---------------------------------------------

namespace metaball {

template <size_t N>
inline constexpr Vector<N>::Vector(Scalar value) noexcept {
  data_.fill(value);
}

template <size_t N>
inline constexpr Vector<N>::Vector(Scalar x, Scalar y) noexcept : data_{x, y} {}

template <size_t N>
inline constexpr void swap(Vector<N>& a, Vector<N>& b) noexcept {
  std::swap(a.data_, b.data_);
}

template <size_t N>
inline constexpr Scalar& Vector<N>::operator[](size_t i) {
  return data_[i];
}

template <size_t N>
inline constexpr const Scalar& Vector<N>::operator[](size_t i) const {
  return data_[i];
}

template <size_t N>
inline constexpr Vector<N> Vector<N>::operator-() const noexcept {
  Vector<N> out;
  LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    out[i] = -data_[i];
  }
  return out;
}

template <size_t N>
inline constexpr Vector<N> Vector<N>::operator+(
    const Vector<N>& other) const noexcept {
  Vector<N> out;
  LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    out[i] = data_[i] + other[i];
  }
  return out;
}

template <size_t N>
inline constexpr Vector<N> Vector<N>::operator-(
    const Vector<N>& other) const noexcept {
  Vector<N> out;
  LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    out[i] = data_[i] - other[i];
  }
  return out;
}

template <size_t N>
inline constexpr Vector<N> Vector<N>::operator*(
    const Scalar& other) const noexcept {
  Vector<N> out;
  LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    out[i] = data_[i] * other;
  }
  return out;
}

template <size_t N>
inline constexpr Vector<N> Vector<N>::operator/(
    const Scalar& other) const noexcept {
  Vector<N> out;
  LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    out[i] = data_[i] / other;
  }
  return out;
}

template <size_t N>
inline constexpr Vector<N> operator*(const Scalar& a,
                                     const Vector<N>& b) noexcept {
  Vector<N> out;
  LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    out[i] = a * b[i];
  }
  return out;
}

template <size_t N>
inline constexpr Vector<N>& Vector<N>::operator+=(
    const Vector<N>& other) noexcept {
  LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    data_[i] += other[i];
  }
  return *this;
}

template <size_t N>
inline constexpr Vector<N>& Vector<N>::operator-=(
    const Vector<N>& other) noexcept {
  LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    data_[i] -= other[i];
  }
  return *this;
}

template <size_t N>
inline constexpr Vector<N>& Vector<N>::operator*=(
    const Scalar& other) noexcept {
  LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    data_[i] *= other;
  }
  return *this;
}

template <size_t N>
inline constexpr Vector<N>& Vector<N>::operator/=(
    const Scalar& other) noexcept {
  LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    data_[i] /= other;
  }
  return *this;
}

template <size_t N>
inline constexpr void Vector<N>::fill(Scalar value) noexcept {
  data_.fill(value);
}

template <size_t N>
inline constexpr void Vector<N>::zero() noexcept {
  fill(0);
}

template <size_t N>
inline Scalar Vector<N>::norm() const {
  return std::sqrt(norm2());
}

template <size_t N>
inline constexpr Scalar Vector<N>::norm2() const noexcept {
  Scalar out = data_[0] * data_[0];
  LOOP_UNROLL(N - 1)
  for (size_t i = 1; i < N; ++i) {
    out += data_[i] * data_[i];
  }
  return out;
}

template <size_t N>
inline bool Vector<N>::isfinite() const noexcept {
  bool out = std::isfinite(data_[0]);
  LOOP_UNROLL(N - 1)
  for (size_t i = 1; i < N; ++i) {
    out = out && std::isfinite(data_[i]);
  }
  return out;
}

template <size_t N>
inline Vector<N> Vector<N>::unit() const {
  return (*this) / norm();
}

template <size_t N>
inline constexpr Vector<N> max(const Vector<N>& a,
                               const Vector<N>& b) noexcept {
  Vector<N> out;
  LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    out[i] = std::max(a[i], b[i]);
  }
  return out;
}

template <size_t N>
inline constexpr Vector<N> min(const Vector<N>& a,
                               const Vector<N>& b) noexcept {
  Vector<N> out;
  LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    out[i] = std::min(a[i], b[i]);
  }
  return out;
}

template <size_t N>
inline constexpr Scalar dot(const Vector<N>& a, const Vector<N>& b) noexcept {
  Scalar out = a[0] * b[0];
  LOOP_UNROLL(N - 1)
  for (size_t i = 1; i < N; ++i) {
    out += a[i] * b[i];
  }
  return out;
}

inline constexpr Vector<3> cross(const Vector<3>& a, const Vector<3>& b) noexcept {
  Vector<3> out;
  out[0] = a[1] * b[2] - a[2] * b[1];
  out[1] = a[2] * b[0] - a[0] * b[2];
  out[2] = a[0] * b[1] - a[1] * b[0];
  return out;
}

}  // namespace metaball
