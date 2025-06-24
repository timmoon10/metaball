#include <fstream>
#include <string>

namespace util {

inline bool file_exists(const std::string& file) {
  return static_cast<bool>(std::ifstream(file.c_str()));
}

}  // namespace util
