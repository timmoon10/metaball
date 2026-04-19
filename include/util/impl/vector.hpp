#include <algorithm>
#include <cmath>
#include <concepts>
#include <utility>

#include "util/error.hpp"
#include "util/functional.hpp"
#include "util/meta.hpp"

// Macro for loop unroll
#define UTIL_DO_PRAGMA(arg) _Pragma(#arg)
#if defined(__clang__)
#define UTIL_LOOP_UNROLL(n) UTIL_DO_PRAGMA(unroll(n))
#elif defined(__GNUC__)
#define UTIL_LOOP_UNROLL(n)
#else
#define UTIL_LOOP_UNROLL(n) UTIL_DO_PRAGMA(unroll(n))
#endif

namespace util {

template <size_t N, typename T>
inline constexpr Vector<N, T>::Vector() noexcept {
  data_.fill(0);
}

template <size_t N, typename T>
template <typename... Ts>
  requires(sizeof...(Ts) == N && (std::convertible_to<Ts, T> && ...))
inline constexpr Vector<N, T>::Vector(Ts... values) noexcept
    : data_{static_cast<T>(values)...} {}

template <vector_instantiation VectorT>
inline constexpr void swap(VectorT& a, VectorT& b) noexcept {
  std::swap(a.data_, b.data_);
}

template <size_t N, typename T>
inline constexpr T& Vector<N, T>::operator[](size_t i) {
  return data_[i];
}

template <size_t N, typename T>
inline constexpr const T& Vector<N, T>::operator[](size_t i) const {
  return data_[i];
}

template <size_t N, typename T>
inline constexpr Vector<N, T> Vector<N, T>::operator-() const noexcept {
  Vector<N, T> result;
  UTIL_LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    result[i] = -data_[i];
  }
  return result;
}

template <size_t N, typename T>
inline constexpr Vector<N, T> Vector<N, T>::operator+(
    const Vector<N, T>& other) const noexcept {
  Vector<N, T> result;
  UTIL_LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    result[i] = data_[i] + other[i];
  }
  return result;
}

template <size_t N, typename T>
inline constexpr Vector<N, T> Vector<N, T>::operator-(
    const Vector<N, T>& other) const noexcept {
  Vector<N, T> result;
  UTIL_LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    result[i] = data_[i] - other[i];
  }
  return result;
}

template <size_t N, typename T>
inline constexpr Vector<N, T> Vector<N, T>::operator*(
    const T& other) const noexcept {
  Vector<N, T> result;
  UTIL_LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    result[i] = data_[i] * other;
  }
  return result;
}

template <size_t N, typename T>
inline constexpr Vector<N, T> Vector<N, T>::operator/(
    const T& other) const noexcept {
  Vector<N, T> result;
  UTIL_LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    result[i] = data_[i] / other;
  }
  return result;
}

template <size_t N, typename T>
inline constexpr Vector<N, T>& Vector<N, T>::operator+=(
    const Vector<N, T>& other) noexcept {
  UTIL_LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    data_[i] += other[i];
  }
  return *this;
}

template <size_t N, typename T>
inline constexpr Vector<N, T>& Vector<N, T>::operator-=(
    const Vector<N, T>& other) noexcept {
  UTIL_LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    data_[i] -= other[i];
  }
  return *this;
}

template <size_t N, typename T>
inline constexpr Vector<N, T>& Vector<N, T>::operator*=(
    const T& other) noexcept {
  UTIL_LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    data_[i] *= other;
  }
  return *this;
}

template <size_t N, typename T>
inline constexpr Vector<N, T>& Vector<N, T>::operator/=(
    const T& other) noexcept {
  UTIL_LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    data_[i] /= other;
  }
  return *this;
}

template <vector_instantiation VectorT, typename S>
inline constexpr VectorT operator*(const S& a, const VectorT& b) noexcept {
  constexpr size_t N = VectorT::ndim;
  VectorT result;
  UTIL_LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    result[i] = a * b[i];
  }
  return result;
}

