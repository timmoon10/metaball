#pragma once

#include <type_traits>

namespace util {

template <typename...>
inline constexpr bool are_all_same_v = true;  // Match empty pack
template <typename T, typename... Ts>
inline constexpr bool are_all_same_v<T, Ts...> =
    (std::is_same_v<T, Ts> && ...);  // Match non-empty pack
template <typename... Ts>
using are_all_same = std::bool_constant<are_all_same_v<Ts...>>;

namespace concepts {

/*! \brief Whether template arguments are all the same. */
template <typename... Ts>
concept homogeneous = are_all_same_v<Ts...>;

}  // namespace concepts

}  // namespace util
