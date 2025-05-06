#pragma once

#include <stdint.h>

inline int64_t decode_leb128(const uint8_t *p) {
    int64_t ret = 0;
    int64_t shift = 0;
    for (;;) {
        uint8_t byte = *p++;
        ret |= (byte & 0x7F) << shift;
        if (!(byte & 0x80)) break;
        shift += 7;
    }
}