template <size_t N, typename T>
inline constexpr Vector<N, T>::operator Vector<N, T>::ContainerType()
    const noexcept {
  return data_;
}

template <size_t N, typename T>
inline constexpr size_t Vector<N, T>::size() noexcept {
  return ndim;
}

template <size_t N, typename T>
inline constexpr void Vector<N, T>::fill(const T& value) noexcept {
  data_.fill(value);
}

template <size_t N, typename T>
inline constexpr void Vector<N, T>::zero() noexcept {
  fill(0);
}

template <size_t N, typename T>
inline T Vector<N, T>::norm() const {
  return std::sqrt(norm2());
}

template <size_t N, typename T>
inline constexpr T Vector<N, T>::norm2() const noexcept {
  T result{0};
  if constexpr (N > 0) {
    UTIL_LOOP_UNROLL(N)
    for (size_t i = 0; i < N; ++i) {
      result += data_[i] * data_[i];
    }
  }
  return result;
}

template <size_t N, typename T>
inline bool Vector<N, T>::isfinite() const noexcept {
  bool result{true};
  if constexpr (N > 0) {
    UTIL_LOOP_UNROLL(N)
    for (size_t i = 0; i < N; ++i) {
      result = result && std::isfinite(data_[i]);
    }
  }
  return result;
}

template <size_t N, typename T>
inline Vector<N, T> Vector<N, T>::unit() const {
  const auto denom_sq = norm2();
  UTIL_CHECK(denom_sq > 0, "Attempted to normalize zero vector");
  return (*this) * (1 / std::sqrt(denom_sq));
}

template <vector_instantiation VectorT>
inline constexpr VectorT max(const VectorT& a, const VectorT& b) noexcept {
  constexpr size_t N = VectorT::ndim;
  VectorT result;
  UTIL_LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    result[i] = std::max(a[i], b[i]);
  }
  return result;
}

template <vector_instantiation VectorT>
inline constexpr VectorT min(const VectorT& a, const VectorT& b) noexcept {
  constexpr size_t N = VectorT::ndim;
  VectorT result;
  UTIL_LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    result[i] = std::min(a[i], b[i]);
  }
  return result;
}

template <vector_instantiation VectorT>
inline constexpr VectorT::ScalarType dot(const VectorT& a,
                                         const VectorT& b) noexcept {
  constexpr size_t N = VectorT::ndim;
  typename VectorT::ScalarType result{0};
  if constexpr (N > 0) {
    UTIL_LOOP_UNROLL(N)
    for (size_t i = 0; i < N; ++i) {
      result += a[i] * b[i];
    }
  }
  return result;
}

template <vector_instantiation VectorT>
  requires(VectorT::ndim == 3)
inline constexpr VectorT cross(const VectorT& a, const VectorT& b) noexcept {
  VectorT result;
  result[0] = a[1] * b[2] - a[2] * b[1];
  result[1] = a[2] * b[0] - a[0] * b[2];
  result[2] = a[0] * b[1] - a[1] * b[0];
  return result;
}

namespace impl {
namespace vector {

inline void make_orthonormal_reversed() {}

template <vector_instantiation VectorT, vector_instantiation... VectorTs>
  requires(homogeneous<VectorT, VectorTs...>)
inline void make_orthonormal_reversed(VectorT& head, VectorTs&... tail) {
  static_assert(sizeof...(tail) + 1 <= VectorT::ndim,
                "Can't make orthonormal basis since number of vectors is "
                "larger than vector dimension");
  make_orthonormal_reversed(tail...);
  ((head -= util::dot(tail, head) * tail), ...);
  head = head.unit();
}

}  // namespace vector
}  // namespace impl

template <vector_instantiation... VectorTs>
  requires(homogeneous<VectorTs...>)
inline void make_orthonormal(VectorTs&... vs) {
  apply_reversed_args(
      [](auto&... vs) { impl::vector::make_orthonormal_reversed(vs...); },
      vs...);
}

}  // namespace util
