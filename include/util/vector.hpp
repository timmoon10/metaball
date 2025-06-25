#pragma once

#include <array>

namespace util {

/*! Real vector */
template <size_t NDim, typename Scalar = double>
class Vector {
 public:
  /*! Number of vector dimensions */
  static constexpr size_t ndim = NDim;
  /*! Scalar type */
  using ScalarType = Scalar;

  constexpr Vector(const Scalar& value = 0) noexcept;
  template <typename S>
  constexpr Vector(std::initializer_list<S> init) noexcept;

  /*! Swap data between two vectors */
  template <size_t N, typename T>
  friend constexpr void swap(Vector<N, T>& a, Vector<N, T>& b) noexcept;

  /*! Get vector element */
  constexpr Scalar& operator[](size_t i);
  /*! Get vector element */
  constexpr const Scalar& operator[](size_t i) const;

  constexpr Vector operator-() const noexcept;
  constexpr Vector operator+(const Vector& other) const noexcept;
  constexpr Vector operator-(const Vector& other) const noexcept;
  constexpr Vector operator*(const Scalar& other) const noexcept;
  constexpr Vector operator/(const Scalar& other) const noexcept;
  template <size_t N, typename S, typename T>
  friend constexpr Vector<N, T> operator*(const S& a,
                                          const Vector<N, T>& b) noexcept;

  constexpr Vector& operator+=(const Vector& other) noexcept;
  constexpr Vector& operator-=(const Vector& other) noexcept;
  constexpr Vector& operator*=(const Scalar& other) noexcept;
  constexpr Vector& operator/=(const Scalar& other) noexcept;

  /*! Set all vector elements to value */
  constexpr void fill(const Scalar& value) noexcept;
  /*! Set all vector elements to zero */
  constexpr void zero() noexcept;

  /*! 2-norm */
  Scalar norm() const;
  /*! Square of 2-norm */
  constexpr Scalar norm2() const noexcept;

  /*! Whether all vector elements are finite */
  bool isfinite() const noexcept;

  /*! Normalize  */
  Vector unit() const;

 private:
  std::array<Scalar, ndim> data_;
};

/*! Entry-wise maximum */
template <size_t N, typename T>
constexpr Vector<N, T> max(const Vector<N, T>& a,
                           const Vector<N, T>& b) noexcept;
/*! Entry-wise minimum */
template <size_t N, typename T>
constexpr Vector<N, T> min(const Vector<N, T>& a,
                           const Vector<N, T>& b) noexcept;

/*! Dot product */
template <size_t N, typename T>
constexpr T dot(const Vector<N, T>& a, const Vector<N, T>& b) noexcept;

/*! Cross product */
template <size_t N, typename T>
constexpr Vector<N, T> cross(const Vector<N, T>& a,
                             const Vector<N, T>& b) noexcept;

}  // namespace util

// Implementation
#include "util/impl/vector.hpp"
