/*!
    * md, a Max external
    * Copyright (c) 2026 Adam James Wilson. All rights reserved.
    * SPDX-License-Identifier: BSD 3-Clause Clear
    */

#ifndef HEX_H
#define HEX_H

#include <stdint.h>
#include "defs.h"

static inline char *hex64_fixed(char *dst, uint64_t v)
{
    static const char hex_chars[] = "0123456789abcdef";
    for (int i = 15; i >= 0; i--) {
        dst[i] = hex_chars[v & 0xF];
        v >>= 4;
    }
    return dst + 16;
}

static inline char *hex128_fixed(char *dst, uint128_t v)
{
    dst = hex64_fixed(dst, (uint64_t)(v >> 64));
    dst = hex64_fixed(dst, (uint64_t) v);
    return dst;
}

static inline char *hex192_fixed(char *dst, uint192_t v)
{
    dst = hex64_fixed(dst, (uint64_t)(v >> 128));
    dst = hex64_fixed(dst, (uint64_t)(v >>  64));
    dst = hex64_fixed(dst, (uint64_t) v);
    return dst;
}

static inline char *hex256_fixed(char *dst, uint256_t v)
{
    dst = hex64_fixed(dst, (uint64_t)(v >> 192));
    dst = hex64_fixed(dst, (uint64_t)(v >> 128));
    dst = hex64_fixed(dst, (uint64_t)(v >>  64));
    dst = hex64_fixed(dst, (uint64_t) v);
    return dst;
}

#endif
