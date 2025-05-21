#pragma once

#include <errno.h>
#include <stdio.h>

#include <source_location>
#include <system_error>

namespace util {
// Throws a std::system_error if errno is non-zero
inline void throw_errno(std::source_location loc = std::source_location::current()) {
    if (errno != 0) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s:%d:%d in %s", loc.file_name(), loc.line(), loc.column(), loc.function_name());
        throw std::system_error(errno, std::generic_category(), buf);
    }
}

// Throws a std::system_error if val is negative and errno is non-zero
template <typename T>
inline T throw_errno(T val, std::source_location loc = std::source_location::current()) {
    if (val < 0) {
        throw_errno(loc);
    }
    return val;
}

inline void throw_assert(bool val, const char* msg = nullptr,
                         std::source_location loc = std::source_location::current()) {
    if (!val) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s:%d:%d in %s: %s", loc.file_name(), loc.line(), loc.column(), loc.function_name(),
                 msg ? msg : "assertion failed");
        throw std::runtime_error(buf);
    }
}
}  // namespace util
