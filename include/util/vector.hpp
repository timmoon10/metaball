#pragma once

#include <array>
#include <concepts>

#include "util/meta.hpp"

namespace util {

template <size_t NDim, typename Scalar = double>
class Vector;

template <typename T>
inline constexpr bool vector_instantiation_v = false;
template <size_t NDim, typename Scalar>
inline constexpr bool vector_instantiation_v<Vector<NDim, Scalar>> = true;

/*! \brief Whether a type is an instantiation of Vector */
template <typename T>
concept vector_instantiation = vector_instantiation_v<T>;

/*! \brief Real vector */
template <size_t NDim, typename Scalar>
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
    requires(sizeof...(Ts) == NDim && (std::convertible_to<Ts, Scalar> && ...))
  constexpr Vector(Ts... values) noexcept;

  /*! \brief Swap data between two vectors */
  template <vector_instantiation VectorT>
  friend constexpr void swap(VectorT& a, VectorT& b) noexcept;

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
  template <vector_instantiation VectorT, typename S>
  friend constexpr VectorT operator*(const S& a, const VectorT& b) noexcept;

  /*! \brief Cast to underlying data container */
  constexpr operator ContainerType() const noexcept;

  /*! \brief Number of vector dimensions */
  static constexpr size_t size() noexcept;

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
template <vector_instantiation VectorT>
constexpr VectorT max(const VectorT& a, const VectorT& b) noexcept;
/*! \brief Entry-wise minimum */
template <vector_instantiation VectorT>
constexpr VectorT min(const VectorT& a, const VectorT& b) noexcept;

/*! \brief Dot product */
template <vector_instantiation VectorT>
constexpr VectorT::ScalarType dot(const VectorT& a, const VectorT& b) noexcept;

/*! \brief Cross product
 *
 * Only supported with 3D vectors.
 */
template <vector_instantiation VectorT>
  requires(VectorT::ndim == 3)
constexpr VectorT cross(const VectorT& a, const VectorT& b) noexcept;

/*! \brief Convert vectors to orthonormal basis */
template <vector_instantiation... VectorTs>
  requires(homogeneous<VectorTs...>)
void make_orthonormal(VectorTs&... vs);

}  // namespace util

// Implementation
#include "util/impl/vector.hpp"
