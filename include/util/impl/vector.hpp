#include <algorithm>
#include <array>
#include <cmath>

#include "util/error.hpp"

// Macro for loop unroll
#define UTIL_DO_PRAGMA(arg) _Pragma(#arg)
#if defined(__clang__)
#define UTIL_LOOP_UNROLL(n) UTIL_DO_PRAGMA(unroll(n))
#elif defined(__GNUC__)
#define UTIL_LOOP_UNROLL(n) UTIL_DO_PRAGMA(GCC unroll(n))
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
inline constexpr Vector<N, T>::Vector(Ts... values) noexcept
    : data_{static_cast<T>(values)...} {
  static_assert(sizeof...(values) == N,
                "attempted to initialize vector with invalid number of values");
}

template <size_t N, typename T>
inline constexpr void swap(Vector<N, T>& a, Vector<N, T>& b) noexcept {
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

template <size_t N, typename S, typename T>
inline constexpr Vector<N, T> operator*(const S& a,
                                        const Vector<N, T>& b) noexcept {
  Vector<N, T> result;
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
  T result = data_[0] * data_[0];
  UTIL_LOOP_UNROLL(N - 1)
  for (size_t i = 1; i < N; ++i) {
    result += data_[i] * data_[i];
  }
  return result;
}

template <size_t N, typename T>
inline bool Vector<N, T>::isfinite() const noexcept {
  bool result = std::isfinite(data_[0]);
  UTIL_LOOP_UNROLL(N - 1)
  for (size_t i = 1; i < N; ++i) {
    result = result && std::isfinite(data_[i]);
  }
  return result;
}

template <size_t N, typename T>
inline Vector<N, T> Vector<N, T>::unit() const {
  const auto denom = norm();
  UTIL_CHECK(denom > 0, "Attempted to normalize zero vector");
  return (*this) / denom;
}

template <size_t N, typename T>
inline constexpr Vector<N, T> max(const Vector<N, T>& a,
                                  const Vector<N, T>& b) noexcept {
  Vector<N, T> result;
  UTIL_LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    result[i] = std::max(a[i], b[i]);
  }
  return result;
}

template <size_t N, typename T>
inline constexpr Vector<N, T> min(const Vector<N, T>& a,
                                  const Vector<N, T>& b) noexcept {
  Vector<N, T> result;
  UTIL_LOOP_UNROLL(N)
  for (size_t i = 0; i < N; ++i) {
    result[i] = std::min(a[i], b[i]);
  }
  return result;
}

template <size_t N, typename T>
inline constexpr T dot(const Vector<N, T>& a, const Vector<N, T>& b) noexcept {
  T result = a[0] * b[0];
  UTIL_LOOP_UNROLL(N - 1)
  for (size_t i = 1; i < N; ++i) {
    result += a[i] * b[i];
  }
  return result;
}

template <size_t N, typename T>
inline constexpr Vector<N, T> cross(const Vector<N, T>& a,
                                    const Vector<N, T>& b) noexcept {
  static_assert(N == 3, "Cross product is only supported with 3D vectors");
  Vector<3, T> result;
  result[0] = a[1] * b[2] - a[2] * b[1];
  result[1] = a[2] * b[0] - a[0] * b[2];
  result[2] = a[0] * b[1] - a[1] * b[0];
  return result;
}

}  // namespace util
