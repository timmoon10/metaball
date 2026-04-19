#pragma once

#include <type_traits>

namespace util {

template <typename...>
inline constexpr bool homogeneous_v = true;  // Match empty pack
template <typename T, typename... Ts>
inline constexpr bool homogeneous_v<T, Ts...> =
    (std::is_same_v<T, Ts> && ...);  // Match non-empty pack

/*! \brief Whether template arguments are all the same. */
template <typename... Ts>
concept homogeneous = homogeneous_v<Ts...>;

}  // namespace util
