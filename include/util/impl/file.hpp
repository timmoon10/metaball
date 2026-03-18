#include <filesystem>
#include <string_view>

namespace util {

inline bool file_exists(const std::string_view& file) {
  return std::filesystem::exists(file);
}

}  // namespace util
