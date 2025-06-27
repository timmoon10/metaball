#include <cmath>

namespace util {

template <typename T>
inline T sigmoid(const T& x) {
  auto exp_x = std::exp(-std::abs(x));
  if (x > 0) {
    return 1 / (1 + exp_x);
  } else {
    return exp_x / (1 + exp_x);
  }
}

}  // namespace util
