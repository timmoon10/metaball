#include <tuple>
#include <utility>

namespace util {

namespace impl {
namespace functional {

template <typename Func, typename Args, size_t... Idxs>
inline auto apply_permuted_args(Func&& func, Args&& args,
                                std::index_sequence<Idxs...>) {
  constexpr size_t num_idxs = sizeof...(Idxs);
  return std::forward<Func>(func)(
      std::get<num_idxs - 1 - Idxs>(std::forward<Args>(args))...);
}

}  // namespace functional
}  // namespace impl

template <typename Func, typename... Args>
inline auto apply_reversed_args(Func&& func, Args&&... args) {
  return impl::functional::apply_permuted_args(
      std::forward<Func>(func),
      std::forward_as_tuple(std::forward<Args>(args)...),
      std::index_sequence_for<Args...>{});
}

}  // namespace util
