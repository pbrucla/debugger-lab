#include "util.hpp"

#include <errno.h>

#include <system_error>

namespace util {
void throw_errno() {
    if (errno != 0) {
        throw std::system_error(errno, std::generic_category());
    }
}
}  // namespace util
