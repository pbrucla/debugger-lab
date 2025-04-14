#pragma once

#include <system_error>

namespace util {
// Throws a std::system_error if errno is non-zero
void throw_errno();

// Throws a std::system_error if val is negative and errno is non-zero
template <typename T>
T throw_errno(T val) {
    if (val < 0 && errno != 0) {
        throw std::system_error(errno, std::generic_category());
    }
    return val;
}
}  // namespace util
