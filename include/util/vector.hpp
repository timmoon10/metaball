#pragma once

#include <array>

namespace util {

/*! \brief Real vector */
template <size_t NDim, typename Scalar = double>
class Vector {
 public:
  /*! \brief Number of vector dimensions */
  static constexpr size_t ndim = NDim;
  /*! \brief Scalar type */
  using ScalarType = Scalar;
  /*! \brief Underlying data container type */
  using ContainerType = std::array<Scalar, ndim>;

  /*! \brief Default constructor
   *
   * The vector is initialized to zero.
   */
  constexpr Vector() noexcept;
  /*! \brief Constructor from values
   *
   * The number of values must match the number of vector dimensions.
   */
  template <typename... Ts>
  constexpr Vector(Ts... values) noexcept;

  /*! \brief Swap data between two vectors */
  template <size_t N, typename T>
  friend constexpr void swap(Vector<N, T>& a, Vector<N, T>& b) noexcept;

  /*! \brief Get vector element */
  constexpr Scalar& operator[](size_t i);
  /*! \brief Get vector element */
  constexpr const Scalar& operator[](size_t i) const;

  // Math operators
  constexpr Vector operator-() const noexcept;
  constexpr Vector operator+(const Vector& other) const noexcept;
  constexpr Vector operator-(const Vector& other) const noexcept;
  constexpr Vector operator*(const Scalar& other) const noexcept;
  constexpr Vector operator/(const Scalar& other) const noexcept;
  constexpr Vector& operator+=(const Vector& other) noexcept;
  constexpr Vector& operator-=(const Vector& other) noexcept;
  constexpr Vector& operator*=(const Scalar& other) noexcept;
  constexpr Vector& operator/=(const Scalar& other) noexcept;
  template <size_t N, typename S, typename T>
  friend constexpr Vector<N, T> operator*(const S& a,
                                          const Vector<N, T>& b) noexcept;

  /*! \brief Cast to underlying data container */
  constexpr operator ContainerType() const noexcept;

  /*! \brief Set all vector elements to value */
  constexpr void fill(const Scalar& value) noexcept;
  /*! \brief Set all vector elements to zero */
  constexpr void zero() noexcept;

  /*! \brief 2-norm */
  Scalar norm() const;
  /*! \brief Square of 2-norm */
  constexpr Scalar norm2() const noexcept;

  /*! \brief Whether all vector elements are finite */
  bool isfinite() const noexcept;

  /*! \brief Normalize to unit vector */
  Vector unit() const;

 private:
  /*! \brief Underlying data container */
  ContainerType data_;
};

/*! \brief Entry-wise maximum */
template <size_t N, typename T>
constexpr Vector<N, T> max(const Vector<N, T>& a,
                           const Vector<N, T>& b) noexcept;
/*! \brief Entry-wise minimum */
template <size_t N, typename T>
constexpr Vector<N, T> min(const Vector<N, T>& a,
                           const Vector<N, T>& b) noexcept;

/*! \brief Dot product */
template <size_t N, typename T>
constexpr T dot(const Vector<N, T>& a, const Vector<N, T>& b) noexcept;

/*! \brief Cross product
 *
 * Only supported with 3D vectors.
 */
template <size_t N, typename T>
constexpr Vector<N, T> cross(const Vector<N, T>& a,
                             const Vector<N, T>& b) noexcept;

}  // namespace util

// Implementation
#include "util/impl/vector.hpp"
